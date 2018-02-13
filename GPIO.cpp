/*
 *  GPIO.cpp
 *
 *  Created on: Nov 11, 2013
 *  Author: Bram van Leeuwen
 */

#include "GPIO.hpp"

#include <assert.h>
#include <stdio.h>


#include <signal.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <sys/types.h>   //not necessary in older version of gcc
#include <unistd.h>      //not necessary in older version of gcc
#include <limits>

extern pthread_mutex_t 	rotary_encoder_mutex;
using namespace std;

extern char slotsfilename[];
extern bool newhavenOLED_mutex_is_unlocked;
extern int forecast_interval;
extern unsigned int forecast_intervals;
extern unsigned int	seconds_after_keypress;


const int GPIOBase::GPIO_Pin_Bank[] =
{
        -1, -1,  1,  1,  1,     // P8_1  -> P8_5
         1,  2,  2,  2,  2,     // P8_6  -> P8_10
         1,  1,  0,  0,  1,     // P8_11 -> P8_15
         1,  0,  2,  0,  1,     // P8_16 -> P8_20
         1,  1,  1,  1,  1,     // P8_21 -> P8_25
         1,  2,  2,  2,  2,     // P8_26 -> P8_30
         0,  0,  0,  2,  0,     // P8_31 -> P9_35
         2,  2,  2,  2,  2,     // P8_36 -> P8_40
         2,  2,  2,  2,  2,     // P8_41 -> P8_45
         2,                     // P8_46
        -1, -1, -1, -1, -1,     // P9_1  -> P9_5
        -1, -1, -1, -1, -1,     // P9_6  -> P9_10
         0,  1,  0,  1,  1,     // P9_11 -> P9_15
         1,  0,  0,  0,  0,     // P9_16 -> P9_20
         0,  0,  1,  0,  3,     // P9_21 -> P9_25
         0,  3,  3,  3,  3,     // P9_26 -> P9_30
         3, -1, -1, -1, -1,     // P9_31 -> P9_35
        -1, -1, -1, -1, -1,     // P9_36 -> P9_40
         0,  0, -1, -1, -1,     // P9_41 -> P9_45
        -1                      // P9_46
};

//=======================================================
//=======================================================

const int GPIOBase::GPIO_Pin_Id[] =
{
        -1, -1,  6,  7,  2,		// P8_1  -> P8_5
         3,  2,  3,  5,  4,		// P8_6  -> P8_10
        13, 12, 23, 26, 15,		// P8_11 -> P8_15
        14, 27,  1, 22, 31,		// P8_16 -> P8_20
        30,  5,  4,  1,  0,     // P8_21 -> P8_25
        29, 22, 24, 23, 25,		// P8_26 -> P8_30
        10, 11,  9, 17,  8,		// P8_31 -> P9_35
        16, 14, 15, 12, 13,		// P8_36 -> P8_40
        10, 11,  8,  9,  6,		// P8_41 -> P8_45
         7,						// P8_46
        -1, -1, -1, -1, -1,		// P9_1  -> P9_5
        -1, -1, -1, -1, -1,		// P9_6  -> P9_10
        30, 28, 31, 18, 16,		// P9_11 -> P9_15
        19,  5,  4, 13, 12,		// P9_16 -> P9_20
         3,  2, 17, 15, 21,		// P9_21 -> P9_25
        14, 19, 17, 15, 16,		// P9_26 -> P9_30
        14, -1, -1, -1, -1,		// P9_31 -> P9_35
        -1, -1, -1, -1, -1,		// P9_36 -> P9_40
        20,  7, -1, -1, -1,		// P9_41 -> P9_45
        -1 						// P9_46
};

//=======================================================
//=======================================================

// Pad Control Register
const unsigned long GPIOBase::GPIO_Pad_Control[] =
{
        0x0000, 0x0000, 0x0818, 0x081C, 0x0808,        // P8_1 -> P8_5
        0x080C, 0x0890, 0x0894, 0x089C, 0x0898,        // P8_6 -> P8_10
        0x0834, 0x0830, 0x0824, 0x0828, 0x083C,        // P8_11 -> P8_15
        0x0838, 0x082C, 0x088C, 0x0820, 0x0884,        // P8_16 -> P8_20
        0x0880, 0x0814, 0x0810, 0x0804, 0x0800,        // P8_21 -> P8_25
        0x087C, 0x08E0, 0x08E8, 0x08E4, 0x08EC,        // P8_26 -> P8_30
        0x08D8, 0x08DC, 0x08D4, 0x08CC, 0x08D0,        // P8_31 -> P8_35
        0x08C8, 0x08C0, 0x08C4, 0x08B8, 0x08BC,        // P8_36 -> P8_40
        0x08B0, 0x08B4, 0x08A8, 0x08AC, 0x08A0,        // P8_41 -> P8_45
        0x08A4,                                        // P8_46
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000,        // P9_1 -> P9_5
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000,        // P9_6 -> P9_10
        0x0870, 0x0878, 0x0874, 0x0848, 0x0840,        // P9_11 -> P9_15
        0x084C, 0x095C, 0x0958, 0x097C, 0x0978,        // P9_16 -> P9_20
        0x0954, 0x0950, 0x0844, 0x0984, 0x09AC,        // P9_21 -> P9_25
        0x0980, 0x09A4, 0x099C, 0x0994, 0x0998,        // P9_26 -> P9_30
        0x0990, 0x0000, 0x0000, 0x0000, 0x0000,        // P9_31 -> P9_35
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000,        // P9_36 -> P9_40
        0x09B4, 0x0964, 0x0000, 0x0000, 0x0000,        // P9_41 -> P9_45
        0x0000 										   // P9_46
};



int GPIOBase::GPIO_Reserved[] =
{
        -1, -1, -1, -1, -1,     // P8_1  -> P8_5
        -1,  0,  0,  0,  0,     // P8_6  -> P8_10
         0,  0,  0,  0,  0,     // P8_11 -> P8_15
         0,  0,  0,  0, -1,     // P8_16 -> P8_20
        -1, -1, -1, -1, -1,     // P8_21 -> P8_25
        -1, -1, -1, -1, -1,     // P8_26 -> P8_30    27-46 Lcd conflict pins ref. manual par.8.1.1
        -1, -1, -1, -1, -1,     // P8_31 -> P9_35
        -1, -1, -1, -1, -1,     // P8_36 -> P8_40
        -1, -1, -1, -1, -1,     // P8_41 -> P8_45
        -1,                     // P8_46
        -1, -1, -1, -1, -1,     // P9_1  -> P9_5
        -1, -1, -1, -1, -1,     // P9_6  -> P9_10
         0,  0,  0,  0,  0,     // P9_11 -> P9_15
         0,  0,  0, -1, -1,     // P9_16 -> P9_20
         0,  0,  0,  0, -1,     // P9_21 -> P9_25
         0,  0, -1, -1,  0,     // P9_26 -> P9_30
        -1, -1, -1, -1, -1,     // P9_31 -> P9_35
        -1, -1, -1, -1, -1,     // P9_36 -> P9_40
        -1, -1, -1, -1, -1,     // P9_41 -> P9_45
        -1                      // P9_46
};  // -1 = used for other purposes by other processes (according to Derek Molloy & reference manual)
    // -2 = used by this program

unsigned int GPIOBase::GetGPIOnr(unsigned int pin_nr)
{	//--pin_nr;
	assert(GPIO_Pin_Bank[pin_nr]>=0);
  	return (unsigned int)(GPIO_Pin_Bank[pin_nr]*32+GPIO_Pin_Id[pin_nr]);
}
//=======================================================
//=======================================================

const unsigned long GPIOBase::GPIO_Control_Module_Registers = 0x44E10000;

//=======================================================
//=======================================================

const unsigned long GPIOBase::GPIO_Base[] =
{
        0x44E07000,        // GPIO0
        0x4804C000,        // GPIO1
        0x481AC000,        // GPIO2
        0x481AE000         // GPIO3
};

GPIO::GPIO()
{  	pin_nr= 0;     //number in array; numbering starts at pin_nr= 0
    gpio_nr=0;
	direction = DIRECTION_NOT_DEFINED;
	signal_name= new char[1];
	signal_name[0]=0;
	portnr=-1;
}

GPIO::GPIO(Header_name nm, unsigned char nr,PIN_DIRECTION dir,char* namestring,PIN_EDGE edge)
{   char c;
    bool endstr=false;
    portnr=-1;
	pin_nr= (unsigned int)nm*PINS_PER_HEADER+nr-1;     //number in array; numbering starts at pin_nr= 0
    assert(GPIO_Pin_Bank[pin_nr]>=0);
    if ((GPIOBase::GPIO_Reserved[pin_nr]<0))
    	printf("Pinnumber %i;Header P%i; Pin %i; Reason to reject: %i ",pin_nr,pin_nr/PINS_PER_HEADER+8,pin_nr%PINS_PER_HEADER+1,GPIOBase::GPIO_Reserved[pin_nr]);
    assert(GPIOBase::GPIO_Reserved[pin_nr]>=0);
    GPIOBase::GPIO_Reserved[pin_nr]=-2;  //reserve this pin number for this instance
	gpio_nr=GetGPIOnr(pin_nr);
	direction = dir;
	gpio_export(gpio_nr);
	gpio_set_dir(gpio_nr, direction);
	if (direction==INPUT_PIN)
		gpio_set_edge(gpio_nr,edge);
	signal_name= new char[COMPARATOR_NAMELENGTH+1];
	for (int i=0;i<COMPARATOR_NAMELENGTH ;i++)
	{   c= namestring[i];
	    if (c==0) endstr=true;
	    signal_name[i]=endstr?' ':c;
	}
	  //strncpy(name,namestring,comparatornamelength);
	signal_name[COMPARATOR_NAMELENGTH]=0;
}

GPIO::~GPIO()
{  	gpio_unexport(gpio_nr);
    delete[] signal_name;
}

unsigned long GPIO::Pad_control()
{
	return GPIO_Pad_Control[pin_nr];
}

PIN_VALUE GPIO::value()    					//gets the value
{   unsigned int value;
    int err_no;
	err_no= gpio_get_value(gpio_nr, &value);
	assert (err_no==0);
	return value ?  HIGH : LOW;
}

void GPIO::value(const PIN_VALUE val)    	//sets the value
{   int err_no=0;
    assert (direction==OUTPUT_PIN);
    err_no= gpio_set_value((unsigned int)gpio_nr, val);
    assert (err_no==0);
}

void GPIO::set_dir(PIN_DIRECTION out_flag)
{   int err_no=0;
    err_no= gpio_set_dir((unsigned int)gpio_nr, out_flag);
    assert (err_no==0);
}

void  GPIO::toggle()
{   if (value()==HIGH) low();
		else high();
}


//bool GPIO::exported()
//{   return gpio_exported(gpio_nr);
//}


//void GPIOs::Add(GPIO *gpio_pter)
//{
//	ivector(gpio_pter);
//}
//=======================================================//
//             Class RotaryEncoder                       //
//=======================================================//
RotaryEncoder::RotaryEncoder()
{  rotary_value=0;
   gpioA_nr=-1;gpioB_nr=-1; lA=-1;lB=-1;pinA_nr=-1;pinB_nr=-1;value_step=0;
   regulated_value= (float*)NIL;
   encoder_position=0;
   //value_step=0;
   reg_max=numeric_limits<float>::max();
   reg_min=numeric_limits<float>::min();
}

RotaryEncoder::RotaryEncoder(Header_name hd, int pinA, int pinB)   //pins with input/ pull up
// Run the rotary encoder in an separate thread: blockung poll
{   char file_descriptor_name[30];
    lA=-1;lB=-1;value_step=0;
    encoder_position=0;
	pinA_nr= pinA;
    pinB_nr= pinB;
    unsigned int pA,pB;
    pA=(unsigned int)hd*46+pinA-1;
    pB=(unsigned int)hd*46+pinB-1;
    assert(GPIOBase::GPIO_Reserved[pA]>=0);
    assert(GPIOBase::GPIO_Reserved[pB]>=0);  //control if not reserved for other purposes
    GPIOBase::GPIO_Reserved[pA]=-2;
    GPIOBase::GPIO_Reserved[pB]=-2;  //reserve for the rotary encoder
    gpioA_nr= GetGPIOnr(pA);
    gpioB_nr= GetGPIOnr(pB);
    gpio_export(gpioA_nr);
    gpio_export(gpioB_nr);
    gpio_set_edge(gpioA_nr,BOTH_EDGES);
   	gpio_set_edge(gpioB_nr,BOTH_EDGES);
    sprintf (log_string,"gpioA_nr: %i gpioA_nr: %i",gpioA_nr,gpioB_nr );
   	log_cl(log_string);
   	sprintf(file_descriptor_name,"/sys/class/gpio/gpio%i/value",gpioA_nr);
   	file_descriptor[A] = open(file_descriptor_name, O_RDONLY);
   	sprintf(file_descriptor_name,"/sys/class/gpio/gpio%i/value",gpioB_nr);
   	file_descriptor[B] = open(file_descriptor_name, O_RDONLY);

   	pfd[A].fd = file_descriptor[A];
   	pfd[A].events = POLLPRI;
   	pfd[A].revents = 0;

   	pfd[B].fd = file_descriptor[B];
   	pfd[B].events = POLLPRI;
   	pfd[B].revents = 0;

	rotary_value=0;
	regulated_value= (float*)NIL;  //couple a value to regulate with function Couple()
	reg_max =	numeric_limits<float>::max();
	reg_min =	numeric_limits<float>::min();
}

RotaryEncoder::~RotaryEncoder()
{   gpio_unexport(gpioA_nr);
    gpio_unexport(gpioB_nr);
}

int RotaryEncoder::get_lead(int fd) {
    lseek(fd, 0, 0);
    char buffer[1024];

    int size = read(fd, buffer, sizeof(buffer));
    if (size != -1) {
       buffer[size] = 0;
       return  atoi(buffer);
    }
    else
       return -1;
}

bool RotaryEncoder::RecordEncoderRotation()
{	poll(pfd, 2, -1);
    //printf("ready: %d", ready);
	if (pfd[A].revents != 0){
	  //printf("\t Lead A");
	  lA = get_lead(file_descriptor[A]);
	}
	if (pfd[B].revents != 0){
	//printf("\t Lead B");
	  lB = get_lead(file_descriptor[B]);
	}
	return RecordRotation(lA, lB);

	// printf("\t RotaryEncoder Value: %i\n\r",value());
}

bool RotaryEncoder::RecordRotation(int valA ,int valB)
{   static int encoder_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};     //used to translate the change of position in change of counting value;
		// 1= clockwise -1 = anticlockwise 0= not coded (=error/ missed in between value)
	static int old_position=0;    // shifting the lower bits up 2
	static int old_AB = 0;
	float oldvalue;
	int step; //encoderstep: -1, 0 or +1
	encoder_position= (int)(valA<<1)+ valB;
	if (old_position==encoder_position)
		return false;
	old_position=encoder_position;
	old_AB <<= 2;                   //remember previous state
	old_AB |= ( encoder_position & 0x03 );  //add current state: four bits of code
	step=encoder_states[( old_AB & 0x0f )];
	rotary_value+= step;
	//if (rotary_value>max)rotary_value=max;
	if(current_menu_item==MENU_INFO)
		OLEDinfo_showoffset+=step;
	if(current_menu_item==MENU_WEATERFORECAST)
	{	if (newhavenOLED_mutex_is_unlocked)
		{	forecast_interval+=step;
	    	if (forecast_interval>=forecast_intervals) forecast_interval=0;
	    	if (forecast_interval<0) forecast_interval=forecast_intervals-1;
		}
	    else return false;
	}
	if (regulated_value!=NIL)
	{	pthread_mutex_lock( &rotary_encoder_mutex);
		oldvalue=*regulated_value;
	    *regulated_value+=value_step*step;
		if (*regulated_value>reg_max ||*regulated_value<reg_min)
			*regulated_value=oldvalue;
		pthread_mutex_unlock( &rotary_encoder_mutex);
	}
	seconds_after_keypress=0;
	return true;
}
//====================================================================//
//            Class ADC :reading the analog > digital converters      //
//  ADC properties 12 bits 125 ns sampletime  -- 1.8 V (do not exeed!)//
//   2 uA current flow 1.8V reference on P9 pin 32                    //
//====================================================================//

const int ADC::P9_Pin_Nrs[]	=		{39,40,37,38,33,36,35};
unsigned int nr_adc__s=0;

ADC::ADC(unsigned int ad,float (*pf) (float))
{  // if (pf==NULL)
	// 	 gauge_function=NullGauge;
	analog_in_value=0;
	analog_voltage=0;
	ifp_ain=(FILE*)NIL;
	signal_name=(char*)NIL;
	gpio_nr=-1;
//	string overlay="cape-bone-iio";
	ad_mean.Init(10,7);
	gauge_function=pf;
	pin_nr= P9_Pin_Nrs[ad];
    //GetFileHandle(ad);
    assert( ad<=NR_ADCS);
    ad_nr=ad;
//    if (::nr_adc__s==0)    //then export the adcvalues
//    	export_overlay(overlay);   //function checks if overlay is present; if not present oberlay is experted
    // printf("%s%i",SYS_ADC_FILE,ad_nr);
    sprintf(value_filename,"%s%i",SYS_ADC_FILE,ad_nr);
    ::nr_adc__s++;
        //printf (value_filename);
}

//ADC::ADC(Header_name nm, unsigned char pinnr)
//{ ad_nr=-1; analog_in_value=0;analog_voltage=0;gauge_function=NIL;gpio_nr=-1;}

ADC::~ADC()
{
    ::nr_adc__s--;
}

void ADC::GetFileHandle_(unsigned int ad)
{
}

float ADC::voltage_value()   			//gets the value of the pin
{ //for (int i=0;i<=0;i++){
	ifp_ain = fopen(value_filename, "r");
	if (ifp_ain == NULL)
		   printf("Unable to open file for adport %i .\n",ad_nr);
	fseek(ifp_ain, 0, SEEK_SET);
	fscanf(ifp_ain, "%f", &analog_in_value);
	fclose(ifp_ain);   ///See http://osdir.com/ml/beagleboard/2013-09/msg01698.html
	//wait 50 ms second read    see the above link about the problem: no solution foud for the problem
		//if (i==0) usleep(100000);
	//}
	ad_mean.Add(analog_in_value/1000.0);
	return (analog_in_value/1000.0);
}

float ADC::Read()   			//gets the value of the pin
{   return 	gauge_function(voltage_value());

}
//=======================================================
//=======================================================
bool export_overlay(string& device_overlay_name) {
    //char overlay[]="DM-GPIO-Bram";
	//first check whether the overlay is already present in the slots file
	ifstream 	 file;
	string 		 line;
	unsigned int offset;
	bool 		notfound=true;
	file.open (slotsfilename );
	if (!file.is_open() )
	{   perror("Write overlay error: slots file does not exist\r\n");
	    return false;
	}
	while(!file.eof())
	{	getline(file,line);
		if ((offset = line.find(device_overlay_name,(string::size_type) 0)) != string::npos)  //found
		{     notfound=false;
		      //printf("Overlay found:%s",device_overlay_name.c_str());
		}
	}
	file.close();

	if (notfound)
	{	ofstream outfile;
	    //printf("Overlay NOT found:%s",device_overlay_name.c_str());
		outfile.open (slotsfilename);
		outfile.write(device_overlay_name.c_str(),(int)device_overlay_name.length());
		if (file.bad())
		{   perror ("Write overlay error\r\n");
	    	outfile.close();
	    	return false;
		}
		else
		{	sprintf (log_string,"Overlay '%s' is written",device_overlay_name.c_str());
		    log_cl(log_string);
		}
		outfile.close();
	}
	cout<<"Overlay \""<< device_overlay_name<<"\" is written to Beaglebone."<<endl;
	return true;
}
/*bool overlay_was_echoed(char* device_overlay_name)
{   ifstream 	file;
	string 		line;
	int 		offset;
	bool 		found=false;
	file.open ("/sys/devices/bone_capemgr.8/slots");
	while(!file.eof())
	{	getline(file,line);
		if ((offset = line.find(device_overlay_name, 0)) != string::npos)
				found=true;
	}
	file.close();
	return found;
}*/
// ***************************************************************
// * devicetree loaded?
// ****************************************************************
bool gpio_devicetree_present(char *treename)
{	char slots[MAX_BUF];
	size_t pos;
	string line;
	snprintf(slots, sizeof(slots),slotsfilename);
	if (file_exists(slots))
	{   ifstream inputstream(slots);
		while(inputstream.good())
		{	pos=0;
			getline(inputstream,line); // get line from file
			pos=line.find(treename); // search
			if (pos != string::npos) // string::npos is returned if string is not found
			{	//cout << "Overlay found" << endl;
				return true;
			}
	    }
	}
	else
		cout << "Slots file not found!" << endl;
	return false;
}



float NullGauge(float in)
{   return in;
}

float TempGauge(float in)
{  //float uit;
   //uit=in+3.34;
   //uit=uit*50-273.13;
   return (in+3.34)*50-273.13;
}

float TempGauge_KTY10_7Wheat(float dV)  //Gauge for temperature sensor Honeywell 777 bèta
{  float R_Temp_Sensor,R0,R1,R2,R3,Vcc,T; //R1 en R2 in serie; R3 en Rts parallel hieraan
                                     //+ aan verbindingspunt   R3 en Rts
                                     //- aan verbindingspunt   R1 en R2
   Vcc=5;
   R1= 1000; //ohm   //parallelResistor in series with R2 opposite to R3
   R2=  947.5;       //parallelResistor in series with R1 opposite to R_Temp_Sensor
   R3= 1000;         //Resistor in series with R_Temp_Sensor
   R0= 1000;  //ohm	  	//Resistance at zero degrees
   float R_vervTemp_SensorenPar	=	R3/((1/(dV/Vcc+R2/(R1+R2)))-1);
   float Rpar=20000;
   R_Temp_Sensor=1/(1/R_vervTemp_SensorenPar-1/Rpar);
   float A= 3.908E-3;			//per degree
   float B= -5.775E-7;			//per degree square
   //float C= -4.183E-12;			//per cubic degree
   float determinant=  4*B*R_Temp_Sensor/R0+A*A-4*B;
   return determinant>0 ? T=(sqrt(determinant)-A)/(2*B) :0 ;
}





float TempGauge_HEL777Wheat(float dV)  //Gauge for temperature sensor Honeywell 777 bèta
													//Wheatstone bridge
{ float R_Temp_Sensor,R0,R1,Vcc,T; //R1 en R2 in serie; R3 en Rts parallel hieraan
                                     //+ aan verbindingspunt   R3 en Rts
                                     //- aan verbindingspunt   R1 en R2
   Vcc=5;
   //float DeltaV=dV;  //test
   R1=   1000; //ohm   //Resistor in series with R_Temp_Sensor
   //Rin= 22100; //input resistance parallel to te to R_Temp_Sensor
   R0=   1000; //ohm	  	//Resistance Temp_Sensor at zero degrees
   dV=dV+2.44;
   float R_vervTemp_SensorenPar	=	(Vcc-dV)*R1/dV;
   float Rpar=8000;
   R_Temp_Sensor=1/(1/R_vervTemp_SensorenPar-1/Rpar);
   float A= 3.908E-3;			//per degree
   float B= -5.775E-7;			//per degree square
   //float C= -4.183E-12;			//per cubic degree
   float determinant=  4*B*R_Temp_Sensor/R0+A*A-4*B;
   T = determinant>0 ?(sqrt(determinant)-A)/(2*B) :0 ;
   return T;
}



