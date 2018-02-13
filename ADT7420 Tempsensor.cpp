/*
 * ADT7420 Tempsensor.cpp
 *
 *  Created on: Apr 27, 2014
 *      Author: bram
 */

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
//#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stropts.h>
#include <stdio.h>
#include <iostream>
#include <math.h>

#include "ADT7420 Tempsensor.hpp"

using namespace std;

#define MAX_BUS 64


ADT7420_Tempsensor::ADT7420_Tempsensor(int bus, int address){
	I2CBus		= bus;      //Take care i2c-1 is the bus IC2 Also I2CBUS=1 is the default bus io P9.19 and P9.20
	I2CAddress	= address;
	connctd =!readSensorState();
	resolution	= AD16_BITS;
}


int ADT7420_Tempsensor::readSensorState(){

    char namebuf[MAX_BUS];
   	snprintf(namebuf, sizeof(namebuf), "/dev/i2c-%d", I2CBus);
    int fd;  //filedescriptor
    if ((fd = open(namebuf, O_RDWR)) < 0){
    	    printf ("Failed to open ADT7420 Sensor on i2-bus%i (= I2%i)\n\r",I2CBus,3-I2CBus);
    	    connctd=false;
    	    return(1);
    }
    if (ioctl(fd, I2C_SLAVE, I2CAddress) < 0){
    		printf("I2C_SLAVE address %i failed...\n\r" ,I2CAddress );
    		connctd=false;
            return(2);
    }
    dataBuffer[0]=0;
    int numberBytes = 20;
    int bytesRead= read(fd, this->dataBuffer, numberBytes);
    if (bytesRead == -1){
    	printf("Failure to read byte stream from ADT4720 in readFullSensorState()\n\r");
    	connctd=false;
    	close(fd);
    	return 1;
    }
    close(fd);
//    for (int i=0;i<12;i++)
//    	printf("%2x ",this->dataBuffer[i]);printf( "\n\r");
    this->temp = convertTemperature();
    return 0;
}


float ADT7420_Tempsensor::convertTemperature()
{   int   i,j;
	short msbyte= dataBuffer[0];
    short lsbyte= dataBuffer[1];
    if (resolution == AD13_BITS)
    	 lsbyte=lsbyte & 0xE0  ;    //set the flagbits to 000
    if (msbyte>>7)        			//if signbit is set: negative
    { msbyte	= ~msbyte+1;   		//convert from two's complement
	  i			= -msbyte;
      lsbyte	= ~lsbyte+1;
      j			= lsbyte;
      negative=true;
    }
    else
    { i			= msbyte;
   	  j     	= lsbyte;
   	  negative=false;
    }
    temp= 2*i+ (float)j/128.0;
    return temp;
}

void ADT7420_Tempsensor::temp_to_string()
{   tempstring[0]=negative?'-':' ';
    float  abs_temp =negative?-temp:temp;
    int i= abs_temp;
	sprintf(tempstring+1,"%2i,%02i",i,(int)((abs_temp*100-i*100)+0.5));
	tempstring[6]=0;
}


void ADT7420_Tempsensor::readFlags()
{  	if (resolution== AD16_BITS)
		printf("Error: Flags not set in 16-bits mode!\n\r");
	short lsbyte= dataBuffer[TEMP_LSB];
    Tcrit= bool(lsbyte && 0x01);
    Thigh= bool(lsbyte && 0x02);
    Tlow = bool(lsbyte && 0x04);
}

ADT7420_Tempsensor::~ADT7420_Tempsensor(){};
