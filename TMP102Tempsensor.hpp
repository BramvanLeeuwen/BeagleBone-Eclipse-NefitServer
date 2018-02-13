/*
 * TMP102Tempsensor.hpp
 *
 *  Created on: Jul 25, 2016
 *      Author: bram
 */

#ifndef TMP102TEMPSENSOR_HPP_
#define TMP102TEMPSENSOR_HPP_

class TMP102_Tempsensor{
public:
	 TMP102_Tempsensor(int bus, int address= 0x48);
private:
	int 	I2CBus, I2CAddress;  //Take care i2c-1 is the bus IC2 Also I2CBUS=1 is the default bus io P9.19 and P9.20
	float 	temp;
	bool    connctd;
    int     fd;  				//filedescriptor

public:
    int   readSensorState();		//returns error number if sensor cannot be read out
	inline float temperature(){return temp;};
	inline bool  connected() {return connctd;};
	char  tempstring[6];
	void temp_to_string();

	virtual ~ TMP102_Tempsensor();
};

int TMP102loop();


#endif /* TMP102TEMPSENSOR_HPP_ */
