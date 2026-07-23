#include "ewmh.h"
#include <X11/Xatom.h>
#include <string.h>

EWMHAtoms ewmh;

void InitEWMH(Display *disp, Window root) {
    ewmh.net_supported            = XInternAtom(disp, "_NET_SUPPORTED", False);
    ewmh.net_supporting_wm_check = XInternAtom(disp, "_NET_SUPPORTING_WM_CHECK", False);
    ewmh.net_wm_name              = XInternAtom(disp, "_NET_WM_NAME", False);
    ewmh.net_client_list          = XInternAtom(disp, "_NET_CLIENT_LIST", False);
    ewmh.net_client_list_stacking = XInternAtom(disp, "_NET_CLIENT_LIST_STACKING", False);
    ewmh.net_active_window        = XInternAtom(disp, "_NET_ACTIVE_WINDOW", False);
    ewmh.net_current_desktop      = XInternAtom(disp, "_NET_CURRENT_DESKTOP", False);
    ewmh.net_number_of_desktops   = XInternAtom(disp, "_NET_NUMBER_OF_DESKTOPS", False);
    ewmh.net_desktop_names        = XInternAtom(disp, "_NET_DESKTOP_NAMES", False);
    ewmh.net_wm_desktop           = XInternAtom(disp, "_NET_WM_DESKTOP", False);
    ewmh.net_wm_state             = XInternAtom(disp, "_NET_WM_STATE", False);
    ewmh.net_wm_state_fullscreen  = XInternAtom(disp, "_NET_WM_STATE_FULLSCREEN", False);
    ewmh.net_wm_window_type       = XInternAtom(disp, "_NET_WM_WINDOW_TYPE", False);
    ewmh.net_wm_window_type_dock  = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_DOCK", False);
    ewmh.net_wm_window_type_dialog= XInternAtom(disp, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    ewmh.net_workarea             = XInternAtom(disp, "_NET_WORKAREA", False);

    // Creiamo la finestra fittizia per _NET_SUPPORTING_WM_CHECK (Priorità #10)
    Window check_win = XCreateSimpleWindow(disp, root, 0, 0, 1, 1, 0, 0, 0);
    
    // Assegniamo il nome al WM (_NET_WM_NAME - Priorità #11)
    XChangeProperty(disp, check_win, ewmh.net_wm_name, XInternAtom(disp, "UTF8_STRING", False), 8,
                    PropModeReplace, (unsigned char *)"ashwm", 5);
    XChangeProperty(disp, check_win, ewmh.net_supporting_wm_check, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&check_win, 1);
    
    // Impostiamo la stessa proprietà sulla Root Window
    XChangeProperty(disp, root, ewmh.net_supporting_wm_check, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&check_win, 1);
    XChangeProperty(disp, root, ewmh.net_wm_name, XInternAtom(disp, "UTF8_STRING", False), 8,
                    PropModeReplace, (unsigned char *)"ashwm", 5);

    UpdateEWMHSupported(disp, root);
}

// Registra la lista degli atom supportati (_NET_SUPPORTED - Priorità #2)
void UpdateEWMHSupported(Display *disp, Window root) {
    Atom net_atoms[] = {
        ewmh.net_supported,
        ewmh.net_supporting_wm_check,
        ewmh.net_wm_name,
        ewmh.net_client_list,
        ewmh.net_client_list_stacking,
        ewmh.net_active_window,
        ewmh.net_current_desktop,
        ewmh.net_number_of_desktops,
        ewmh.net_desktop_names,
        ewmh.net_wm_desktop,
        ewmh.net_wm_state,
        ewmh.net_wm_state_fullscreen,
        ewmh.net_wm_window_type,
        ewmh.net_wm_window_type_dock,
        ewmh.net_wm_window_type_dialog,
        ewmh.net_workarea
    };

    XChangeProperty(disp, root, ewmh.net_supported, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)net_atoms, sizeof(net_atoms) / sizeof(Atom));
}

void SetWindowState(Display *disp, Window w, Atom state, int action) {
    // action: 1 = add, 0 = remove
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = NULL;
    Atom *atoms = NULL;
    int found = 0;

    if (XGetWindowProperty(disp, w, ewmh.net_wm_state, 0, 1024, False, XA_ATOM,
                           &actual_type, &actual_format, &nitems, &bytes_after, &prop) == Success && prop) {
        atoms = (Atom *)prop;
        for (unsigned long i = 0; i < nitems; i++) {
            if (atoms[i] == state) {
                found = 1;
                break;
            }
        }
    }

    if (action && !found) {
        // Aggiunge lo stato se non c'era
        XChangeProperty(disp, w, ewmh.net_wm_state, XA_ATOM, 32,
                        PropModeAppend, (unsigned char *)&state, 1);
    } else if (!action && found) {
        // Rimuove lo stato se c'era
        Atom new_atoms[1024];
        int count = 0;
        for (unsigned long i = 0; i < nitems; i++) {
            if (atoms[i] != state) {
                new_atoms[count++] = atoms[i];
            }
        }
        XChangeProperty(disp, w, ewmh.net_wm_state, XA_ATOM, 32,
                        PropModeReplace, (unsigned char *)new_atoms, count);
    }

    if (prop) XFree(prop);
}
