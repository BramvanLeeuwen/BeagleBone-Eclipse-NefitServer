/*
 * Sensors.cpp

 *
 *  Created on: Jul 5, 2017
 *      Author: bram
 */
#include <iostream> // std::cout
#include <string>   // std::string, std::to_string
#include <sstream>
#include <iomanip>
#include <locale>


#include "Sensors.hpp"
#include "CharacterDisplay.hpp"
#include "Tcp.hpp"

extern SPIBus_Device::OLEDCharacterDisplay* OLEDdisplay;
//extern bool  				newhavenOLED_mutex_is_unlocked;
//extern pthread_mutex_t 		newhavenOLED_mutex;
extern TcpClient			tcp_easy_client;
extern int          		client_tcpsocket;
extern string				localhost;
extern ADT7420_Tempsensor 	temperature_sensor;
extern pthread_mutex_t 		weather_mutex;

template <typename T>
string ToString(T val){
    stringstream stream;
    stream << val;
    return stream.str();
}

SenseAndDisplay::SenseAndDisplay(){
	value 				=-99.9;
	for (int i=0;i<2;i++)
	{	display[i].row			= 0;
		display[i].col			= 10;
		display[i].decimals 	= 1;
		display[i].minus_sign	= false;
		display[i].enabled		= false;
		display[i].displaystring="*void*";
	}
}


void SenseAndDisplay::SetDisplay(enum OLEDDISPLAY d,unsigned int r, unsigned int c){
	display[d].row		= r;
	display[d].col		= c;
	display[d].enabled	= true;
}

void SenseAndDisplay::Show(){
  if (display[CHARACTER_OLED].enabled || display[GRAPHICAL_OLED].enabled){
	stringstream strstr;
	strstr << fixed << showpoint << setprecision(1) << value;
	string str=strstr.str();
	for (unsigned int i=1;i<str.length();i++)
		if (str[i]=='.'){
			str[i]=',';
			break;
	}
	if (display[CHARACTER_OLED].enabled){// && newhavenOLED_mutex_is_unlocked){
			OLEDdisplay->setCursorRC(display[CHARACTER_OLED].row, 14);
			OLEDdisplay->print("     ");//clean last 5 characters in the row
		    OLEDdisplay->setCursorRC(display[CHARACTER_OLED].row, display[CHARACTER_OLED].col);
			display[CHARACTER_OLED].displaystring=str;
			OLEDdisplay->print(display[CHARACTER_OLED].displaystring);
		}
		if (display[GRAPHICAL_OLED].enabled)
			display[GRAPHICAL_OLED].displaystring=str;
  }
}

void SenseAndDisplay::ShowUnit(){
	if (display[CHARACTER_OLED].enabled){//} && newhavenOLED_mutex_is_unlocked){
		OLEDdisplay->write(DEGREE_EUR_FONT);
		OLEDdisplay->write('C');
	}
}

TemperatureSensorEasy ::TemperatureSensorEasy(){
	display[CHARACTER_OLED].row=  0;
	display[CHARACTER_OLED].col= 10;
	display[CHARACTER_OLED].enabled=true;
	display[GRAPHICAL_OLED].enabled=false;
}

bool TemperatureSensorEasy ::Read(){
	char server_reply[40];
	int nr_bytes;
	if (!tcp_easy_client.connected)
		tcp_easy_client.Open(localhost,4008);
	tcp_easy_client.Send("Tin");
	nr_bytes=recv(client_tcpsocket,server_reply,40,0);
	if (nr_bytes<0)
	    printf("no reply om easy easy temperature request");
	else
		server_reply[nr_bytes]=0;
	//printf("Nr_bytes= %i Reply= %s",nr_bytes,server_reply);
	float oldvalue=value;
	if (server_reply[20]==':' && server_reply[21]==' '){
		value=atof(server_reply+22);
	}
	return oldvalue!=value;
}

void TemperatureSensorEasy ::Show(){
	SenseAndDisplay::Show();
	ShowUnit();
}

TemperatureSensorIntern ::TemperatureSensorIntern(){
	display[CHARACTER_OLED].row= 1;
	display[CHARACTER_OLED].col= 10;
	display[CHARACTER_OLED].enabled=true;
	display[GRAPHICAL_OLED].enabled=false;
}

bool TemperatureSensorIntern ::Read(){
	float oldvalue=value;
	temperature_sensor.readSensorState();
	value=temperature_sensor.temperature();
	return oldvalue!=value;
}

void TemperatureSensorIntern ::Show(){
	temperature_sensor.temp_to_string();
	SenseAndDisplay::Show();
	ShowUnit();
}

AirPressureSensor ::AirPressureSensor(){
	weather=NULL;
	display[CHARACTER_OLED].row= 4;
	display[CHARACTER_OLED].col= 10;
	display[CHARACTER_OLED].decimals= 0;
	display[CHARACTER_OLED].enabled=true;
	display[GRAPHICAL_OLED].enabled=false;
}

void AirPressureSensor ::SetJSon(json* wthr){
	weather=wthr;
}

bool AirPressureSensor ::Read(){
	float oldvalue=value;
	if (weather->loaded())
	{	//pthread_mutex_lock(&weather_mutex);
	    json mainOpenWeather(weather->subobject("main"));
		value= mainOpenWeather.numbervalue("pressure");
		//pthread_mutex_unlock(&weather_mutex);
	}
	return (oldvalue!=value);
}

void AirPressureSensor ::Show(){
	if (weather->loaded())
	{	//json mainOpenWeather(weather->subobject("main"));
		//value= mainOpenWeather.numbervalue("pressure");
		if (display[CHARACTER_OLED].enabled){
			//newhavenOLED_mutex_is_unlocked=false;
			//pthread_mutex_lock( &newhavenOLED_mutex);
			OLEDdisplay->setCursorRC(display[CHARACTER_OLED].row, display[CHARACTER_OLED].col);
			OLEDdisplay->print(display[CHARACTER_OLED].displaystring=ToString(value));
			OLEDdisplay->print(" mbar");

			//pthread_mutex_unlock( &newhavenOLED_mutex);
			//newhavenOLED_mutex_is_unlocked=true;
		}
		if (display[GRAPHICAL_OLED].enabled){
			//OLEDdisplay->setCursorPosition(display[CHARACTER].row, display[CHARACTER].col);
			display[GRAPHICAL_OLED].displaystring=value;
			//OLEDdisplay->print(display[GRAPHICAL].displaystring);
		}
	}
}

HumiditySensor ::HumiditySensor(){
	weather=NULL;
	display[CHARACTER_OLED].row= 5;
	display[CHARACTER_OLED].col= 10;
	display[CHARACTER_OLED].decimals= 0;
	display[CHARACTER_OLED].enabled=true;
	display[GRAPHICAL_OLED].enabled=false;
}
void HumiditySensor ::SetJSon(json* wthr){
	weather=wthr;
}

bool HumiditySensor ::Read(){
	float oldvalue=value;
	if (weather->loaded())
	{	pthread_mutex_lock(&weather_mutex);
        json mainOpenWeather(weather->subobject("main"));
		value= mainOpenWeather.numbervalue("humidity");
		pthread_mutex_unlock(&weather_mutex);
	}
	return oldvalue!=value;
}

void HumiditySensor ::Show(){
	if (weather->loaded())
	{	if (display[CHARACTER_OLED].enabled){
		    //newhavenOLED_mutex_is_unlocked=false;
		    //pthread_mutex_lock( &newhavenOLED_mutex);
			OLEDdisplay->setCursorRC(display[CHARACTER_OLED].row, display[CHARACTER_OLED].col);
			OLEDdisplay->print(display[CHARACTER_OLED].displaystring=ToString(value));
			OLEDdisplay->print("%");
			//pthread_mutex_unlock( &newhavenOLED_mutex);
			//newhavenOLED_mutex_is_unlocked=true;
		}
		if (display[GRAPHICAL_OLED].enabled){
			//OLEDdisplay->setCursorPosition(display[CHARACTER].row, display[CHARACTER].col);
			display[GRAPHICAL_OLED].displaystring=value;
			//OLEDdisplay->print(display[GRAPHICAL].displaystring);
		}
	}
}
TemperatureSensorOutside ::TemperatureSensorOutside(){
	display[CHARACTER_OLED].row= 2;
	display[CHARACTER_OLED].col= 10;
	display[CHARACTER_OLED].enabled=true;
	display[GRAPHICAL_OLED].enabled=false;
}

bool TemperatureSensorOutside ::Read(){
	char server_reply[40];
	int nr_bytes;
	float oldvalue;

	if (!tcp_easy_client.connected)
			tcp_easy_client.Open(localhost,4008);
	tcp_easy_client.Send("Tout");
	nr_bytes=recv(client_tcpsocket,server_reply,40,0);
	if (nr_bytes<0)
		    printf("no reply on outside temperature request");
	else
	     server_reply[nr_bytes]=0;
	//printf("Nr_bytes= %i Reply= %s",nr_bytes,server_reply);
	if (server_reply[21]==':' && server_reply[22]==' ')
			value=atof(server_reply+23);
	return oldvalue!=value;
}

void TemperatureSensorOutside ::Show(){
	SenseAndDisplay::Show();
	ShowUnit();
}

WaterPressureSensor ::WaterPressureSensor(){
	display[CHARACTER_OLED].row=  3;
	display[CHARACTER_OLED].col=  10;
	display[CHARACTER_OLED].enabled=true;
	display[GRAPHICAL_OLED].enabled=false;
}

bool WaterPressureSensor ::Read(){
	char server_reply[30];
	int nr_bytes;
	float oldvalue;
	if (!tcp_easy_client.connected)
		tcp_easy_client.Open(localhost,4008);
	tcp_easy_client.Send("Press");
	nr_bytes=recv(client_tcpsocket,server_reply,30,0);
	if (nr_bytes<0)
	    printf("no press.reply");
	else
		server_reply[nr_bytes]=0;
	//printf("Nr_bytes= %i Reply= %s",nr_bytes,server_reply);
	server_reply[22]=0;
	if (server_reply[13]==':' && server_reply[14]==' ')
		value=atof(server_reply+15);
	return oldvalue!=value;
}

void WaterPressureSensor ::Show(){
	SenseAndDisplay::Show();
	if (display[CHARACTER_OLED].enabled )
		OLEDdisplay->print(" bar");
}
