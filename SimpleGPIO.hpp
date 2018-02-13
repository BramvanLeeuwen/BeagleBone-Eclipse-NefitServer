/*
 * SimpleGPIO.h
 *
 * Copyright Derek Molloy, School of Electronic Engineering, Dublin City University
 * www.derekmolloy.ie
 *
 * Based on Software by RidgeRun
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

#ifndef SIMPLEGPIO_H_
#define SIMPLEGPIO_H_
#include <string.h>
 /****************************************************************
 * Constants
 ****************************************************************/

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define POLL_TIMEOUT (3 * 1000) /* 3 seconds */
#define MAX_BUF 64
#define SYSFS_OMAP_MUX_DIR "/sys/kernel/debug/omap_mux/"
#define SLOTS_FILE      "/sys/devices/bone_capemgr.9/slots"


enum PIN_DIRECTION{
	INPUT_PIN=0,
	OUTPUT_PIN=1,
	DIRECTION_NOT_DEFINED=2
};

enum PIN_VALUE{
	LOW=0,
	HIGH=1
};

enum PIN_EDGE{
	NON_BLOCKING=0,
	RISING_EDGE=1,
	FALLING_EDGE=2,
	BOTH_EDGES=3,
};

/****************************************************************
 * gpio_export
 ****************************************************************/
int  gpio_export	(unsigned int gpio);       // exports a gpio pin
int  gpio_unexport	(unsigned int gpio);
bool gpio_exported	(unsigned int gpio);       //controls whether the gpio is exported
bool gpio_devicetree_present(char *treename);  //controls whether the devicetree ovelay is loaded
int  gpio_set_dir	(unsigned int gpio, PIN_DIRECTION out_flag);
int  gpio_set_value	(unsigned int gpio, PIN_VALUE value);
int  gpio_get_value	(unsigned int gpio, unsigned int *value);
int  gpio_set_edge	(unsigned int gpio, char *edge);   // helper function
int  gpio_set_edge	(unsigned int gpio, PIN_EDGE edg); // sets the pin to blocking (edge: rising edge, falling e or both)
                                                       // or non blocking
int  gpio_fd_open	(unsigned int gpio);
int  gpio_fd_close	(int fd);
int  gpio_omap_mux_setup(const char *omap_pin0_name, const char *mode);
/****************************************************************
 *export all seven adconverter ports
 ****************************************************************/
int   adc_export();

void  adapt_slots_file_name_to_distro();

bool  file_exists(char* filename);

#endif /* SIMPLEGPIO_H_ */
