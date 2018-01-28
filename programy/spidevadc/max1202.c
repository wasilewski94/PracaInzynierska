#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/time.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
static uint8_t mode = SPI_MODE_0;
static uint32_t speed = 500000;
static uint8_t deselect = 0;
static uint8_t bits = 8;
static uint16_t delay = 1000000;
static const char *device = "/dev/spidev0.0";
static uint8_t input = 2;
static float lsb = 0.001;

FILE *plik;

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

static uint8_t inputtable[] = { 0x8f, 0xcf, 0x9f, 0xdf, 0xaf, 0xef, 0xef, 0xbf, 0xff };

//timestamp
unsigned long timestamp = 0;

long getMicrotime(){
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * (uint64_t)1e6 + currentTime.tv_usec;
}

int main(int argc, char *argv[])
{
	int fd;
	int ret = 0;
	uint8_t transmitbyte = 0;
	transmitbyte = inputtable[input];
	//printf("transmitbyte: %x\n", transmitbyte);
	
	//otwieram plik z danymi
	if ((plik=fopen("dane.txt", "w"))==NULL) {
		printf ("Nie mogę otworzyć pliku dane.txt\n");
		exit(1);
	} 
	
	while(1){
		
		fd = open(device, O_RDWR);
		if (fd < 0) {
			printf("%s\n", device);
			perror("can't open device");
			abort();
		}
 
		uint8_t tx[] = {transmitbyte, 0, 0};
		uint8_t rx[ARRAY_SIZE(tx)] = {0,};

		struct spi_ioc_transfer tr = {
			.tx_buf = (unsigned long)tx,
			.rx_buf = (unsigned long)rx,
			.len = ARRAY_SIZE(tx),
			.delay_usecs = delay,
			.speed_hz = speed,
			.bits_per_word = bits,
			.cs_change = deselect,
		};

		ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
		if (ret < 1) {
			close(fd);
			perror("can't send spi message");	
			abort();	
		}
		//timestamp
		timestamp = getMicrotime();
		close(fd);

		unsigned int first_byte = rx[ARRAY_SIZE(rx) - 3];
		unsigned int second_byte = rx[ARRAY_SIZE(rx) - 2];
		unsigned int third_byte = rx[ARRAY_SIZE(rx) - 1];

		//printf("First byte: %d\n", first_byte);
		//printf("Second byte %d\n", second_byte);
		//printf("Third byte %d\n", third_byte);
	
		signed long reading;
		reading = third_byte >> 3; 
		reading |= second_byte << 5;	
		float volt = reading * lsb;
		printf("%lu, %f\n", timestamp, volt); 
		fflush(stdout);
		fprintf(plik, "%lu, %f\n", timestamp, volt); 
		fflush(plik);
	}
	
	return 0;
}
