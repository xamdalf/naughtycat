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

#include <sys/socket.h> //used for socket to communicate with the animation layer
#include <sys/un.h>
#include <unistd.h> //used for unlink(), close(), and write() functions
#include "naughty_cat.h"

#define EXT_ERR_TERMINATED -1 //error code for when the program is terminated by a signal


volatile sig_atomic_t running = true;

void handle_signal(int sig) {
    if (sig == SIGINT) {
        running = false; //set the running flag to false to indicate that the program should terminate
    }
}


int main() {
    
    int data_socket;
    struct sockaddr_un addr;
    int ret; //used to store return values from socket functions for error checking
    int r;
    int last_paw = 0;
    key_event_t event; //used to store the simplified key event information received from the daemon


    data_socket = socket(AF_UNIX, SOCK_STREAM, 0); //SOCK_STREAM: read bytes as they arrive
    if (data_socket < 0) {
        fprintf(stderr, "Failed to create socket (%s)\n", strerror(errno));
        exit(EXT_ERR_TERMINATED);
    }

    memset(&addr, 0, sizeof(addr)); //zero out the address struct
    addr.sun_family = AF_UNIX; //set the address family to AF_UNIX

    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);
    ret = connect(data_socket, (const struct sockaddr *) &addr,
                   sizeof(addr));
    if (ret == -1) {
        fprintf(stderr, "The server is down.\n");
        exit(EXIT_FAILURE);
    }


    while(running) {

        r = read(data_socket, &event, sizeof(event));
        if (r == -1) {
            fprintf(stderr, "Failed to read (%s)\n", strerror(errno));
            break;
        }
        if (r == 0) {
            fprintf(stderr, "Daemon disconnected\n");
            break;
        }


        switch (event.key_boop) {

            case 0:
                //animate idle state with occasional blinking
                printf("IDLE\n");
                break;
            
            case 1:
                last_paw = !last_paw; //toggle between left and right boop animations
                if (last_paw) {
                    //animate right paw
                    printf("left boop\n");
                }
                else {
                    //animate right paw
                    printf("right boop\n");
                }
                break;

            default:
                //animate idle state with occasional blinking
                printf("weird behaviour... ?\n");
                break;
        }

    }

}