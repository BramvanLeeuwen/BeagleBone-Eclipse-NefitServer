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

#ifndef __BONELIB_KBDIO__
#define __BONELIB_KBDIO__

namespace BeagleBone {

/** Raw, unbuffered keyboard IO API */
class kbdio
{
public:
  /** Return the number of characters pending in the keyboard buffer */
  static int kbhit();
  
  /** Return the next character from the keyboard buffer or -1 if error (blocking)*/
  static char getch();
};

}

#endif
