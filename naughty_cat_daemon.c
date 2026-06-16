/*

NAUGHTY CAT DAEMON

This program is a daemon that runs in the background and listens for keyboard input events using the libevdev library. 
It sets up a UNIX domain socket server to communicate with an animation layer and sends simplified key event information 
whenever a key is pressed or released.

    |\__/,|   (`\
  _.|o o  |_   ) )
-(((---(((--------
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
#include <unistd.h> //used for unlink(), close(), and write() functions
#include <sys/stat.h> //used to handle user access to socket
#include "naughty_cat.h"

#define BACKLOG 5 //number of pending connections the socket can have before refusing new ones
#define EXT_ERR_TERMINATED -1 //error code for when the program is terminated by a signal



key_event_t simplify(struct input_event ev) {
    if (ev.type == EV_KEY) {
        return (key_event_t) { .key_boop = ev.value }; //return a simplified struct with only the key press information
    }
    return (key_event_t) { .key_boop = -1}; //not a valid key event. return invalid
}

volatile sig_atomic_t running = true;

void handle_signal(int sig) {
    if (sig == SIGINT) {
        running = false; //set the running flag to false to indicate that the program should terminate
    }
}


int main() {
    
    struct sigaction sa = { .sa_handler = handle_signal, .sa_flags = 0 }; // set up for signal handler when SIGINT is received (Ctrl+C)
    sigemptyset(&sa.sa_mask); // initialize the signal set to empty
    sigaction(SIGINT, &sa, NULL); // set the signal handler for SIGINT

    int connection_socket;
    int data_socket;
    struct sockaddr_un addr;
    int ret; //used to store return values from socket functions for error checking

    // char buffer[BUFFER_SIZE];
    // int w;
    // int result;

    struct input_event ev;
    struct libevdev *dev = NULL;
    int fd;
    int rc = 1;


    //setup server socket for communication with the animation layer
    connection_socket = socket(AF_UNIX, SOCK_STREAM, 0); //SOCK_STREAM: read bytes as they arrive
    if (connection_socket < 0) {
        fprintf(stderr, "Failed to create socket (%s)\n", strerror(errno));
        exit(EXT_ERR_TERMINATED);
    }

    memset(&addr, 0, sizeof(addr)); //zero out the address struct
    addr.sun_family = AF_UNIX; //set the address family to AF_UNIX
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1); //set the socket path
    unlink(SOCKET_NAME); //remove any existing socket file at the path to prevent bind errors
    ret = bind(connection_socket, (const struct sockaddr *) &addr, sizeof(addr)); //bind the socket to the address
    if (ret < 0) {
        fprintf(stderr, "Failed to bind socket (%s)\n", strerror(errno));
        close(connection_socket);
        exit(EXT_ERR_TERMINATED);
    }

    //make socket accessible to animation level user group
    chmod(SOCKET_NAME, 0660);  // owner + group only

    //listen
    ret = listen(connection_socket, BACKLOG); //listen for incoming connections
    if (ret == -1) {
        fprintf(stderr, "Failed to listen on socket (%s)\n", strerror(errno));
        close(connection_socket);
        exit(EXT_ERR_TERMINATED);
    }


    //setup device input reading with libevdev
    fd = open("/dev/input/event4", O_RDONLY); //open the input device file for the keyboard (this may need to be changed depending on the system and keyboard configuration)

        if(fd < 0) { //error
        fprintf(stderr, "Failed to open device (%s)\n", strerror(errno));
        exit(EXT_ERR_TERMINATED); //exit immediately with termination error code
    }

    rc = libevdev_new_from_fd(fd, &dev);

    if(rc < 0) { //error
        fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
        exit(EXT_ERR_TERMINATED); //exit immediately with termination error code
    }



    while(running) {

        data_socket = accept(connection_socket, NULL, NULL); //accept an incoming connection
        if (data_socket == -1) {
            fprintf(stderr, "Failed to accept connection (%s)\n", strerror(errno));
            exit(EXT_ERR_TERMINATED);         
        }


        while(running) {

            rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
            
            if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {    
                key_event_t event = simplify(ev);
                
                if (event.key_boop != -1) { //only send events that are valid key presses/releases
                    
                    if (write(data_socket, &event, sizeof(event)) == -1) {
                    
                        fprintf(stderr, "Renderer disconnected (%s)\n", strerror(errno));
                        break;  // exit inner loop, go back to accept() for new connection
                    }
                }

                    // if (simplify(ev).key_boop == 1) {
                    //     printf("boop\n");
                    //     w = write(data_socket, "1", 1); //send a byte to the animation layer to indicate a key press
                    // }
                    // else if (simplify(ev).key_boop == 2) {
                    //     printf("booooop\n");
                    //     w = write(data_socket, "2", 1); //send a byte to the animation layer to indicate a key release
                    // }
                    // else if (simplify(ev).key_boop == 0) {
                    //     printf("unbooped\n");
                    //     w = write(data_socket, "0", 1); //send a byte to the animation layer to indicate a key release
                    // }
            }
        }    

        close(data_socket); //close the connection socket
    }

    close(connection_socket); //close the server socket when the program is terminating
    unlink(SOCKET_NAME); //remove the socket file when the program is terminating
    fprintf(stderr, "Program terminated\n");
    exit(EXT_ERR_TERMINATED);
}




/* --------------- TO DO -----------------



   --------------------------------------- */



/* --------------- BUG LOG ---------------



   --------------------------------------- */