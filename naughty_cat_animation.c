/*

NAUGHTY CAT ANIMATION LAYER

This program is an animation layer that runs in the background and listens for simplified key event information
from the naughty cat daemon using a UNIX domain socket.

 _._     _,-'""`-._
(,-.`._,'(       |\`-/|
    `-.-' \ )-`( , o o)
          `-    \`_`"'-
artist: unkown  [ASCII art archive]
*/


#define _POSIX_C_SOURCE 200809L //used to enable sigaction() function and sig_atomic_t type

#include <stdio.h>
#include <stdbool.h> //used for bool type True/False constants instead of 1 | 0
#include <stdlib.h> //used for exit() function
#include <string.h> //used for strerror() function to get error messages
#include <errno.h>

#include <signal.h> //used to handle signals like SIGINT (Ctrl+C)
#include <libevdev/libevdev.h>
#include <fcntl.h>

#include <sys/socket.h> //used for socket to communicate with the animation layer
#include <sys/un.h>


#define SOCKET_NAME "/tmp/naughty_cat.sock" //path for the UNIX domain socket
#define BACKLOG 5 //number of pending connections the socket can have before refusing new ones
#define EXT_ERR_TERMINATED -1 //error code for when the program is terminated by a signal


