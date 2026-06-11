#define _POSIX_C_SOURCE 200809L //used to enable sigaction() function and sig_atomic_t type

#include <stdio.h>
#include <stdbool.h> //used for bool type True/False constants instead of 1 | 0
#include <stdlib.h> //used for exit() function
#include <string.h> //used for strerror() function to get error messages


#include <signal.h> //used to handle signals like SIGINT (Ctrl+C)
#include <libevdev/libevdev.h>
#include <fcntl.h>

#define EXT_ERR_TERMINATED -1 //error code for when the program is terminated by a signal

volatile sig_atomic_t running = true;

void handle_signal(int sig) {
    if (sig == SIGINT) {
        running = false; //set running to false to exit the main loop
    }
}


int main() {

    struct sigaction sa = { .sa_handler = handle_signal, .sa_flags = SA_RESTART }; // signal handler for SIGINT (Ctrl+C)
    sigemptyset(&sa.sa_mask); // initialize the signal set to empty
    sigaction(SIGINT, &sa, NULL); // set the signal handler for SIGINT


    struct libevdev *dev = NULL;
    int fd;
    int rc = 1;

    fd = open("/dev/input/event4", O_RDONLY); // | NONBLOCK); //BLOCKING or NONBLOCKING??
    rc = libevdev_new_from_fd(fd, &dev);

    if(rc != 0) { //error 
        fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
        running = false;
    }

    while(running) {
        fprintf(stderr, "yippeee\n");

    }




    printf("Program terminated by signal.\n");
    exit(EXT_ERR_TERMINATED);
}