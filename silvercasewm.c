#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

const unsigned int FRAME_OFFSET = 20;
const unsigned int BORDER_WIDTH = 10;
const unsigned int BORDER_COLOR = 0xc8ffff;
const unsigned int BG_COLOR = 0x000000;
const unsigned int BUTTON_WIDTH = 4;
const unsigned int BUTTON_COLOR = 0xffffff;
const unsigned int INITIAL_CLIENTS_SIZE = 5;

typedef struct {
    int xid;
    int frame_xid;
} Client;

static Display *dpy;
static Window root;
static Client *clients;
static int clients_amount = 0;

int getClientIndex(Window client);

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
        wa->width,
        wa->height,
        BORDER_WIDTH,
        BORDER_COLOR,
        BG_COLOR
    );

    XAddToSaveSet(dpy, w);

    XReparentWindow(dpy, w, frame, 0,0);
    XMapWindow(dpy, frame);
    
    XGrabButton(dpy, 1, AnyModifier, frame, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XSelectInput(dpy, w, SubstructureNotifyMask);

    int add_to_index = clients_amount;
    for (int i=0;i<clients_amount;i++){
        if (clients[i].xid == 0){
            add_to_index = i;
            break;
        }
    }
    
    clients[add_to_index] = (Client) {(int) w, (int) frame};
    clients_amount += add_to_index == clients_amount ? 1 : 0;
    
    printf("\n-----new client id %d \n", (int) w);
    for (int i=0;i<clients_amount;i++){
        printf("  [%d] = %d %d \n", i, clients[i].xid, clients[i].frame_xid);
    }
}

void unmanage(int client_index){
    XDestroyWindow(dpy, (Window) clients[client_index].frame_xid);
    
    clients[client_index].xid = 0;
    clients[client_index].frame_xid = 0;
}

void scan()
{   
    XGrabServer(dpy);
    unsigned int i, num_win;
    Window wroot, wparent;
    Window *wchildren;
    XWindowAttributes wa;

    if (XQueryTree(dpy, root, &wroot, &wparent, &wchildren, &num_win)){
        for(i=0;i<num_win;i++){
            Window client_frame;
            if (! XGetWindowAttributes(dpy, wchildren[i], &wa) || wa.override_redirect || getClientIndex(wchildren[i]) > -1){
                // puts("scan failed");
                continue;

            }
            if (wa.map_state == IsViewable)
                // puts("scan success");
                manage(wchildren[i], &wa);
        }
        if (wchildren){
            XFree(wchildren);
        }
    }
    // puts("scanning end");
    XUngrabServer(dpy);
}

int main()
{   
    clients = (Client *) malloc(INITIAL_CLIENTS_SIZE * sizeof(*clients)) ;

    if (clients == NULL){
        printf("memory alloc failed");
        return 1;
    }

    // printf("\nstart");
    // printf("%d", CreateNotify);
    // printf("\nstart");
    // printf("%d", MapNotify);
    // printf("\nstart");
    // printf("%d", CirculateNotify);
    XWindowAttributes attr;
    XButtonEvent start;
    XEvent ev;

    if (! (dpy=XOpenDisplay(0x0)) ) return 1;

    root = XDefaultRootWindow(dpy);
    XSelectInput(dpy, root, SubstructureNotifyMask);

    start.window = None;
    scan();

    for(;;)
    {
        XNextEvent(dpy, &ev);

        if (ev.type != ButtonPress && ev.type != MotionNotify && ev.type != ButtonRelease){
            printf("-----EVENT: %d-----\n\n", ev.type);
        }
        switch (ev.type)
        {
            case MapNotify:
                puts("map notify");
                printf("  map client %d \n", (int) ev.xcreatewindow.window);
                XWindowAttributes wa;
                XGetWindowAttributes(dpy, ev.xcreatewindow.window, &wa);
                if (! wa.override_redirect && getClientIndex(ev.xcreatewindow.window) == -1){
                    manage(ev.xcreatewindow.window, &wa);
                }
                break;
            case DestroyNotify:
                int client_index;
                puts("destroy notify");
                printf(" destroy client %d \n", (int) ev.xdestroywindow.event);
                if ((client_index = getClientIndex(ev.xdestroywindow.event)) > -1){
                    unmanage(client_index);
                }
            default:
                
                break;
        }

        if(ev.type == KeyPress && ev.xkey.window != None)
            XRaiseWindow(dpy, ev.xkey.window);
        else if(ev.type == ButtonPress && ev.xbutton.window != None)
        {
            XGetWindowAttributes(dpy, ev.xbutton.window, &attr);
            start = ev.xbutton;
        }
        else if(ev.type == MotionNotify && start.window != None)
        {
            int xdiff = ev.xbutton.x_root - start.x_root;
            int ydiff = ev.xbutton.y_root - start.y_root;
            // printf("\n moving : diff: (");
            // printf("%d", xdiff);
            // printf(", ");
            // printf("%d", ydiff);
            // printf(") | framexy : (");
            // printf("%d", attr.x);
            // printf(", ");
            // printf("%d", attr.y);
            XMoveResizeWindow(dpy, start.window,
                attr.x + (start.button==1 ? xdiff : 0),
                attr.y + (start.button==1 ? ydiff : 0),
                MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
                MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
        }
        else if(ev.type == ButtonRelease)
            start.window = None;
        
        XSync(dpy, 1);


    }
}

// returns -1 if not found
int getClientIndex(Window client){
    for (unsigned int i=0;i<clients_amount;i++){
        int c_xid = (int) client;
        printf("\n [%d] client xid/frame : %d %d compare to %d \n", i, clients[i].xid, clients[i].frame_xid, c_xid);
        printf("found match : %d \n", clients[i].frame_xid == c_xid);
        if (clients[i].xid == c_xid || clients[i].frame_xid == c_xid)
            return i;
    }
    return -1;
}