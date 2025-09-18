#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>

// void create_window(XCreateWindowEvent ev){

// }

#define ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))
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
static int clients_id[5];

int getClientFrame(Window client_key, Window *client_frame);
int hasClient(Window client);


int count_arr(int arr[], int sizeof_arr){
    unsigned int length = 0;
    for (unsigned int i=0;i<sizeof_arr;i++){
        if (arr[i] != 0)
            length += 1;
    }
    return length;
}

void manage(Window w, XWindowAttributes *wa)
{
    puts("Managing");
    if (hasClient(w) == 0){
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

        int real_client_size = count_arr(clients_id, ARRAY_SIZE(clients_id));
        clients_id[real_client_size] = (int) w;
        // clients_id[real_client_size+1] = (int) frame;
        printf("\nnew client id %d \n", (int) w);
        printf("last length of clients %d \n", real_client_size);
        for (int i=0;i<ARRAY_SIZE(clients_id);i++){
            printf("  [%d] = ", i);
            printf("%d \n", clients_id[i]);
        }
        printf("\n");
        
        // int client_size = sizeof(clients_key) / sizeof(clients_key[0]);
        // clients_key[client_size] = w;
        // clients_frame[client_size] = frame;
    }
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
            if (! XGetWindowAttributes(dpy, wchildren[i], &wa) || wa.override_redirect || getClientFrame(wchildren[i], &client_frame) > -1){
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
            case CreateNotify:
                puts("Create notify");
                printf("has client %d \n", hasClient(ev.xcreatewindow.window));
                // XWindowAttributes wa;
                // XGetWindowAttributes(dpy, ev.xcreatewindow.window, &wa);
                // manage(ev.xcreatewindow.window, &wa);
                break;
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


int hasClient(Window client){
    puts("compare from clients to client");
    for (unsigned int i=0;i<ARRAY_SIZE(clients_id);i++){
        printf("  [%d] ", i);
        printf("%d to ", clients_id[i]);
        printf("%d \n", (int) client);
        if (clients_id[i] == (int) client){
            printf("found!");
            return 1;
        }else{
            continue;
        }
    }
    return 0;
}

// returns index if found, -1 if not
int getClientFrame(Window client_key, Window *client_frame)
{
    // printf("search client frame : \n");
    for (unsigned int i=0;i<sizeof(clients_key);i++){
        // printf("%u", i);
        if (clients_key[i] == client_key){
            // printf("%p", &client_key);
            // printf("\n found! \n");
            *client_frame = clients_frame[i];
            return i;
        }else{
            // printf("+");
            continue;
        }
    }
    return -1;
}