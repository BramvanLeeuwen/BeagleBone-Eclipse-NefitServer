//
// Copyright (c) 2012 Janick Bergeron
// All Rights Reserved
//
//   Licensed under the Apache License, Version 2.0 (the
//   "License"); you may not use this file except in
//   compliance with the License.  You may obtain a copy of
//   the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in
//   writing, software distributed under the License is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
//   CONDITIONS OF ANY KIND, either express or implied.  See
//   the License for the specific language governing
//   permissions and limitations under the License.
//

#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>

#include "leds.hpp"

#define NMALLOC(n,t) ((t*) malloc((n)*sizeof(t)))
#define MALLOC(t)    NMALLOC(1,t)

namespace BeagleBone {

static led *m_singletons[BONE_NUM_LEDS];


led::led(short n)
{
  char *p = NMALLOC(256, char);

  sprintf(p,"/sys/class/leds/beaglebone:green:usr%d/", n);

  m_dev = p;
  m_devcat = p + strlen(p);
  *m_devcat = '\0';

  if (n == 0) m_trig = HEARTBEAT;
  else if (n == 1) m_trig = MMC0;
  else m_trig = NONE;

  m_is_on = 1;
}
  
led*
led::get(short n)
{
  if (n < 0 || n > BONE_NUM_LEDS-1) {
    fprintf(stderr, "ERROR: Invalid LED #%d. Must be in 0..%d.\n", n, BONE_NUM_LEDS-1);
    return NULL;
  }

  if (m_singletons[n] == NULL) m_singletons[n] = new led(n);

  return m_singletons[n];
}


void
led::restore_defaults()
{
  if (m_singletons[0] != NULL) m_singletons[0]->set_trigger(HEARTBEAT);
  if (m_singletons[1] != NULL) m_singletons[1]->set_trigger(MMC0);
  for (int i = 2; i < BONE_NUM_LEDS; i++) {
    if (m_singletons[i] != NULL) {
      m_singletons[i]->set_trigger(NONE);
      m_singletons[i]->off();
    }
  }
}


void
led::set_trigger(trigger_t trigger)
{
  static const char* trigvals[8] = {
    "none", "nand_disk", "mmc0", "timer", "heartbeat", "backlight", "gpio", "default_on"
  };
  FILE *fp;

  strcpy(m_devcat, "trigger");
  fp = fopen(m_dev, "w");
  if (fp == NULL) {
    fprintf(stderr, "ERROR: Cannot open \"%s\" for writing: ", m_dev);
    perror(0);
    return;
  }

  fprintf(fp, "%s", trigvals[trigger]);

  fclose(fp);
}


bool
led::is_on()
{
  return m_is_on;
}


void
led::on()
{
  FILE *fp;

  strcpy(m_devcat, "brightness");
  fp = fopen(m_dev, "w");
  if (fp == NULL) {
    fprintf(stderr, "ERROR: Cannot open \"%s\" for writing: ", m_dev);
    perror(0);
    return;
  }

  fprintf(fp, "255");
  m_is_on = 1;

  fclose(fp);
}


void
led::off()
{
  FILE *fp;

  strcpy(m_devcat, "brightness");
  fp = fopen(m_dev, "w");
  if (fp == NULL) {
    fprintf(stderr, "ERROR: Cannot open \"%s\" for writing: ", m_dev);
    perror(0);
    return;
  }

  fprintf(fp, "0");
  m_is_on = 0;

  fclose(fp);
}


void
led::timer(int on, int off)
{
  FILE *fp;

  if (m_trig != TIMER) set_trigger(TIMER);

  strcpy(m_devcat, "delay_on");
  fp = fopen(m_dev, "w");
  if (fp == NULL) {
    fprintf(stderr, "ERROR: Cannot open \"%s\" for writing: ", m_dev);
    perror(0);
    return;
  }

  fprintf(fp, "%d", on);

  fclose(fp);

  strcpy(m_devcat, "delay_off");
  fp = fopen(m_dev, "w");
  if (fp == NULL) {
    fprintf(stderr, "ERROR: Cannot open \"%s\" for writing: ", m_dev);
    perror(0);
    return;
  }

  fprintf(fp, "%d", off);

  fclose(fp);
}

}


#ifdef TEST
int
main(int argc, char *argv[])
{
  BeagleBone::led *led[4];

  for(int i = 0; i < 4; i++) {
    led[i] = BeagleBone::led::get(i);
    led[i]->set_trigger(BeagleBone::led::NONE);
    led[i]->off();
  }

  for(int i = 0; i < 4; i++) {
    sleep(1);
    led[i]->on();
  }
  for(int i = 0; i < 4; i++) {
    sleep(1);
    led[i]->off();
  }

  sleep(1);

  for(int i = 0; i < 4; i++) {
    led[i]->on();
  }
  sleep(1);

  for(int i = 0; i < 4; i++) {
    led[i]->off();
  }
  sleep(1);

  led[0]->timer(500, 500);
  led[1]->timer(1500, 500);
  led[2]->timer(1000, 1000);
  led[3]->timer(500, 1500);
  sleep(10);

  BeagleBone::led::restore_defaults();

  printf("Done!\n");
}

#endif
