//============================================================================
// Name        :  NefitMainModule.cpp
// Author      :  Bram van Leeuwen
// Version     :
// Copyright   : Bram van Leeuwen
// Description : testing various functions
//============================================================================


#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstdlib>
#include <errno.h>
#include <pthread.h>

#include "TCPTest.hpp"
#include "GlobalIncludes.hpp"
#include "leds.hpp"
#include "kbdio.hpp"
#include "GPIO.hpp"
//#include "ADT7420 Tempsensor.hpp"
#include "TMP102Tempsensor.hpp"
//#include "MyI2C.hpp"
#include "TaskManager.hpp"
#include "Tcp.hpp"              // my own TCP header
#include "LcdDisplay.h"

#include <sstream>    			// include necessary for spi oled display
#include "CharacterDisplay.hpp" // include necessary for spi oled display
using namespace SPIBus_Device;  // include necessary for spi oled display
#include "Sensors.hpp"
#include "Adafruit_GFX.h"
#include "BBB_OLED.h"
#include "Display_Page.hpp"
#include "json.hpp"

static void toggle(BeagleBone::led *ledio);
static void make_blink(BeagleBone::led *ledio);

#define PORT 		0x0FA0
                             //TCP server is listening  on port 4000 (decimal)
#define DIRSIZE 	8192

#include <sys/time.h>
#include <iostream>
#include <unistd.h>    ///only? necessary for function usleep
#include <fstream>

using namespace std;
using namespace gpio_namespace;

#define NUM_THREADS     		9
#define ADAFRUIT_I2C_ADDRESS	0x3D /* 011110+SA0+RW - 0x3C or 0x3D */

struct thread_data{
   int   thread_id;
   char  *message;
   float *floatingpoint;
};
const string DAY[]={"Zo","Ma","Di","Wo","Do","Vr","Za"};

string              controller_name,device_overlay,weather_hostserver,api_call;
RotaryEncoder  		*rotary_encoder=NULL;                   //enables global access of rotary encoder value
Outport				LEDP83(P9,14,(char*)"Led op breadbrd");
Outport				Boiler(P9,13,(char*)"Boiler");
Outport				O3(P9,11,(char*)"Output3");
#ifdef M_OSSO
Outport				O3(P9,11,OUTPUT_PIN,(char*)"Output3");
Outport				Relais1(P9,12,(char*)"mOsso Relais1");   /Resetpin AdafruitOled
Outport				Relais2(P9,15,(char*)"mOsso Relais2");
Outport				Relais3(P9,23,(char*)"mOsso Relais3");
Outport				Relais4(P8, 9,(char*)"mOsso Relais4");
Outport*			mosso_relais[4];
Inport				OptoCoupler1(P8,15,(char*)"mOsso input1",false);//to test connect pin 1 with ground value changes from true to false (high to low)
Inport				OptoCoupler2(P8,11,(char*)"mOsso input2",false);
Inport				OptoCoupler3(P8,14,(char*)"mOsso input3",true);
Inport				OptoCoupler4(P8,12,(char*)"mOsso input4",true);
Inport*				mosso_input[4];
#endif
Inport              	PushbuttonL(P8,12,(char*)"PushButtonL",true);    		//debounced button Left
Inport              	PushbuttonR(P8,13,(char*)"PushButtonL",true);    		//debounced button Right
Inport              	PushbuttonRot(P8,14,(char*)"RotaryPushButton",true);    //debounced button on Rotary encoder
Comparators				comparators;
NefitSchedules			nefitschedules;
TaskSchedules			schedules;
TaskManager 			taskmanager;
TcpMessagerClient		tcpclient;
TcpClient				tcp_easy_client;
OLEDCharacterDisplay* 	OLEDdisplay;
BBB_OLED* 				adafruitdisplay;
json					weather,weatherforecast; //weatherinfo from openweather
json_array				*weatherforecasts;
int 					forecast_interval;
unsigned int   			forecast_intervals;
ADT7420_Tempsensor 			temperature_sensor(1, 0x49);
TMP102_Tempsensor   		temperature_sensor2(1,0x48);
TemperatureSensorEasy 		easyRoomTemp;
TemperatureSensorOutside 	outsideTemp;
TemperatureSensorIntern 	internTemp;
WaterPressureSensor  		pressuresensor;
AirPressureSensor			airpressuresensor;
HumiditySensor              humiditysensor;

char 				tcpserver_outgoing_message2[1024];
char                log_string[80];

pthread_mutex_t 	adc_mutex,temperature_sensorADT7420_mutex,rotary_encoder_mutex,clienttcp_mutex,newhavenOLED_mutex,adafruitOLED_mutex,
						adafruitOLED2_mutex,nefiteasy_mutex,weather_mutex;
pthread_cond_t		adafruit_refresh,weatherthread_hasread;
bool                newhavenOLED_mutex_is_unlocked,adafruitOLED_mutex_islocked,adafruitOledMustRefresh=true;
ADCs   				adcs;

bool 		 		keepThreadsRunning,logging=true;
unsigned int 		nrRunningThreads;
int                 tcpserver_internal_portnr,client_tcpsocket,client_mess_tcpsocket,tcp_ip_port;
sockaddr			pin;
bool				tcpserver_connected;  //true when tcp socket is bound to external socket

MENUITEM			current_menu_item;
unsigned int		current_submenu;
int					OLEDinfo_showoffset;
unsigned int	    seconds_after_keypress;

string localhost	= "127.0.0.1";

float				temp_target,temp_target_last_shown,time_target,time_target_last_shown,time_start_target,time_start_target_last_shown,ok_answer;

extern char slotsfilename[];//= SLOTS_FILE;  //  "/sys/devices/bone_capemgr.9/slots";
string beaufort_scaleS[13];
float  beaufort_scaleV[13];
extern string wind_dir[17];

Outports			outports;//does not own the outports!
Inports             inports; //does not own !

void   PrintHelp();
void   ReadIniFile();
//int    LCD();
void   OLEDTime();
//void   OLEDTemperature();
//void   *NewhavenOLEDThread(void*);
void   *AdafruitOLEDThread(void*);

void *PrintHello(void *threadarg)
{
  struct thread_data *my_data;
  my_data = (struct thread_data *) threadarg;
  sprintf(log_string, "Thread ID : %i  Message : %s", my_data->thread_id ,my_data->message);
  log_cl(log_string);
  nrRunningThreads--;
  pthread_exit(NULL);
  return NULL;
}

void *SendAnalogThread(void *)
{ // sends analog values every 0,5 s to the tcp peer (client) computer
  struct timeval tim1,tim2;
  int dt;

  //my_data = (struct thread_data *) threadarg;
  log_cl((char*) "Analog thread is started up.\r");
  while (keepThreadsRunning) {
	if (tcpserver_connected)
	{	gettimeofday(&tim1, NULL);
	 	tcpclient.SendAnalog();
	 	gettimeofday(&tim2, NULL);
	 	dt=(tim2.tv_sec-tim1.tv_sec)*1000000+tim2.tv_usec-tim1.tv_usec; //time needed to axexute this task
	 	//printf("dt= %i\n\r",dt);
	 	usleep(500000-dt);  // cycle time 0,5 s = 500000 us
	}
  }
  nrRunningThreads--;
  pthread_exit(NULL);
  return NULL;
}

void *SensorUpdateThread(void *)
{ bool changedEt,changedIt,changedOt,changedPr;
  log_cl((char*) "Sensor update thread is started up.\r");
  unsigned int counter=0;
  while (keepThreadsRunning) {
	  pthread_mutex_lock( &nefiteasy_mutex);
	  changedEt=easyRoomTemp.Read();
	  changedIt=internTemp.Read(); //measure inside temperature from ADT7420;if changed show
	  if (changedEt || changedIt){
		  newhavenOLED_mutex_is_unlocked=false;
		  pthread_mutex_lock( &newhavenOLED_mutex);
		  if (changedEt) easyRoomTemp.Show();
	      if (changedIt) internTemp.Show();
	      pthread_mutex_unlock( &newhavenOLED_mutex);
	      newhavenOLED_mutex_is_unlocked=true;
	  }
	  if (!counter%6){   //every minute pressure and outside temperature
		  changedOt = outsideTemp.Read();  //if changed
		  changedPr = pressuresensor.Read();  //if changed
		  if (changedOt || changedPr){
		  	  newhavenOLED_mutex_is_unlocked=false;
		  	  pthread_mutex_lock( &newhavenOLED_mutex);
		  	  if (changedOt) outsideTemp.Show();
		  	  if (changedPr) pressuresensor.Show();
		  	  pthread_mutex_unlock( &newhavenOLED_mutex);
		  	  newhavenOLED_mutex_is_unlocked=true;
		  }
	  }
	  pthread_mutex_unlock( &nefiteasy_mutex);
	  seconds_after_keypress+=10;
	  sleep(20);   //20 seconds sleep
	  counter++;

  }
  nrRunningThreads--;
  pthread_exit(NULL);
  return NULL;
}

void *WeatherInfoThread(void *json_weather_ptr)
{  int     portno   = 80;
	//const char    *host    = const_cast<char*>("api.openweathermap.org");
	const char    *message = const_cast<char*>("GET /data/2.5/weather?q=Haarlem,nl&appid=0149cbfdb15146551afc827547a74956 \r\n\r\n");
	struct  hostent *weatherserver;
	struct  sockaddr_in serv_addr;
	int     sockfd, bytes, sent, received, total;
	char    resp[4096];
	json    *response;
	airpressuresensor.SetJSon(&weather);
	humiditysensor.SetJSon(&weather);
	response=(json*)json_weather_ptr;

	//lookup the ip address
	//weatherserver = gethostbyname(host);//(weather_hostserver.c_str());
	//printf("Server:\n%s\n",weather_hostserver.c_str());//api_call.c_str())

	weatherserver = gethostbyname(weather_hostserver.c_str());
	if (weatherserver == NULL) error("ERROR openweathermap.org not found");
	//	 fill in the structure //
	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	memcpy(&serv_addr.sin_addr.s_addr,weatherserver->h_addr,weatherserver->h_length);
	// fill in the parameters /
	log_cl((char*) "Weatherinfo thread is started up.\n\r");
	while (keepThreadsRunning) {
		//create a socket
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) error("ERROR opening socket");
		// connect the socket //
		if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
							error("ERROR connecting openweathermap");
		// send the request //
		total = strlen(message);
		sent = 0;
		do {	bytes = write(sockfd,message+sent,total-sent);
				if (bytes < 0)
		        error("ERROR writing message to socket");
				if (bytes == 0)
		        break;
				sent+=bytes;
		}
		while (sent < total);

		//receive the response //
		memset(resp,0,sizeof(resp));
		total = sizeof(resp)-1;
		received = 0;
		do {	bytes = read(sockfd,resp+received,total-received);
				if (bytes < 0)
		        error("ERROR reading response from socket");
				if (bytes == 0)
		        break;
				received+=bytes;
		} while (received < total);

		if (received == total)
				error("ERROR storing complete response from socket");
		//	 close the socket //
		close(sockfd);
		//	 process response //
		pthread_mutex_lock( &weather_mutex);
		printf("Response:\n%s\n",resp);
		response->read(resp);
		cout << endl <<"\r"<< "base: "<< response->stringvalue("base") << endl <<"\r" ;
		cout <<"cod: "<< response->numbervalue("cod") << endl<<"\r" ;
		json mainOpenWeather(response->subobject("main"));
		cout<< "OpenW temperature: " << mainOpenWeather.numbervalue("temp")-273.15 << " degC"<< endl<<"\r" ;
		cout<< "OpenW pressure   : " << mainOpenWeather.numbervalue("pressure")    << " mbar"<< endl<<"\r" ;

		pthread_mutex_unlock( &weather_mutex);
		pthread_cond_signal(&weatherthread_hasread);
	    sleep(1800);   //sleep half an hour
  }
  nrRunningThreads--;
	pthread_exit(NULL);
  return NULL;
}

void *RotaryEncoderThread(void *threadarg)
{ struct thread_data *my_data;
  my_data = (struct thread_data *) threadarg;
  RotaryEncoder rotaryencoder(P8,16,17);
  rotary_encoder = &rotaryencoder;
  log_cl( (char*)"Rotary encoder thread\n\r");
  rotaryencoder.CoupleFloat(my_data->floatingpoint,0.1);

  pthread_mutex_lock( &rotary_encoder_mutex);
  pthread_cond_wait(&weatherthread_hasread,&rotary_encoder_mutex);
  pthread_mutex_unlock( &rotary_encoder_mutex);
  usleep(300000); //Give the weatherinfo time to load weatherinformation

  while (keepThreadsRunning){
  {		if (rotaryencoder.RecordEncoderRotation())
  			if(newhavenOLED_mutex_is_unlocked && (current_menu_item==MENU_INFO || current_menu_item==MENU_WEATERFORECAST))
  				RefreshNewhavenOLED();
  			if (current_menu_item==MENU_SET_ACTUAL_TEMP)
  				RefreshAdafruitOLED();//==	pthread_cond_signal(&adafruit_refresh);
  		}
  }
  printf("\n\r");
  nrRunningThreads--;
  log_cl((char*)"Rotaryencoderthread is closed\n\r");
  pthread_exit(NULL);
  return NULL;
}

void *TaskManagerThread(void*)
{ while (keepThreadsRunning)
	  taskmanager.ManageTasks();
  nrRunningThreads--;
  pthread_exit(NULL);
  return NULL;
}

void *PushButtonThread(void *inputport)
{   bool key_changed=false,key_ispressed=false;
    Inport *button;
    button = (Inport*)inputport;
    struct timespec t;
    t.tv_nsec=(long)5E6;   //5 ms sleeptime
    t.tv_sec=0;
    while (keepThreadsRunning)
    {   button->debounceSwitch(&key_changed,&key_ispressed);
    	if (key_changed)
        {   pthread_mutex_lock(&(button->key_mutex));
        	button->debouncedKeyPress= key_ispressed;
        	button->action_to_take=((button->Keytype()==taskmanager_namespace::pressbutton) && key_ispressed==LOW)?false:key_changed;
            if (button->action_to_take)
            	button->ToggleSwitch();
            pthread_mutex_unlock(&(button->key_mutex));
        }
        nanosleep(&t, NULL);//delay_ms(5);  //check every 5 ms
    }
    nrRunningThreads--;
    pthread_exit(NULL);
    return NULL;
}


void RefreshNewhavenOLED()
{   bool enabled;
	unsigned int row,maxrow=6;
    int i=0;
    char str[23];
    string st;
    row=abs((int)((OLEDinfo_showoffset+i++)%maxrow));
    newhavenOLED_mutex_is_unlocked=false;
    pthread_mutex_lock( &newhavenOLED_mutex);

    OLEDdisplay->clear();
    enabled=(current_menu_item==MENU_INFO);
    pressuresensor.display[CHARACTER_OLED].enabled	= enabled;
    outsideTemp. display[CHARACTER_OLED].enabled	= enabled;
  	easyRoomTemp.display[CHARACTER_OLED].enabled	= enabled;
    internTemp  .display[CHARACTER_OLED].enabled	= enabled;
    switch((int)current_menu_item)
    { case MENU_INFO:
    	easyRoomTemp.display[CHARACTER_OLED].col		= 10;
    	easyRoomTemp.display[CHARACTER_OLED].enabled=(enabled=row<4);
    	if (row<4){
    	  	OLEDdisplay->setCursorRC(row,0);
    	  	OLEDdisplay->print("Kamer");
    		easyRoomTemp.display[CHARACTER_OLED].row= row;
    		easyRoomTemp.Show();
    	}
    	row=abs(int((OLEDinfo_showoffset+i++)%maxrow));
    	internTemp.display[CHARACTER_OLED].enabled=(enabled=row<4);
    	if (enabled){
    	  	OLEDdisplay->setCursorRC(row,0);
    	    internTemp.display[CHARACTER_OLED].row= row;
    		OLEDdisplay->print("Intern");
    		//internTemp.Read();
    		internTemp.Show();
    	}

    	row=abs((int)((OLEDinfo_showoffset+i++)%maxrow));
    	outsideTemp.display[CHARACTER_OLED].enabled=(row<4);
    	if (row<4){
    		OLEDdisplay->setCursorRC(row,0);
    		OLEDdisplay->print("Buiten");
    		outsideTemp.display[CHARACTER_OLED].row= row;
    		outsideTemp.Show();
    	}

    	row=abs((int)((OLEDinfo_showoffset+i++)%maxrow));
    	pressuresensor.display[CHARACTER_OLED].enabled=(row<4);
    	if (row<4){
    		OLEDdisplay->setCursorRC(row,0);
    		OLEDdisplay->print("Waterdruk");
    		pressuresensor.display[CHARACTER_OLED].row= row;
    		pressuresensor.Show();
    	}

    	row=abs((int)((OLEDinfo_showoffset+i++)%maxrow));
    	enabled=row<4;
    	airpressuresensor.display[CHARACTER_OLED].enabled=(row<4);
    	if (enabled){
    		   OLEDdisplay->setCursorRC(row,0);
    		   OLEDdisplay->print("Luchtdruk           ");
    		   airpressuresensor.display[CHARACTER_OLED].row= row;
    		   airpressuresensor.Read();
    		   airpressuresensor.Show();
    	}
//#ifdef TEST
    	row=abs(int((OLEDinfo_showoffset+i++)%maxrow));
    	enabled=row<4;
    	humiditysensor.display[CHARACTER_OLED].enabled=(row<4);
    	if (enabled){
    		OLEDdisplay->setCursorRC(row,0);
    		OLEDdisplay->print("Vochtighd.          ");
    		humiditysensor.display[CHARACTER_OLED].row= row;
    		humiditysensor.Read();
    		humiditysensor.Show();
    	}
//#endif
    	break;
        case MENU_SET_ACTUAL_TEMP:
        	easyRoomTemp.display[CHARACTER_OLED].row		= 0;
      		easyRoomTemp.display[CHARACTER_OLED].col		= 14;
      		easyRoomTemp.display[CHARACTER_OLED].enabled	= true;
      		OLEDdisplay->clear();                   // Clear the character LCD module
      		OLEDdisplay->home ();                   // Move the cursor to the (0,0) position
      		OLEDdisplay->print("Gemeten:         "); // String to display on the first row
      		easyRoomTemp.Show();
      		temp_target=temp_target_last_shown;
      		sprintf(str,"Streefwaarde: %i,%i%cC ",(int)temp_target,(int)(temp_target*10)%10,DEGREE_EUR_FONT);  //char(248)= degree
      		OLEDdisplay->setCursorRC(1,0);
      		OLEDdisplay->print(str);
      		OLEDdisplay->setCursorRC(2,0);
      		time_target=time_target_last_shown;
      		sprintf(str,"Tot  : %02i.%02i uur",(((int)time_target)/6),((int)(time_target)%6*10));
      		OLEDdisplay->print(str);
      		OLEDdisplay->setCursorRC(3,0);
      		time_start_target=time_start_target_last_shown;
      		sprintf(str,"Vanaf: %02i.%02i uur",(((int)time_start_target)/6),((int)(time_start_target)%6*10));
      		OLEDdisplay->print(str);

      	break;
      	case MENU_PROGRAM:
      	    OLEDdisplay->clear();                   // Clear the character LCD module
      		OLEDdisplay->home();                    // Move the cursor to the (0,0) position
      		OLEDdisplay->print("Stel programma in   "); // String to display on the first row
      		OLEDdisplay->setCursorRC(1,0);
      		OLEDdisplay->print("Begintijd:          ");
      		OLEDdisplay->setCursorRC(2,0);
      		OLEDdisplay->print("Tot:                ");
      		OLEDdisplay->setCursorRC(3,0);
      		OLEDdisplay->print("Temperatuur:        ");

      	break;
  //    	case MENU_NEFITPROGRAM:
  //    		OLEDdisplay->clear();
  //    		OLEDdisplay->home();                    // Move the cursor to the (0,0) position

  //      break;

      	case MENU_WEATERFORECAST:
          float velocity,winddirection;
          int speed;
      	  if (forecast_intervals==0 ||forecast_interval==0)
      		 ReadWeatherforecast();
      		OLEDdisplay->home();                    // Move the cursor to the (0,0) position
      		//printf("forecast_interval%i",forecast_interval);
      		json weathernow(weatherforecasts->at(forecast_interval));
      		//st=weathernow.stringvalue("dt_txt");
      		//st.substr(11,5);
      		time_t timefc=weathernow.numbervalue("dt");
      		tm * timeinfo;
      		timeinfo=localtime(&timefc);
      		OLEDdisplay->print(DAY[timeinfo->tm_wday]);//day fill in
      		OLEDdisplay->print(" ");
      		json values(weathernow.subobject("main"));
      		OLEDdisplay->print(weathernow.stringvalue("dt_txt").substr(11,5));
      		sprintf(str,"%3.1f%cC",values.numbervalue("temp")-273.15 ,DEGREE_EUR_FONT);
      		OLEDdisplay->setCursorRC(0,14);
      		OLEDdisplay->print(str);
      		OLEDdisplay->setCursorRC(1,0);
      		sprintf(str,"%4.0fmbar ",values.numbervalue("pressure"));
      		OLEDdisplay->print(str);
      			json wind=weathernow.subobject("wind");
      		velocity=wind.numbervalue("speed");
      		i=0;
      		while (beaufort_scaleV[i]<velocity && i<12)
      			i++;
      		OLEDdisplay->setCursorRC(2,0);OLEDdisplay->print(beaufort_scaleS[i]);
     // 		velocity=beaufort_scaleV[2];
      //		printf("%2.2fm/s",beaufort_scaleV[2]);
      		if (velocity <9,5)
            	sprintf(str,"%1.0fm/s",velocity);
            else
            	sprintf(str,"%2.0fm/s",velocity);
      		if (i<5 ||(i>6 && i<11)) OLEDdisplay->print(str);
      		i=((int)((wind.numbervalue("deg")+11.25)/22.5))%17;
      		OLEDdisplay->setCursorRC(2,17);OLEDdisplay->print(wind_dir[i]);
      		//OLEDdisplay->setCursorRC(2,0);
      		OLEDdisplay->setCursorRC(1,15);
      		sprintf(str,"%3.0f%\%",values.numbervalue("humidity"));
      		OLEDdisplay->print(str);
      		//cout           << "temperatuur: " << values.numbervalue("temp")-273.15  << endl<<"\r";
      		//cout           << "druk: " << values.numbervalue("pressure") << "mbar " << endl<<"\r";
      		json_array	ar(weathernow.arrayvalue("weather"));
      	    json a(ar[0]);
      		OLEDdisplay->setCursorRC(3,0);
      		OLEDdisplay->print(a.stringvalue("description"));
      		break;

      }
    usleep(50000);

    pthread_mutex_unlock( &newhavenOLED_mutex);
    newhavenOLED_mutex_is_unlocked=true;

}

//*void RefreshAdafruitOLED()
//{  pthread_mutex_lock( &adafruitOLED_mutex);
//    pthread_cond_signal(&adafruit_refresh);
    //pthread_mutex_unlock( &adafruitOLED_mutex);
//}

void *AdafruitOLEDThread(void *threadarg)
{   char str[11];
	bool ok;
	while (!adafruitOLED_mutex_islocked && keepThreadsRunning)
    {   adafruitOLED_mutex_islocked=true;
        pthread_mutex_lock( &adafruitOLED_mutex);
    	pthread_cond_wait(&adafruit_refresh,&adafruitOLED_mutex);
        adafruitdisplay->clearDisplay();

        switch(current_menu_item)
        { case MENU_INFO:
              break;
          case MENU_IDLE:
              break;
          case MENU_SET_ACTUAL_TEMP:
          {	  adafruitdisplay->setTextSize(2);
          	  adafruitdisplay->setTextColor(WHITE);
          	  adafruitdisplay->setCursorRC(0,0);
    	  	  switch (current_submenu){
    		   	   case 0:
    		   		   adafruitdisplay->print("Streeftemp");
    		   		   adafruitdisplay->setCursorXY(0,24);
    		   		   sprintf(str,"%i,%i",(int)temp_target,(int)(temp_target*10)%10);
    		   		   temp_target_last_shown=temp_target;
    		   		   adafruitdisplay->print(str);
    		   		   adafruitdisplay->setCursorXY(50,24); //was 52
    		   		   sprintf(str,"%cC",248);  //char(248)= degree
    		   		   adafruitdisplay->print(str);
    		   	   break;
    		   	   case 1:
    		   		   adafruitdisplay->print("Eindtijd:");
    		   		   adafruitdisplay->setCursorXY(0,24);
    		   		   sprintf(str,"%02i.%02i uur",(((int)time_target)/6),(int)(time_target)%6*10);
    		   		   adafruitdisplay->print(str);
    		   		   time_target_last_shown=time_target;
    		   	   break;
    		   	   case 2:
    		   	    	adafruitdisplay->print("Begintijd");
     		   		   adafruitdisplay->setCursorXY(0,24);
     		   		   sprintf(str,"%02i.%02i uur",(((int)time_start_target)/6),(int)(time_start_target)%6*10);
     		   		   adafruitdisplay->print(str);
     		   		   time_start_target_last_shown=time_start_target;
    		   	   break;
    		   	   case 3:
    		   		    ok=(ok_answer<3);
    		   		    adafruitdisplay->setTextColor(ok?BLACK:WHITE,ok?WHITE:BLACK) ;
    		   	    	adafruitdisplay->print("    OK    ");
    		   	    	adafruitdisplay->setCursorRC(1,0);
    		   	    	adafruitdisplay->setTextColor(ok?WHITE:BLACK,ok?BLACK:WHITE) ;
    		   	    	adafruitdisplay->print(" Annuleer ");
    		   	   break;
    	  	  }
          }
          break;
          case MENU_PROGRAM:
        	    adafruitdisplay->setTextSize(2);
              	adafruitdisplay->setTextColor(WHITE);
              	adafruitdisplay->setCursorRC(0,0);
              	adafruitdisplay->print("Begin");
              	adafruitdisplay->setCursorXY(22,26);
              	adafruitdisplay->print("vandaag");
              	adafruitdisplay->drawRect(16, 20, 96, 32, WHITE);
           break;
          case NR_MENUITEMS:
          break;
        }
    	adafruitdisplay->display();

    	pthread_mutex_unlock( &adafruitOLED_mutex);
    	adafruitOLED_mutex_islocked=false;
    }
    pthread_exit(NULL);
    return NULL;
}


int main (int nrarg,char *arg[])
{   pthread_t threads[NUM_THREADS];
    string strg;
    string overlay,overlayad;


	Comparator *roomcomparator; //,*boilercomparator,*comp3;
//	unsigned char reg;
	char 	*dt, str[30];//,str2[30];
	int 	rc, i, nr, adc_nr,taskmanager_thread_nr;
	NefitTime nefitt;
//	bool    mOssoRelais	= true;
	overlay				= "BB-SPIDEV0";
	current_menu_item	= MENU_INFO;
    export_overlay(overlay);  //Overlay for OLED at SPI-port
    adafruitOLED_mutex_islocked=false;
    newhavenOLED_mutex_is_unlocked=true;
    if (port_is_free(4008))     //nefit-easy is not loaded yet
    {   printf("****tcp server \"nefit-easy\" is not ready****\n\r****  program aborted  ***\n\r");
    	exit(1);
    }
    pthread_mutex_init(&adafruitOLED_mutex,NULL);
    pthread_cond_init(&adafruit_refresh,NULL);
    pthread_mutex_init(&rotary_encoder_mutex,NULL);
    pthread_cond_init(&weatherthread_hasread,NULL);
    pthread_mutex_init(&nefiteasy_mutex,NULL);
    //DisplayPage oled_pagina(DISPLAY_NEWHAVENOLED);
    weatherforecasts=NULL;
    forecast_intervals=0;
    seconds_after_keypress=0;
    WindscaleInit();
    forecast_interval=0;
	//tcp_easy_client.Open(localhost,4008);
	//if (!tcp_easy_client.connected)
	 //  system("./nefit-easy < /dev/null &> /dev/null &");
    //	&> file redirects stdout and stderr to file
     //sleep(3);
	//Setup Newhaven Oled 20x4 character display
		//opening spi bus for the spi controlled OLed display via the 74HC595 serial to parallel conversion
		SPIDevice *busDevice = new SPIDevice(1,0); //Using first SPI bus (both loaded)
		busDevice->setSpeed(1000000);      // Have access to SPI Device object
		OLEDCharacterDisplay display(busDevice, 20 , 4); // Construct 20x4 LCD Display
		newhavenOLED_mutex_is_unlocked=false;
		pthread_mutex_lock( &newhavenOLED_mutex);

		OLEDdisplay=&display;
		OLEDdisplay->clear();                   // Clear the character LCD module
		OLEDdisplay->home();
		// Move the cursor to the (0,0) position
		OLEDdisplay->print("Home controller"); // String to display on the first row
		pthread_mutex_unlock( &newhavenOLED_mutex);
		OLEDTime();   //kas his own locking mechanism
		newhavenOLED_mutex_is_unlocked=true;
	// End setup Newhaven Oled 20x4 character display
	// Setup Adafruit Oled 128 x 64 graphic display
	BBB_OLED displaygr(1,ADAFRUIT_I2C_ADDRESS); // bus /dev/ic-1 P9_20 and P9_19  I2c address 3d hex
	adafruitdisplay=&displaygr;
	adafruitdisplay->init(OLED_I2C_RESET);
	adafruitdisplay->begin();
	adafruitdisplay->clearDisplay();
	//DisplayPage oled_pagina2(DISPLAY_ADAFRUITOLED);
	// End setup Adafruit Oled 128 x 64 graphic display
	//BBB_OLED adafruitdisplay2(1,ADAFRUIT_I2C_ADDRESS); // bus /dev/ic-1 P9_20 and P9_19  I2c address 3d hex
	//serialib serialLCDdisp();
	//BBB_OLED adafruitdisplay(1,ADAFRUIT_I2C_ADDRESS); // bus /dev/ic-1 P9_20 and P9_19  I2c address 3d hex
	//adafruitdisplay.init(OLED_I2C_RESET);
	//LCD();
#ifdef M_OSSO
	mosso_relais[0]=&Relais2;mosso_relais[1]=&Relais2;mosso_relais[2]=&Relais3;mosso_relais[3]=&Relais4;
	mosso_input[0]=&OptoCoupler1;mosso_input[1]=&OptoCoupler2;mosso_input[2]=&OptoCoupler3;mosso_input[3]=&OptoCoupler4;
#endif
	for (i=1;i<nrarg;i++)
	{	 if (strcmp(arg[i],"-l")==0)
		       logging =true;
	}
	adapt_slots_file_name_to_distro();
	ReadIniFile();
	overlay=device_overlay;
    export_overlay(overlay);
    overlayad="cape-bone-iio";
    export_overlay(overlayad);
	//strg="Kamertemperatuurregeling";
	roomcomparator  = new Comparator(&LEDP83,&temperature_sensor, 23.5, 20,strg="Kamerthermostaat");
	//boilercomparator= new Comparator(&Boiler,&temperature_sensor, 23.5, 80,strg="Boilerthermostaat");
	//comp3= new Comparator(&O3,&temperature_sensor, 20, 20,strg="Thermostaat3");
//	b2=new Comparator(&Boiler,&temperature_sensor, 70, 80,strg="Boilerthermosteet");
	outports.Add(LEDP83,roomcomparator);
	//outports.Add(Boiler,boilercomparator);
	//outports.Add(O3,comp3);

//	boilercomparator->state=low_treshold;
//	roomcomparator->state=low_treshold;
//	comp3->state=low_treshold;
	tcpserver_internal_portnr=-1;   //notdefined yet serverthread sets portnummer on creating the internal tcp/ip port
	tcpserver_connected=false;
	//int     ax,ay,az;  //acceleration values from BMA180
	time_t 	now ;
	timeval now_tv;
	char device_overlay2[]= "BL-GPIO-I2CT";
	TaskSchedule *tsksched;
	string schstr("Nieuwe taak");
	//tsksched = new TaskSchedule(schstr,now, repeat_daily);
	//tsksched->SetName(schstr);
	//schedules.Add(tsksched);
	bool dayss[7]={true,false,true,false,true,false,true};
	tsksched = new TaskSchedule(repeating_weekly,  dayss, comparator_high_tr, roomcomparator,strg="Eerste taak");
	schedules.Add(tsksched);
	//dayss[0]=false; dayss[1]=true;
	//tsksched = new TaskSchedule(repeating_weekly,  dayss, comparator_low_tr, boilercomparator,strg="Tweede taak");
	//schedules.Add(tsksched);

	//printf("%s\n\r",tsksched->Name());
	//bool duplo;
    string mess;
    string address;

		   //Planning: On the first message of the client time is send, answer by my name (name of host)
		       	   //timeformat yy/mm/dd hh:mm:ss w Day:0041520  days are  since 1/1/1900 00:00:00
		       	   // convert now to tm struct for UTC

		   			//   now,tzp);

           for (i=0;i<NR_ADCS;i++)    //form array of analog digital converters
        	   adcs.Add(new ADC(i));
		   struct thread_data 	td[NUM_THREADS];


		   keepThreadsRunning= true;
           nrRunningThreads  = 0;
           //OLEDTemperature();
		   for( i=0; i < NUM_THREADS; i++ )
		   {  //printf("main() : creating thread,%i \n\r " ,i);
		     td[i].thread_id = i;
		     td[i].message   =(char*) "Started up";
		     nrRunningThreads++;
		     switch (i)
		     {	case 0: rc = pthread_create(&threads[i], NULL,	TaskManagerThread,	NULL);
		      	  	  	taskmanager_thread_nr=i;
		        	    break;
		        case 1: td[1].floatingpoint= &(roomcomparator->high_treshold_top);
		        	    rc = pthread_create(&threads[i], NULL,	RotaryEncoderThread,(void *)&td[i]);
		               //wait with coupling of floats until rotarybthread is constucted fully
		        	    break;
		        case 2: rc = pthread_create(&threads[i], NULL,	TcpServerThread,	NULL);
		                break;
		        //case 3: rc = pthread_create(&threads[i], NULL,  SendAnalogThread,	NULL);
		       // 		break;
		        case 3: rc=  pthread_create(&threads[i], NULL,  PushButtonThread,	(void *)&PushbuttonL);
		                break;
		        case 4: rc=  pthread_create(&threads[i], NULL,  PushButtonThread,	(void *)&PushbuttonR);
		        		break;
		        case 5: rc=  pthread_create(&threads[i], NULL,  PushButtonThread,	(void *)&PushbuttonRot);
		        		 break;
		        case 6:
		        		rc = pthread_create(&threads[i], NULL, WeatherInfoThread,  (void *)&weather);
		        		 break;
		        case 7:	rc = pthread_create(&threads[i], NULL,  SensorUpdateThread,	NULL);
		                				break;
		        case 8: rc=  pthread_create(&threads[i], NULL,  AdafruitOLEDThread,	NULL);
		                 break;
		  		  	//		printf("AF Oled thread() Error:unable to create thread \n\r");
		        default:rc = pthread_create(&threads[i], NULL,  PrintHello,			(void *)&td[i]);
		        		break;
		      }
		      if (rc){
		    	 printf("main() : Error:unable to create thread,%i \n\r", rc );
		         exit(-1);
		      }
		   }
      BeagleBone::led *ledio[4];
	  char c;

	  for (int i = 0; i < 4; i++) {
	    ledio[i] = BeagleBone::led::get(i);
	    ledio[i]->off();
	  }
	  //dit werkt op  pin 12 en 14 van P9,
	  //ook op pin 13 en 19 van P8
	  //niet op 3 van P8
	  /*BeagleBone::gpio* Gpio[30];
	  for (i=0;i<1;i++)
	  {   Gpio[i]=BeagleBone::gpio::P9(12+i);
	  	  Gpio[i]->configure(BeagleBone::pin::OUT);
	  }*/
	  //GPIO LEDP83(P9,12,OUTPUT_PIN);
	  // GPIO SCLOCK (P9,BBB_GPIO_SHT1x_SCK,OUTPUT_PIN);   //defined in module SHT75.cpp
	  	  // GPIO DATA   (P9,BBB_GPIO_SHT1x_DATA,OUTPUT_PIN);  //defined in module SHT75.cpp

	  	  //LEDP83->configure(BeagleBone::pin::OUT);
	  //try 			{rotary_encoder->CoupleFloat(&(roomcomparator->high_treshold_top),0.1);}
	  //catch(...) 	{perror("Totary thread not fully constructed");}
	  log_cl((char*)"Coupled");
      printf ("     Hit 'h' or '?' to get help. 'q' to quit\n");

	  while ((c = BeagleBone::kbdio::getch()) != 'q') {
	  	 switch (c) {
	  	 case '1':
	     case '2':
	     case '3':
	     case '4':
	  	          toggle(ledio[c - '1']);
	  	          break;
//	     case '5':
//	    	      Comparator *comp;
//	    	      comp=LEDP83.GetComparator();
//	  	          if (comp->enabled){
//	  	        	  comp->enabled=false;
//	  	        	  LEDP83.high();
//				    printf("Disables comparator and sets led on P9.12\n\r");
//	  	          }
//	  	          else{
//	  	          	if (LEDP83.value()==HIGH){
//	  	          		LEDP83.low();
//	  	          	    printf("Resets led on P9.12 (comparator is Disabled) \n\r");
//	  	          	}
//					else{
//						comp->enabled=true;
//						printf("Enables comparator on P9.12\n\r");
//					}
//	  	          }
//	  	          break;


	  	 case '!':
	          make_blink(ledio[0]);
	          break;

	      case '@':
	          make_blink(ledio[1]);
	          break;

	      case '#':
	          make_blink(ledio[2]);
	          break;

	      case '$':
	          make_blink(ledio[3]);
	          break;
	      case 'a':
	    	  printf ("Which adc will we read? (a= all f=full set five times repeated 1x");
	    	  c = BeagleBone::kbdio::getch();
	    	  adc_nr= (int)(c-'0');
	    	  if (adc_nr>=0 && adc_nr<=7)
	    	  {   ADC adc(adc_nr);
	    		  printf ("how many times? (1=1 2=4 3=10)\n\r");
	    		  c = BeagleBone::kbdio::getch();
	    	      nr=0;
	    	      printf("reading: \n\r");
	    	      if (c=='1') nr=1;
	    	      if (c=='2') nr=4;
	    	      if (c=='3') nr=10;
	    	      for (i=0; i<nr; i++)
	    	      { printf("     ADC%i value: %f\n\r",adc_nr,adc.voltage_value());
	    	        usleep(50000);  //sleep 0.05 second == 50,000 microseconds
	    	      }
	    	  }
	    	  if (c=='f')
	    		for (int k=0;k<=1;k++){
	    		  for (i=0;i<=7;i++)  //full test
	    		  {   ADC adc(i);
	    		      printf("\n\r 2 ADC%i values: %f ",i,adc.voltage_value());
	    		  	  for (int j=0; j<5; j++)
	    		  	  {  printf("nr %i voltage=%f ",adc_nr,adc.voltage_value());
	    		  	     usleep(20000);  //sleep 0.02 second == 20,000 microseconds
	    		  	  }
	    			  if (i==7) printf("\n\r");
	    		  }
	    		}
	    	  if (c=='g')
	    	    for (int k=0;k<=5;k++)
	    		  {	  for (i=0;i<=1;i++)
	    		  	  {   ADC adc(i);
	    		  	      printf("\n\r 2 ADC%i values: %f ",i,adc.voltage_value());
	    		  	  	  for (int j=0; j<5; j++)
	    		  	  	  {  printf("adcnr: %i %f ",adc_nr,adc.voltage_value());
	    		  	  	     usleep(300000);  //sleep 0.3 second == 20,000 microseconds
	    		  	  	  }
	    		  		  if (i==1) printf("\n\r");
	    		  	  }
	    		  }
	    	  if (c=='a')
	    	   for (i=0;i<=7;i++)
    		  	 {   ADC adc(i);
    			  	 printf("     ADC%i value: %f\n\r",i,adc.voltage_value());
    		  	 }
	    	  break;
	      case 'A':
	    	    tcpclient.analog_to_send[0]=true;  //send temperature
	    	    tcpclient.AnalogToClientMessage();
	    	    tcpclient.Send();
	    	  break;
	      case 'c':
	    	  // ComparatorToTcpData_buf(tcpserver_outgoing_message2,0);
	    	    //printf(tcpserver_outgoing_message2);
	    	    break;
	      case 'C':
	    	    tcpclient.SendOutportStatus();
	    	  break;

	      case 'd':
	    	  get_distro(strg);
	    	  adapt_slots_file_name_to_distro();
	    	  cout<<" Linux distro:"<< strg<<";-- Slots file:"<< slotsfilename <<endl;

	    	  break;

	      case 'h':
	      case '?':
	    	  PrintHelp();
	    	  break;
	      case 'I':
	      	  printf ("Which register will we read? (1 t/m B supported)");
	      	  //duplo =true;
	      	  c = BeagleBone::kbdio::getch();
	      	  //reg= (int)(c-'0');
	      	  if (c>64)
	      	   	  { //duplo =false;
	      	   	    //reg= 10+(int)(c-'A');
	      	   	  }
	     /* 	   	  if (duplo)
	      	   		  tempsensor2.Read_Multi_Byte(reg,12);
	      	    	 else
	      	    	  tempsensor2.Read_I2C_Byte(reg);
	      	    	 printf( "\n\r");
	      	    	 for (i=0;i<12;i++)
	      	    		  	printf("%2x ",tempsensor2.I2C_RD_Buf[i]);
	      	    	 printf( "\n\r");*/
	      	         break;

	      	       case 'i':
	                     //int t=1;
	                     //printf("Bitscount %i %i %i %i %i %i\n\r",bitsCount(0),bitsCount(1),bitsCount(2),bitsCount(3),bitsCount(4),bitsCount(5));
	         	      if (PushbuttonRot.value()==HIGH)
	      	      	  		  	printf("HIGH\n\r");
	      	      	 else printf( "LOW\n\r");
	      	      	 break;
	      case 'l':
	    	  logging=!logging;
	    	  if (logging)
	    		  log_cl((char*)"Logging to commandline switched on.");
	    	  break;
#ifdef M_OSSO
	      case 'm':
	      	  if (mOssoRelais)
	      		  printf("Give mOsso command 1234 relais i read inputs");
	      	  c = BeagleBone::kbdio::getch();
	      	  if (c>='1' && c<='4')	      		  //switch relais
	      	       mosso_relais[c-'1']->toggle();
	      	  if (c=='i' || c=='I'){
	      		  printf("--optocoupler values are: ");
	      		  for (i=0;i<4;i++){
	      			 if (mosso_input[i]->value()==HIGH) printf ("true ");//input info
	      			 else printf ("false ");
	      			 if (i<3)printf (";");
	      		  }
	      	     printf("\n\r");
	      	  }
	      	  mOssoRelais=false;
	      	  break;
#endif
	      	      case 'n':
	      	    schedules[0]->NextTask();
	      	    printf("Next task (first taskschedule).\n\r");
	      	    break;
	      case 'o':
	      	    export_overlay(overlay=device_overlay);
	      	    cout<<"Overlay \""<< overlay<<"\"is written to Beaglebone.\n\r"<<endl;
	      	    break;
	      case 'O':
                export_overlay(overlay=device_overlay2);
                cout<<"Overlay2 \""<< overlay<<"\"is written to Beaglebone.\n\r"<<endl;
                break;
	      case 'p':
	    	  nefitschedules.Read();
	    	  //strg="Bram";
	    	  //oled_pagina.line[1]->EditLine(strg);
	    	  //oled_pagina.Display();
	    	  //oled_pagina2.line[1]->EditLine(strg);
	    	  //oled_pagina2.Display();
	    	    break;
	      //case 'q'
	      case 'r':
	    	  printf("Rotary encoder value: %i.\n\r",rotary_encoder->value());
	      	  break;
	      /*case 's':
	    	  LCD();

		  	  break;*/

	      case 'T':
	      	  dt=str;
	      	  gettimeofday(&now_tv,NULL);
	      	  now= now_tv.tv_sec;
	      	  dt = ctime(&now);
	      	  printf(  "Local date and time: Day: %i %s",weekday(now),dt) ;
              nefitt.convert_tm(*localtime(&now));
	      	  time_t sun,lastsun;
	      	  sun=nefitt;
	      	  dt = ctime(&sun);
	      	  printf(  "Local date and time: Day: %i %s",weekday(sun),dt) ;
	      	  lastsun=nefitt.LastSunday();
	      	   dt = ctime(&lastsun);
	      	  printf(  "Last Sunday: %i %s",weekday(lastsun),dt) ;
	      		      	  //time_to_tcpstring(str2, now);
	      	  //printf("Tcp: %s",str2);
	            break;
	      case 't':
	    	  if (temperature_sensor2.connected()){   //readout of TMP102
	    		  temperature_sensor2.readSensorState();
	    		  temperature_sensor2.temp_to_string();
	    		  printf("%s \u2103  ",temperature_sensor2.tempstring);
	    		  printf("%6.3f deg)\n\r",temperature_sensor2.temperature());
	    	  }
	    	  if (temperature_sensor.connected()){   //readout of ADT4720
	    		  temperature_sensor.readSensorState();
	    		  temperature_sensor.temp_to_string();
	    		  printf("%s \u2103   ",temperature_sensor.tempstring);
	    		  printf("(%7.4f deg) \n\r",temperature_sensor.temperature());
	    	  }
	    	 // OLEDTemperature();
	          break;

	      case 'x':
	      	   /* 	  printf("Setting up the I2C Display output!\n");
	      	    	  if(!adafruitdisplay2.init(OLED_I2C_RESET)){
	      	    	        perror("Failed to set up the display\n");
	      	    	        return -1;
	      	    	   }*/
	      	    	  adafruitdisplay->clearDisplay();
	      	    	  adafruitdisplay->setTextSize(1);
	      	    	  adafruitdisplay->setTextColor(WHITE);
	      	    	  adafruitdisplay->setCursorRC(0,0);
	      	    	  adafruitdisplay->print("Informatie:1");
	      	    	  adafruitdisplay->setCursorRC(1,0);
	      	    	  adafruitdisplay->setTextColor(BLACK,WHITE);  //text color: black on white
	      	    	  adafruitdisplay->print("Temperat.2:\n");
	      	    	  adafruitdisplay->setTextColor(WHITE);//text color: white on whiteblack
	      	    	  adafruitdisplay->print("Ketelinformatie:3");
	      	    	  adafruitdisplay->setCursorXY(0,24);
	      	    	  adafruitdisplay->print("Ketelinformatie:4");
	      	    	  adafruitdisplay->setCursorXY(0,32);
	      	    	  adafruitdisplay->print("Weergrootheden5:");
	      	    	  adafruitdisplay->setCursorXY(0,40);
	      	    	  adafruitdisplay->print("Instellingen6:");
	      	    	  adafruitdisplay->setCursorXY(0,48);
	      		      adafruitdisplay->print("123456789012345678901234567890");
	      	    	  adafruitdisplay->setCursorXY(0,10);
	      	    	  adafruitdisplay->drawRect(16, 32, 96, 32, WHITE);
	      	    	  adafruitdisplay->display();

	      	      	    break;

	      case 'z':
	      	      	   /* 	  printf("Setting up the I2C Display output!\n");
	      	      	    	  if(!adafruitdisplay2.init(OLED_I2C_RESET)){
	      	      	    	        perror("Failed to set up the display\n");
	      	      	    	        return -1;
	      	      	    	   }*/
	      	      	    	  adafruitdisplay->clearDisplay();
	      	      	    	  adafruitdisplay->setTextSize(1);
	      	      	    	  adafruitdisplay->setTextColor(WHITE);
	      	      	    	  adafruitdisplay->setCursorRC(0,0);
	      	      	    	  adafruitdisplay->print("Informatie:1");
	      	      	    	  adafruitdisplay->setCursorRC(1,0);
	      	      	    	  adafruitdisplay->setTextColor(BLACK,WHITE);  //text color: black on white
	      	      	    	  adafruitdisplay->print("Temperat.2:\n");
	      	      	    	  adafruitdisplay->setTextColor(WHITE);//text color: white on whiteblack
	      	      	    	  adafruitdisplay->display();


	      	      	      	    break;

	      }
	    }
	  if (tcpserver_connected)
		    tcpclient.Send("Quit");
	  if (tcp_easy_client.connected)
		  tcp_easy_client.Close();
	  sleep(1);  //wait one second
	  keepThreadsRunning=false;
	  BeagleBone::led::restore_defaults();
	  cout << "Joining threads with main: ";
	  pthread_join(threads[taskmanager_thread_nr],NULL);
	  cout<< "Taskmanager thread is joined.\r"<<endl;
	  adafruitdisplay->close();
	  return 1;
}

void toggle(BeagleBone::led *ledio)
{
  ledio->set_trigger(BeagleBone::led::NONE);
  if (ledio->is_on())
	  ledio->off();
  else
	  ledio->on();
}

void make_blink(BeagleBone::led *ledio)
{
  if (!ledio->is_on())
	  ledio->on();
  ledio->timer(250, 250);
}

int weekday(time_t& t)
{   return (localtime(&t))->tm_wday;
}

int bitsCount(unsigned x)
 {  int b;
	for (b = 0; x != 0; x >>= 1)
	 if (x & 01)
	     b++;
	return b;
 }

int delay_ms(unsigned int msec)
{
  int ret;
  struct timespec a;
  if (msec>999)
  {
    fprintf(stderr, "delay_ms error: delay value needs to be less than 1000\n");
    msec=999;
  }
  a.tv_nsec=((long)(msec))*1E6;
  a.tv_sec=0;
  if ((ret = nanosleep(&a, NULL)) != 0)
  {
    fprintf(stderr, "delay_ms error: %s\n", strerror(errno));
  }
  return(0);
}

void error(const char *msg)
{ perror(msg);
  exit(0);
}
void log_cl(char* str)
{  if (logging)
	cout << str << endl;
}

void PrintHelp()
{  printf("**************************************************************\n");
   printf("Hit '1-4' switch LED on board. Hit SHIFT/1-4 to make it blink\n");
   printf("    'a#'  read adc-input number #\n");
   printf("    'B'   read acceleration and temperature  \n");
   printf("		      from I2C bus 1 with BMA180 accelerometer\n");
   printf("    'C/c' set reset clock pin on P9 pin 17\n");
   printf("    'D/d' get the name of the Linux distribution\n");
   printf("    'l'   toggle logging to command line\n");
   printf("    'o'   write 12c overlay 'O' for device overlay\n");
   printf("    'm'   mOsso command\n");
   printf("    'n'   next scheduled timepoint (first schedule)\n");
   printf("    'r'   read rotary value\n");
   printf("    's'   test serial Led on P9_21\n");
   printf("    't'   temperature from ADT7420 and TMP102\n");
   printf("    'T'   time \n");
   printf("    'q'   quit.\n");

}

void ReadIniFile()
{	char buffer[MAX_BUF];
    //bool found=true;
	snprintf(buffer, sizeof(buffer),"BeagleWhisperer.ini");
	//string str2,str3;
	if (file_exists(buffer))
	{  ifstream inputstream(buffer);
	    string key;
	    //ReadIniString(key="IP_PEER",host_tcpserver, inputstream) ;
	   	ReadIniString(key="DEVICE_OVERLAY",	 device_overlay,  inputstream) ;
	   	ReadIniString(key="CONTROLLER_NAME"	,controller_name, inputstream);
	    ReadIniString(key="WEATHER_SERVER" 	,weather_hostserver,  inputstream);
	    ReadIniString(key="API_CALL"		,api_call, 		  inputstream);

	    printf("Gelezen3:%s\n\r",api_call.c_str());
	    tcp_ip_port      = ReadIniInt(key="LISTEN_PORT",	  inputstream);
	   // printf("Listen port:%i",listenport);
	}
	else
		cout << "Ini file not found!" << endl;
}

bool ReadIniString(string& key, string& result, ifstream& inputstream){
   size_t pos;
   string::size_type l,eq_pos;
   string line;
   while(inputstream.good()){
       pos=0;
   	   getline(inputstream,line); 				// get line from file
   	   line.erase(line.find_first_of(";"));   	//comment, begins with semicolon
   	   pos=line.find(key); 						// search
   	   eq_pos=line.find_first_of('=');
   	   if ((pos != string::npos ) && (eq_pos!=string::npos) ){ // string::npos is returned if string is not found
   		   l=line.find_first_of('"');
   	       if(l==string::npos)  				//quotation not found
   	        	 line=line.substr(eq_pos+1,string::npos);
   	       else {
   	        	line=line.substr((l+1),string::npos);
   	        	line.erase(line.find_first_of('"'));   //skip on next quotation
   	       }
   	       result=line.erase( line.find_last_not_of(' ')+1);  //skip trailing spaces
   	        //cout<<"line:"<<line<<"**result:**"<<result<<"**\"" <<endl;
   	       return true;
   	   }
    }
    //cout << "Key \" "<< key<< "\"not found not in ini file."<< endl;
    return false;
}

int  ReadIniInt(string& key, ifstream& inputstream)
{   string result;
    ReadIniString(key, result, inputstream);
    return atoi(result.c_str());
}


/*
int LCD()
{
    serialib LS;                                                            // Object of the serialib class
    int Ret;
    // Used for return values
    string overlay="BB-UART2";  //uses only the TDX pin from UART2 P9_21
    //char Buffer[128];
    export_overlay(overlay);
    // Open serial port
    Ret=LS.Open(DEVICE_PORT,9600);                                        // Open serial link at 115200 bauds
    if (Ret!=1) {                                                           // If an error occured...
        printf ("LCD(): Error while opening serial LCD port. Permission problem ?\n");        // ... display a message ...
        return Ret;                                                         // ... quit the application
    }
    printf ("Serial LCD port opened successfully..,");
    usleep(600);
    LS.WriteChar(254); // move cursor to beginning of first line
    LS.WriteChar(128);
    LS.WriteString("                "); // clear display
    LS.WriteString("                ");
    LS.WriteChar(254); // move cursor to beginning of first line
    LS.WriteChar(128);

    LS.WriteString("Hello here Bram!");
    usleep(900000);  //wait 0,9s
    LS.WriteChar(254);

    LS.WriteChar(128);
    LS.WriteString("again           ");

    // Close the connection with the device

    LS.Close();
    printf ("and is closed again !\n");
    return 0;
}*/

void   OLEDTime()
{ char 	*dt, str[30];
  time_t 	now ;
  timeval now_tv;
  dt=str;
  gettimeofday(&now_tv,NULL);
  now= now_tv.tv_sec;
  dt = ctime(&now);
  dt[20]=0;
  newhavenOLED_mutex_is_unlocked=false;
  pthread_mutex_lock( &newhavenOLED_mutex);
  OLEDdisplay->setCursorRC(1,0);
  OLEDdisplay->print(dt);
  pthread_mutex_unlock( &newhavenOLED_mutex);
  newhavenOLED_mutex_is_unlocked=true;
}

/*void   OLEDTemperature()
{ char 	str[10];
  if (temperature_sensor.connected()){
	temperature_sensor.readSensorState();
	temperature_sensor.temp_to_string();

	sprintf(str,"%s",temperature_sensor.tempstring);
	//sprintf(str,"%6.3f",temperature_sensor.temperature());
  }
  newhavenOLED_mutex_is_unlocked=false;
  pthread_mutex_lock( &newhavenOLED_mutex);
  OLEDdisplay->setCursorRC(2,0);
  OLEDdisplay->print(str);
  OLEDdisplay->write(DEGREE_EUR_FONT);
  OLEDdisplay->write('C');
  pthread_mutex_unlock( &newhavenOLED_mutex);
  newhavenOLED_mutex_is_unlocked=true;
}*/

Translation::Translation(string e, string d)
{ en=e; du=d;
}

Translations::Translations(){
	::sivector<Translation>();
	Add (new Translation("clear sky","onbewolkt"));
	Add (new Translation("clear sky","onbewolkt"));
}

string Translations::Translate(std::string en)
{   Translation* tr=new Translation("clear sky","");
	NextItemNr(tr);
}

void WindscaleInit(){
	beaufort_scaleV[0]=  0.03; beaufort_scaleS[ 0]="windstil    ";
	beaufort_scaleV[1]=  1.55; beaufort_scaleS[ 1]="zwakke wind ";
	beaufort_scaleV[2]=  3.35; beaufort_scaleS[ 2]="zwakke wind ";
	beaufort_scaleV[3]=  5.45; beaufort_scaleS[ 3]="matige wind ";
	beaufort_scaleV[4]=  7.95; beaufort_scaleS[ 4]="matige wind ";
	beaufort_scaleV[5]= 10.75; beaufort_scaleS[ 5]="vrij krachtige w";
	beaufort_scaleV[6]= 13.85; beaufort_scaleS[ 6]="krachtige wind  ";
	beaufort_scaleV[7]= 17.15; beaufort_scaleS[ 7]="harde  wind ";
	beaufort_scaleV[8]= 20.75; beaufort_scaleS[ 8]="stormachtig ";
	beaufort_scaleV[9]= 24.45; beaufort_scaleS[ 9]="storm       ";
	beaufort_scaleV[10]=28.45; beaufort_scaleS[10]="zware storm ";
	beaufort_scaleV[11]=32.65; beaufort_scaleS[11]="zeer zware storm";
	beaufort_scaleV[12]=10000; beaufort_scaleS[12]="orkaan      ";
	wind_dir[0] =" N  ";
	wind_dir[1] ="NNO";
	wind_dir[2] =" NO";
	wind_dir[3] ="ONO";
	wind_dir[4] =" O ";
	wind_dir[5] ="OZO";
	wind_dir[6] =" ZO";
	wind_dir[7] ="ZZ0";
	wind_dir[8] =" Z ";
	wind_dir[9] ="ZZW";
	wind_dir[10]=" ZW";
	wind_dir[11]="WZW";
	wind_dir[12]=" W ";
	wind_dir[13]="WWN";
	wind_dir[14]=" NW";
	wind_dir[15]="NNW";
	wind_dir[16]=" N ";
}

