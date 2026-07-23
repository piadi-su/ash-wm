#ifndef EWMH_H
#define EWMH_H

#include <X11/Xlib.h>

typedef struct {
    Atom net_supported;
    Atom net_supporting_wm_check;
    Atom net_wm_name;
    Atom net_client_list;
    Atom net_client_list_stacking;
    Atom net_active_window;
    Atom net_current_desktop;
    Atom net_number_of_desktops;
    Atom net_desktop_names;
    Atom net_wm_desktop;
    Atom net_wm_state;
    Atom net_wm_state_fullscreen;
    Atom net_wm_window_type;
    Atom net_wm_window_type_dock;
    Atom net_wm_window_type_dialog;
    Atom net_workarea;
} EWMHAtoms;

extern EWMHAtoms ewmh;

void InitEWMH(Display *disp, Window root);
void UpdateEWMHSupported(Display *disp, Window root);
void SetWindowState(Display *disp, Window w, Atom state, int action);
void UpdateClientList(Display *disp, Window root);

#endif
