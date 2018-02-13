/*
 * Weatherforecast.cpp
 *
 *  Created on: Jan 4, 2018
 *      Author: bram
 */

#include "GlobalIncludes.hpp"
#include "Weatherforecast.hpp"
//#include <stdio.h>
#include <sys/socket.h>
//#include <sys/time.h>
//#include <netinet/in.h>
#include <netdb.h>
//#include "kbdio.hpp"

#include <sstream>    			// include necessary for spi oled display
#include <unistd.h>

#include "CharacterDisplay.hpp" // include necessary for spi oled display
#include "json.hpp"

extern string 		weather_hostserver;
extern json 		weatherforecast;
extern json_array	*weatherforecasts;
extern int forecast_intervals;
using namespace std;
using namespace SPIBus_Device;  // include necessary for spi oled display

//ion windforce beaufort_scale[13];

string wind_dir[17];

void ReadWeatherforecast(){
	int     portno   = 80;
	const char    *message = const_cast<char*>("GET /data/2.5/forecast?q=Haarlem,nl&appid=0149cbfdb15146551afc827547a74956&lang=nl \r\n\r\n");
	struct  hostent *weatherserver;
	struct  sockaddr_in serv_addr;
	int     sockfd, bytes, sent, received, total;
	char    resp[16384];
	time_t  now;
	static time_t time_last_call=0;

	time(&now);  // get current time; same as: now = time(NULL)  //
    if (difftime(now,time_last_call)<20*60)   //forecast loaded less than 20 minutes ago
    		return;
	weatherserver = gethostbyname(weather_hostserver.c_str());
	if (weatherserver == NULL) error("ERROR openweathermap.org not found");

	//	 fill in the structure //
	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	memcpy(&serv_addr.sin_addr.s_addr,weatherserver->h_addr,weatherserver->h_length);

	// fill in the parameters //
	log_cl((char*) "reading Weatherforecast\r");
	//create a socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) error("ERROR opening socket");
	// connect the socket     //
	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
		error("ERROR connecting openweathermap");
	// send the request      //
	total = strlen(message);
	sent = 0;
	do {
		bytes = write(sockfd,message+sent,total-sent);
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
	do {
		bytes = read(sockfd,resp+received,total-received);
		if (bytes < 0)
	        error("ERROR reading weather forecast response from socket");
		if (bytes == 0)
	        break;
		received+=bytes;
	} while (received < total);

	if (received == total)
		error("ERROR storing complete response from socket");
	//	 close the socket //
	close(sockfd);
	//	 process response //

	weatherforecast.read(resp);
	struct json_array_s* forecasts;
	forecasts=weatherforecast.arrayvalue("list");
//	if (weatherforecasts!=NULL)
//		delete weatherforecasts;
	weatherforecasts=new json_array(forecasts);
	forecast_intervals=weatherforecasts->size();
/*
    //cout <<endl    << "base: "<< response.stringvalue("base") << endl;
    cout           << "cod: " << weatherforecast.stringvalue("cod")  << endl<<"\r";
    cout           << "cnt: " << weatherforecast.numbervalue("cnt")  << endl<<"\r";

    struct json_array_s* forecasts;
    forecasts=weatherforecast.arrayvalue("list");
    json_array_element_s *today=forecasts->start;
    json todayfc(forecasts->start->value);
    cout << "today: "<< todayfc.numbervalue("dt") << endl<<"\r";
    cout << "today: "<< todayfc.stringvalue("dt_txt") << endl<<"\r";
    json todayfc2(weatherforecast.arrayvalue("list")->start->value);
    cout << "today: "<< todayfc2.numbervalue("dt") << endl<<"\r";
    cout << "today: "<< todayfc2.stringvalue("dt_txt") << endl<<"\r";
    cout << "forecsts length: "<< forecasts->length <<" **- cnt:"<< weatherforecast.numbervalue("cnt")<< endl;
    json todaymain(todayfc2.subobject("main"));
    json_array arr(forecasts);
    json arraylement(arr[1]);
    json todaymain2(arraylement.subobject("main"));
    cout           << "temperatuur: " << todaymain2.numbervalue("temp")-273.15  << endl<<"\r";
    cout           << "druk: " << todaymain2.numbervalue("pressure") << "mbar " << endl<<"\r";
    json arraylement2((new json_array(forecasts))->at(2));
    json todaymain3(arraylement2.subobject("main"));
    cout             << "today: "<< arraylement2.stringvalue("dt_txt") << endl;
    cout           << "temperatuur: " << todaymain3.numbervalue("temp")-273.15  << endl<<"\r";
    cout           << "druk: " << todaymain3.numbervalue("pressure") << "mbar " << endl<<"\r";
    json arraylement4((new json_array(forecasts))->at(3));
    json todaymain4(arraylement4.subobject("main"));
    cout             << "today: "<< arraylement4.stringvalue("dt_txt") << endl<<"\r";
    cout           << "temperatuur: " << todaymain4.numbervalue("temp")-273.15  << endl<<"\r";
    cout           << "druk: " << todaymain4.numbervalue("pressure") << "mbar " << endl<<"\r";
    json arraylement5((new json_array(forecasts))->at(6));
    json todaymain5(arraylement5.subobject("main"));
    cout             << "today: "<< arraylement5.stringvalue("dt_txt") << endl<<"\r";
    cout           << "temperatuur: " << todaymain5.numbervalue("temp")-273.15  << endl<<"\r";
    cout           << "druk: " << todaymain5.numbervalue("pressure") << "mbar " << endl<<"\r";*/
    time_last_call=now;
}
/*
void WindscaleInit(){
	beaufort_scaleV[0]= 0.03;	beaufort_scaleS[0]="windstil        ";
	beaufort_scaleV[1]= 1.55;   beaufort_scaleS[1]="zwakke wind     ";
	beaufort_scaleV[2]= 3.35;   beaufort_scaleS[2]="zwakke wind     ";
	beaufort_scaleV[3]= 5.45;   beaufort_scaleS[3]="matige wind     ";
	beaufort_scaleV[4]= 7.95; 	beaufort_scaleS[4]="matige wind     ";
	beaufort_scaleV[5]=10.75; 	beaufort_scaleS[5]="vrij krachtige w";
	beaufort_scaleV[6]=13.85; 	beaufort_scaleS[6]="krachtige wind  ";
	beaufort_scaleV[7]=17.15; beaufort_scaleS[7]="harde  wind     ";
	beaufort_scaleV[8]=20.75; beaufort_scaleS[8]="stormachtig     ";
	beaufort_scaleV[9]=24.45; beaufort_scaleS[9]="storm           ";
	beaufort_scaleV[10]=28.45; beaufort_scaleS[10]="zware storm    ";
	beaufort_scaleV[11]=32.65; beaufort_scaleS[11]="zeer zware storm";
	beaufort_scaleV[12]=10000; beaufort_scaleS[12]="orkaan         ";
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
}*/


