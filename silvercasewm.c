#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

const unsigned int FRAME_OFFSET = 30;
const unsigned int SELECTABLE_OFFSET = 15;
const unsigned int MOVING_SELECTABLE_OFFSET = 40;
const unsigned int BORDER_WIDTH = 5;
const unsigned int BORDER_COLOR = 0xc8ffff;
const unsigned int BG_COLOR = 0x00000000;
const unsigned int BUTTON_WIDTH = 20;
const unsigned int BUTTON_COLOR = 0xffffff;
const unsigned int BUTTON_BORDER_WIDTH = 2;
const unsigned int BUTTON_BORDER_COLOR = 0x000000;
const unsigned int INITIAL_CLIENTS_SIZE = 5;

typedef struct {
    int xid;
    int frame_xid;
    int x;
    int y;
    int width;
    int height;
    int reparenting;
} Client;

enum ClientColumn{
    BASEWINDOW,
    FRAMEWINDOW
};

Display *dpy;
GC gc;
int screen;

Window root;
int root_width;
int root_height;

Client *clients;
int clients_amount = 0;
int focused_client = 0;
XButtonEvent last_pressed_event;

int getClientIndex(Window client);
int getClientIndexWithColumnIndex(Window client, int *column_index);

void update_client(int client_index){
    XWindowAttributes f_wa;
    XGetWindowAttributes(dpy, clients[client_index].frame_xid, &f_wa);
    
    clients[client_index].x = f_wa.x;
    clients[client_index].y = f_wa.y;
    clients[client_index].width = f_wa.width;
    clients[client_index].height = f_wa.height;
}

void manage(Window w, XWindowAttributes *wa)
{
    XSetWindowBorderWidth(dpy, w, 0);

    int window_width = wa->width;
    int window_height = wa->height;
    int half_frame_offset = FRAME_OFFSET/2;

    const Window frame = XCreateSimpleWindow(
        dpy, 
        root, 
        wa->x,
        wa->y,
        wa->width + FRAME_OFFSET,
        wa->height + FRAME_OFFSET,
        BORDER_WIDTH,
        BORDER_COLOR,
        BG_COLOR
    );

    int btn_initial_pos = wa->x + (wa->width - (FRAME_OFFSET/2));
    int btn_y_pos = wa->y - ((BUTTON_WIDTH/2) + BORDER_WIDTH);
    const Window erase_btn = XCreateSimpleWindow(
        dpy, frame,
        btn_initial_pos,
        btn_y_pos,
        BUTTON_WIDTH,
        BUTTON_WIDTH,
        BUTTON_BORDER_WIDTH,
        BUTTON_BORDER_COLOR,
        BUTTON_COLOR
    );
    const Window maximize_btn = XCreateSimpleWindow(
        dpy, frame,
        btn_initial_pos - BUTTON_WIDTH,
        btn_y_pos,
        BUTTON_WIDTH,
        BUTTON_WIDTH,
        BUTTON_BORDER_WIDTH,
        BUTTON_BORDER_COLOR,
        BUTTON_COLOR
    );
    const Window minimize_btn = XCreateSimpleWindow(
        dpy, frame,
        btn_initial_pos - (BUTTON_WIDTH*2),
        btn_y_pos,
        BUTTON_WIDTH,
        BUTTON_WIDTH,
        BUTTON_BORDER_WIDTH,
        BUTTON_BORDER_COLOR,
        BUTTON_COLOR
    );

    // XSelectInput(dpy, root, 0);
    XAddToSaveSet(dpy, w);
    
    XReparentWindow(dpy, w, frame, FRAME_OFFSET/2,FRAME_OFFSET/2);
    XGrabButton(dpy, 1, AnyModifier, frame, True, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    XGrabButton(dpy, 1, AnyModifier, frame, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    
    XMapWindow(dpy, erase_btn);
    XMapWindow(dpy, maximize_btn);
    XMapWindow(dpy, minimize_btn);
    XMapWindow(dpy, frame);
    XSync(dpy, 0);

    XSelectInput(dpy, frame, SubstructureNotifyMask);

    int add_to_index = clients_amount;
    for (int i=0;i<clients_amount;i++){
        if (clients[i].xid == 0){
            add_to_index = i;
            break;
        }
    }
    
    clients[add_to_index].xid = w;
    clients[add_to_index].frame_xid = frame;
    clients[add_to_index].reparenting = 1;
    update_client(add_to_index);

    clients_amount += add_to_index == clients_amount ? 1 : 0;
    
    printf("    add new client : %d \n", (int) w);
    printf("    all clients :\n");
    for (int i=0;i<clients_amount;i++){
        printf("    --[%d] = w : %d, f : %d \n", i, clients[i].xid, clients[i].frame_xid);
    }
}

void unmanage(int client_index){
    XDestroyWindow(dpy, (Window) clients[client_index].frame_xid);
    
    clients[client_index].xid = 0;
    clients[client_index].frame_xid = 0;
    clients_amount -= 1;
}

void on_button_press(XButtonPressedEvent ev){
    int client_index;
    if ((client_index = getClientIndex(ev.window)) != -1){
        XRaiseWindow(dpy, ev.window);

        XWindowAttributes attr;
        XGetWindowAttributes(dpy, ev.window, &attr);
        if (
            ev.x <= SELECTABLE_OFFSET||
            ev.x >= attr.width - SELECTABLE_OFFSET||
            ev.y <= SELECTABLE_OFFSET||
            ev.y >= attr.height - SELECTABLE_OFFSET
        ){
            last_pressed_event = ev;
            focused_client = client_index;
        }
        else{
            last_pressed_event.window = None;
        }
    }
    XAllowEvents(dpy, ReplayPointer, ev.time);
    // XSync(dpy, 1);
}

void on_button_release(XButtonReleasedEvent ev){
    last_pressed_event.window = None;

    if (focused_client < clients_amount){
        update_client(focused_client);
    }
}

void on_motion_notify(XButtonEvent ev){
    printf("    is pressed? : %d\n", last_pressed_event.window != None);
    printf("    is not constrained? : ");
    if (
        last_pressed_event.window != None
    ){
        printf("yes\n");
        int constrained_x = MIN(MAX(ev.x_root, FRAME_OFFSET), root_width-FRAME_OFFSET);
        int constrained_y = MIN(MAX(ev.y_root, FRAME_OFFSET), root_height-FRAME_OFFSET);
        
        int xdiff = constrained_x - last_pressed_event.x_root;
        int ydiff = constrained_y - last_pressed_event.y_root;
        printf("    x,y root from ev : %d, %d\n", ev.x_root, ev.y_root);
        printf("    x,y root from client : %d, %d\n", last_pressed_event.x_root, last_pressed_event.y_root);
        printf("    x,y diff: %d, %d\n", xdiff, ydiff);

        XMoveWindow(dpy, ev.window,
            clients[focused_client].x + (last_pressed_event.button==1 ? xdiff : 0),
            clients[focused_client].y + (last_pressed_event.button==1 ? ydiff : 0));
    }
    else{
        printf("no\n");
    }
}

void on_reparenting(Window w){
    int client_index = getClientIndex(w);
    if (client_index > -1){
        clients[client_index].reparenting = 0;
    }
}

void map_win(Window w){
    printf("    map client %d \n", (int) w);
    XWindowAttributes wa;
    if (XGetWindowAttributes(dpy, w, &wa) && ! wa.override_redirect && getClientIndex(w) == -1 && wa.map_state == IsViewable){
        manage(w, &wa);
    }
}

void unmap_win(Window w){
    int client_index;
    printf("    destroy client %d \n", (int) w);
    if ((client_index = getClientIndex(w)) > -1 && clients[client_index].reparenting == 0){
        unmanage(client_index);
    }
}

void configure_win(Window w){
    // int client_index;
    // if ((client_index = getClientIndex(w)) != -1){
    //     update_client(client_index);
    // }
}

void scan()
{   
    printf("---SCAN FOR WINDOWS\n");
    XGrabServer(dpy);
    unsigned int i, num_win;
    Window wroot, wparent;
    Window *wchildren;
    XWindowAttributes wa;

    if (XQueryTree(dpy, root, &wroot, &wparent, &wchildren, &num_win)){
        for(i=0;i<num_win;i++){
            map_win(wchildren[i]);
        }
        if (wchildren){
            XFree(wchildren);
        }
    }
    XUngrabServer(dpy);
    printf("---SCAN COMPLETE\n\n");
}

void start(){
    screen = DefaultScreen(dpy);
    root = XDefaultRootWindow(dpy);
    root_width = DisplayWidth(dpy, screen);
    root_height = DisplayHeight(dpy, screen);
    gc = XCreateGC(dpy, root, 0, NULL);

    XFontStruct* font = XLoadQueryFont(dpy, "fixed");
    XSetFont(dpy, gc,font->fid);
    // XSync(dpy, 0);
}

void run(){
    XWindowAttributes attr;
    XButtonEvent start;
    XEvent ev;
    start.window = None;
    XSelectInput(dpy, root, SubstructureNotifyMask);

    scan();
    for(;;)
    {
        XNextEvent(dpy, &ev);

        switch (ev.type)
        {
            case CreateNotify:
                puts("\n---CREATE NOTIFY");
                // map_win(ev.xcreatewindow.window);
                break;
            case ReparentNotify:
                puts("\n---REPARENTING");
                on_reparenting(ev.xreparent.window);
                break;
            case ConfigureNotify:
                puts("\n---CONFIGURE WINDOW");
                configure_win(ev.xconfigure.window);
                break;
            case MapNotify:
                puts("\n---MAP NOTIFY");
                map_win(ev.xmap.window);
                puts(" ");
                break;
            case UnmapNotify:
                puts("\n---UNMAP NOTIFY");
                unmap_win(ev.xunmap.window);
                puts(" ");
                break;
            case ButtonPress:
                printf("---BUTTON PRESS\n");
                on_button_press(ev.xbutton);
                break;
            case MotionNotify:
                printf("---BUTTON MOTION\n");
                on_motion_notify(ev.xbutton);
                break;
            case ButtonRelease:
                printf("---BUTTON RELEASE\n");
                on_button_release(ev.xbutton);
                break;
            default:
                printf("-----UNKNOWN EVENT----[%d]\n", ev.type);
                break;
        }

        // scan();
        XSync(dpy, 0);
    }
}

int main()
{   
    clients = (Client *) malloc(INITIAL_CLIENTS_SIZE * sizeof(*clients)) ;

    if (clients == NULL){
        printf("memory alloc failed");
        return 1;
    }

    if (! (dpy=XOpenDisplay(0x0)) ) return 1;

    start();
    run();
    XCloseDisplay(dpy);
    return 1;
    
}

// returns -1 if not found
// column index is for shich window is the client (if is 'base' window then 1, if frame window then 2)
int getClientIndex(Window client){
    int c_xid = (int) client;
    printf("    search for %d\n", c_xid);
    for (unsigned int i=0;i<clients_amount;i++){
        printf("    --[%d] w: %d f: %d", i, clients[i].xid, clients[i].frame_xid);
        if (clients[i].xid == c_xid || clients[i].frame_xid == c_xid){
            printf(" == %d [found]\n", c_xid);
            return i;
        }
        printf(" != %d\n", c_xid);
    }
    return -1;
}

int getClientIndexWithColumnIndex(Window client, int *column_index){
    int client_index;
    if ((client_index = getClientIndex(client)) != -1){
        int c_xid = (int) client;
        if (clients[client_index].xid == c_xid){
            printf("found on base window \n");
            *column_index = BASEWINDOW;
        }
        else if (clients[client_index].frame_xid == c_xid){
            printf("found on frame window \n");
            *column_index = FRAMEWINDOW;
        }
        return 1;
    }
    return -1;
}
