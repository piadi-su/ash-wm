/*
 * ashwm is a simple x11 tyling window manger
 * inspired by dwm 
 *
 * code start write date 08/07/2026
 *
 */


#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

#include <X11/extensions/Xinerama.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xatom.h>

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>


//my files
#include "config.h"
#include  "func.h"




//-----------------------------//

//workspace ipc
void
UpdateBarIPC(Display *disp, Window root)
{
    char status[128] = "";
    
	// find acrive monitor
    Window dummy_win; int dummy_int; unsigned int dummy_mask;
    int mouse_x, mouse_y; int mon_idx = 0;
    XQueryPointer(disp, root, &dummy_win, &dummy_win, &mouse_x, &mouse_y, &dummy_int, &dummy_int, &dummy_mask);
    for (int i = 0; i < monitors_count; i++) {
        if (mouse_x >= monitors[i].x && mouse_x < (monitors[i].x + monitors[i].width) &&
            mouse_y >= monitors[i].y && mouse_y < (monitors[i].y + monitors[i].height)) {
            mon_idx = i; break;
        }
    }
    int active_ws = monitors[mon_idx].current_ws;

    for (int i = 0; i < WORKSPACES_X_MONITOR; i++) {
        int real_ws = (mon_idx * WORKSPACES_X_MONITOR) + i;
        char ws_status = 'E'; // Empty         
        if (real_ws == active_ws) {
            ws_status = 'A'; // Active 
        } else if (workspaces[real_ws].list_Cl != NULL) {
            ws_status = 'O'; // Occupied 
        }
        
        char buf[16];
        snprintf(buf, sizeof(buf), "%d:%c ", i + 1, ws_status);
        strcat(status, buf);
    }

	// creating the atom
    Atom prop = XInternAtom(disp, "_ASHWM_WORKSPACES", False);
    
	//wirite string to the root window
    XChangeProperty(disp, root, prop, XA_STRING, 8, PropModeReplace, 
                    (unsigned char *)status, strlen(status));
    XFlush(disp);
}

//EWMH
Strut GetWindowStrut(Display *disp, Window w) {
    Strut s = {0, 0, 0, 0};
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = NULL;

	//try with new standard
    Atom prop = XInternAtom(disp, "_NET_WM_STRUT_PARTIAL", True);
    if (prop == None) {
        prop = XInternAtom(disp, "_NET_WM_STRUT", True);
    }

    if (prop != None) {
		//ask x for the bar props
        if (XGetWindowProperty(disp, w, prop, 0, 4, False, XA_CARDINAL,
                               &actual_type, &actual_format, &nitems, &bytes_after,
                               &data) == Success && data != NULL) {
            
            if (nitems >= 4) {
                unsigned long *values = (unsigned long *)data;
                s.left   = values[0];
                s.right  = values[1];
                s.top    = values[2];
                s.bottom = values[3];
            }
            XFree(data);
        }
    }
    return s;
}

int 
IsDock(Display *disp, Window w) {
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = NULL;
    int is_dock = 0;

    Atom net_wm_window_type = XInternAtom(disp, "_NET_WM_WINDOW_TYPE", False);
    Atom net_wm_window_type_dock = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_DOCK", False);

    if (XGetWindowProperty(disp, w, net_wm_window_type, 0, 32, False, XA_ATOM,
                           &actual_type, &actual_format, &nitems, &bytes_after, &prop) == Success) {
        if (prop) {
            Atom *atoms = (Atom *)prop;
            for (unsigned long i = 0; i < nitems; i++) {
                if (atoms[i] == net_wm_window_type_dock) {
                    is_dock = 1;
                    break;
                }
            }
            XFree(prop);
        }
    }
    return is_dock;
}

//-----------------------------//
Client* FindClientByWindow(Window w, int *out_ws) {
    for (int ws = 0; ws < WORKSPACES; ws++) {
        Client *head = workspaces[ws].list_Cl;
        if (head == NULL) continue;

        Client *curr = head;
        do {
            if (curr->id == w) {
                if (out_ws) *out_ws = ws;
                return curr;
            }
            curr = curr->next;
        } while (curr != head);
    }
    if (out_ws) *out_ws = -1;
    return NULL;
}

//-----------------------------//



//func to change worksapce focus
void 
FocusWindow(Display *disp, Window w) {
    if (w == None) return;

    // Protezione: Se è una barra, dale pure il focus di X11 (se serve) ma non toccare il tiling o i bordi
    if (IsDock(disp, w)) {
        XSetInputFocus(disp, w, RevertToParent, CurrentTime);
        return;
    }

    XWindowAttributes attrs;
    if (XGetWindowAttributes(disp, w, &attrs)) {
        if (attrs.map_state != IsViewable) {
            return; 
        }
    }

    // Assign focus
    XSetInputFocus(disp, w, RevertToParent, CurrentTime);
    XRaiseWindow(disp, w); 

    // Find window workspace
    int target_ws = -1;
    Client *found = FindClientByWindow(w, &target_ws);

    if (target_ws == -1 || found == NULL) return;

    RaiseFloatingWindows(disp, target_ws);

    // Update borders
    Client *curr = workspaces[target_ws].list_Cl;
    if (curr == NULL) return;

    do {
        XSetWindowBorderWidth(disp, curr->id, BORDER_WIDTH);
        if (curr->id == w) {
            XSetWindowBorder(disp, curr->id, GetXColor(disp, COLOR_FOCUS));
        } else {
            XSetWindowBorder(disp, curr->id, GetXColor(disp, COLOR_UNFOCUS));
        }
        curr = curr->next;
    } while (curr != workspaces[target_ws].list_Cl);

    XFlush(disp);
}



/*
 * add a new window to the list 
 * from the Ev Ev.xmaprequest.window
 * */
void 
AddWindowList(Display *disp, Window w, Window root)
{

	XWindowAttributes wa;
    XGetWindowAttributes(disp, w, &wa);

	// see if window is made for bypass the wm
	if (wa.override_redirect || IsDock(disp, w)) {
		XSelectInput(disp, w, StructureNotifyMask);
		XMapWindow(disp, w); // Mappa la barra ma NON inserirla nella lista di tiling!
		return;
	}

	Client *new_window = calloc(1, sizeof(Client));
	if (new_window == NULL)
		return;
	
	//add window id to struct
	new_window->id = w;
	new_window->monitor_id = 0;
	
	
	Window dummy_win;
    int dummy_int;
    unsigned int dummy_mask;
    int mouse_x, mouse_y;
	int target_monitor = 0; 

	XQueryPointer(disp, root, &dummy_win, &dummy_win, &mouse_x, &mouse_y, &dummy_int, &dummy_int, &dummy_mask);


	for (int i = 0; i < monitors_count; i++) {
        if (mouse_x >= monitors[i].x && mouse_x < (monitors[i].x + monitors[i].width) &&
            mouse_y >= monitors[i].y && mouse_y < (monitors[i].y + monitors[i].height)) {
            target_monitor = i;
            break;
        }
    }
	
	int active_ws = monitors[target_monitor].current_ws;


	DEBUG_LOG("[ASH-WM] window %lu spowned on Monitor %d -> saved in Workspace %d\n", 
           w, target_monitor, active_ws);

	
	//-------------------------------------------//

	/*
	 *  / c \
	 *  b    d
	 *  \ a /
     *
	 * */
		
	//-------------------------------------------//
	
	XSelectInput(disp, w, EnterWindowMask | PropertyChangeMask);

	// window as prev and next 
	if (workspaces[active_ws].list_Cl == NULL) {
		workspaces[active_ws].list_Cl = new_window;
		workspaces[active_ws].list_Cl->next = workspaces[active_ws].list_Cl;
		workspaces[active_ws].list_Cl->prev = workspaces[active_ws].list_Cl;

	}

	// circle ds
	else {
		Client *head = workspaces[active_ws].list_Cl;
		new_window->next = head;
		new_window->prev = head->prev;
		head->prev->next = new_window;
		head->prev = new_window;
	}


	Dwindle(disp, active_ws);
	UpdateBarIPC(disp, root);
}



void
ChangeWorkspace(Display *disp, Window root, int target_local_id) 
{
    Window dummy_win;
    int dummy_int;
    unsigned int dummy_mask;
    int mouse_x, mouse_y;
    int mon_idx = 0;

    XQueryPointer(disp, root, &dummy_win, &dummy_win, &mouse_x, &mouse_y, &dummy_int, &dummy_int, &dummy_mask);

    for (int i = 0; i < monitors_count; i++) {
        if (mouse_x >= monitors[i].x && mouse_x < (monitors[i].x + monitors[i].width) &&
            mouse_y >= monitors[i].y && mouse_y < (monitors[i].y + monitors[i].height)) {
            mon_idx = i;
            break;
        }
    }

    int old_ws = monitors[mon_idx].current_ws;
    
	// calcolate worksapce es 0-9, 10-19
    int new_ws = (mon_idx * WORKSPACES_X_MONITOR) + (target_local_id % WORKSPACES_X_MONITOR);

    if(old_ws == new_ws)
        return;

    DEBUG_LOG("[ASH-WM] Monitor %d mv form WS %d to WS %d\n", mon_idx, old_ws, new_ws);

	//hide old worksapce window
    workspaces[old_ws].monitor_id = -1; 
	Dwindle(disp, old_ws);
	//activate new worksapce
    monitors[mon_idx].current_ws = new_ws;
    workspaces[new_ws].monitor_id = mon_idx;

    // show window in the new workspace
	Dwindle(disp, new_ws);

	if(workspaces[new_ws].list_Cl != NULL) {
		FocusWindow(disp, workspaces[new_ws].list_Cl->id);
	} else {
		XSetInputFocus(disp, root, RevertToParent, CurrentTime);
	}

    XSync(disp, False);
	UpdateBarIPC(disp, root);
}



//move to worksapce
void 
MoveToWorkspace(Display *disp, Window root, int target_local_id) {
    Window focused_win;
    int revert_to;

    XGetInputFocus(disp, &focused_win, &revert_to);
    if(focused_win == None || focused_win == root) return;
    
    int source_ws = -1;
    Client *cursor = NULL;
    Client *found = NULL;

    // 1. Trova il workspace di origine
    for(int i = 0; i < WORKSPACES; i++) {
        cursor = workspaces[i].list_Cl;
        if(cursor != NULL) {
            do {
                if(cursor->id == focused_win) {
                    source_ws = i;
                    found = cursor;
                    break;
                }
                cursor = cursor->next;
            } while(cursor != workspaces[i].list_Cl);
        }
        if(source_ws != -1) break;
    }

    if(source_ws == -1) return;


	
    Window dummy_win;
    int dummy_int;
    unsigned int dummy_mask;
    int mouse_x, mouse_y;
    int target_monitor = 0;

    XQueryPointer(disp, root, &dummy_win, &dummy_win, &mouse_x, &mouse_y, &dummy_int, &dummy_int, &dummy_mask);

    for (int i = 0; i < monitors_count; i++) {
        if (mouse_x >= monitors[i].x && mouse_x < (monitors[i].x + monitors[i].width) &&
            mouse_y >= monitors[i].y && mouse_y < (monitors[i].y + monitors[i].height)) {
            target_monitor = i;
            break;
        }
    }

    int ws_target = (target_monitor * WORKSPACES_X_MONITOR) + (target_local_id % WORKSPACES_X_MONITOR);
    if(source_ws == ws_target) return;

    DEBUG_LOG("[ASH-WM] mv window %lu from WS %d to WS %d (Monitor %d)\n", focused_win, source_ws, ws_target, target_monitor);

    if(found->next == found) {
        workspaces[source_ws].list_Cl = NULL;
    } else {
        found->prev->next = found->next;
        found->next->prev = found->prev;
        if(workspaces[source_ws].list_Cl == found) {
            workspaces[source_ws].list_Cl = found->next;
        }
    }

    found->x = monitors[target_monitor].x + GAPS;
    found->y = monitors[target_monitor].y + GAPS;
    XMoveWindow(disp, focused_win, found->x, found->y);



    if(workspaces[ws_target].list_Cl == NULL) {
        workspaces[ws_target].list_Cl = found;
        found->next = found;
        found->prev = found;
    } else {
        Client *head = workspaces[ws_target].list_Cl;
        Client *tail = head->prev;
        
        found->next = head;
        found->prev = tail;
        tail->next = found;
        head->prev = found;
    }

	Dwindle(disp, ws_target);
	Dwindle(disp, source_ws);

    XSync(disp, False);

    if(workspaces[ws_target].monitor_id != -1) {
        FocusWindow(disp, found->id);
    } else {
        if (workspaces[source_ws].list_Cl != NULL) {
            FocusWindow(disp, workspaces[source_ws].list_Cl->id);
        } else {
            XSetInputFocus(disp, root, RevertToParent, CurrentTime);
        }
    }
    
    XSync(disp, False);
	UpdateBarIPC(disp, root);
}



void 
RemoveWindowList(Display *disp, Window w, Window root)
{
	int ws_index = -1;
	Client *found = FindClientByWindow(w, &ws_index);
	


	if(found == NULL)
		return;



	if(found->next == found) {
		workspaces[ws_index].list_Cl = NULL;
	}
	else {
		found->prev->next = found->next;
		found->next->prev = found->prev;

		if(workspaces[ws_index].list_Cl == found) {
			workspaces[ws_index].list_Cl = found->next;
		}

		if (workspaces[ws_index].monitor_id != -1) {
            FocusWindow(disp, workspaces[ws_index].list_Cl->id);
        }
	}

	DEBUG_LOG("[-] window %lu removed (WS %d)\n", w, ws_index);
	free(found);

	Dwindle(disp, ws_index);
	UpdateBarIPC(disp, root);

}



void
KillWindow(Display  *disp, Window root)
{

	Window focused_win;
	int revert_to;

	XGetInputFocus(disp, &focused_win, &revert_to);

	if (focused_win == None || focused_win == root)
        return;

    DEBUG_LOG("[ASH-WM] Uccido la finestra hardware: %lu\n", focused_win);
    
    XDestroyWindow(disp, focused_win);
}



//layout func
void
Dwindle(Display *disp, int ws_index)
{
    if (ws_index < 0 || ws_index >= WORKSPACES) return;

    Client *head = workspaces[ws_index].list_Cl;
    Client *fullscreen_client = NULL;

    if(head == NULL)
        return;

    int count_ws = 0;
    Client *cursor = head;
    do {
        if (!cursor->is_floating && !cursor->is_fullscreen) {
            count_ws++;
        }
        if (cursor->is_fullscreen) {
            fullscreen_client = cursor;
        }
        cursor = cursor->next;
    } while(cursor != head);

    int mod_index = workspaces[ws_index].monitor_id;
    
    if(mod_index == -1)
    {
        cursor = head;
        do {
            XUnmapWindow(disp, cursor->id);
            cursor = cursor->next;
        } while(cursor != head);
        return;
    }

    if (fullscreen_client != NULL) {
        cursor = head;
        do {
            if (cursor == fullscreen_client) {
                cursor->x = monitors[mod_index].x;
                cursor->y = monitors[mod_index].y;
                cursor->w = monitors[mod_index].width;
                cursor->h = monitors[mod_index].height;

                XSetWindowBorderWidth(disp, cursor->id, 0);
                XMoveResizeWindow(disp, cursor->id, cursor->x, cursor->y, cursor->w, cursor->h);
                XMapWindow(disp, cursor->id);
                XRaiseWindow(disp, cursor->id);
            } else {
                XUnmapWindow(disp, cursor->id);
            }
            cursor = cursor->next;
        } while(cursor != head);
        return; 
    }

    int my = monitors[mod_index].y ;
    int mh = monitors[mod_index].height ;
    int mx = monitors[mod_index].x;
    int mw = monitors[mod_index].width;

	unsigned long pad_top = 0;
    unsigned long pad_bottom = 0;
    unsigned long pad_left = 0;
    unsigned long pad_right = 0;

    const unsigned int MIN_WIDTH = 30;
    const unsigned int MIN_HEIGHT = 30;

	Window root_return, parent_return, *children;
    unsigned int nchildren;

	if (XQueryTree(disp, DefaultRootWindow(disp), &root_return, &parent_return, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; i++) {
			//read if w require space 
            Strut s = GetWindowStrut(disp, children[i]);
            
            if (s.top > pad_top)       pad_top = s.top;
            if (s.bottom > pad_bottom) pad_bottom = s.bottom;
            if (s.left > pad_left)     pad_left = s.left;
            if (s.right > pad_right)   pad_right = s.right;
        }
        if (children) XFree(children);
    }

	mx += pad_left;
    mw -= (pad_left + pad_right);
    my += pad_top;
    mh -= (pad_top + pad_bottom);



	// all floatig window
    if (count_ws == 0) {
        cursor = head;
        do {
            XSetWindowBorderWidth(disp, cursor->id, BORDER_WIDTH);
            XMoveResizeWindow(disp, cursor->id, cursor->x, cursor->y, cursor->w, cursor->h);
            XMapWindow(disp, cursor->id);
            XRaiseWindow(disp, cursor->id); 
            cursor = cursor->next;
        } while(cursor != head);
        return;
    }

	// window in tl
    if(count_ws == 1)
    {
        cursor = head;
        do {
            if (!cursor->is_floating && !cursor->is_fullscreen){
                int target_w = mw - (GAPS * 2);
                int target_h = mh - (GAPS * 2);

                cursor->x = mx + GAPS;
                cursor->y = my + GAPS;
                cursor->w = target_w < (int)MIN_WIDTH ? MIN_WIDTH : (unsigned int)target_w;
                cursor->h = target_h < (int)MIN_HEIGHT ? MIN_HEIGHT : (unsigned int)target_h;

                XSetWindowBorderWidth(disp, cursor->id, BORDER_WIDTH);
                XMoveResizeWindow(disp, cursor->id, cursor->x, cursor->y, cursor->w, cursor->h);
                XMapWindow(disp, cursor->id); 
            }
            cursor = cursor->next;
        } while(cursor != head);

        cursor = head;
        do {
            if(cursor->is_floating) {
                XSetWindowBorderWidth(disp, cursor->id, BORDER_WIDTH);
                XMoveResizeWindow(disp, cursor->id, cursor->x, cursor->y, cursor->w, cursor->h);
                XMapWindow(disp, cursor->id);
                XRaiseWindow(disp, cursor->id); 
            }
            cursor = cursor->next;
        } while(cursor != head);
        return;
    }

	//more wm in tl
    cursor = head;
    int wx = mx;
    int wy = my;
    int ww = mw;
    int wh = mh;
    int tiling_idx = 0;

    do {
        if (cursor->is_floating) {
            cursor = cursor->next;
            continue;
        }

        XSetWindowBorderWidth(disp, cursor->id, BORDER_WIDTH);

        if(tiling_idx == count_ws - 1)
        {
            int target_x = wx + GAPS;
            int target_y = wy + GAPS;
            int target_w = ww - (GAPS * 2);
            int target_h = wh - (GAPS * 2);

            cursor->x = target_x;
            cursor->y = target_y;
            cursor->w = target_w < (int)MIN_WIDTH ? MIN_WIDTH : (unsigned int)target_w;
            cursor->h = target_h < (int)MIN_HEIGHT ? MIN_HEIGHT : (unsigned int)target_h;
        }
        else
        {
            if(tiling_idx % 2 == 0)
            {
                ww /= 2;
                if (ww < (int)MIN_WIDTH) ww = MIN_WIDTH; 

                int target_w = ww - (GAPS * 2);
                cursor->x = wx + GAPS;
                cursor->y = wy + GAPS;
                cursor->w = target_w < (int)MIN_WIDTH ? MIN_WIDTH : (unsigned int)target_w;
                cursor->h = (wh - (GAPS * 2)) < (int)MIN_HEIGHT ? MIN_HEIGHT : (unsigned int)(wh - (GAPS * 2));
                wx += ww;
            }
            else
            {
                wh /= 2;
                if (wh < (int)MIN_HEIGHT) wh = MIN_HEIGHT;

                int target_h = wh - (GAPS * 2);
                cursor->x = wx + GAPS;
                cursor->y = wy + GAPS;
                cursor->w = (ww - (GAPS * 2)) < (int)MIN_WIDTH ? MIN_WIDTH : (unsigned int)(ww - (GAPS * 2));
                cursor->h = target_h < (int)MIN_HEIGHT ? MIN_HEIGHT : (unsigned int)target_h;
                wy += wh;
            }
        }

        if (cursor->x + (int)cursor->w > mx + mw) cursor->x = (mx + mw) - (int)cursor->w;
        if (cursor->y + (int)cursor->h > my + mh) cursor->y = (my + mh) - (int)cursor->h;
        if (cursor->x < mx) cursor->x = mx;
        if (cursor->y < my) cursor->y = my;

        XMoveResizeWindow(disp, cursor->id, cursor->x, cursor->y, cursor->w, cursor->h);
        XMapWindow(disp, cursor->id); 
        
        tiling_idx++;
        cursor = cursor->next;
    } while(cursor != head);

    cursor = head;
    do {
        if (cursor->is_floating) {
            XSetWindowBorderWidth(disp, cursor->id, BORDER_WIDTH);
            XMoveResizeWindow(disp, cursor->id, cursor->x, cursor->y, cursor->w, cursor->h);
            XMapWindow(disp, cursor->id);
            XRaiseWindow(disp, cursor->id); 
        }
        cursor = cursor->next;
    } while(cursor != head);
}



void 
UpdateCurrentMonitor(Display *disp, Window root)
{
	Window dummy_win;
	int dummy_int;
	unsigned int dummy_mask;
	int mouse_x, mouse_y;
	int target_monitor = 0;

	XQueryPointer(disp, root, &dummy_win, &dummy_win, &mouse_x , &mouse_y , &dummy_int, &dummy_int, &dummy_mask);
	
	for(int i = 0; i< monitors_count; i++)
	{
		if (mouse_x >= monitors[i].x && mouse_x < (monitors[i].x + monitors[i].width) &&
				mouse_y >= monitors[i].y && mouse_y < (monitors[i].y + monitors[i].height)) {
			target_monitor = i;
			break;
		}

	}


	int active_ws = monitors[target_monitor].current_ws;

	if(workspaces[active_ws].monitor_id != target_monitor)
	{
		DEBUG_LOG("[ASH-WM] mouse active in Monitor %d (WS %d)\n", target_monitor, active_ws);
		workspaces[active_ws].monitor_id = target_monitor;
	}
}



void 
MoveWindowToMonitor(Display *disp, Window root, Window w)
{
    if (monitors_count <= 1 || w == None || w == root)
        return;

    int source_ws = -1;
    Client *found = FindClientByWindow(w, &source_ws);

    if (source_ws == -1 || found == NULL)
        return;

    int current_monitor = source_ws / WORKSPACES_X_MONITOR;
    int target_monitor = (current_monitor + 1) % monitors_count;
    int ws_target = monitors[target_monitor].current_ws;

    DEBUG_LOG("[ASH-WM] mv window %lu from Monitor %d (WS %d) to Monitor %d (WS %d)\n", 
           w, current_monitor, source_ws, target_monitor, ws_target);

    XUnmapWindow(disp, found->id);

    if (found->next == found) {
        workspaces[source_ws].list_Cl = NULL; 
    } else {
        found->prev->next = found->next;
        found->next->prev = found->prev;
        if (workspaces[source_ws].list_Cl == found) {
            workspaces[source_ws].list_Cl = found->next;
        }
    }

    if (workspaces[ws_target].list_Cl == NULL) {
        workspaces[ws_target].list_Cl = found;
        found->next = found;
        found->prev = found;
    } else {
        Client *head = workspaces[ws_target].list_Cl;
        Client *tail = head->prev;
        
        found->next = head;
        found->prev = tail;
        tail->next = found;
        head->prev = found;
    }

    Dwindle(disp, ws_target);
    Dwindle(disp, source_ws);

    if (monitors[target_monitor].current_ws == ws_target) {
        XMapWindow(disp, found->id); 
        XWarpPointer(disp, None, found->id, 0, 0, 0, 0, found->w / 2, found->h / 2);
        FocusWindow(disp, found->id);
    } else {
        if (workspaces[source_ws].list_Cl != NULL) {
            FocusWindow(disp, workspaces[source_ws].list_Cl->id);
        } else {
            XSetInputFocus(disp, root, RevertToParent, CurrentTime);
        }
    }

    XSync(disp, False);
}



unsigned long 
GetXColor(Display *disp, unsigned long hex_color)
{
	XColor col;
	col.red   = ((hex_color >> 16) & 0xFF) * 257;
    col.green = ((hex_color >> 8)  & 0xFF) * 257;
    col.blue  = (hex_color         & 0xFF) * 257;
    col.flags = DoRed | DoGreen | DoBlue;
    XAllocColor(disp, DefaultColormap(disp, DefaultScreen(disp)), &col);
    return col.pixel;
}



void 
CycleFocus(Display *disp, int direction) {
    Window dummy_win;
    int dummy_int;
    unsigned int dummy_mask;
    int mouse_x, mouse_y;
    int mon_idx = 0;

    XQueryPointer(disp, DefaultRootWindow(disp), &dummy_win, &dummy_win, &mouse_x, &mouse_y, &dummy_int, &dummy_int, &dummy_mask);
    for (int i = 0; i < monitors_count; i++) {
        if (mouse_x >= monitors[i].x && mouse_x < (monitors[i].x + monitors[i].width) &&
            mouse_y >= monitors[i].y && mouse_y < (monitors[i].y + monitors[i].height)) {
            mon_idx = i;
            break;
        }
    }

    int ws = monitors[mon_idx].current_ws; 
    Client *head = workspaces[ws].list_Cl;
    if (head == NULL) return; 

    Window current_focus;
    int revert_to;
    XGetInputFocus(disp, &current_focus, &revert_to);

    Client *curr = head;
    Client *target = NULL;

    do {
        if (curr->id == current_focus) {
            target = curr;
            break;
        }
        curr = curr->next; 
    } while (curr != head);

    if (target == NULL) {
        FocusWindow(disp, head->id);
        return;
    }

    Client *next_node = (direction == 1) ? target->next : target->prev;

    if (next_node != NULL) {
        FocusWindow(disp, next_node->id);
		RaiseFloatingWindows(disp, ws);
    }
}



void 
ToggleFullscreen(Display *disp, Window root) {
    Window focused_win;
    int revert_to;
    XGetInputFocus(disp, &focused_win, &revert_to);
    if (focused_win == None || focused_win == root) return;

    int ws = -1;
    Client *found = NULL;
    for (int i = 0; i < WORKSPACES; i++) {
        Client *curr = workspaces[i].list_Cl;
        if (curr != NULL) {
            do {
                if (curr->id == focused_win) {
                    ws = i;
                    found = curr;
                    break;
                }
                curr = curr->next;
            } while (curr != workspaces[i].list_Cl);
        }
        if (ws != -1) break;
    }

    if (!found || workspaces[ws].monitor_id == -1) return;

    int mon = workspaces[ws].monitor_id;

    if (!found->is_fullscreen) {
		// save gemetry before go full screnn
        found->old_x = found->x;
        found->old_y = found->y;
        found->old_w = found->w;
        found->old_h = found->h;

        found->x = monitors[mon].x;
        found->y = monitors[mon].y;
        found->w = monitors[mon].width;
        found->h = monitors[mon].height;

        found->is_fullscreen = 1;
        
        XSetWindowBorderWidth(disp, found->id, 0);
        XMoveResizeWindow(disp, found->id, found->x, found->y, found->w, found->h);
        XRaiseWindow(disp, found->id);
    } else {
        found->is_fullscreen = 0;
        XSetWindowBorderWidth(disp, found->id, BORDER_WIDTH);
        
        if (!found->is_floating) {
            Dwindle(disp, ws);
        } else {
            found->x = found->old_x;
            found->y = found->old_y;
            found->w = found->old_w;
            found->h = found->old_h;
            XMoveResizeWindow(disp, found->id, found->x, found->y, found->w, found->h);
        }
    }
    XSync(disp, False);
}



void 
SwapDwindleDirectional(Display *disp, int direction) { 
    Window focused_win;
    int revert_to;
    XGetInputFocus(disp, &focused_win, &revert_to);
    if (focused_win == None) return;

    Window dummy_win; int dummy_int; unsigned int dummy_mask;
    int mouse_x, mouse_y; int mon_idx = 0;
    XQueryPointer(disp, DefaultRootWindow(disp), &dummy_win, &dummy_win, &mouse_x, &mouse_y, &dummy_int, &dummy_int, &dummy_mask);
    for (int i = 0; i < monitors_count; i++) {
        if (mouse_x >= monitors[i].x && mouse_x < (monitors[i].x + monitors[i].width) &&
            mouse_y >= monitors[i].y && mouse_y < (monitors[i].y + monitors[i].height)) {
            mon_idx = i; break;
        }
    }

    int ws = monitors[mon_idx].current_ws;
    Client *head = workspaces[ws].list_Cl;
    
    if (head == NULL || head->next == head) return;

    Client *curr = head;
    Client *this_client = NULL;
    do {
        if (curr->id == focused_win) {
            this_client = curr;
            break;
        }
        curr = curr->next;
    } while (curr != head);

    if (!this_client || this_client->is_fullscreen) return;

    Client *target_client = (direction == 1) ? this_client->next : this_client->prev;

    Window temp_id = this_client->id;
    int temp_fs = this_client->is_fullscreen;

    this_client->id = target_client->id;
    this_client->is_fullscreen = target_client->is_fullscreen;

    target_client->id = temp_id;
    target_client->is_fullscreen = temp_fs;

    Dwindle(disp, ws);

    XRaiseWindow(disp, target_client->id);
    XSetInputFocus(disp, target_client->id, RevertToParent, CurrentTime);
    XSync(disp, False);
}



void
RaiseFloatingWindows(Display *disp, int ws_index)
{
    if (ws_index < 0 || ws_index >= WORKSPACES) return;

    Client *head = workspaces[ws_index].list_Cl;
    if (head == NULL) return;

    Client *cursor = head;
    do {
        if (cursor->is_floating && !cursor->is_fullscreen) {
            XRaiseWindow(disp, cursor->id);
        }
        cursor = cursor->next;
    } while (cursor != head);
}


//----
int
XErrorHandlerImpl(Display *disp, XErrorEvent *ee)
{
	(void)disp;
    if (ee->error_code == BadWindow ||
        (ee->request_code == 12 && ee->error_code == BadMatch) || // ConfigureWindow BadMatch
        (ee->request_code == 42 && ee->error_code == BadWindow))  // SetInputFocus BadWindow
    {
        DEBUG_LOG("[ASH-WM] Error X11 safely.\n");
        return 0;
    }
    
    fprintf(stderr, "[ASH-WM] Error fatal X11: major %d, error %d\n", ee->request_code, ee->error_code);
    return 0; 
}







int main(int argc, char *argv[])
{

    if (argc > 1 && strcmp(argv[1], "-h") == 0)
    {
        printf("-h show this.\n");
        printf("-v show ashwm version.\n");
		return 0;
    }
	
    if (argc > 1 && strcmp(argv[1], "-v") == 0)
    {
        printf("ashwm Version: %d\n", VERSION);
		return 0;
    }


	XEvent Ev;
	unsigned int clean_state;

	XWindowAttributes mouse_attr;
    XButtonEvent mouse_start;
    mouse_start.subwindow = None;

	Window click_win;
	Window focused_win;


	Display *disp = XOpenDisplay(NULL);
	if(disp == NULL){
		fprintf(stderr, "error opening the display");
		return 1;
	}

	//error handler
	XSetErrorHandler(XErrorHandlerImpl);

	
	int sc = DefaultScreen(disp);

	//create root window/desktop
	Window root = RootWindow(disp, sc);

	//for the cursor 
	Cursor cursor = XcursorLibraryLoadCursor(disp, "left_ptr");

    if (cursor != None)
        XDefineCursor(disp, root, cursor);

    XFlush(disp);


	//add to the wait
	XSelectInput(disp, root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask | PointerMotionMask);

	XSync(disp, False);

	//read config.h binds
	unsigned int modifiers[] = { 0, LockMask, Mod2Mask, LockMask | Mod2Mask };

	for (int i = 0; i < num_keys; i++) {
		KeyCode code = XKeysymToKeycode(disp, keys[i].keysym);

		for (unsigned int j = 0; j < 4; j++) {
			XGrabKey(disp, code, keys[i].mod | modifiers[j], root, True, GrabModeAsync, GrabModeAsync);
		}
	}

    XGrabButton(disp, Button1, MODIFIER, root, True,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync, None, None);

    XGrabButton(disp, Button3, MODIFIER, root, True,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync, None, None);
	
	
	//get monitors
	if(XineramaIsActive(disp))
	{
		int n;
		XineramaScreenInfo *info = XineramaQueryScreens(disp, &n);

		monitors_count = n < N_MONITORS ? n : N_MONITORS;

		
		for(int i = 0; i < monitors_count; i++)
		{
			monitors[i].id = info[i].screen_number;
			monitors[i].x = info[i].x_org;
			monitors[i].y = info[i].y_org;
			monitors[i].width = info[i].width;
			monitors[i].height = info[i].height;


			DEBUG_LOG("[ASH-WM] Salvato Monitor %d: X=%d, Y=%d, W=%d, H=%d\n", 
               i, monitors[i].x, monitors[i].y, monitors[i].width, monitors[i].height);
		}
		XFree(info);
	}
	else
	{
		monitors_count = 1;
		monitors[0].id = 0;
		monitors[0].x = 0;
		monitors[0].y = 0;
		monitors[0].width = DisplayWidth(disp, sc);
		monitors[0].height = DisplayHeight(disp, sc);
	}
	

	


	//inizialize all 20 workspaces
	for(int i = 0 ; i < WORKSPACES; i++)
	{
		workspaces[i].id = 0;
		workspaces[i].list_Cl = NULL;
		workspaces[i].monitor_id = -1;
	}
	
	for(int i = 0 ; i < monitors_count; i++)
	{
		int start_ws = i * WORKSPACES_X_MONITOR; // Monitor 0 -> WS 0 | Monitor 1 -> WS 10
		monitors[i].current_ws = start_ws;
		workspaces[start_ws].monitor_id = i;
	}

	
	signal(SIGCHLD, SIG_IGN);
	while(1)
	{

		XNextEvent(disp, &Ev);

		switch (Ev.type) {

			case MapRequest:	 
				AddWindowList(disp, Ev.xmaprequest.window, root );

				DEBUG_LOG("[+] new window id: %lu\n", Ev.xmaprequest.window);

				FocusWindow(disp, Ev.xmaprequest.window);
				break;



				case KeyPress:
                clean_state = Ev.xkey.state & ~(LockMask | Mod2Mask);

                for(int i = 0 ; i < num_keys; i++)
                {
                    if((Ev.xkey.keycode == XKeysymToKeycode(disp, keys[i].keysym)) 
                            && (clean_state == keys[i].mod))
                    {
                        if(keys[i].cmd != NULL)
                        {
                            if(fork() == 0)
                            {
                                if (disp) close(ConnectionNumber(disp)); 
                                execvp(keys[i].cmd[0], keys[i].cmd);
                                exit(0);
                            }
                        }
                        else 
                        {    
                            switch (keys[i].action) 
                            {
                                case ACTION_KILL:
                                    KillWindow(disp, root);
                                    break;
                                    
                                case ACTION_MOVE_MONITOR:
									focused_win = None;
									int revert_to;

									// Chiediamo a X11 qual è la finestra attiva in questo millesimo di secondo
									XGetInputFocus(disp, &focused_win, &revert_to);

									// Se c'è una finestra effettivamente focalizzata (e non è il desktop/root)
									if (focused_win != None && focused_win != root) {
										MoveWindowToMonitor(disp, root, focused_win);
									}
                                    break;
                                    
                                case ACTION_FOCUS_NEXT:
                                    CycleFocus(disp, 1);
                                    break;
                                    
                                case ACTION_FOCUS_PREV:
                                    CycleFocus(disp, -1);
                                    break;
                                    
                                case ACTION_TOGGLE_FULLSCREEN:
                                    ToggleFullscreen(disp, root);
                                    break;

                                case ACTION_SWAP_NEXT:
                                    SwapDwindleDirectional(disp, 1);
                                    break;
                                    
                                case ACTION_SWAP_PREV:
                                    SwapDwindleDirectional(disp, -1);
                                    break;

                                case ACTION_TOGGLE_FLOATING:
                                    {
                                        Window focused_win; int rev;
                                        XGetInputFocus(disp, &focused_win, &rev);
                                        if (focused_win != None) {
                                            Window dummy_win; int dummy_int; unsigned int dummy_mask;
                                            int mouse_x, mouse_y; int mon_idx = 0;
                                            XQueryPointer(disp, root, &dummy_win, &dummy_win, &mouse_x, &mouse_y, &dummy_int, &dummy_int, &dummy_mask);
                                            for (int m = 0; m < monitors_count; m++) {
                                                if (mouse_x >= monitors[m].x && mouse_x < (monitors[m].x + monitors[m].width) &&
                                                        mouse_y >= monitors[m].y && mouse_y < (monitors[m].y + monitors[m].height)) {
                                                    mon_idx = m; break;
                                                }
                                            }
                                            int ws = monitors[mon_idx].current_ws;

                                            Client *curr = workspaces[ws].list_Cl;
                                            if (curr != NULL) {
                                                do {
                                                    if (curr->id == focused_win && !curr->is_fullscreen) {
                                                        curr->is_floating = !curr->is_floating; 
                                                        break;
                                                    }
                                                    curr = curr->next;
                                                } while(curr != workspaces[ws].list_Cl);
                                            }
                                            Dwindle(disp, ws); 
                                        }
                                    }
                                    break;

								case ACTION_WORKSPACE:
									{
										int ws_target = keys[i].arg % WORKSPACES_X_MONITOR; 

										if (keys[i].mod & WS_MODIFIER) {
											MoveToWorkspace(disp, root, ws_target);
										} else {
											ChangeWorkspace(disp, root, ws_target);
										}
									}
									break;


                                default:
                                    break;
                            } 
                        } 
                        break;
                    } 
                } 
                break; 



			case ButtonPress:

				click_win = Ev.xbutton.subwindow;

                // Se clicchi sulla barra, ignoriamo il click: non si trascina!
                if (IsDock(disp, click_win)) {
                    break; 
                }

				if (Ev.xbutton.subwindow != None) {
					XGetWindowAttributes(disp, Ev.xbutton.subwindow, &mouse_attr);

					mouse_start = Ev.xbutton;

					Window dummy_win; int dummy_int; unsigned int dummy_mask;
					int mouse_x, mouse_y; int mon_idx = 0;
					XQueryPointer(disp, root, &dummy_win, &dummy_win, &mouse_x, &mouse_y, &dummy_int, &dummy_int, &dummy_mask);
					for (int m = 0; m < monitors_count; m++) {
						if (mouse_x >= monitors[m].x && mouse_x < (monitors[m].x + monitors[m].width) &&
								mouse_y >= monitors[m].y && mouse_y < (monitors[m].y + monitors[m].height)) {
							mon_idx = m; break;
						}
					}
					int ws = monitors[mon_idx].current_ws;

					if (Ev.xbutton.state & MODIFIER) {
						Client *h = workspaces[ws].list_Cl;
						if (h != NULL) {
							Client *c = h;
							do {
								if (c->id == Ev.xbutton.subwindow && !c->is_fullscreen) {
									c->is_floating = 1;
									break;
								}
								c = c->next;
							} while (c != h);
						}
						Dwindle(disp, ws);
					}

					FocusWindow(disp, Ev.xbutton.subwindow);
				}

				XAllowEvents(disp, ReplayPointer, CurrentTime);
				break;

			case MotionNotify:

				if (IsDock(disp, mouse_start.subwindow)) {
                    break;
                }

				if (mouse_start.subwindow != None) {
					// calcolate mouse mv 
					int xdiff = Ev.xbutton.x_root - mouse_start.x_root;
					int ydiff = Ev.xbutton.y_root - mouse_start.y_root;

					if (mouse_start.button == Button1) {
						//mv window
						XMoveWindow(disp, mouse_start.subwindow, 
								mouse_attr.x + xdiff, 
								mouse_attr.y + ydiff);

						int current_mouse_x = Ev.xmotion.x_root;
						int current_mouse_y = Ev.xmotion.y_root;
						int target_monitor = -1;

						for (int m = 0; m < monitors_count; m++) {
							if (current_mouse_x >= monitors[m].x && current_mouse_x < (monitors[m].x + monitors[m].width) &&
									current_mouse_y >= monitors[m].y && current_mouse_y < (monitors[m].y + monitors[m].height)) {
								target_monitor = m;
								break;
							}
						}

						if (target_monitor != -1) {
							int active_ws_target = monitors[target_monitor].current_ws;

							int source_ws = -1;
							Client *c_to_move = NULL;

							for (int w = 0; w < WORKSPACES; w++) {
								Client *curr = workspaces[w].list_Cl;
								if (curr != NULL) {
									do {
										if (curr->id == mouse_start.subwindow) {
											source_ws = w;
											c_to_move = curr;
											break;
										}
										curr = curr->next;
									} while (curr != workspaces[w].list_Cl);
								}
								if (c_to_move != NULL) break;
							}

							if (c_to_move != NULL && source_ws != active_ws_target) {

								// rm window from old workspace 
								if (c_to_move->next == c_to_move) {
									workspaces[source_ws].list_Cl = NULL;
								} else {
									c_to_move->prev->next = c_to_move->next;
									c_to_move->next->prev = c_to_move->prev;
									if (workspaces[source_ws].list_Cl == c_to_move) {
										workspaces[source_ws].list_Cl = c_to_move->next;
									}
								}
								Dwindle(disp, source_ws);  

								Client *target_head = workspaces[active_ws_target].list_Cl;
								if (target_head == NULL) {
									c_to_move->next = c_to_move;
									c_to_move->prev = c_to_move;
									workspaces[active_ws_target].list_Cl = c_to_move;
								} else {
									c_to_move->next = target_head;
									c_to_move->prev = target_head->prev;
									target_head->prev->next = c_to_move;
									target_head->prev = c_to_move;
								}

								c_to_move->is_floating = 1; 

								Dwindle(disp, active_ws_target);
								DEBUG_LOG("[ASH-WM] window moved in %lu in the Monitor %d (Workspace %d)\n", 
										c_to_move->id, target_monitor, active_ws_target);
							}
						}
					} 
					else if (mouse_start.button == Button3) {
						int new_w = mouse_attr.width + xdiff;
						int new_h = mouse_attr.height + ydiff;

						if (new_w < 60) new_w = 60;
						if (new_h < 60) new_h = 60;

						XResizeWindow(disp, mouse_start.subwindow, new_w, new_h);
					}
				} else {
					UpdateCurrentMonitor(disp, root);
				}
				break;

			case ButtonRelease:
				click_win = Ev.xbutton.subwindow; // o ev.xbutton.window

				if (IsDock(disp, click_win)) {
					break; // Ignora il click, non avviare il dragging/floating!
				}

				if (mouse_start.subwindow != None) {
					int ws = -1;
					Client *found_client = NULL;

					for (int w = 0; w < WORKSPACES; w++) {
						Client *curr = workspaces[w].list_Cl;
						if (curr != NULL) {
							do {
								if (curr->id == mouse_start.subwindow) {
									ws = w;
									found_client = curr;
									break;
								}
								curr = curr->next;
							} while (curr != workspaces[w].list_Cl);
						}
						if (found_client != NULL) break;
					}

					if (found_client != NULL) {
						XWindowAttributes final_attr;
						XGetWindowAttributes(disp, found_client->id, &final_attr);
						found_client->x = final_attr.x;
						found_client->y = final_attr.y;
						found_client->w = final_attr.width;
						found_client->h = final_attr.height;

						Dwindle(disp, ws);
						FocusWindow(disp, found_client->id);
					}
					mouse_start.subwindow = None; 
				}
				break;





				//mouse movement
			case EnterNotify:


				if (Ev.xcrossing.mode != NotifyNormal || Ev.xcrossing.detail == NotifyInferior)
					break;

				UpdateCurrentMonitor(disp, root);

				XWindowAttributes wa;
				XGetWindowAttributes(disp, Ev.xcrossing.window, &wa);
				if (wa.map_state == IsViewable) {
					DEBUG_LOG("[ASH-WM] Il mouse è entrato nella finestra %lu. Cambio focus.\n", Ev.xcrossing.window);
					FocusWindow(disp, Ev.xcrossing.window);
				}
				break;


			case FocusIn:
				// focusing the floating window
				{
					Window dummy_win; int dummy_int; unsigned int dummy_mask;
					int mouse_x, mouse_y; int mon_idx = 0;
					XQueryPointer(disp, root, &dummy_win, &dummy_win, &mouse_x, &mouse_y, &dummy_int, &dummy_int, &dummy_mask);
					for (int m = 0; m < monitors_count; m++) {
						if (mouse_x >= monitors[m].x && mouse_x < (monitors[m].x + monitors[m].width) &&
								mouse_y >= monitors[m].y && mouse_y < (monitors[m].y + monitors[m].height)) {
							mon_idx = m; break;
						}
					}
					int ws = monitors[mon_idx].current_ws;
					RaiseFloatingWindows(disp, ws);
				}
				break;

			case DestroyNotify:	 
				RemoveWindowList(disp ,Ev.xdestroywindow.window, root);
				break;






			default:
				break;

		}

	}


	XCloseDisplay(disp);
	return 0;
}
