/*
 * TMP102Tempsensor.cpp
 *
 *  Created on: Jul 25, 2016
 *      Author: bram
 */
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include "TMP102Tempsensor.hpp"
int fd;
int __fd_TMP102sens;
#define MAX_BUS 64

void
signal_handler(int signum)
{ assert(0 == close(fd));
  exit(signum);
}

void
signal_handlerTMP102(int signum)
{ assert(0 == close(__fd_TMP102sens));
  exit(signum);
}

int TMP102loop(){
  char 	*bus = "/dev/i2c-1"; /* Pins P9_19 and P9_20 */
  int 	addr = 0x48;          /* The I2C address of TMP102 */
  char 	buf[2] = {0};
  int 	temp;
  unsigned char MSB, LSB;
  float c;

  if ((fd = open(bus, O_RDWR)) < 0) {/* ERROR HANDLING: you can check errno to see what went wrong */
    perror("Failed to open the i2c bus");
    exit(1);
  }
 if (ioctl(fd, I2C_SLAVE, addr) < 0) {
   perror("Failed to acquire bus access and/or talk to slave.\n");/* ERROR HANDLING; you can check errno to see what went wrong */
   exit(1);
 }
 signal(SIGINT, signal_handler);/* Register the signal handler */
 while(1){// Using I2C Read
     if (read(fd,buf,2) != 2)
       perror("Failed to read from the i2c bus.\n");/* ERROR HANDLING: i2c transaction failed */
     else {
       MSB = buf[0];
       LSB = buf[1];
       temp = ((MSB << 8) | LSB) >> 4;/* Convert 12bit int using two's compliment */
       c = temp*0.0625;
       printf("Temp : %f degree Celsius\n", c);
     }
     sleep(1);
   }
}

TMP102_Tempsensor::TMP102_Tempsensor(int bus, int address){
	char namebuf[MAX_BUS];
	I2CBus		= bus;      //Take care i2c-1 is the bus IC2 Also I2CBUS=1 is the default bus io P9.19 and P9.20
	I2CAddress	= address;
	snprintf(namebuf, sizeof(namebuf), "/dev/i2c-%d", I2CBus);
	if ((fd = open(namebuf, O_RDWR)) < 0) {/* ERROR HANDLING: you can check errno to see what went wrong */
	    perror("Failed to open the i2c bus");
	    exit(1);
	  }
	__fd_TMP102sens=fd;
	 if (ioctl(fd, I2C_SLAVE, address) < 0) {
	   perror("Failed to acquire bus access and/or talk to slave.\n");/* ERROR HANDLING; you can check errno to see what went wrong */
	   exit(1);
	 }
	signal(SIGINT, signal_handlerTMP102);/* Register the signal handler */
	connctd=!readSensorState();
}


int TMP102_Tempsensor::readSensorState(){
	char 	dataBuffer[2] = {0};
	short	MSB,LSB;
	int   tempint;
	if (read(fd,dataBuffer,2) != 2){
		printf("Failed to read TMP102 from he i2-bus %i (= I2%i)\n\r",I2CBus,3-I2CBus);
	    //perror("Failed to read TMP102 from the i2c bus.\n");/* ERROR HANDLING: i2c transaction failed */
	    connctd=false;
	    return 1;
	}
	else {
		MSB = dataBuffer[0];
	    LSB = dataBuffer[1];
	    tempint = ((MSB << 8) | LSB) >> 4;/* Convert 12bit int using two's compliment */
	    temp = tempint*0.0625;
	}
    return 0;
}

void TMP102_Tempsensor::temp_to_string()
{   int i= temp;
	sprintf(tempstring,"%2i,%02i",i,(int)((temp*100-i*100)+0.5));
	tempstring[5]=0;
}

TMP102_Tempsensor::~TMP102_Tempsensor(){};




