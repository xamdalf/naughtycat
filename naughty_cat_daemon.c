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
#include <dirent.h>
#include <poll.h>
#include <fcntl.h>

#include <sys/socket.h> //used for socket to communicate with the animation layer
#include <sys/un.h>
#include <unistd.h> //used for unlink(), close(), and write() functions
#include <sys/stat.h> //used to handle user access to socket
#include "naughty_cat.h"

#define BACKLOG 5 //number of pending connections the socket can have before refusing new ones
#define EXT_ERR_TERMINATED -1 //error code for when the program is terminated by a signal
#define MAX_KEYBOARDS 20 //who runs more than 3 keyboards??? utter freaks. <3



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


int find_keyboards(int *fds, struct libevdev **devs) {
    struct dirent *entry;
    DIR *dir = opendir("/dev/input");
    if (!dir) return 0;

    int count = 0;

    while ((entry = readdir(dir)) != NULL && count < MAX_KEYBOARDS) {
        if (strncmp(entry->d_name, "event", 5) != 0) continue;

        char path[64];
        snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);

        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            fprintf(stderr, "  skipped %s: open failed (%s)\n", path, strerror(errno));
            continue;
        }

        struct libevdev *dev = NULL;
        if (libevdev_new_from_fd(fd, &dev) < 0) {
            fprintf(stderr, "  skipped %s: libevdev failed\n", path);
            close(fd);
            continue;
        }

        if (libevdev_has_event_type(dev, EV_KEY)) {
            fprintf(stderr, "  accepted %s: %s\n", path, libevdev_get_name(dev));
            fds[count] = fd;
            devs[count] = dev;
            count++;
        } else {
            fprintf(stderr, "  skipped %s: no EV_KEY (%s)\n", path, libevdev_get_name(dev));
            libevdev_free(dev);
            close(fd);
        }
    }

    closedir(dir);
    return count;
}



int main() {
    
    struct sigaction sa = { .sa_handler = handle_signal, .sa_flags = 0 }; // set up for signal handler when SIGINT is received (Ctrl+C)
    sigemptyset(&sa.sa_mask); // initialize the signal set to empty
    sigaction(SIGINT, &sa, NULL); // set the signal handler for SIGINT

    int connection_socket;
    int data_socket;
    struct sockaddr_un addr;
    int ret; //used to store return values from socket functions for error checking


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
    if (ret < 0) { //error
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


    int kb_fds[MAX_KEYBOARDS];
    struct libevdev *kb_devs[MAX_KEYBOARDS];
    int kb_count = 0;

    kb_count = find_keyboards(kb_fds, kb_devs);
    if (kb_count == 0) {
        fprintf(stderr, "Failed to find any keyboard device\n");
        exit(EXT_ERR_TERMINATED);
    }

    fprintf(stderr, "Found %d keyboard(s)\n", kb_count);
    for (int i = 0; i < kb_count; i++) {
        fprintf(stderr, "  keyboard %d: fd=%d name=%s\n", 
            i, kb_fds[i], libevdev_get_name(kb_devs[i]));
    }



    struct pollfd pollfds[MAX_KEYBOARDS];
    for (int i = 0; i < kb_count; i++) {
        pollfds[i].fd = kb_fds[i];
        pollfds[i].events = POLLIN;
    }

    while (running) {                          // OUTER: wait for renderer to connect

        data_socket = accept(connection_socket, NULL, NULL);

        while (running) {                      // INNER: read keyboard events
            int ready = poll(pollfds, kb_count, -1);
            if (ready == -1) {
                if (errno == EINTR) break;
                break;
            }

            for (int i = 0; i < kb_count; i++) {
                if (!(pollfds[i].revents & POLLIN)) continue;

                fprintf(stderr, "event from device %d: %s\n", i, libevdev_get_name(kb_devs[i]));

                struct input_event ev;
                int rc;
                while ((rc = libevdev_next_event(kb_devs[i], LIBEVDEV_READ_FLAG_NORMAL, &ev)) >= 0) {   // drains one device's queue
                    if (rc == LIBEVDEV_READ_STATUS_SYNC) {
                        while ((rc = libevdev_next_event(kb_devs[i], LIBEVDEV_READ_FLAG_SYNC, &ev)) == LIBEVDEV_READ_STATUS_SYNC)
                            ;                  // sync drain
                        continue;
                    }
                    // handle event, write to socket
                    key_event_t event = simplify(ev);
                    if (event.key_boop != -1) {
                        if (write(data_socket, &event, sizeof(event)) == -1) {
                            fprintf(stderr, "Renderer disconnected (%s)\n", strerror(errno));
                            goto renderer_disconnected; //ik, ik. don't hate.
                        }
                    }
                }
            }   
        }

        renderer_disconnected:
        close(data_socket);
    } 

    // cleanup
    for (int i = 0; i < kb_count; i++) {
        libevdev_free(kb_devs[i]);
        close(kb_fds[i]);
    }

    close(connection_socket);
    unlink(SOCKET_NAME);
    fprintf(stderr, "Program terminated\n");
    exit(EXT_ERR_TERMINATED);
}




/* --------------- TO DO -----------------



   --------------------------------------- */



/* --------------- BUG LOG ---------------



   --------------------------------------- */