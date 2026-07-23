#include "icccm.h"
#include <stdio.h>

ICCCMAtoms icccm;

void InitICCCM(Display *disp) {
    icccm.wm_protocols     = XInternAtom(disp, "WM_PROTOCOLS", False);
    icccm.wm_delete_window = XInternAtom(disp, "WM_DELETE_WINDOW", False);
    icccm.wm_take_focus    = XInternAtom(disp, "WM_TAKE_FOCUS", False);
    icccm.wm_state         = XInternAtom(disp, "WM_STATE", False);
    icccm.wm_change_state  = XInternAtom(disp, "WM_CHANGE_STATE", False);
}

void ApplyNormalHints(Display *disp, Window w, int *x, int *y, unsigned int *w_out, unsigned int *h_out) {
    XSizeHints hints;
    long supplied;

    if (XGetWMNormalHints(disp, w, &hints, &supplied)) {
        if (hints.flags & PMinSize) {
            if (*w_out < (unsigned int)hints.min_width)  *w_out = hints.min_width;
            if (*h_out < (unsigned int)hints.min_height) *h_out = hints.min_height;
        }
        if (hints.flags & PMaxSize) {
            if (hints.max_width > 0 && *w_out > (unsigned int)hints.max_width)   *w_out = hints.max_width;
            if (hints.max_height > 0 && *h_out > (unsigned int)hints.max_height) *h_out = hints.max_height;
        }
        if (hints.flags & PResizeInc) {
            if (hints.width_inc > 0)  *w_out -= (*w_out - (hints.flags & PBaseSize ? hints.base_width : 0)) % hints.width_inc;
            if (hints.height_inc > 0) *h_out -= (*h_out - (hints.flags & PBaseSize ? hints.base_height : 0)) % hints.height_inc;
        }
    }
}
