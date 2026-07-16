#ifndef FUNC_H
#define FUNC_H

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/Xinerama.h>

#include "config.h"

#define VERSION 1.1

//for debug purpuse
#ifdef DEBUG
	#define DEBUG_LOG(...) printf(__VA_ARGS__)
#else
    #define DEBUG_LOG(...) do {} while (0)
#endif


#define WORKSPACES_X_MONITOR 10
#define WORKSPACES (WORKSPACES_X_MONITOR * N_MONITORS)


//for bar positioning
typedef struct {
    unsigned long left, right, top, bottom;
} Strut;


// strcut to define monitors,
typedef struct {
	int id;
	int x,y;
	int width,height;
	int current_ws;
} Monitors ;



//strcut for client
typedef struct Client {
	Window id;
	int monitor_id;

    int is_floating;   // 1 mouse, 0 tiling
    int is_fullscreen; // 1 if full screen


    int x, y; 
	unsigned int w, h; 
	

    int old_x, old_y; 
    unsigned int old_w, old_h;

	struct Client *next;
	struct Client *prev;

}Client;



//struct for workspace
typedef struct{
	int id;
	int monitor_id;
	Client *list_Cl;
}Workspace ;

Monitors monitors[N_MONITORS];
int monitors_count = {0};
Workspace workspaces[WORKSPACES];
Atom wm_delete_window;




//IPC
void UpdateBarIPC(Display *disp, Window root);

Client* FindClientByWindow(Window w, int *out_ws);

void FocusWindow(Display *disp, Window w);
void AddWindowList(Display *disp, Window w, Window root);
void ChangeWorkspace(Display *disp, Window root, int new_ws);
void MoveToWorkspace(Display *disp, Window root, int ws_target);
void RemoveWindowList(Display *disp, Window w, Window root);
void KillWindow(Display  *disp, Window root);
void Dwindle(Display *disp, int ws_index);
void UpdateCurrentMonitor(Display *disp, Window root);
void MoveWindowToMonitor(Display *disp, Window root, Window w);
unsigned long GetXColor(Display *disp, unsigned long hex_color);
void CycleFocus(Display *disp, int direction);
void ToggleFullscreen(Display *disp, Window root);
void SwapDwindleDirectional(Display *disp, int direction);
void ResizeFocusedWindow(Display *disp, Window root, int direction);
void RaiseFloatingWindows(Display *disp, int ws_index);
int XErrorHandlerImpl(Display *disp, XErrorEvent *ee);
void sigchld(int unused);
void CleanupWM(Display *disp, Window root);


#endif
