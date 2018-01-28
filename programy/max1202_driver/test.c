#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <poll.h>

#include "timer.h"

unsigned char *v;
unsigned char *buffer; // adres bufora

extern int errno;  
static const char *dev = "/dev/kw_adc0";
static float lsb = 0.001;

unsigned int input = 2;
static uint8_t inputtable[] = { 0x8f, 0xcf, 0x9f, 0xdf, 0xaf, 0xef, 0xef, 0xbf, 0xff };

struct buf_pointers bp; //truct containing head and tail of ring buffer
long long total_len = 0;

FILE *plik;

//timestamp
unsigned long long timestamp = 0;

int main(int argc, char* argv[]) {
    
    const char *file_path;
    int fd = open(dev, O_RDWR);
  
    if(fd < 0) {
		printf("Cannot open device %s!%d\n", file_path, fd);
		printf("Value of errno: %d\n", errno);
		perror("open perror:");
		exit(1);
    }
    
	//otwieram plik z danymi
	if ((plik=fopen("dane.txt", "w"))==NULL) {
		printf ("Nie mogę otworzyć pliku dane.txt\n");
		exit(1);
    }    
    
	/*
	Pomiar single-ended i unipolar, wybor kanalu:
	format komendy: 
	1XXX1111
	000 - CH0 --> 8f
	100 - CH1 --> cf 
	001 - CH2 --> 9f
	101 - CH3 --> df
	010 - CH4 --> af
	110 - CH5 --> ef
	011 - CH6 --> bf
	111 - CH7 --> ff
	*/
	
	int ret = 0;
	int res = 0;
	int pres;
	unsigned long sampling_period = 1000000000;
	signed long value;
	float volt;
	
	if(argc > 1){
		int c = atoi(argv[1]);
		sampling_period = c < 10000 ? 10000 : c;
	}
 

	//memory mapping
    v = (unsigned char *)mmap(NULL, MY_BUF_LEN, PROT_READ|PROT_WRITE , MAP_PRIVATE ,fd ,0);
    if(v == MAP_FAILED){
		printf("mmap failed\n");
		exit(1);
    }
	printf("adres zmapowanej pamieci: %x\n", v);

	//set timer
    ret = ioctl(fd, ADC_SET, sampling_period);
    printf("ustawienie okresu probkowania: %lu \n", sampling_period);
    if(ret < 0) {
		printf("Value of errno: %d\n", errno);
		perror("open perror:");
    }
	//start timer
	ret = ioctl(fd, ADC_START);
	printf("wlaczenie timera\n");
      
    if(ret < 0) {
		printf("Value of errno: %d\n", errno);
		perror("open perror:");
    }

    while(1){
		
		int len=0; // dlugosc bufora
		int pres;
		struct pollfd pfd[1] = {{.fd = fd, .events = POLLIN, .revents = 0},};
		pres = poll(pfd,1,-1);
		if(pres<0) {
			perror("Error in poll:");
			exit(1);	
		}
		if(pfd[0].revents) {
			len = ioctl(fd, ADC_READPTR, &bp);
			//zapisuje timestamp
			total_len += len; 
			printf("len=%d total=%lld head:%d tail:%d\n",len,total_len,bp.head, bp.tail);
			while (bp.head != bp.tail){
				//skladam z 8 bajtow 64-bitowy timestamp
				timestamp = 0;				
				for(int i=0; i<8; i++){
					timestamp |=((unsigned long long)*(v+bp.tail+i+2)<<(56-i*8));
				}						
				value = *(v+bp.tail+1) >> 3;
				value |= *(v+bp.tail) << 5;
				volt = value * lsb;
				printf( "%llu, %f\n", timestamp, volt);
				fflush(stdout);
				fprintf(plik, "%llu, %f\n", timestamp, volt);
				fflush(plik);
				bp.tail=(bp.tail+10) & MY_BUF_LEN_MASK; //Adjust tail pointer modulo len-1
			} 
			ret = ioctl(fd, ADC_WRITEPTR, len);
		}    
		fflush(stdout);
	}
    
    //wylaczamy timer
    ret = ioctl(fd, ADC_STOP);
    printf("wylaczenie timera\n");
    if(ret < 0) {
		printf("Value of errno: %d\n", errno);
		perror("open perror:");
    }
    fclose (plik);
    munmap(v, MY_BUF_LEN);
    close(fd);
    return 0;
}	
