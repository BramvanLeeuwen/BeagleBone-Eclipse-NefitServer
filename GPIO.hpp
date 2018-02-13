/*
 * GPIO.hpp#define SYSFS_GPIO_DIR "/sys/class/gpio"#define SYSFS_GPIO_DIR "/sys/class/gpio"
 *
 *  Created on: Nov 11, 2013
 *  Author: Bram van Leeuwen
 */

#ifndef GPIO_HPP_
#define GPIO_HPP_

#include <poll.h>
//#include <stdio.h>

#include "GlobalIncludes.hpp"
//#include "SimpleGPIO.hpp"        //the SimpleGPIO of Derek Molloy with soma extensions
//#include "Vectorclasses.hpp"     //Templates for vectorclasses used for the containers
#include <string.h>

//ADC system directories
#define SYS_ADC_FILE    "/sys/devices/ocp.2/helper.14/AIN"
//Another directory in DEREK MOLLOY   on the web I found :
//                       /sys/devices/ocp.2/helper.13/AIN
//						     Why this confusion? It works on my BBB Angstrom version 2012.12
#define SYS_ADCRAW_FILE "/sys/bus/iio/devices/iio:device0/in_voltagex_raw:"

#define NR_ADCS    				 7       //seven ad converters
//#define NAME_LENGTH  			16
#define PINS_PER_HEADER 		46
//class   Comparator;

namespace gpio_namespace
{   enum Header_name{
	 	P8=0,
		P9=1
    };
    typedef enum Header_name 	header_name;

}
//enum BUTTON_STATE{
//	STABLE_LOW=0,
//	STABLE_HIGH=1,
//};

//-----  Gauge functions for temperature sensors      -----//
float NullGauge(float in);
float TempGauge(float in);
float TempGauge_HEL777(float dV);
float TempGauge_HellWheatstone(float dV);
float TempGauge_KTY10_7(float dV);
float TempGauge_KTY81_222(float dV);
float TempGauge_PT1000_V(float dV);
float TempGauge_PT1000_4_20mA(float dV) ;

using namespace gpio_namespace;

class GPIOBase{
protected:
    static const int 		   GPIO_Pin_Bank[]; 				// IO Banks for GPIOs
    static const int 		   GPIO_Pin_Id[];					// Pin Id for GPIOs
    static const unsigned long GPIO_Pad_Control[];				// Pad Control Register
    static const unsigned long GPIO_Control_Module_Registers; 	// Base address of Control Module Registers
    static const unsigned long GPIO_Base[];						// Base addresses of GPIO Modules
    static int				   GPIO_Reserved[];					// Reserved pins; initially according to Derek Molloy's list

	header_name 	expansion_header;

	unsigned int GetGPIOnr(unsigned int pinnr);
};


class GPIO   		:public GPIOBase			// *** GeneralPurposeInputOutput control class ****
{ private:

  protected:
	unsigned int 	pin_nr;
	unsigned int    gpio_nr;
	char 			*signal_name;
	PIN_DIRECTION	direction;
  public:
    GPIO();
	GPIO(Header_name nm, unsigned char nr,PIN_DIRECTION	dirctn,
			             char* namestring ,PIN_EDGE edge=NON_BLOCKING);//constructor exports the pin; reserves memory for name
	~GPIO();  													//destructor unexports the pin;  frees the memory for name
	unsigned long 	Pad_control();
	void            set_dir(PIN_DIRECTION out_flag);
	PIN_VALUE 		value();    								//gets the value of the pin (input or output)
	void			value(const PIN_VALUE val); 				//sets the value 			(output)
	void			toggle();
	inline void		high()		{value(HIGH);	};				//sets high
	inline void		low() 		{value(LOW);};
	inline bool     exported()	{return gpio_exported(gpio_nr);};
	inline unsigned int  PinNr(){return pin_nr;};
	inline unsigned int  GPIO_nr(){return gpio_nr;};
	inline char*    Name()		{return signal_name;};
	unsigned int	portnr;
};



class GPIOs :public virtual ivector<GPIO>   		//class for the collection of GeneralPurposeInputOutputs
{  	public:

	 //GPIOs(){owner=false;};
	 GPIOs(bool own=false){owner=own;};        // does not own elements by default
	~GPIOs(){};
};
//=======================================================//
//             Class RotaryEncoder                       //
//=======================================================//
class RotaryEncoder  :public GPIOBase
{
	public:
		RotaryEncoder();
		RotaryEncoder(Header_name header, int pinA, int pinB);   //pins with input/ pull up
		~RotaryEncoder();

        bool RecordEncoderRotation();
        bool RecordRotation(int A, int B);  // Run the rotary encoder in an separate thread: blockung poll within this function
	private:
		signed int 		  rotary_value;
		float			  *regulated_value,value_step,reg_max,reg_min;
		signed int 		  encoder_position;
		unsigned int 	  pinA_nr, pinB_nr;
		unsigned int      gpioA_nr, gpioB_nr;
		static const int  A=0, B=1;
		int 			  lA,lB;
		int 			  file_descriptor[2];
		struct pollfd 	  pfd[2];

		int get_lead(int fd);


	public:
		inline signed int value()							{ return rotary_value;};
		inline void		  CoupleFloat(float *fl,float step)	{ regulated_value=fl; value_step=step;};
	friend class TaskManager;
};

//=======================================================================//
//            Class ADC :reading the analog > digital converters         //
//  ADC properties 12 bits 125 ns sampletime  -- 1.8 V (do not exeed!)   //
//   2 uA current flow 1.8V reference on P9 pin 32                       //
//=======================================================================//
//  !!!!   before using the adc's:   adjust the device tree overlay to enable the analog inputs
//				echo cape-bone-iio > /sys/devices/bone_capemgr.8/slots
//   the number (8) can differ!

//to do :is it possible tot detect whether te adcs are enabled or not? in that case the  write can be done by the constructor

class ADC           // AnalogDigitalConverter control class
{ private:
	static const int 	P9_Pin_Nrs[];				// Pin on header P9 for Analog->digitalConverters
	unsigned int 		pin_nr;
	unsigned int		ad_nr;
	unsigned int    	gpio_nr;
	char 				*signal_name;
	char            	value_filename [35];
	FILE 				*ifp_ain;					//file handle for the file with information
	float				analog_in_value;
	float				analog_voltage;
	float   			(*gauge_function)(float);
	Fifo				ad_mean;                    //fifo to calculate the mean value of a numberr of consecutive readings

  public:
	ADC()			{ad_nr=-1;analog_in_value=0;analog_voltage=0;gpio_nr=-1;signal_name=(char*)NIL;pin_nr=-1;gauge_function=NullGauge;ifp_ain=(FILE*)NIL;}//ifp_ain=NIL;
	//ADC(Header_name nm, unsigned char pinnr);
	ADC(unsigned int ad_nr,float (*pf)(float)=NullGauge);//constructor ad-s numbered from 0 to 6
	                                                //adds a gauge funcion to transform the voltage to a pghysical quantity
	~ADC();  										//destructor unexports the pin;  frees the memory for names
	float 			voltage_value();    			//gets the value of the pin
	float			Read();                         //gets the gauged value (pysical quantity)
    inline int      adnr()        {return ad_nr;              };
    inline float	mean_voltage(){return ad_mean.Mean();     };
    inline float    LastVoltage() {return ad_mean.last_input; };
    inline float   	Deviation()   {return ad_mean.Deviation();};
  private:
	void			GetFileHandle_(unsigned int ad);
};

class ADCs :public virtual ivector<ADC>  //class for the collection of AdCs
{  public:
	ADCs() {};
	~ADCs(){};
};

class Translation
{   public:
	    Translation(){};
	    Translation(string en,string du);
	    inline bool operator<  (const Translation& comp){return en< comp.en;};
	    inline bool operator>  (const Translation& comp){return en> comp.en;};
	    inline bool operator== (const Translation& comp){return en==comp.en;};
	    inline bool operator<= (const Translation& comp){return en<=comp.en;};
	    inline bool operator>= (const Translation& comp){return en>=comp.en;};
	    inline bool operator<  (string comp){return en< comp;};
	    inline bool operator>  (string comp){return en> comp;};
	    inline bool operator== (string comp){return en== comp;};
	    inline bool operator<= (string comp){return en<= comp;};
	    inline bool operator>= (string comp){return en>= comp;};
	private:
		string en;
		string du;

};

class Translations:public virtual sivector<Translation>
{
	public: Translations();

	string Translate(string en);

};


bool export_overlay(string& device_overlay_name);    //exports the device tree ovelay (if not already present)
bool gpio_devicetree_present(char *treename);
char  get_distro(string& distro_name);

extern RotaryEncoder 	*rotary_encoder;
#endif /* GPIO_HPP_ */
