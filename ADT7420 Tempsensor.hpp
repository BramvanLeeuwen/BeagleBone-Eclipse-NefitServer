/*
 * ADT7420 Tempsensor.hpp
 *
 *  Created on: Apr 27, 2014
 *      Author: bram
 */

#ifndef ADT7420_TEMPSENSOR_HPP_
#define ADT7420_TEMPSENSOR_HPP_

#define ADT7420_I2C_BUFFER 0x80


enum AD_RESOLUTION {
	AD13_BITS		 		= 0,
	AD16_BITS				= 1
};

enum ADT7420_REGISTERS
{	TEMP_MSB		=0x00,
	TEMP_LSB		=0x01,
	ADT7420_STATUS	=0x02,
	ADT7420_CONFIG  =0x03,
	THIGHSET_MSB 	=0x04,
	THIGHSET_LSB 	=0x05,
	TLOWSET_MSB 	=0x06,
	TLOWSET_LSB 	=0x07,
	TCRIT_MSB		=0x08,
	TCRIT_LSB		=0x09,
	THYST			=0x0A,
	ADT7420_ID      =0x0B,		//fixed value 0xCB
	ADT7420_RESET	=0x2F,
};

class ADT7420_Tempsensor{
public:
	ADT7420_Tempsensor(int bus, int address);
private:
	int 	I2CBus, I2CAddress;  //Take care i2c-1 is the bus IC2 Also I2CBUS=1 is the default bus io P9.19 and P9.20
	char 	dataBuffer[ADT7420_I2C_BUFFER];
	float 	temp;
	bool    negative;
    bool    connctd;
	AD_RESOLUTION     resolution ;

	int  convertTemperature(int msb_addr, int lsb_addr);
	int  writeI2CDeviceByte(char address, char value);
	//char readI2CDeviceByte(char address);
    bool Tcrit, Thigh, Tlow;   //flags for

public:
    int   readSensorState();   //returns error number if sensor cannot be read out
	float convertTemperature();
	inline float temperature(){return temp;};
	inline bool  connected() {return connctd;};
	void  readFlags();
	char  tempstring[7];
	void temp_to_string();

	virtual ~ADT7420_Tempsensor();
};



#endif /* ADT7420_TEMPSENSOR_HPP_ */
