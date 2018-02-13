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

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>

#include "kbdio.hpp"

namespace BeagleBone {

int
kbdio::kbhit()
{
  // From: http://kasperd.net/~kasperd/comp.os.linux.development/termios.txt
  int cnt = 0;
  int error;
  static struct termios Otty, Ntty;

  tcgetattr(0, &Otty);
  Ntty = Otty;

  Ntty.c_iflag     = 0;       /* input mode*/
  Ntty.c_oflag     = 0;       /* output mode*/
  Ntty.c_lflag    &= ~ICANON; /* raw mode */
  Ntty.c_cc[VMIN]  = 1;       /* minimum chars to wait for */
  Ntty.c_cc[VTIME] = 1;       /* minimum wait time*/

  if (0 == (error = tcsetattr(0, TCSANOW, &Ntty))) {
    struct timeval tv;
    error     += ioctl(0, FIONREAD, &cnt);
    error     += tcsetattr(0, TCSANOW, &Otty);
    tv.tv_sec  = 0;
    tv.tv_usec = 100;   /* insert a minimal delay */
    select(1, NULL, NULL, NULL, &tv);
  }

  return (error == 0 ? cnt : -1 );
}
  

char
kbdio:: getch()
{
  char ch;
  int error;
  static struct termios Otty, Ntty;

  fflush(stdout);
  tcgetattr(0, &Otty);
  Ntty = Otty;

  Ntty.c_iflag  =  0;/* input mode*/
  Ntty.c_oflag  =  0;/* output mode*/
  Ntty.c_lflag &= ~ICANON;/* line settings */

#if 1
  /* disable echoing the char as it is typed */
  Ntty.c_lflag &= ~ECHO;/* disable echo */
#else
  /* enable echoing the char as it is typed */
  Ntty.c_lflag |=  ECHO;/* enable echo */
#endif

  Ntty.c_cc[VMIN]  = 1;/* minimum chars to wait for */
  Ntty.c_cc[VTIME] = 1;/* minimum wait time*/

#if 1
  /*
   * use this to flush the input buffer before blocking for new input
   */
#define FLAG TCSAFLUSH
#else
  /*
   * use this to return a char from the current input buffer, or block if
   * no input is waiting.
   */
#define FLAG TCSANOW

#endif

  if (0 == (error = tcsetattr(0, FLAG, &Ntty))) {
    error  = read(0, &ch, 1 );          /* get char from stdin */
    error += tcsetattr(0, FLAG, &Otty);   /* restore old settings */
  }

  return (error == 1 ? (int) ch : -1 );
}

}

#ifdef TEST
#include <ctype.h>

int
main(void)
{
  int ch, cnt;
  static struct termios Otty, Ntty;

  tcgetattr(0, &Otty);
  Ntty = Otty;
  Ntty.c_lflag &= ~ECHO;

  printf("You must enter 10 characters to get\nthis program to continue:  ");
  fflush(stdout);
  /* collect 10 characters */
  while (1) {
    if (10 <= (cnt = BeagleBone::kbdio::kbhit())) {
      break;
    }
  }

  tcsetattr(0, TCSANOW, &Ntty); /* disable echoing of further input */
  printf("\nSTOP!");
  fflush(stdout);
  printf("\nNow type <Enter> to continue!");
  fflush(stdout);
  ch = getchar();
  tcsetattr(0, TCSANOW, &Otty); /* enable echoing of further input */
  printf("\n\nThe first five characters are:  \"");
  /*
   * print a few chars, note that calling getch() will flush
   * remaining buffered input.
   */
  cnt = 4;
  do {
    printf("%c", ch);
    ch = getchar();
  } while  (--cnt);
  printf("%c\"\n\n", ch);
  
  printf("\n\n      ***  Demo Menu  ***\n\n");
  printf("      Option       Action\n");
  printf("        A          Exit and print an A word\n");
  printf("        B          Exit and print a  B word\n");
  printf("        C          Exit and print a  C word\n");
  printf("\n      Enter your choice:  ?\b");
  fflush(stdout);

  while(1) {
    switch (ch = BeagleBone::kbdio::getch()) {
    case 'a': case 'A':
      printf("%c\n\nThat is an *awesome* function!\n", toupper(ch));
      break;
    case 'b': case 'B':
      printf("%c\n\nThat is a *beneficial* function!\n", toupper(ch));
      break;
    case 'c': case 'C':
      printf("%c\n\nThat is a *critical* function!\n", toupper(ch));
      break;
    default:
      continue;
    }
    break;
  }

  printf("Done!\n");

  return 0;
}

#endif

