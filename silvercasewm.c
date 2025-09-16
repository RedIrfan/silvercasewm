#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>

// void create_window(XCreateWindowEvent ev){

// }

#define MAX(a, b) ((a) > (b) ? (a) : (b))

const unsigned int FRAME_OFFSET = 20;
const unsigned int BORDER_WIDTH = 10;
const unsigned int BORDER_COLOR = 0xc8ffff;
const unsigned int BG_COLOR = 0x000000;
const unsigned int BUTTON_WIDTH = 4;
const unsigned int BUTTON_COLOR = 0xffffff;

static Display *dpy;
static Window root;
static Window clients_key[2] = {};
static Window clients_frame[2] = {};

int getClientFrame(Window client_key, Window *client_frame);



void manage(Window w, XWindowAttributes *wa)
{
    puts("manage");
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

    XGrabButton(dpy, 1, Mod1Mask, w, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

    int client_size = sizeof(clients_key) / sizeof(clients_key[0]);

    clients_key[client_size] = w;
    clients_frame[client_size] = frame;
    // XLowerWindow(dpy, del_btn);
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
            if (! (XGetWindowAttributes(dpy, wchildren[i], &wa) || wa.override_redirect) ){
                puts("scan failed");
                continue;

            }
            if (wa.map_state == IsViewable)
                puts("scan success");
                manage(wchildren[i], &wa);
        }
        if (wchildren){
            XFree(wchildren);
        }
    }
    puts("scanning end");
    XUngrabServer(dpy);
}

int main()
{
    XWindowAttributes attr;
    XButtonEvent start;
    XEvent ev;

    if (! (dpy=XOpenDisplay(0x0)) ) return 1;

    root = XDefaultRootWindow(dpy);

    start.window = None;

    scan();
    for(;;)
    {
        // scan();

        XNextEvent(dpy, &ev);

        if (ev.type == MapRequest){
            puts("map request");
            XWindowAttributes wa;
            XGetWindowAttributes(dpy, ev.xmaprequest.window, &wa);
            manage(ev.xmaprequest.window, &wa);
        }

        if(ev.type == KeyPress && ev.xkey.window != None)
            XRaiseWindow(dpy, ev.xkey.window);
        else if(ev.type == ButtonPress && ev.xbutton.window != None)
        {
            XGetWindowAttributes(dpy, ev.xbutton.window, &attr);
            start = ev.xbutton;
            puts("pressing on frame");
        }
        else if(ev.type == MotionNotify && start.window != None)
        {
            puts("motion");
            Window client_frame;

            if (getClientFrame(start.window, &client_frame)){
                int xdiff = ev.xbutton.x_root - start.x_root;
                int ydiff = ev.xbutton.y_root - start.y_root;
                XMoveResizeWindow(dpy, client_frame,
                    attr.x + (start.button==1 ? xdiff : 0),
                    attr.y + (start.button==1 ? ydiff : 0),
                    MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
                    MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
            }
        }
        else if(ev.type == ButtonRelease)
            start.window = None;
        
        XSync(dpy, 1);


    }
}


int getClientFrame(Window client_key, Window *client_frame)
{
    puts("get client frame");
    for (unsigned int i=0;i<sizeof(clients_key);i++){
        if (clients_key[i] == client_key){
            puts("Found");
            *client_frame = clients_frame[i];
            return 1;
        }else{
            puts("continuing");
            continue;
        }
    }
    return False;
}
