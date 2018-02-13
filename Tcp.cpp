/*
 * Tcp.cpp
 * Implements a tcp-server and client class
 *
 * First version created on: Aug 30, 2013
 *      Author: Bram van Leeuwen
 */


#include <stdio.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
//#include "kbdio.hpp"
#include "TCPTest.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#include "TaskManager.hpp"
#include "Tcp.hpp"

#include <sys/time.h>


using namespace std;


char     tcpserver_incoming_message[TCP_MESSBUF_SIZE];  // buffer used for incomming message, and
char     tcpserver_outgoing_message[TCP_MESSBUF_SIZE];  // and buffer for outgoing data
char     tcpclient_incoming_message[TCP_MESSBUF_SIZE];  // buffer used for incomming message
char     tcpclient_outgoing_message[TCP_MESSBUF_SIZE];  // buffer used for incomming message
//test!!!!
//extern char*				tcpserver_outgoing_message2;
//test!!!!
extern string               controller_name;
extern unsigned int 		nrRunningThreads;
//extern bool 				tcp_thread_must_update_outports;
extern ADCs		   			adcs;
extern pthread_mutex_t 	adc_mutex,temperature_sensorADT7420_mutex,rotary_encoder_mutex;//,clienttcp_mutex;
extern ADT7420_Tempsensor 	temperature_sensor;
extern TaskSchedules		schedules;
extern Comparators			comparators;
//extern Outports 				outports;
extern GPIOs 				outports;
extern Inports              inports;
int      					socket_descriptor_current;
extern int					tcpserver_internal_portnr;
extern bool				    tcpserver_connected; //true when tcp socket is bound to external socket
extern bool				    keepThreadsRunning;
extern sockaddr				pin;
extern int                  client_tcpsocket,client_mess_tcpsocket;
extern TcpMessagerClient	tcpclient;
void *TcpServerThread(void *threadarg) //creates a blocking tcp server
//thread lives as long as the main program does not close
{
	int				listening_socket,len_incoming,i;
	unsigned int  	addrlen;
	sockaddr_in 	sockin;
	//sockaddr 		pin;   global variable
	bool			client_did_not_close_connection,bound=false;

	log_cl ((char*)"Tcp Ip Server thread started\n\r");

	listening_socket=  socket(AF_INET,SOCK_STREAM,0);
	if (listening_socket <0)
		printf("Error in opening socket\n\r");

	memset(&sockin, 0, sizeof(sockin));     // complete the socket structure
	sockin.sin_family 		= AF_INET;   	//allows for other network protocols
	sockin.sin_addr.s_addr 	= INADDR_ANY;	//0

	i=0;
	do {      // bind the socket to the port number
		//sockin.sin_port 	= htons(TCP_IP_PORT+i);  //Host TO Network Short :Convert port number to "network byte order" MOST SIGNIFICANT BYTE FIRST
		sockin.sin_port 	= htons(TCP_IP_PORT+i);  //Host TO Network Short :Convert port number to "network byte order" MOST SIGNIFICANT BYTE FIRST
		bound= (bind (listening_socket, (__CONST_SOCKADDR_ARG)&sockin, sizeof(sockin)) == 0);
	}	while (++i<=4 && !bound );
	tcpserver_internal_portnr=TCP_IP_PORT+i-1;
	if (!bound){
		perror("tcp server didn't bind");
		std::exit(1);
	}

	if (listen(listening_socket, 5) == -1){    // show that we are willing to listen
		perror("listen");
		std::exit(1);
	}

    while (keepThreadsRunning)
    { printf("Waiting to accept a client\n\r");
	  addrlen = sizeof(pin);
	  //	wait for a new client to welcome the client
	  socket_descriptor_current= accept(listening_socket,&pin, &addrlen);    //socket_descriptor_current is the actual socket descriptor for communication with this client
	  if (socket_descriptor_current == -1){
	  		perror("accept");
	  		std::exit(1);
	  }
	  // tcpClient.SetSocketDescriptors
	  TcpServer   tcpServer(listening_socket,socket_descriptor_current);
	  int   peerport=((int)(unsigned char)pin.sa_data[0])*256+(int)(unsigned char)pin.sa_data[1];
	  printf("Client accepted, from IP address and portnumber client: %i.%i.%i.%i:%i\n\r",(int)(unsigned char)pin.sa_data[2],(int)(unsigned char)pin.sa_data[3],(int)(unsigned char)pin.sa_data[4],(int)(unsigned char)pin.sa_data[5],peerport);
	  client_did_not_close_connection=true;
	  tcpserver_connected=true;
	  struct sockaddr_in *spin = (struct sockaddr_in *) &pin;
	  in_addr_t serverip=(in_addr_t)spin->sin_addr.s_addr;
	  tcpclient.SetHostSocketAddress(serverip);


      while(client_did_not_close_connection){                                 //while client did not order to close the connection
		  // wait for the client to accept (next) order
		  // get the message from the client
		  printf("Awaiting next order from (peer)client\n\r");
		  len_incoming=recv(socket_descriptor_current, tcpserver_incoming_message, TCP_MESSBUF_SIZE-1, 0);
		  if (len_incoming == -1){
		  		perror("recv");
	  			std::exit(1);
		  }
		  tcpserver_incoming_message[len_incoming]=0;
		  printf("**Message recieved:%s**\n\r",tcpserver_incoming_message);
		  if (tcpServer.strcmp_recieved_tcp_data("Close"))
	  			client_did_not_close_connection=false;
	  		  //client wants to close the socket
	  		  // close up both sockets
		  else
			  // otherwise process the incoming order
			  // process order  return 1 if order is valid and processed
		  {   if (tcpServer.process_socket())
			  	  // return 1 on non valid order
			  	  // answering..: send message
	  				printf("Order processed.\n\r");
	  		  else{
	  		  		printf("Invalid order recieved from client! Length %i.\n\r",len_incoming);
	  		        if (len_incoming== 0){
	  		        	client_did_not_close_connection=false;
	  		            printf("Client disconnected?\n\r");
	  		        }
	  		  }

	  		}
            //printf("1: Test before end while cient didnt close\n\r");

	  	} //end while client did notclose connection
	  	// close up both sockets

	    printf("Connection is closed by client on: %i.%i.%i.%i:%i\n\r",(int)(unsigned char)pin.sa_data[2],(int)(unsigned char)pin.sa_data[3],(int)(unsigned char)pin.sa_data[4],(int)(unsigned char)pin.sa_data[5],peerport);
	    strcpy(tcpserver_outgoing_message,"203 Server socket closed\n\r");
	    if (send(socket_descriptor_current, tcpserver_outgoing_message,strlen(tcpserver_outgoing_message), 0) == -1) {
	    	  	perror("send");
	    	  	std::exit(1);}// give client a chance to properly shutdown
	    tcpserver_connected=false;
	    sleep(1);
	    close(socket_descriptor_current);
	    printf("TCP server socket closed\n\r");
	    tcpserver_incoming_message[0]=0;
	    tcpclient.Close();

	}   //while mainprogram is not closing : awaiting next client
	    //until main thread wants to close
	close(listening_socket);
	tcpclient.Close();
	cout<<"TCP Listening client socket closed; ";
	nrRunningThreads--;
	cout<<"Rotaryencoderthread is closed"<<endl;
	pthread_exit(NULL);
    return NULL;
}

//____________________________________________________________________________________________
//____________________________________________________________________________________________
//____________________________________________________________________________________________
//           ********************* TcpMessagerClient class *********************
//           * specialized TCPClient for messages to the server on the peer PC *
//           * these messages are initiated by the linux device                *
//           * (without request from the peer computer)                        *
//___________*******************************************************************_________________
//____________________________________________________________________________________________
TcpMessagerClient::TcpMessagerClient()
{	connected= false;
	host_portnr= 4016;
	memset(&clientsockin, 0, sizeof(clientsockin));   // complete the socket structure
	clientsockin.sin_family 		= AF_INET;   	  //allows for other network protocols
	clientsockin.sin_addr.s_addr 	= INADDR_ANY;	  //0
	clientsockin.sin_port 			= 0;
	clientsockin.sin_port 			= htonl(INADDR_ANY);
	socketaddressserver_is_set=false;
	socketaddressserver=-1;
	for (int i=0;i<NR_ANALOG_INPUTS;i++)
		analog_to_send[i]=false;
}

TcpMessagerClient::~TcpMessagerClient()
{
}

/*void TcpClient::Open(string& ip_address,unsigned int portnr)  //external address and portnumber
{	in_addr_t socketaddressserver= inet_addr(ip_address.c_str());
	Open(socketaddressserver,portnr);
}*/

void TcpMessagerClient::Open()
{	//sockaddr_in 	clientsockin;
	sockaddr_in     server_address;
	bool			bound=false;

	client_mess_tcpsocket=  socket(AF_INET,SOCK_STREAM,0);
	if ( client_mess_tcpsocket <0)
		printf("Error in opening socket\n\r");
	//else
	//	printf("Client socket is opened\n\r");
	/*memset(&clientsockin, 0, sizeof(clientsockin));   // complete the socket structure
	clientsockin.sin_family 		= AF_INET;   	  //allows for other network protocols
	clientsockin.sin_addr.s_addr 	= INADDR_ANY;	  //0
	clientsockin.sin_port 			= 0;
	clientsockin.sin_port 			= htonl(INADDR_ANY);*/

	bound= (bind (client_mess_tcpsocket, (__CONST_SOCKADDR_ARG)&clientsockin, sizeof(clientsockin)) == 0);
	if (!bound){
		perror("tcp client didn't bind");
		std::exit(1);
	}
	//else
	//	cout<< "BBBs Tcp client is bound\n\r";
    if (!socketaddressserver_is_set)
    	cout<< "Program error: Socketadressserver is not set\n\r";
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr 	=socketaddressserver;

	server_address.sin_port = htons(host_portnr);
    //printf(" connecting with server on ipaddress %s\n\r",socketaddressserver);

	if (connect(client_mess_tcpsocket, (struct sockaddr*) &server_address,sizeof(server_address))< 0){
		perror("\rconnection of client to server on host computer failed");
		std::exit(1);
	}
	//else printf("Client (Beaglebone) is connected to server\n\r");
	connected= true;
}

/*void TcpClient::Open(in_addr_t& socket_addressserver,unsigned int portnr)  //external address and portnumber
{	sockaddr_in 	clientsockin;
	sockaddr_in     server_address;
	bool			bound=false;

	client_tcpsocket=  socket(AF_INET,SOCK_STREAM,0);
	if ( client_tcpsocket <0)
		printf("Error in opening socket\n\r");
	else
		printf("Client socket is opened\n\r");
	memset(&clientsockin, 0, sizeof(clientsockin));   // complete the socket structure
	clientsockin.sin_family 		= AF_INET;   	  //allows for other network protocols
	clientsockin.sin_addr.s_addr 	= INADDR_ANY;	  //0
	clientsockin.sin_port 			= 0;
	clientsockin.sin_port 			= htonl(INADDR_ANY);
	bound= (bind (client_tcpsocket, (__CONST_SOCKADDR_ARG)&clientsockin, sizeof(clientsockin)) == 0);
	if (!bound)
	{	perror("tcp client didn't bind");
		std::exit(1);
	}
	else
		cout<< "BBBs Tcp client is bound\n\r";

	server_address.sin_family 		= AF_INET;
	server_address.sin_addr.s_addr 	=socket_addressserver;
	//server_address.sin_addr.s_addr 	=inet_addr("192.168.2.1" );

	server_address.sin_port = htons(portnr);
    //printf(" connecting with server on ipaddress %s\n\r",socketaddressserver);

	if (connect(client_tcpsocket, (struct sockaddr*) &server_address,sizeof(server_address))< 0)
	{	perror("connection of client to server on host computer failed");
		std::exit(1);
	}
	else printf("Client (Beaglebone) is connected to server\n\r");
	connected= true;
}*/

void TcpMessagerClient::SetHostSocketAddress(in_addr_t& socket_addressserver)
{   socketaddressserver= socket_addressserver;
    socketaddressserver_is_set=true;
}

void TcpMessagerClient::Close()
{	close(client_mess_tcpsocket);
	//printf("Client tcp/ip socket closed\n\r");
	connected=false;
}

void TcpMessagerClient::Send(const string& stringtosend)
{	Open();
	SendTCPClient(stringtosend);
	Close();
}

/*void TcpClient::Send(string& stringtosend)
{	Open();
	SendTCPClient(stringtosend);
	Close();
}*/

void TcpMessagerClient::Send()
{   //string mess=tcpclient_outgoing_message;
	//pthread_mutex_lock( &clienttcp_mutex);
	Send(tcpclient_outgoing_message);
	//pthread_mutex_unlock( &clienttcp_mutex);
}

/*void TcpClient::Send(const string& stringtosend,const string& ipaddresstring, const unsigned int portnr)
{   string addr=ipaddresstring;
	Open(addr ,portnr);
   	SendTCPClient(stringtosend);
	Close();
}*/

void TcpMessagerClient::SendTCPClient(const string& str)
{	strcpy(tcpclient_outgoing_message,str.c_str());
	unsigned int    length=strlen(tcpclient_outgoing_message);
	if (tcpclient_outgoing_message[length-2]!='\n' ){//&& tcpclient_outgoing_message[length-1]!='\r')
		tcpclient_outgoing_message[length++]='\n';
	    //tcpclient_outgoing_message[length++]='\r';
	}
	tcpclient_outgoing_message[length]=0;
	if (send(client_mess_tcpsocket,tcpclient_outgoing_message,length,0) !=(int)length){
		perror("ERROR writing to socket");
	 	std::exit(1);
	}
	tcpclient_outgoing_message[length-1]=0;
	//printf("tcpclient send message: %s length %i.\r\n",tcpclient_outgoing_message,length);
}

void TcpMessagerClient::SendOutportStatus()
{   string message="Out ";
    for (int i=0; i<outports.size(); i++){
    	message += outports[i]->value()== HIGH 	?	'1' : '0';
    }
    Send(message);
}

void TcpMessagerClient::SendInportChanged(int inport_nr)
{   string message="In  x x";
    message[4]='0'+inport_nr;
    message[6]= inports[inport_nr]->switchstate()== HIGH 	?	'1' : '0';
    Send(message);
    printf("inmessage:%s",message.c_str());// <<message<<endl;
}

void TcpMessagerClient::AnalogToClientMessage()
{   char *data;
    data =tcpclient_outgoing_message;
    for (int i=0;i<NR_ANALOG_INPUTS;i++)
	    if (analog_to_send[i])
	       data+= AnalogToClientMessage(i, data);
}

int TcpMessagerClient::AnalogToClientMessage(unsigned int ad_in_nr, char *data)
{   ADC  			*adc;
    static Fifo     temp;
	int dev,outvalue=0,tempout;
	bool sending=false;
	if (ad_in_nr>0 && ad_in_nr<=NR_ADCS){
	   //real analog digital converters    counting nw on BBB from 1 to 7
		pthread_mutex_lock( &adc_mutex );
		adc=adcs[ad_in_nr-1];
		outvalue = adc->mean_voltage()  +0.5;  //Voltage in units of 0.1 mV
		dev  = adc->Deviation()+0.5;
		pthread_mutex_unlock( &adc_mutex );
		sending=true;
	}
	if (ad_in_nr==0 ){   //for I2C-temperature sensor    on Beaglebone
		pthread_mutex_lock( &temperature_sensorADT7420_mutex);
		temperature_sensor.readSensorState();
		tempout = temperature_sensor.temperature()*1000+0.5 ;//temperature in units of millikelvins
		temp.Add(tempout);
		outvalue=temp.Mean();
		pthread_mutex_unlock( &temperature_sensorADT7420_mutex);
		dev=temp.Deviation()+0.05;
		sending=true;
	}
	sprintf(data,"Ad%1.1i %5.5i %5.5i; ",ad_in_nr,outvalue,dev);  //answer in returnbuffer
	return sending? strlen(data): 0;
}


 void TcpMessagerClient::SendAnalog()
 {   analog_to_send[0]=temperature_sensor.connected() ; //send temperature
	 AnalogToClientMessage();
	 if(strlen(tcpclient_outgoing_message)>0)
	    Send();
 }
 //_____________________________ *  end TCPClient class *_______________________________________________//
 //____________________________________________________________________________________________
 //____________________________________________________________________________________________
 //____________________________________________________________________________________________
 //           ********************* TcpClient class **************************
 //           * TCP Client is used for messages to the server on the peer PC *
 //           * these messages are initiated by the linux device             *
 //           * (without request from the peer computer)                     *
 //___________****************************************************************_________________
 //____________________________________________________________________________________________
 TcpClient::TcpClient()
 {	connected= false;
 	memset(&clientsockin, 0, sizeof(clientsockin));   // complete the socket structure
 	clientsockin.sin_family 		= AF_INET;   	  //allows for other network protocols
 	clientsockin.sin_addr.s_addr 	= INADDR_ANY;	  //0
 	clientsockin.sin_port 			= 0;
 	clientsockin.sin_port 			= htonl(INADDR_ANY);
 }

 TcpClient::~TcpClient()
 {
 }

 void TcpClient::Open(string& ip_address,unsigned int portnr)  //external address and portnumber
 {	in_addr_t socketaddressserver= inet_addr(ip_address.c_str());
 	Open(socketaddressserver,portnr);
 }


void TcpClient::Open(in_addr_t& socket_addressserver,unsigned int portnr)  //external address and portnumber
 {	sockaddr_in 	clientsockin;
 	sockaddr_in     server_address;
 	bool			bound=false;

 	client_tcpsocket=  socket(AF_INET,SOCK_STREAM,0);
 	if ( client_tcpsocket <0)
 		printf("Error in opening socket\n\r");
 	else
 		printf("Client socket is opened\n\r");
 	memset(&clientsockin, 0, sizeof(clientsockin));   // complete the socket structure
 	clientsockin.sin_family 		= AF_INET;   	  //allows for other network protocols
 	clientsockin.sin_addr.s_addr 	= INADDR_ANY;	  //0
 	clientsockin.sin_port 			= 0;
 	clientsockin.sin_port 			= htonl(INADDR_ANY);
 	bound= (bind (client_tcpsocket, (__CONST_SOCKADDR_ARG)&clientsockin, sizeof(clientsockin)) == 0);
 	if (!bound)
 	{	perror("tcp client didn't bind");
 		std::exit(1);
 	}
 	else
 		cout<< "BBBs Tcp client is bound\n\r";

 	server_address.sin_family 		= AF_INET;
 	server_address.sin_addr.s_addr 	=socket_addressserver;
 	server_address.sin_port = htons(portnr);
     //printf(" connecting with server on ipaddress %s\n\r",socketaddressserver);

 	if (connect(client_tcpsocket, (struct sockaddr*) &server_address,sizeof(server_address))< 0)
 	{	perror("connection of client to server on host computer failed");
 		//std::exit(1);
 	    connected=false;
 	    Close();
 	}
 	else printf("Client (Beaglebone) is connected to server\n\r");
 	connected= true;
 }


 void TcpClient::Close()
 {	close(client_tcpsocket);
 	printf("Client tcp/ip socket closed\n\r");
 	connected=false;
 }

 void TcpClient::SendTCPClient(const string& str)
 {	strcpy(tcpclient_outgoing_message,str.c_str());
 	unsigned int    length=strlen(tcpclient_outgoing_message);
 	if (tcpclient_outgoing_message[length-2]!='\n' ){//&& tcpclient_outgoing_message[length-1]!='\r')
 		tcpclient_outgoing_message[length++]='\n';
 	    //tcpclient_outgoing_message[length++]='\r';
 	}
 	tcpclient_outgoing_message[length]=0;
 	if (send(client_tcpsocket,tcpclient_outgoing_message,length,0) !=(int)length){
 		perror("ERROR writing to socket");
 	 	std::exit(1);
 	}
 	tcpclient_outgoing_message[length-1]=0;
 	//printf("tcpclient send message: %s length %i.\r\n",tcpclient_outgoing_message,length);
 }


void TcpClient::Send(const string& stringtosend,const string& ipaddresstring, const unsigned int portnr)
 {   string addr=ipaddresstring;
 	 Open(addr ,portnr);
     SendTCPClient(stringtosend);
 }


void TcpClient::Send(const string& stringtosend)
 {    SendTCPClient(stringtosend);
 }
 //_____________________________ *  end TCPClient class *_______________________________________________//

 //_____________________________________________________________________________________________________//
 //_____________________________________________________________________________________________________//
 //_____________________________________________________________________________________________________//
 //           ********************* TcpServer class **************************                       	//
 //           * TCP server is used for handling requests from the peer PC    * 							//
 //___________****************************************************************__________________________//
 //_____________________________________________________________________________________________________//

 TcpServer::TcpServer(int socket_nr,int sock_desc_currrent)
 {  listening_socket=socket_nr;
    current_socket_descriptor= sock_desc_currrent;
 }

 bool TcpServer::process_socket()  						//returns true on recognized order, false otherise
{   char error_answer[10] = "400 Error";
    char ok_answer[4]	  = "OK!";
    Comparator		*comp;
    //TaskSchedule	*sched;
	time_t			tim;
	struct timeval	now;
	bool			retval,is_edit;
	int				answer,i,nr;
	char            ch;
	//char nv[19];
	if (strcmp_recieved_tcp_data("Tim")) {             /// recieved info time & date; response with ok or name of recieving module
		 retval=tcpstring_to_time((tcpserver_incoming_message+3),tim);
		 if (retval){
			 now.tv_sec=tim;
			 now.tv_usec=0;
			 printf("%i seconds elapsed",(int)now.tv_sec);
			 settimeofday(&now, NULL); //DST_WET //WesternEuropeanTimeZone
		 	 printf("settimeofday() successful.\n\r");
		 }
		 else {
		 	  printf("settimeofday() failed, ");
		 	  return false;
		 }
		 //string cn="BeagleBone Black\r\n";
		 send(controller_name);
		// controller_name="203 "+controller_name;
		 //controller_name.copy(tcpserver_outgoing_message,controller_name.length(),0);
		 //send(tcpserver_outgoing_message);
		 return true;
	 }
	 else
	 if (strcmp_recieved_tcp_data("Ad0-")){
	 	 AdToTcpData_buf();
	    // strcpy(tcpserver_outgoing_message,ok_answer);
	     cout << tcpserver_incoming_message;
	 	 send(ok_answer);
	 }
	 else
	 if (strcmp_recieved_tcp_data("Out")){
	    //printf("Buffer:%s ",tcpserver_incomingng_message);
		 answer=ReadOutportFromTcpBuffer();
	 	 if (answer==1 )			// outport toggled
	 		 send(ok_answer);
	 	 else
	 		 if (answer==-1)
	 			 send(error_answer);
	 		 else if (answer==2){	//outport status request
	 		 	OutportStatusToTcp();
	 	      	send(tcpserver_outgoing_message);
	 		 }
	 }  //if (strcmp_recieved_tcp_data("Out"))
	 else
	 if (strcmp_recieved_tcp_data("CSR")){	//outportnemes, comparatornamess+ schedulenames refresh
	 	 Comp_Sch_namesToTcpData_buf();
	 	 send(tcpserver_outgoing_message);
	 }
	 else
	 if (strcmp_recieved_tcp_data("Sch"))	 //schedule must be updated
		 	 	 	 	 	 	 	 	 	 // Format: 13/03/27 00:00:00W1010000AE"                "
		 	 	 	 	 	 	 	 	 	 //       2013 march 27
		 	 	 	 	 	 	 	 	 	 //        0         1         2         3         4
		 	 	 	 	 	 	 	 	 	 //         1   5    0    5    0    5    0 2  5
		 	 	 	 	 	 	 	 	 	 // June 5, 1998, Friday, 13:55:30, the
		 	 	 	 	 	 	 	 	 	 // unsigned char t[14] = { 5, 9, 8, 0, 6, 0, 5, 1, 3, 5, 5, 3, 0 };   0    5
		    // New schedule:
		    //             sched.nr date time  compnr days(wk) treshold
		    //           N                           D        H
		    //		     E							 W        L
		    //           D                           Y        N
		    //                                       M
		    //										 S
		    //                                       N
		    //       0         1         2         3         4         5
		    //*******0123456789 123456789 123456789 123456789 123456789 1
		    //		 Sch N xvx 01/05/15 10:10:10 C01 D1111111 H1.2345e+01
		    // Edit schedule":
		    //		 Sch E 123 01/05/15 10:10:10 C01 D1111111 L1.2345e+01
		    //or     Sch E 123 01/05/15 10:10:10 C01 S 59     L1.2345e+01
		    // Dlete schedule":
		    //		 Sch D 123

	 {   //printf("incoming:%s\r\n",tcpserver_incoming_message);
		 TaskSchedule *recieved_schedule;
	 	 int sch_nr,comp_nr;
	 	 char  *buffer,*nm;
	 	 string name;
	 	 struct tm  tm_to_act;
	 	 //float newtreshold;
	 	 buffer= tcpserver_incoming_message;
	 	 char  editmode=tcpserver_incoming_message[4];
	 	 sch_nr=atoi(tcpserver_incoming_message+6);
	 	 switch (editmode)
	 	 {	case 'E':  				//editing number follows
	 	    case 'N':
	 	    {     is_edit=(editmode=='E');
	 	          send(ok_answer);   //Edited or New schedule is written

	 	          //printf("New"),
	 	          recieved_schedule=new TaskSchedule();
	 	         // New schedule:
	 	         //          sched.nr date    time   days(wk)cmpnr1 nwtreshold  cn2
	 	         //          N                        D           HH            C01 H
	 	         //		     E						  W           LL                L
	 	         //          D                        Y           Nx                E
	 	         //                                   M                             D
	 	         //									  S                         C99 x
	 	         //                                   N
	 	         //       0         1         2         3         4         5         6
	 	         //*******0123456789 123456789 123456789 123456789 123456789 123456789 12345
	 	         //		  Sch N ### 01/05/15 10:10:10 D1111111 C01 HL 1.2345e+01 C02 H " Name		"



	 	          //       0         1         2         3         4         5
	 	          //*******0123456789 123456789 123456789 123456789 123456789 1
	 	          //       Sch N ### 01/05/15 10:10:10 D1111111 C01 HL 1.2345e+01 C02 H " Name		"
	 	         //printf("incoming:%s\n\r",buffer+11);
	 	          try{

	 	          format_string_to_struct_tm(buffer+10, &tm_to_act);
	 	          recieved_schedule->timedate_to_act=mktime(&tm_to_act);
	 	          buffer=buffer+28;

	 	          //                                   0         1         2
	 	          //*******                            0123456789 123456789 123456
	 	          //       Sch N ### 01/05/15 10:10:10 D1111111 C01 HL 1.2345e+01 C02 H " Name		"
	 	          switch (*buffer)
	 	          {  case 'W': recieved_schedule->repeat_interval= repeating_weekly;break;
	 	             case 'Y': recieved_schedule->repeat_interval= repeating_yearly;break;
	 	             case 'D': recieved_schedule->repeat_interval= repeating_daily ;break;
	 	             case 'M': recieved_schedule->repeat_interval= repeating_sectick ;break;
	 	             case 'S': recieved_schedule->repeat_interval= repeating_mintick ;break;
	 	             case 'N': recieved_schedule->repeat_interval= not_repeating     ;break;
	 	          }
	 	          recieved_schedule->repeating= (*(buffer++)!='N');

	 	          for (i=0;i<7;i++)
	 	        	  recieved_schedule->repeat_on_day[i]= (*buffer++=='1');
	 	          //Control whether first char is c ;then read comparator number

	 	          //                                            0         1         2
	 	          //*******                                     0123456789 123456789 123456/
	 	          //       Sch N ### 01/05/15 10:10:10 D1111111 C01 HL 1.2345e+01 C02 H " Name
	 	          buffer++; //skip the space
	 	          // printf("incoming:%s\n\r",buffer);
	 	          if (*(buffer++)!='C')
	 	             throw 1;
	 	          comp_nr=atoi(buffer);
	 	          recieved_schedule->comparator=comparators[comp_nr];
	 	          buffer+=3;
	 	          //printf("incoming2:%s\n\r",buffer);
	 	          ch=*(buffer++);
	 	          //*******                                     0123456789 123456789 123456/
	 	          //       Sch N ### 01/05/15 10:10:10 D1111111 C01 HL 1.2345e+01 C02 H " Name
	 	          // now H for high_treshold L for low treshold of comparator1 D=task disabled
	 	          switch (ch)
	 	          {  case 'H': recieved_schedule->command=comparator_high_tr;recieved_schedule->enabled=true;break;
	 	             //case 'h': recieved_schedule->command=comparator_sethigh;recieved_schedule->enabled=true;break;
	 	             case 'L': recieved_schedule->command=comparator_low_tr;recieved_schedule->enabled=true;break;
	 	             //case 'l': recieved_schedule->command=comparator_setlow;recieved_schedule->enabled=true;break;
	 	             case 'T': recieved_schedule->command=comparator_toggle;recieved_schedule->enabled=true;break;
	 	             case 'D': recieved_schedule->enabled=false;break;
	 	             default:cout<<"error reading schedule1"<<endl; exit(-1);break;
	 	          }
	 	          ch=*(buffer++);
	 	          switch (ch)
	 	          {  case 'H': recieved_schedule->adjust_treshold=comparator_high_tr	;break;
	 	             case 'L': recieved_schedule->adjust_treshold=comparator_low_tr		;break;
	 	         	 case 'N': recieved_schedule->adjust_treshold=comparator_notr_adjust;break;
	 	         	 default:throw 0													;break;
	 	          }
	 	          buffer++; //skip the space
                  if (recieved_schedule->adjust_treshold!=comparator_notr_adjust)
	 	        	 recieved_schedule->new_treshold=atof(buffer);
	 	          buffer+=11;
	 	          //printf("incoming2:%s\n\r",buffer);

	 	          if (*(buffer++)!='C')
	 	               throw 1;
	 	          comp_nr=atoi(buffer);
	 	          //if (comp_nr!=99)
	 	          //	 recieved_schedule->comparator2=comparators[comp_nr];
	 	          //else
	 	          // recieved_schedule->comparator2=(Comparator*)NIL;
	 	          recieved_schedule->comparator2=(comp_nr!=99)?
	 	        		  comparators[comp_nr]:
	 	        		  (Comparator*)NIL;
	 	          switch (buffer[3])
	 	          {
	 	            case 'D': recieved_schedule->command2=comparator_disabld; break;

	 	         	 case 'H': recieved_schedule->command2=comparator_high_tr; break;
	 	         	 case 'L': recieved_schedule->command2=comparator_low_tr ; break;
	 	         	 case 'T': recieved_schedule->command2=comparator_toggle ; break;
	 	         	 default:throw 2;break;
	 	          }
	 	          if (!recieved_schedule->comparator2 && recieved_schedule->command2!=comparator_disabld)
	 	        	  throw 3;
	 	         buffer+=6;
	 	         nm=buffer;   //read schedule name
	 	         //printf ("Schedulename%s\r\n",nm);

	 	          buffer+=SCHEDULE_NAMELENGTH;
	 	          if (*buffer!='"')
	 	              	throw 4;
	 	          *buffer=0;
	 	          name=nm;
	 	          recieved_schedule->SetName(name);
	 	          if (is_edit)
	 	        	  schedules.Update(sch_nr,recieved_schedule);   //deletes the old schedule
	 	          else
	 	        	  schedules.Add(recieved_schedule);
	 	          //printf("New schedule is read:%s\n\r",nm);
	 	          break;  //dase E or N

	 	     // break;
	 	      } //end try
	 	      catch (int ex_nr)
	 	      { cout<<"Exception in tcp reading of schedule: nr "<<ex_nr<<endl;
	 	      }
	 	      catch (...)
	 	      { cout<<"Unhandled exception in tcp reading of schedule";
	 	      }
	 	      break;

	 	    }    //END  case E N
	 	    case 'D':
	 	       // Delete schedule":
	 	       //		Sch D 123
	 	    	  schedules.Delete(sch_nr);
	 	    	  send(ok_answer);
	 	    	  break;
	 	    case 'R':
	 	    	printf("Sends schedule %2.2i\r\n",sch_nr);
	 	    	ScheduleToTcpData_buf(sch_nr);
	 	    	send_socket_buf();
	 	    break;

	 	    case 'I':  //change schedule order: interchange sch_nr with following number
	 	   	 	 schedules.Interchange(sch_nr,atoi(tcpserver_incoming_message+9));
                 send(ok_answer);
	 	   	break;
	 	 }  //END  switch editmode
	 }  //END   if (strcmp_recieved_tcp_data("Sch"))
	 else
	 if (strcmp_recieved_tcp_data("GPIO_Info"))
	 {	SendGPIO_Inf();
	 }
	 else
	 if (strcmp_recieved_tcp_data("Cmp "))
	 {	 //format:
		 //			Cmp E01 BL01 O01>L+2.0000e+01 +2.0000e+01 2.0000e-01 "Kamerthermostaat"
		 //             Edit           low value  high value  hysteresis name comparator
		 //                nr
		 //                 bool/analog etc
		 //                   low high disabled
		 //                     input nr
		 nr=atoi(tcpserver_incoming_message+5);
	     ch=tcpserver_incoming_message[4];
		 if (nr<comparators.size() || (nr==comparators.size()&& ch=='N')){
		 switch (ch)
		 { case 'R':   //  format :"Cmp R01"                          //request comparator nr 01
		     printf("Sends comparator %2.2i\r\n",nr);
		 	 ComparatorToTcpData_buf(nr);   //MISUSE BUFFEER
		 	 send_socket_buf();
		 	 break;
		 	 case 'D' :  //  format :"Cmp D01"                        //delete comparator nr 01
		 		  comparators.Delete(nr);
		 		  send(ok_answer);
		 		  break;
		 	 case 'N':   // format :"Cmp Nxx"                         //new  comparator
		 	 case 'E' :  // format :"Cmp E01"                         //edit comparator nr 01
		 	 switch (ch)
		 	 {	  case 'N':
		 	         comp=new Comparator;
		 	         comparators.Add(comp);
		 	 	  break;
		 	 	  case 'E':
		 			 comp=comparators[nr];
		 			 break;
		 	 }
		 	 if (ReadComparatorFromTCPbuf (comp))
		 		 send(ok_answer);
		 	 break;

		 }
		 }
	 }
	// if (strcmp_recieved_tcp_data("CSR"))   //comparatornamess+ schedulenames refresh
   //      send_socket_buf(Comp_Sch_namesToTcpData_buf());

	 else
	 	 {	strcpy(tcpserver_outgoing_message,"TCP/IP order is not valid");
	 	    return false;
	 	 }

	  	 return true;

}

int TcpServer::Comp_Sch_namesToTcpData_buf()
{    int i;
     i=schedules.first_task;
     if (i<0 || i>99) i=99;
     sprintf(tcpserver_outgoing_message,"203 #%2i #%2i #%2i #%2i #%2i Cp",comparators.size(),schedules.size(),COMPARATOR_NAMELENGTH,SCHEDULE_NAMELENGTH,i);     //only roomcomparator on et

     int str_length=26;  //was 19
     char comparator_properties;
     Comparator *comp;
     // send comparator properties now
     // 1= all possibilities
     // 2= restricted to one state : high_treshold
     // 3= not input dependant
     for (int i=0;i< comparators.size();i++)
     {	comp=comparators[i];
        if (comp->inputclass==no_inputdependancy) comparator_properties='3';
        else if (comp->state==single_treshold) comparator_properties='2';
        else comparator_properties='1';
        *(tcpserver_outgoing_message+str_length++)=comparator_properties;
     }
     for (int i=0;i< outports.size();i++)
     {	 strncpy(tcpserver_outgoing_message+str_length," \"",2);
         strncpy(tcpserver_outgoing_message+str_length+2,outports[i]->Name(),COMPARATOR_NAMELENGTH);
         str_length+=COMPARATOR_NAMELENGTH+2;
         tcpserver_outgoing_message[str_length++]='\"';
     }
     for (int i=0;i< comparators.size();i++)
     {	 strncpy(tcpserver_outgoing_message+str_length," \"",2);
         strncpy(tcpserver_outgoing_message+str_length+2,comparators[i]->Name(),COMPARATOR_NAMELENGTH);
    	 str_length+=COMPARATOR_NAMELENGTH+2;
         tcpserver_outgoing_message[str_length++]='\"';

     }
     for (int i=0;i< schedules.size();i++)
     {  strncpy(tcpserver_outgoing_message+str_length," \"",2);// see above
	 	strncpy(tcpserver_outgoing_message+str_length+2,schedules[i]->Name(),SCHEDULE_NAMELENGTH);
     	str_length+=SCHEDULE_NAMELENGTH+2;
     	tcpserver_outgoing_message[str_length++]='\"';
     }
     tcpserver_outgoing_message[str_length]=0;
     return str_length;
 }

int  TcpServer::ScheduleToTcpData_buf(int nr)
{   //       204 01/05/15 10:10:10 D1111111 C01 HL 1.2345e+01 C02 H " Name

	char *buffer, comm_chr1,comm_chr2,tr_chr,rp_char;
	if (nr==2) printf("Taak2");
    TaskSchedule *schedule;
    schedule=schedules[nr];
	strncpy(tcpserver_outgoing_message,"204 ",4);
	time_t actiontime;
	actiontime =schedule->timedate_to_act;
	buffer=tcpserver_outgoing_message;
	time_to_tcpstring(buffer+4,actiontime); //adds timestring of 18 char incl trailing zero 4+17=21
	                                        //trailing zero on buffer[21]
	buffer[21]=' ';
	switch (schedule->repeat_interval)
	{ case repeating_weekly: rp_char='W'; break;
      case repeating_yearly: rp_char='Y'; break;
      case repeating_daily : rp_char='D'; break;
      case repeating_sectick:rp_char='M'; break;
      case repeating_mintick:rp_char='S'; break;
      case not_repeating    :rp_char='N'; break;
	}
	buffer[22]=rp_char;
	buffer+=23;
	try{
	   for (int i=0;i<7;i++)
		   *(buffer++)= (schedule->repeat_on_day[i])?'1':'0';
	   switch (schedule->command)     // H high Low D disabled
	   {   case comparator_high_tr:  comm_chr1='H'; break;
	   	   case comparator_low_tr:   comm_chr1='L'; break;
	   	   case comparator_disabld:  comm_chr1='D'; break;
	   	   default:throw 1; break;
	   }
	   switch (schedule->command2)     // H high Low D disabled
	   {   case comparator_high_tr:  comm_chr2='H'; break;
	   	   case comparator_low_tr:   comm_chr2='L'; break;
	   	   case comparator_disabld:  comm_chr2='D'; break;
	   	   default: comm_chr2='?';throw 2; break;
	   }
	   switch(schedule->adjust_treshold)
	   {   case comparator_high_tr:  		tr_chr='H'; break;
		   case comparator_low_tr:   		tr_chr='L'; break;
		   case comparator_notr_adjust:  	tr_chr='N'; break;
		   default: tr_chr='?';throw 3; break;
	   }
     } //end try
    catch (int excnr)
    {  cout<<"Error schedule format"<<excnr<<endl;
    }
    Comparator* c2=schedule->comparator2;
    int comp2nr=(c2==NIL ||schedule->command2==comparator_disabld)?99:comparators.nr(schedule->comparator2);
	sprintf(buffer," C%02i %c%c %+10.4e C%02i %c \"%s\"\r\n\0",
			comparators.nr(schedule->comparator),comm_chr1,tr_chr,schedule->new_treshold,
			comp2nr,comm_chr2,
			schedule->Name());
	tcpserver_outgoing_message[strlen(tcpserver_outgoing_message)]=0;
	//printf("Test sch output:%s\n",tcpserver_outgoing_message);
	return 0;
}

int  TcpServer::ComparatorToTcpData_buf(int nr)
{   Comparator* comp;
	char ch;
	char *buffer=tcpserver_outgoing_message;
	int innr,outnr;
	comp= comparators[nr];
	strncpy(buffer,"204 ",100);

	switch (comp->inputclass)  //{variable_input,adc_input,i2c_input,ADC7420_temperature_input,notdefined_input};
	{ 	case adc_input:
			ch='A';
			break;
		case adc_gauged_input:
			ch='G';
			break;
		case ADC7420_temperature_input:
			ch='T';
			break;
		case boolean_variable_input:
			ch='B';
			break;
		case variable_input:
			ch='V';
			break;
		case no_inputdependancy:
			ch='N';
			break;
		default:
			ch='V';
			break;
	}
	buffer=tcpserver_outgoing_message+4;
	*buffer++=ch;


//    switch (comp->direction)
//    { case top_true:
//   	ch='>';
//   	break;
//    case bottom_true:
		ch='<';
//    	break;
//    default:
//    	ch='=';
//}
		*buffer++=ch;
		switch (comp->state)
		{ 	case high_treshold:
				ch='H';
				break;
			case low_treshold:
				ch='L';
				break;
			case single_treshold:
				ch='S';
				break;
			default:  //disabled
				ch='D';
				break;
		}
		*buffer++=ch;

		bool adinp= comp->inputclass== adc_input|| comp->inputclass== adc_gauged_input;
		innr		= adinp ? comp->input_Adc->adnr() : 0;
		outnr	= outports.nr(comp->outputport);
		sprintf(buffer," I%02i O%02i ",innr,outnr);
		buffer+=8;
		pthread_mutex_lock( &rotary_encoder_mutex);
		sprintf (buffer,"%+10.4e %+10.4e %+10.4e ;",comp->high_treshold_top,comp->low_treshold_top,comp->hysteresis);
		pthread_mutex_unlock( &rotary_encoder_mutex);
		buffer+=36;  //3 * 12 positions (sign, float, space)
		*(buffer++)='"';
		strncpy(buffer,comp->name,COMPARATOR_NAMELENGTH);
		buffer+= COMPARATOR_NAMELENGTH;
		strncpy(buffer,"\r\n",3);;
		*(buffer+3)=0;
	    return -1;
}



bool TcpServer::ReadComparatorFromTCPbuf (Comparator* comp)
{   char *ts=tcpserver_incoming_message+8,*nm;
    char ch;
    int i,j;
    //format:
    		 //			Cmp E01 BL01 O01>L+2.0000e+01 +2.0000e+01 2.0000e-01 "Kamerthermostaat"
    		 //             Edit           low value  high value  hysteresis name comparator
    		 //                nr
    		 //                 bool/analog etc
    		 //                   low high disabled
    		 //                     input nr


    ch=*ts++;
    switch (ch)  //{variable_input,adc_input,i2c_input,ADC7420_temperature_input,notdefined_input};
    { 	case'A':
    		comp->inputclass= adc_input;
        	break;
        case 'G':
        	comp->inputclass= adc_gauged_input;
        	break;
        case 'T':
        	comp->inputclass= ADC7420_temperature_input;
        	break;
        case 'I':
            comp->inputclass= i2c_input;
        	break;
        case 'V':
             comp->inputclass= variable_input;
             break;
        case 'B':
             comp->inputclass= boolean_variable_input;
             break;

        case 'N':
             comp->inputclass= no_inputdependancy;
             break;
        default:
        	  printf("Error in TCPStream:%s",ts);
             return false;
        	break;
        }
    printf("Voor lezen ints:%s \n\r",ts);
    comp->SetInportNr(i=atoi(++ts));
    comp->SetOutportNr(j=atoi(ts+4));
    printf("%s Inout In%2.2i Out%2.2i",ts,i,j);
    ts+=6;
//    comp->direction=(*ts=='>')?top_true:((*ts=='=')?equal_true:bottom_true);



    switch (*ts)
    {  case 'H' :
    		comp->state=high_treshold;	break;
       case 'L':
    	    comp->state=low_treshold; 	break;
       case 'S':
    	    comp->state=single_treshold;break;
       default:
    	   printf("Error in TCPStream:%s",ts);
    	   exit(-1);
    	   return false;
    }
    pthread_mutex_lock( &rotary_encoder_mutex);
    comp->high_treshold_top	=atof(++ts);
    comp->low_treshold_top	=atof(ts+12);
    pthread_mutex_unlock( &rotary_encoder_mutex);
    comp->hysteresis		=atof(ts+24);
    printf("STRING:%f;%f;%f;\n\r" ,comp->high_treshold_top,comp->low_treshold_top,comp->hysteresis);
    ts+=35;
    //printf("String:%s",ts);
    if (*(ts++) != '"'  )
    {  printf ("error reading comparator");
       return false;
    }
    nm=ts;
    ts+=COMPARATOR_NAMELENGTH;
    if (*ts!='"')
     	printf ("error reading comparator2\r\n");
    *ts=0;
    string name=nm;
    comp->SetName(name);
    //printf("STRING:%s\r\n"  ,nm);
    return true;
}

bool format_string_to_struct_tm(char *ts, tm *t)

{	//printf("time:%s",ts);

	if (ts[14]!=':')
       return false;
//     string format: 11/05/02 15:40:23  (18 char long incl trailing zero)
//     m_sec	int	seconds after the minute	0-61*
//     tm_min	int	minutes after the hour	0-59
//     tm_hour	int	hours since midnight	0-23
//     tm_mday	int	day of the month	1-31
//     tm_mon	int	months since January	0-11
//     tm_year	int	years since 1900
//     tm_wday	int	days since Sunday	0-6
//     tm_yday	int	days since January 1	0-365
//     tm_isdst	int	Daylight Saving Time flag

     t->tm_wday = 8 ;
     t->tm_year=  twochar_to_int(ts)+100; ts+=3;
     t->tm_mon =  twochar_to_int(ts)-1;   ts+=3;
     t->tm_mday=  twochar_to_int(ts); ts+=3;
     t->tm_hour=  twochar_to_int(ts); ts+=3;
     t->tm_min =  twochar_to_int(ts); ts+=3;
     t->tm_sec =  twochar_to_int(ts);
     return true;
}

int twochar_to_int(char *ch)
{  int i1 = *(ch++)- '0';
   int i2 = *(ch++)- '0';
   return 10*i1+i2;
}
/*
bool CTime::TimeString (char *ts)          //Time to string format: 11/05/02 15:40:23  (18 char long incl traoling zero)
{    int i,j;
	  unsigned char *timestring=&(time.year10);
     strcpy(ts,"00/00/00 00:00:00");

     for ( i=0;i<6;i++)
     {  for(j=0;j<2;j++)
            *(ts++)=*(timestring--)+'0';
        ts++;
     }
     ts[17]=0;   //trailing zero
}*/


void TcpServer::AdToTcpData_buf()
{   unsigned int 	ad_in_nr= tcpserver_incoming_message[2]-'0';     //format is AdO Ad1 Ad9 .... AdA ..
        //or Ad0-On || Ad0-Off
	if (tcpserver_incoming_message[4]== 'O')
    {    tcpclient.analog_to_send[ad_in_nr]= (tcpserver_incoming_message[5]=='n');
         return;
    }
	ADC  			*adc;
	int dev,outvalue=0;
	if (ad_in_nr>0 && ad_in_nr<=NR_ADCS)
	{  //real analog digita converters    counting nw on BBB from 1 to 7
		pthread_mutex_lock( &adc_mutex );
		adc=adcs[ad_in_nr-1];
		outvalue = adc->mean_voltage()  +0.5;  //Voltage in units of 0.1 mV
		dev  = adc->Deviation()+0.5;
		pthread_mutex_unlock( &adc_mutex );
	}
	if (ad_in_nr==0)   //for I2C-temperature sensor    on Beaglebone
	{
		pthread_mutex_lock( &temperature_sensorADT7420_mutex);
		temperature_sensor.readSensorState();
		outvalue = temperature_sensor.temperature()*1000+0.5 ;//temperature in units of millikelvins
		pthread_mutex_unlock( &temperature_sensorADT7420_mutex);
		dev=0;
	}
	sprintf(tcpserver_outgoing_message,"203 %2.2i %5.5i %5.5i",ad_in_nr,outvalue,dev);  //answer in returnbuffer
}


void TcpServer::OutportStatusToTcp()
{   char *answer=tcpserver_outgoing_message+4;
	strcpy(tcpserver_outgoing_message,"203 ");
	for (int i=0;i<outports.size();i++)
	   *(answer++) =(outports[i]->value()==HIGH)?'1':'0';      //State outport: true or false
    *answer=0;
    //printf(tcpserver_outgoing_message);
}

void TcpServer::SendGPIO_Inf()
{   GPIO* port;
	char comm[TCP_MESSBUF_SIZE],*buf;
	buf=comm;
    for (int i=0; i<outports.size();i++)
    { port=outports[i];
      sprintf(buf,"O%2i ",port->PinNr()+1);
      buf+=4;
    }
    for (int i=0; i<inports.size();i++)
    { port=inports[i];
      sprintf(buf,"I%2i ",port->PinNr()+1);
      buf+=4;
    }
    *(--buf)=0;
    //printf("Gpio info: %s\r\n",comm);
    send(comm);
}

int ReadOutportFromTcpBuffer()
{ char *ts=tcpserver_incoming_message+3;
  {  if (ts[0]=='R') // "Refresh"
     {	 return 2;
     }
     int portnr = (*(ts)-'0');      //max poortnr 32
     if (portnr<outports.size() )
     {  outports[portnr]->value(((*++ts)=='0')?LOW:HIGH);
        return 1;
     }
     return -1;
   }
   return -2;
}


void TcpServer::SetSocketDescriptors(int socket_nr,int sock_desc_currrent)
{
   listening_socket=socket_nr;
   current_socket_descriptor= sock_desc_currrent;
}

bool TcpServer::strcmp_recieved_tcp_data(char const* str)
{ return
    strncmp(tcpserver_incoming_message,str,strlen(str))==0;
}

void TcpServer::send(string& str)
{  //str=controller_name;
   char buff[1024];
   str.copy(buff,str.length(),(string::size_type)0);
   send(buff);
}

void TcpServer::send(char* str,int length)
{  char nrbuf[5];
   if (length ==-1)
       length= strlen(str);
   strcpy(tcpserver_outgoing_message,str);
   strncpy(nrbuf,str,3);
   nrbuf[3]=0;
   int i=atoi(nrbuf);
   if (i<100 || i>999 ||str[3]!= ' ')              //Default command number is 203 meaning: OK
   {   strncpy(tcpserver_outgoing_message,"203 ",4);
       strncpy(tcpserver_outgoing_message+4,str,length);
       length+=4;
   }
   else
   strcpy(tcpserver_outgoing_message,str);
   if (str[length-2]!= '\r'||str[length-1]!= '\n')      //ends string with return & linefeed?
   {  tcpserver_outgoing_message[length++]='\r';
      tcpserver_outgoing_message[length++]='\n';
      tcpserver_outgoing_message[length]=0;
   }
   ::send(current_socket_descriptor,tcpserver_outgoing_message,length,0);
}

void TcpServer::send_socket_buf(int length)      //sock_busf must be prepared with strinh without \r\n
{   tcpserver_outgoing_message[length++]='\r';
	tcpserver_outgoing_message[length++]='\n';
	tcpserver_outgoing_message[length]=0;
    ::send(current_socket_descriptor,tcpserver_outgoing_message,length,0);
}

void TcpServer::send_socket_buf()      //sock_busf must be prepared with strinh without \r\n
{   send_socket_buf(strlen(tcpserver_outgoing_message));
}


bool tcpstring_to_time(char* timestring, time_t&  time_in)
{
	 //takes timestring from tcp as character string converts it in between to struct tm format
	 // finally mktime converts it to time_t format ann returns a pointer to time_t
	 // original format: 11/05/02 15:40:23  (18 char long incl trailing zero)
	 time_t time_t_in;
	 tm 	tim;
	 char 	str[4];
	 int i,j;
     if (timestring[14]!=':')
       return false;
     int *value;
     value= &(tim.tm_year);
     for (i=0;i<6;i++)
     {  for(j=0;j<2;j++)
           str[j]=*(timestring++);
     	timestring++;
        str[2]=0;
        *value=atoi(str);  //converting string to int
        if (i==0) *value +=100; //from 21' to 20' century
        if (i==1) (*value)--    ; //month 0 == januar in tcp file jan=' 1' , dec ='12'
        value--; 		   //next element of structure tm
     }
     //Return the `time_t' representation of TP and normalize TP.  /
     time_t_in =mktime (&tim);
     time_in= time_t_in;
     return true;
}

void time_to_tcpstring(char* timestring, const time_t&  time_out)
{   // converts time_out to string format for tcp
	// target: 11/05/02 15:40:23  (18 char long, trailing zero incl)
	 struct tm 	  *tim;
	 char 		  *editstring;
	 int 		  i,val,*value;
	 //unsigned int i,val,*value;
	 editstring	= timestring;
	 tim		= localtime(&time_out);
	 value		= &tim->tm_year;
     for (i=0; i<6; i++)
     {  val=*value;
        if (i==0)
        { val -=100; //from 20' to 21' century
          if (val<0) val=0;
        }
        if (i==1) val++    ; // in tcp file jan=' 1' , dec ='12' to month 0 == januar
    	sprintf(editstring,"%2.2i", val);
    	editstring+=2;
        *(editstring++)=i<2?'/':i==2?' ':':';
        value--; 		   //next element of structure tm
     }
     timestring[17]=0;
}
bool port_is_free(int portnr)
{	struct sockaddr_in sin;
	int sockfd;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1)
	  return false;

	sin.sin_port = htons(portnr);
	sin.sin_addr.s_addr = 0;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_family = AF_INET;

	if (bind(sockfd, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) == -1) {
	   if (errno != EADDRINUSE)
	  		printf("port_is_free() could not bind\n\r");
	   return false;   //else port was in use
	}
	return true;
}

///struct tm
//{
//  int tm_sec;			Seconds.	[0-60] (1 leap second)
//  int tm_min;			Minutes.	[0-59]
//  int tm_hour;		Hours.	[0-23]
//  int tm_mday;		ay.		[1-31]
//  int tm_mon;			Month.	[0-11]
//  int tm_year;		Year	- 1900.
//  int tm_wday;		Day of week.	[0-6] 	ignored by mktime
//  int tm_yday;		Days in year.[0-365] 	ignored by mktime
//  int tm_isdst;		DST.		[-1/0/1]
//
//#ifdef	__USE_BSD
//  long int tm_gmtoff;		/* Seconds east of UTC.  */
//  __const char *tm_zone;	/* Timezone abbreviation.  */
//#else
//  long int __tm_gmtoff;		/* Seconds east of UTC.  */
//  __const char *__tm_zone;	/* Timezone abbreviation.  */
//#endif
//};
