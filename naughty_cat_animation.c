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
#include <glib-unix.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>
#include <gtk/gtk.h>
#include "naughty_cat.h"

#define EXT_ERR_TERMINATED -1 //error code for when the program is terminated by a signal
#define CAT_HEIGHT 80
#define CAT_WIDTH (int)(2.4 * CAT_HEIGHT)


int data_socket;
volatile int last_paw = 0;

static int cat_x = 0;   // distance from right edge
static int cat_y = 30;    // distance from bottom edge
static volatile int flipped = 0;

typedef struct {
    GtkWidget *picture;
    GtkWindow *window;
    int screen_width;
} DragData;



void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("Program terminated by signal\n");
        g_application_quit(g_application_get_default());
    }
}



void set_frame(GtkWidget *picture, const char *filename) {
    gtk_picture_set_filename(GTK_PICTURE(picture), filename);
}

void refresh_frame(GtkWidget *picture) {
    set_frame(picture, flipped ? "idle_flipped.png" : "idle.png");
    printf("flipped!\n");
}


gboolean on_socket_data(gint fd, GIOCondition condition, gpointer userdata) {
    GtkWidget *picture = GTK_WIDGET(userdata);
    key_event_t event;

    int r = read(fd, &event, sizeof(event));
    if (r <= 0) {
        fprintf(stderr, "Daemon disconnected\n");
        return G_SOURCE_REMOVE;
    }
    
    switch (event.key_boop) {
        case 0:
            set_frame(picture, flipped ? "idle_flipped.png" : "idle.png");
            printf("IDLE\n");
            break;
        case 1:
            if (flipped) {
                last_paw = !last_paw;
                set_frame(picture, last_paw ? "left_boop_flipped.png" : "right_boop_flipped.png");
                
                if (last_paw) {
                    printf("left boop\n");
                }
                else {
                    printf("right boop\n");
                }
            }
            else {
                last_paw = !last_paw;
                set_frame(picture, last_paw ? "left_boop.png" : "right_boop.png");
                
                if (last_paw) {
                    printf("left boop\n");
                }
                else {
                    printf("right boop\n");
                }
            }
            
            break;
        case 2:
            if (flipped) {
                last_paw = !last_paw;
                set_frame(picture, last_paw ? "left_boop_flipped.png" : "right_boop_flipped.png");
                
                printf("booooop\n");
            }
            else {
                last_paw = !last_paw;
                set_frame(picture, last_paw ? "left_boop.png" : "right_boop.png");
                
                printf("booooop\n");
            }
    }
    return G_SOURCE_CONTINUE;
}


void update_flip(int screen_width) {
    flipped = (cat_x > screen_width / 2);
}


void on_drag_update(GtkGestureDrag *gesture, double dx, double dy, gpointer userdata) {
    DragData *data = (DragData *)userdata;

    cat_x -= (int)dx;
    cat_y -= (int)dy;

    if (cat_x < 0) cat_x = 0;
    if (cat_y < 0) cat_y = 0;

    int was_flipped = flipped;
    update_flip(data->screen_width);
    if (flipped != was_flipped) {
        refresh_frame(data->picture);
    }
    gtk_layer_set_margin(data->window, GTK_LAYER_SHELL_EDGE_RIGHT, cat_x);
    gtk_layer_set_margin(data->window, GTK_LAYER_SHELL_EDGE_BOTTOM, cat_y);
}



static void on_activate(GtkApplication *app, gpointer user_data) {

    //CSS
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css, "window { background: transparent; }");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    
    //window
    GtkWidget *window = gtk_application_window_new(app);
    
    //get screen width
    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor = g_list_model_get_item(gdk_display_get_monitors(display), 0);
    GdkRectangle geometry;
    gdk_monitor_get_geometry(monitor, &geometry);
    int screen_width = geometry.width;


    //layer shell
    gtk_layer_init_for_window(GTK_WINDOW(window));
    gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);
    gtk_layer_set_keyboard_mode(GTK_WINDOW(window), GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, 0);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_BOTTOM, cat_y);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, cat_x);
    gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, 0);

    //image
    GtkWidget *picture;
    picture = gtk_picture_new_for_filename("idle.png");
    gtk_picture_set_content_fit(GTK_PICTURE(picture), GTK_CONTENT_FIT_CONTAIN);
    gtk_window_set_default_size(GTK_WINDOW(window), CAT_WIDTH, CAT_HEIGHT);
    // gtk_widget_set_size_request(picture, CAT_WIDTH, CAT_HEIGHT);  // half native size, keeps ratio
    gtk_window_set_child(GTK_WINDOW(window), picture);

    //drag window
    GtkGesture *drag = gtk_gesture_drag_new();
    gtk_widget_add_controller(GTK_WIDGET(picture), GTK_EVENT_CONTROLLER(drag));
    DragData *drag_data = g_new(DragData, 1);
    drag_data->picture = picture;
    drag_data->window = GTK_WINDOW(window);
    drag_data->screen_width = screen_width;

    g_signal_connect_data(drag, "drag-update", G_CALLBACK(on_drag_update), drag_data,
                      (GClosureNotify)g_free, 0);

    //watch socket
    g_unix_fd_add(data_socket, G_IO_IN, on_socket_data, picture);
    // click-through — empty input region
    gtk_widget_set_can_focus(window, FALSE);   // window can't receive keyboard focus
    gtk_widget_set_can_target(window, TRUE);  // window can't receive mouse input events
    gtk_widget_set_visible(window, TRUE);
}





int main(int argc, char *argv[]) {

    // socket setup — before GTK starts
    struct sockaddr_un addr;
    data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (data_socket < 0) {
        fprintf(stderr, "Failed to create socket (%s)\n", strerror(errno));
        exit(EXT_ERR_TERMINATED);
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);
    if (connect(data_socket, (const struct sockaddr *) &addr, sizeof(addr)) == -1) {
        fprintf(stderr, "The server is down.\n");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa = { .sa_handler = handle_signal, .sa_flags = 0 };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    // GTK takes over from here
    GtkApplication *app = gtk_application_new("cat.naughty", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    close(data_socket);
    return status;
}


/* --------------- TO DO -----------------



   --------------------------------------- */



/* --------------- BUG LOG ---------------

--the window dimensions aren't sized correctly: 
the left side goes out of the screen, 
but the right side seems to be well placed?

^need to adjust window dimension definitions

   --------------------------------------- */