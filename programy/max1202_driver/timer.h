#ifndef _TIMER_H_

#define IOCTL_TYPE (253)
#define ADC_SET				_IOW(IOCTL_TYPE, 1, unsigned long)
#define ADC_START			_IO(IOCTL_TYPE, 2)
#define ADC_STOP			_IO(IOCTL_TYPE, 3)
#define ADC_WRITEPTR		_IO(IOCTL_TYPE, 4)
#define ADC_READPTR			_IOR(IOCTL_TYPE, 5, struct buf_pointers)


/* Capacity of kernel buffer (mmapped into user space)*/
#define MY_BUF_LEN 262144
#define MY_BUF_LEN_MASK (MY_BUF_LEN-1)

#define WAKE_UP_THR 10

struct buf_pointers {
  int head;
  int tail;
} __attribute__ ((__packed__));

#define _TIMER_H_
#endif
