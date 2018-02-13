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

#ifndef __BONELIB_LEDS__
#define __BONELIB_LEDS__

namespace BeagleBone {

#define BONE_NUM_LEDS 4

class led
{
public:
  /** Trigger modes */
  typedef enum {NONE, NAND_DISK, MMC0, TIMER, HEARTBEAT, BACKLIGHT, GPIO, DEFAULT_ON} trigger_t;

private:
  char *m_dev;         // Root path to LED device directory
  char *m_devcat;      // Where to append LED device file names to yield a full device file path

  bool      m_is_on;   // Is LED on?
  trigger_t m_trig;    // Current trigger mode for the LED

  /** Create an API class for LED #n */
  led(short n);
  
public:
  /** Get the API class singleton for LED #n */
  static led* get(short n);

  /** Restore the default LED settings */
  static void restore_defaults();

  /** Set the trigger for the LED to the specified mode */
  void set_trigger(trigger_t trigger);

  /** Return true if the LED is ON */
  bool is_on();

  /** Turn the LED ON */
  void on();

  /** Turn the LED off */
  void off();

  /** Set the LED to TIMER trigger mode and set the timer values to the specified # of msec.
   *  Somehow, some values cause the timer to hang... Have not figure out the common cause yet.
   */
  void timer(int on, int off);
};

}

#endif
