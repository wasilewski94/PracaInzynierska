#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/errno.h>
#include <linux/kref.h>
#include <linux/of.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/hrtimer.h>
#include <asm/atomic.h>
#include <linux/wait.h>
#include <linux/poll.h>

#include "timer.h"

#define DEVICE_NAME "kw_adc"

DECLARE_WAIT_QUEUE_HEAD (read_queue);

atomic_t adc_flag = ATOMIC_INIT(1);
atomic_t hrtimer_flag = ATOMIC_INIT(1);
//debugging mode
int debug = 1;

struct max1202 {
	struct spi_transfer		*adc1_xfer; 
	struct spi_message	   	*adc1_msg;
	struct cdev             *my_cdev;
	u8			            *tx;
	u8			            *rx;
	struct hrtimer 	      	 adc_timer;
	struct spi_device 	    *spi_adc_dev;
	ktime_t  		   	     adc_sampling_period;
	unsigned char	        *buffer;
	volatile int 			 head;
    volatile int 			 tail;
    rwlock_t 				 ptrs_lock; //Used to protect the head and tail pointers
    char					 err_flag;
};

struct max1202 *dev; // dev pointer
dev_t my_dev=0;
static struct class *kw_adc_class = NULL;

int kw_adc_open(struct inode *inode, struct file *filp)
{
	if (debug)
		printk(KERN_ALERT "funkcja open zostala wywolana");
	hrtimer_init(&dev->adc_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	
    dev->head = MY_BUF_LEN - 10;
    dev->tail = MY_BUF_LEN - 12;
    dev->err_flag = 0;
	return 0;
}

// SPI completion routines
void adc_complete(void * context)
{
	if(atomic_dec_and_test(&adc_flag)) {
		if(dev->rx[0] != 0){
			if(debug)
				printk(KERN_ALERT "ADC error - first byte of transfer is not 0, ignoring results.");
		}
		else{    
			//add received data to buffer and update H 
			write_lock_bh(&dev->ptrs_lock);        
            dev->buffer[dev->head] = dev->rx[1];
			dev->buffer[dev->head+1] = dev->rx[2];
			dev->head+=2;
			write_unlock_bh(&dev->ptrs_lock);
			/* When we have moved the head pointer, we can try to wake up the reading processes */
            wake_up_interruptible(&read_queue);
			if(debug)
				printk(KERN_ALERT "dodano do bufora 2 bajty");
		}
	} 
}

//HRtimer interrupt routine
enum hrtimer_restart adc_timer_proc(struct hrtimer *my_timer)
{
	if(atomic_dec_and_test(&hrtimer_flag)) {  
		struct max1202 *dev = container_of(my_timer, struct max1202, adc_timer);
		if (dev == NULL)
			return 0;
		if(debug)
			printk(KERN_ALERT "Obsluga przerwania timera, flaga: %d", hrtimer_flag);
		//check the ring buffer
		if(dev->head == MY_BUF_LEN_MASK - 2){
			printk(KERN_ALERT "Bufor sie przekrecil: %d", dev->head);
			dev->head = 0;
			return HRTIMER_RESTART;
		}
		if(dev->tail == MY_BUF_LEN_MASK - 2){
			printk(KERN_ALERT "tail sie przekrecil: %d", dev->tail);
			dev->tail = 0;
			return HRTIMER_RESTART;
		}
		if(dev->head == (dev->tail)- 2){
			printk(KERN_ALERT "bufor sie przekrecil: %d", dev->tail);
			dev->head = 0;
			dev->tail = 0;
			return HRTIMER_RESTART;
		}
		//initialization of the spi_messages
		spi_message_init(dev->adc1_msg);
		dev->adc1_msg->is_dma_mapped = 0;
		memset(dev->adc1_xfer, 0, sizeof(dev->adc1_xfer));
		dev->adc1_xfer->tx_buf = dev->tx;
		dev->adc1_xfer->rx_buf = dev->rx;
		dev->adc1_xfer->len = 3;
		spi_message_add_tail(dev->adc1_xfer, dev->adc1_msg);
		dev->adc1_msg->complete = adc_complete;
		//submission of the messages
		spi_async(dev->spi_adc_dev, dev->adc1_msg);
		//mark the fact, that message is submited
		atomic_set(&adc_flag, 1);
		if (debug){
			printk("tx=%x %x %x\n", dev->tx[0], dev->tx[1], dev->tx[2]);
			printk("rx=%x %x %x\n", dev->rx[0], dev->rx[1], dev->rx[2]);
		}
		hrtimer_forward(my_timer, my_timer->_softexpires, dev->adc_sampling_period);
		atomic_set(&hrtimer_flag, 1); //przerwanie zakonczone - ustawiam flage  
		return HRTIMER_RESTART;
	}
	else{
	if (debug)
		printk(KERN_ALERT "Proba otwarcia przerwania przed zakonczeniem trwajacego, %d", hrtimer_flag);
	return HRTIMER_RESTART;
	}
} 

int kw_adc_release(struct inode *inode, struct file *filp)
{
	//Make sure, that daq_timer is already switched off
	hrtimer_try_to_cancel(&dev->adc_timer);
	//Make sure that the spi_messages are serviced
	//while (atomic_read(&adc_flag)) {};
	//Now we can be sure, that the spi_messages are serviced
	return 0;
}

static int kw_adc_probe(struct spi_device *spi)
{	
	printk(KERN_ALERT "Probe startuje!");
	int retval = -ENOMEM;
	spi->max_speed_hz = 2000000;
	spi->bits_per_word = 8;
	spi->mode = SPI_MODE_0;
	retval = spi_setup(spi);
	if (retval < 0) {
	    printk(KERN_ALERT "<1> Nie udalo sie ustawic parametrow urzadzenia!\n");
	    return retval;
	}
	/* Store the reference to the spi_device */
	dev->spi_adc_dev = spi;
	return 0;
}

static int kw_adc_remove(struct spi_device *spi)
{
	return 0;
}

static const struct of_device_id spidev_dt_ids[] = {           
  
	{.compatible = "kw_adc"},
    {}
};

static struct spi_driver spidev_kw_adc_driver = {
	.driver	= {
		.name		= "kw_adc",
		.owner		= THIS_MODULE,
		.of_match_table = of_match_ptr(spidev_dt_ids),
	},
	.probe		= kw_adc_probe,
	.remove		= kw_adc_remove,
};
//module_spi_driver(spidev_kw_adc_driver);

static long kw_adc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
if (dev == NULL)
    return -ENODEV;
int retval=0;
int err = 0;	

if (_IOC_TYPE(cmd) != IOCTL_TYPE)
	return -ENOTTY;
if (_IOC_DIR(cmd) & _IOC_READ)
	err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
else if (_IOC_DIR(cmd) & _IOC_WRITE)
	err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
if (err)
	return -EFAULT;

if(dev->head == MY_BUF_LEN - 2){
	printk(KERN_ALERT "head sie przekrecil: %d", dev->head);
	dev->head = 0;
	return -EFAULT;
}
if(dev->tail == MY_BUF_LEN_MASK - 2){
	printk(KERN_ALERT "tail sie przekrecil: %d", dev->tail);
	dev->tail = 0;
	return -EFAULT;
}
if(dev->head == (dev->tail)-2){
	printk(KERN_ALERT "bufor sie przekrecil: %d", dev->tail);
	dev->head = 0;
	dev->tail = 0;
	return -EFAULT;
}

switch(cmd){

	case ADC_WRITEPTR: //write buffer pointer
	{
		int rptr;
        int wptr;
        int available_data;
		//We need to check if the amount of consumed data is correct
        write_lock_bh(&dev->ptrs_lock);
        wptr = dev->head;
        rptr = dev->tail;
        available_data = (wptr - rptr) & MY_BUF_LEN_MASK;
        if (arg>available_data)
        {
            write_unlock_bh(&dev->ptrs_lock);
            return -EINVAL;
        }
        //If the number of consumed bytes is correct, update the number of bytes        		
		if(debug)
			printk(KERN_ALERT "Poprawnie odczytano %d bajtow", arg);
        dev->tail = (rptr + arg) & MY_BUF_LEN_MASK;
        write_unlock_bh(&dev->ptrs_lock);
        return 0;
	}		

	case ADC_READPTR: //Wait and read buffer pointer
	{
		int ret;
		void* res = (void *) arg;
		struct buf_pointers bp;
		if (!access_ok(VERIFY_WRITE,res,sizeof(bp))) {
				return -EFAULT;
		}
		else{
			read_lock_bh(&dev->ptrs_lock);
			bp.head=dev->head;
			bp.tail=dev->tail;
            if(debug)
				printk(KERN_ALERT "dev->head: %d, dev->tail: %d",dev->head, dev->tail);
            read_unlock_bh(&dev->ptrs_lock);
            __copy_to_user(res,&bp,sizeof(bp));
            if(debug)
				printk(KERN_ALERT "available data: %d", (bp.head-bp.tail) & MY_BUF_LEN_MASK);
			return (bp.head-bp.tail) & MY_BUF_LEN_MASK;
		}
	}
 
	case ADC_SET: //Set sampling frequency
		dev->adc_sampling_period =  ktime_set(0, arg);
		dev->adc_timer.function = adc_timer_proc;
		if(debug)
			printk(KERN_ALERT "ustawiono okres probkowania: %lu", arg);
		return 0; 
  
	case ADC_START:
		hrtimer_start(&dev->adc_timer, dev->adc_sampling_period, HRTIMER_MODE_REL);
		if(debug)
			printk(KERN_ALERT "Start timera, konwersja rozpocznie sie w przerwaniu");
		return 0;
    
	case ADC_STOP: //Stop acquisition
		hrtimer_try_to_cancel(&dev->adc_timer);
		if(debug)
			printk(KERN_ALERT "koniec konwersji!");
		return 0;  
	}   
return 0; 
}

/* Memory mapping */
void kw_adc_vma_open(struct vm_area_struct * area)
{  }

void kw_adc_vma_close(struct vm_area_struct * area)
{  }

static struct vm_operations_struct kw_adc_vm_ops = {
    kw_adc_vma_open,
    kw_adc_vma_close,
};

int kw_adc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	//struct max1202 *dev = filp->private_data;
    unsigned long vsize = vma->vm_end - vma->vm_start;
    unsigned long psize = MY_BUF_LEN;
    if (vsize>psize)
        return -EINVAL;
    remap_vmalloc_range(vma, dev->buffer, 0);
    if(debug)
		printk(KERN_ALERT "mmaped buffer pointer: %x", dev->buffer);
    if (vma->vm_ops)
        return -EINVAL; //It should never happen...
    vma->vm_ops = &kw_adc_vm_ops;
    kw_adc_vma_open(vma); //No open(vma) was called, we have called it ourselves
    return 0;
}

unsigned int kw_adc_poll(struct file *filp,poll_table *wait)
{
	unsigned int mask =0;
	unsigned int data_available;
	poll_wait(filp,&read_queue,wait);
	read_lock_bh(&dev->ptrs_lock);
	data_available = (dev->head - dev->tail) & MY_BUF_LEN_MASK;
	if (data_available >= WAKE_UP_THR) 
		mask |= POLLIN |POLLRDNORM;
	if(debug)
		printk(KERN_INFO "poll head: %d tail: %d data: %d prog: = %d\n",dev->head,dev->tail, data_available, WAKE_UP_THR);
	//Check if the error occured
    if (dev->err_flag) mask |= POLLERR;
		read_unlock_bh(&dev->ptrs_lock);

	return mask;
 }


const struct file_operations kw_adc_fops = {
	.owner			= THIS_MODULE,
	.unlocked_ioctl	= kw_adc_ioctl,
	.open			= kw_adc_open,
	.release		= kw_adc_release,
	.mmap           = kw_adc_mmap,
	.poll      		= kw_adc_poll,
};

static void kw_adc_cleanup(void)
{
	printk("Clean up");
	/* Usuwamy urządzenie z klasy */
	if(my_dev && kw_adc_class) {
		device_destroy(kw_adc_class,my_dev);
	}
	/* Usuwamy strukturę urządzenia z systemu*/
	if(dev->my_cdev) { 
		cdev_del(dev->my_cdev);
		dev->my_cdev=NULL;
	}
	/* Zwalniamy numer urządzenia */
	if(my_dev) {
		unregister_chrdev_region(my_dev, 1);
	}
	/* Wyrejestrowujemy klasę */
	if(kw_adc_class) {
		class_destroy(kw_adc_class);
		kw_adc_class=NULL;
	}
	printk(KERN_ALERT "do widzenia");
	if(dev->adc1_msg || dev->adc1_xfer){
		kfree(dev->adc1_msg);
		kfree(dev->adc1_xfer);
	}
	if(dev->tx || dev->rx){
		kfree(dev->tx);
		kfree(dev->rx);
	}
	if(dev->buffer)
		vfree(dev->buffer);
	
	if(dev) {
		kfree(dev);
		dev = NULL;
	}
}

static int kw_adc_init(void)
{
	//todo dodac obsluge bledow
	//goto err1 i wyifowac gdy pamiec niezaalokowana
	printk(KERN_ALERT "Init startuje!");
	int res;
	 
	dev = kmalloc(sizeof(*dev), GFP_KERNEL | __GFP_ZERO);
	if (!dev)
		goto err1;  
	dev->adc1_xfer = kmalloc(sizeof(struct spi_transfer), GFP_KERNEL);
	if (!dev->adc1_xfer)
		goto err1;  
	dev->adc1_msg = kmalloc(sizeof (struct spi_message), GFP_KERNEL);
	if (!dev->adc1_msg)
		goto err1;  
	dev->tx = kmalloc(3, GFP_KERNEL);
	if (!dev->tx)
		goto err1;  
	dev->rx = kmalloc(3, GFP_KERNEL);
	if (!dev->rx)
		goto err1;  
	dev->buffer = vmalloc_user(MY_BUF_LEN);
	if(!dev->buffer)
		goto err1;

	dev->tx[0] = 0x9f;
	dev->tx[1] = 0;
	dev->tx[2] = 0;
   
	/* Allocate the device number */
	res=alloc_chrdev_region(&my_dev, 0, 1, DEVICE_NAME);
	if(res) {
		printk (KERN_ALERT "Alocation of the device number for %s failed\n",
        DEVICE_NAME);
		return res; 
	};  
	
	/* Create the device class for udev */
	kw_adc_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(kw_adc_class)) {
		printk(KERN_ERR "Error creating kw_adc_class.\n");
		res=PTR_ERR(kw_adc_class);
		goto err1;
	}
  
	/* Allocate the character device structure */
	dev->my_cdev = cdev_alloc();
	dev->my_cdev->ops = &kw_adc_fops;
	dev->my_cdev->owner = THIS_MODULE;
		
	rwlock_init(&dev->ptrs_lock);
	
	/* Add the character device to the system */
	res=cdev_add(dev->my_cdev, my_dev, 1);
	if(res) {
		printk (KERN_ALERT "Registration of the device number for %s failed\n",
        DEVICE_NAME);
		res=-EFAULT;
		goto err1;
	};
  
	device_create(kw_adc_class,NULL,my_dev,NULL,"%s%d", DEVICE_NAME, MINOR(my_dev));
	printk (KERN_ALERT "%s The major device number is %d.\n", "Registeration is a success.",
	MAJOR(my_dev));
	printk(KERN_ALERT "Zaladowano moj modul");
	
	res = spi_register_driver(&spidev_kw_adc_driver);
   
	return res;
	err1:
		kw_adc_cleanup();
		return res;
}
module_init(kw_adc_init);

static void kw_adc_exit(void)
{
	kw_adc_cleanup();
	spi_unregister_driver(&spidev_kw_adc_driver);
}
module_exit(kw_adc_exit);


//MODULE_DEVICE_TABLE(of, of_match_ptr(spidev_dt_ids)); 
/* Information about this module */
MODULE_DESCRIPTION("Minimal SPI ADC driver");
MODULE_AUTHOR("Krzysztof Wasilewski");
MODULE_LICENSE("Dual BSD/GPL");
