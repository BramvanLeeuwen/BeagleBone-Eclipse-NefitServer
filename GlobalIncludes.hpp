/*
 *  GlobalIncludes.hpp
 *
 *  Created on: Aug 30, 2013
 *      Author: Bram van Leeuwen
 */

#ifndef GLOBALINCLUDES_HPP_
#define GLOBALINCLUDES_HPP_

#define COMPARATOR_NAMELENGTH	20
#define SCHEDULE_NAMELENGTH 	20
#define NR_OUTPORTS				 4
#define NR_ADCS					 7

#define NIL						(void*)0

#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstring>
#include <string>
#include "SimpleGPIO.hpp"
#include "Vectorclasses.hpp"
#include "ADT7420 Tempsensor.hpp"
#include "Weatherforecast.hpp"
using namespace std;

//#define CONTROLLER_NAME  		"BeagleBone Black\r\n"//now in ini file
//# define CONTROLLER_NAME ="Beaglebone Black/n/r"    //now in ini file
//#define	DEVICE_OVERLAY	 	 	"BvL-GPIO"        //now in ini file

extern char log_string[];             	//log to commandline
extern pthread_cond_t   	adafruit_refresh;

int 	weekday(time_t& t);
int 	bitsCount(unsigned x);
int 	delay_ms(unsigned int msec);
void 	log_cl(char* str);               //log to commandline
void 	error(const char *msg);
bool 	ReadIniString(string& key,string& rest, ifstream& inputstream);
int  	ReadIniInt(string& key, ifstream& inputstream);

void 		RefreshNewhavenOLED();
inline void RefreshAdafruitOLED(){ pthread_cond_signal(&adafruit_refresh);};

enum MENUITEM{
	MENU_INFO,
	MENU_SET_ACTUAL_TEMP,
	MENU_PROGRAM,
	//MENU_NEFITPROGRAM,
	MENU_WEATERFORECAST,
	MENU_IDLE,
	NR_MENUITEMS
} ;



/*
class Translations: public virtual sivector <Translation>
{
	public: Translations(){::sivector<Translation>();};

	string Translate(string&);
};*/

const int 				nr_menuitems=5;
extern MENUITEM			current_menu_item;
extern unsigned int		current_submenu;
const unsigned int 		nr_submenus[] = {0, 4, 4, 0, 0};
extern int				OLEDinfo_showoffset;
extern float			temp_target,temp_target_last_shown,time_target,time_target_last_shown,time_start_target,time_start_target_last_shown,ok_answer;





#endif /* GLOBALINCLUDES_HPP_ */
