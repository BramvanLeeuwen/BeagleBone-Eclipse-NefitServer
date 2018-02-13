/*
 * TcpServer.hpp
 *
 *  Created on: Aug 30, 2013
 *      Author: Bram van Leeuwen
 */

#ifndef TCPSERVER_HPP_
#define TCPSERVER_HPP_
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
//#include "TCPTest.hpp"

#include "GlobalIncludes.hpp"
#include "leds.hpp"
#include "kbdio.hpp"
#include "GPIO.hpp"
//#include "RotaryEncoder.hpp"
//#include "ADT7420 Tempsensor.hpp"
#include "TMP102Tempsensor.hpp"
//#include "MyI2C.hpp"
#include "TaskManager.hpp"


enum client_or_server_kind {client,server,http_server} ;

#define TCP_IP_PORT 		0x0FA0    // TCP/IP listening on port 0x0FA0 = 4000 (decimal)

#define TCP_MESSBUF_SIZE 	1024	  // Size of message buffer for TCP/IP

#define  NR_ANALOG_INPUTS 	NR_ADCS+1    //one tempsensor I2C and 7 analog inputs

void *TcpServerThread (void *threadarg);  //creates a blocking tcp server on port 4000

///*****************************************************************/////
////*******************         TCP-CLIENT         ******************////
////*****************************************************************////
// The tcpclient is used to send unrequested messages to the peer computer
// the ip-adress of the peer is provided by the server: the peer computer is responsible for the firrst contact

class TcpMessagerClient
{  public:
	TcpMessagerClient();
	~TcpMessagerClient();
	//void Open(string& ip_address,unsigned int portnr); //external address and portnumber
	//void Open(in_addr_t& socketaddressserver,unsigned int portnr) ; //external address and portnumbervoid SendTCPClient(const string& str);
    void Open();

	void SetHostSocketAddress(in_addr_t& socketaddressserver);
	void Close();
	void Send(const string& strtosend);
	void Send();
	//void Send(const string& strtosend,const string& ipaddressstring, const unsigned int portnr);
	//void Open((struct in_addr& socketaddressserver,unsigned int portnr)  //external address and portnumber
	void SendOutportStatus();                                      //sends a record of the state of the outports
	void SendInportChanged(int inport_nr);                         //reports a change of state of an inport
	int  AnalogToClientMessage(unsigned int ad_in_nr, char *data);
	void AnalogToClientMessage();
	void SendAnalog();                                             //sends the voltage/ pysical value of a sensor or ad-converter to the peer
    inline bool Socketaddressserver_is_set() {return socketaddressserver_is_set;};

    unsigned int  host_portnr;    ///the portnumber on the host computer
    bool analog_to_send[8];
    bool connected;

private:
	void SendTCPClient(const string& str);
	bool socketaddressserver_is_set;
	in_addr_t socketaddressserver;
	sockaddr_in 	clientsockin;
};

class TcpClient
{  public:
	TcpClient();
	~TcpClient();
	void Open(string& ip_address,unsigned int portnr); //external address and portnumber
	void Open(in_addr_t& socketaddressserver,unsigned int portnr) ; //external address and portnumbervoid SendTCPClient(const string& str);


	void Close();
	void Send(const string& strtosend,const string& ipaddressstring, const unsigned int portnr);
	void Send(const string& stringtosend);
	void Open(struct in_addr& socketaddressserver,unsigned int portnr);  //external address and portnumber
	bool connected;
private:
	void SendTCPClient(const string& str);

	sockaddr_in 	clientsockin;
};

bool tcpstring_to_time(char* ts,time_t& time_in);

///*****************************************************************/////
////*******************         TCP-SERVER        ******************////
////*****************************************************************////
// The tcp-server is used answer unrequests of the peer computer
//  listens on the ports 4000 / 4004
//  supplies the ip-adress of the peer to the tcp-client

class TcpServer
{  public:
	  int listening_socket;
	  int current_socket_descriptor;

      TcpServer(){listening_socket=-1;current_socket_descriptor= -1;};
      TcpServer(int socket_nr,int sock_desc_currrent);
      ~TcpServer(){};

      void SetSocketDescriptors(int socket_nr,int sock_desc_currrent);
      bool process_socket();
      void send(char *str,int i=-1);
      void send(string& str);
      void send_socket_buf (int length); //sends prepared databuf /r/n added on end
      void send_socket_buf() ;
      bool strcmp_recieved_tcp_data(char const* str);
      void AdToTcpData_buf();
      void OutportStatusToTcp();
      int  Comp_Sch_namesToTcpData_buf();   //prepares comparator names basic properties and schedule names for the data buffer
      int  ScheduleToTcpData_buf(int nr);   //prepares  schedule properties
      int  ComparatorToTcpData_buf(int nr); //prepares  full comparator properties
      bool ReadComparatorFromTCPbuf (Comparator *comp);  //reads  full comparator properties
      void SendGPIO_Inf();                  //send portinfo (header and pin input or output
};

int twochar_to_int(char *ch);
bool format_string_to_struct_tm(char *ts, tm *t);
void time_to_tcpstring(char* ts, const time_t&  time_out);

bool port_is_free(int portnr);
int  ReadOutportFromTcpBuffer();

#endif /* TCPSERVER_HPP_ */
