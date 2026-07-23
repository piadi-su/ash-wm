#ifndef ICCCM_H
#define ICCCM_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

// Struttura per memorizzare gli Atom ICCCM globali
typedef struct {
    Atom wm_protocols;
    Atom wm_delete_window;
    Atom wm_take_focus;
    Atom wm_state;
    Atom wm_change_state;
} ICCCMAtoms;

extern ICCCMAtoms icccm;

void InitICCCM(Display *disp);
void ApplyNormalHints(Display *disp, Window w, int *x, int *y, unsigned int *w_out, unsigned int *h_out);

#endif
