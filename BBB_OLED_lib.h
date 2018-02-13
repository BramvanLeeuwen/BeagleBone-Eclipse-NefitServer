/******************************************************************
 This is the common ArduiPi include file for ArduiPi board
 
02/18/2013  Charles-Henri Hallard (http://hallard.me)
            Modified for compiling and use on Raspberry ArduiPi Board
            LCD size and connection are now passed as arguments on 
            the command line (no more #define on compilation needed)
            ArduiPi project documentation http://hallard.me/arduipi

07/26/2013  Charles-Henri Hallard (http://hallard.me)
            Done generic library for different OLED type
            
 Written by Charles-Henri Hallard for Fun .
 All text above must be included in any redistribution.
            
 ******************************************************************/

#ifndef _ArduiPi_OLED_lib_H
#define _ArduiPi_OLED_lib_H

#include <stdio.h>
#include <stdarg.h>  
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#define OLED_I2C_RESET 12 		  /* adapted for Beaglebone GPIO pinnumber on P9  */

// OLED type I2C Address
#define ADAFRUIT_I2C_ADDRESS   	  0x3D /* 011110+SA0+RW - 0x3C or 0x3D */
  
// Oled supported display
#define OLED_ADAFRUIT_SPI_128x32  0
#define OLED_ADAFRUIT_SPI_128x64  1
#define OLED_ADAFRUIT_I2C_128x32  2
#define OLED_ADAFRUIT_I2C_128x64  3
#define OLED_LAST_OLED            4 /* always last type, used in code to end array */

// Arduino Compatible Macro
#define _BV(bit) (1 << (bit))

// GCC Missing
//#define max(a,b) (a>b?a:b)
//#define min(a,b) (a<b?a:b)

#endif
