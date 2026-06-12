#define _POSIX_C_SOURCE 200809L //used to enable sigaction() function and sig_atomic_t type

#include <stdio.h>
#include <stdbool.h> //used for bool type True/False constants instead of 1 | 0
#include <stdlib.h> //used for exit() function
#include <string.h> //used for strerror() function to get error messages
#include <errno.h>

#include <signal.h> //used to handle signals like SIGINT (Ctrl+C)
#include <libevdev/libevdev.h>
#include <fcntl.h>

#define EXT_ERR_TERMINATED -1 //error code for when the program is terminated by a signal


typedef struct {
    int key_boop; //key is pressed (used to strip ev struct of other key information)
} key_event_t;

key_event_t simplify(struct input_event ev) {
    if (ev.type == EV_KEY) {
        return (key_event_t) { .key_boop = ev.value }; //return a simplified struct with only the key press information
    }
    return (key_event_t) { .key_boop = -1}; //not a key event. return invalid
}


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


    struct input_event ev;

    struct libevdev *dev = NULL;
    int fd;
    int rc = 1;

    fd = open("/dev/input/event4", O_RDONLY); // | NONBLOCK); //BLOCKING or NONBLOCKING??

        if(fd < 0) { //error
        fprintf(stderr, "Failed to open device (%s)\n", strerror(errno));
        running = false;
    }


    rc = libevdev_new_from_fd(fd, &dev);

    if(rc < 0) { //error
        fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
        running = false;
    }


    while(running) {

        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {

            // if (ev.type == EV_KEY) {
                if (simplify(ev).key_boop == 1) {
                    printf("boop\n");
                }
                else if (simplify(ev).key_boop == 2) {
                    printf("booooop\n");
                }
                else if (simplify(ev).key_boop == 0) {
                    printf("unbooped\n");
                }
            // }


            // if (ev.type == EV_KEY) {
            //     if (ev.value == 1){
            //         printf("boop\n");
            //     }
            //     else if (ev.value == 2){
            //         printf("boooop\n");
            //     }
            //     else if (ev.value == 0){
            //         printf("unbooped\n");
            //     }
            // }
        }
        
        // fprintf(stderr, "yippeee\n");

    }

    printf("Program terminated by signal.\n");
    exit(EXT_ERR_TERMINATED);
}




/*

key_event_t simplify(struct input_event ev) {
    if (ev.type == EV_KEY) {
        return (key_event_t) { .key_boop = ev.value }; //return a simplified struct with only the key press information
    }

    // does else need to be handled?? -------------------------------------------------

}


want to remove ev.type from main()

if (ev.type == EV_KEY) {
                if (simplify(ev).key_boop == 1) {
                    printf("boop\n");
                }
                else if (simplify(ev).key_boop == 2) {
                    printf("booooop\n");
                }
                else if (simplify(ev).key_boop == 0) {
                    printf("unbooped\n");
                }
            }

removing ev.type from main causes 'unbooped' to trigger, even on boooop

might be losing ev.value == 2 due to if (ev.type == EV_KEY) in simplify()??

*/