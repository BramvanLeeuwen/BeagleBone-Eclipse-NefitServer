
#ifndef TTCPTEST_HPP
#define TTCPTEST_HPP

/*

  HELPER.H
  ========


#ifndef PG_SOCK_HELP
#define PG_SOCK_HELP


#include <unistd.h>             //  for ssize_t data type

#define LISTENQ        (1024)   //  Backlog for listen()
*/


/*  Function declarations  */

ssize_t Readline(int fd, void *vptr, size_t maxlen);
ssize_t Writeline(int fc, const void *vptr, size_t maxlen);

void *PrintHello(void *threadid);
void *TcpServerThread (void *threadarg);

#endif  /*  PG_SOCK_HELP  */
