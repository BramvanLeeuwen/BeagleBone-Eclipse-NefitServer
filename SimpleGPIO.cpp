/*
 * SimpleGPIO.cpp
 *
 * Modifications by Derek Molloy, School of Electronic Engineering, DCU
 * www.derekmolloy.ie
 * Almost entirely based on Software by RidgeRun:
 *
 * Copyright (c) 2011, RidgeRun
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the RidgeRun.
 * 4. Neither the name of the RidgeRun nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY RIDGERUN ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL RIDGERUN BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "SimpleGPIO.hpp"
//#include "RotaryEncoder.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

//extern char slotsfilename[strlen(SLOTS_FILE)];// =      "/sys/devices/bone_capemgr.x/slots";  x=8 on Angstrom x=9 on Debian and Ubuntu
char slotsfilename[]= SLOTS_FILE;  //  "/sys/devices/bone_capemgr.9/slots";
/****************************************************************
 * gpio_export
 ****************************************************************/
int gpio_export(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);

	return 0;
}

/****************************************************************
 * gpio_unexport
 ****************************************************************/
int gpio_unexport(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
	return 0;
}
// ***************************************************************
// * gpio_exported  test function whether gpio was exported; true if gpio is already exported
// ****************************************************************
bool gpio_exported(unsigned int gpio)
{	char buf[MAX_BUF];
	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
	return file_exists(buf);
}



bool file_exists(char* filename)
{  //printf(filename);printf("\n\r");
   return (access( filename, F_OK ) != -1 );
}


/****************************************************************
 * gpio_set_dir
 ****************************************************************/
int gpio_set_dir(unsigned int gpio, PIN_DIRECTION out_flag)
{	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/direction");
		return fd;
	}

	if (out_flag == OUTPUT_PIN)
		write(fd, "out", 4);
	else
		write(fd, "in", 3);

	close(fd);
	return 0;
}

/****************************************************************
 * gpio_set_value
 ****************************************************************/
int gpio_set_value(unsigned int gpio, PIN_VALUE value)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		printf ("Error gpio nr.%i; value: %i",gpio,value);perror("gpio/set-value");
		return fd;
	}

	if (value==LOW)
		write(fd, "0", 2);
	else
		write(fd, "1", 2);

	close(fd);
	return 0;
}

/****************************************************************
 * gpio_get_value
 ****************************************************************/
int gpio_get_value(unsigned int gpio, unsigned int *value)
{
	int fd;
	char buf[MAX_BUF];
	char ch;

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		perror("gpio/get-value");
		return fd;
	}

	read(fd, &ch, 1);

	if (ch != '0') {
		*value = 1;
	} else {
		*value = 0;
	}

	close(fd);
	return 0;
}


/****************************************************************
 * gpio_set_edge
 ****************************************************************/

int gpio_set_edge(unsigned int gpio, char *edge)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/set-edge");
		return fd;
	}

	write(fd, edge, strlen(edge) + 1);
	close(fd);
	return 0;
}

int gpio_set_edge(unsigned int gpio, PIN_EDGE edg){
	char edge[8];
	switch (edg){
		case NON_BLOCKING: {
			strcpy(edge,"none");
			break;
		}
		case RISING_EDGE:  {
			strcpy(edge,"rising");  //set a blocking read on rising edge
			break;
		}
		case FALLING_EDGE: {
			strcpy(edge,"falling");
					break;
		}
		case BOTH_EDGES:   {
			strcpy(edge,"both");
			break;
		}
	}
	return gpio_set_edge(gpio,edge);
}


//****************************************************************//
// * gpio_fd_open
//****************************************************************//

int gpio_fd_open(unsigned int gpio)
{
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

	fd = open(buf, O_RDONLY | O_NONBLOCK );
	if (fd < 0) {
		perror("gpio/fd_open");
	}
	return fd;
}

// ****************************************************************//
// * gpio_fd_close
// ****************************************************************//

int gpio_fd_close(int fd)
{
	return close(fd);
}

char get_distro(string& opname)
{	char buffer[MAX_BUF];
	string::size_type pos,r,l;
	string line;
	snprintf(buffer, sizeof(buffer),"/etc/os-release");
	if (file_exists(buffer))
	{   ifstream inputstream(buffer);
		while(inputstream.good())
		{	pos=0;
			getline(inputstream,line); // get line from file
			pos=line.find("NAME="); // search
			if (pos != string::npos) // string::npos is returned if string is not found
			{	//cout << "Distro name found" << endl;
			    l=line.find_first_of('"');
			       if (l==string::npos)   //no quotation marks: use position 5 als eerste letter NAME= takes 5 positions
			          l=4;
			    r=line.find_last_of('"');
			    opname=line.substr(l+1,r-l-1);
			    pos=opname.find("Debian");
			    if (pos != string::npos)  //found
			    	return 'D';
			    pos=opname.find("Angstrom");
			    if (pos != string::npos)  //found
			    	return 'A';
			    else return '?';
			}
	    }
	}
	else
		cout << "Distro name file not found!" << endl;
	return '?';
}

void adapt_slots_file_name_to_distro()
{   string distroname;
	char distro;
	distro=get_distro(distroname);
	  if (distro=='A')  //Angstrom
	   slotsfilename[26]='8';
}

// ****************************************************************//
// * adc_export
// ****************************************************************//
int adc_export()
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SLOTS_FILE, O_WRONLY);
	if (fd < 0) {
		perror("ad_export");
		return fd;
	}
	len = snprintf(buf, sizeof(buf), "cape-bone-iio");
	write(fd, buf, len);
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_omap_mux_setup - Allow us to setup the omap mux mode for a pin
 ****************************************************************/
int gpio_omap_mux_setup(const char *omap_pin0_name, const char *mode)
{
	int fd;
	char buf[MAX_BUF];
	snprintf(buf, sizeof(buf), SYSFS_OMAP_MUX_DIR "%s", omap_pin0_name);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("failed to open OMAP_MUX");
		return fd;
	}
	write(fd, mode, strlen(mode) + 1);
	close(fd);
	return 0;
}


