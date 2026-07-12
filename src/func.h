#ifndef FUNC_H
#define FUNC_H

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

#include <X11/extensions/Xinerama.h>



void FocusWindow(Display *disp, Window w);
void AddWindowList(Display *disp, Window w, Window root);
void ChangeWorkspace(Display *disp, Window root, int new_ws);
void MoveToWorkspace(Display *disp, Window root, int ws_target);
void RemoveWindowList(Display *disp, Window w);
void KillWindow(Display  *disp, Window root);
void Dwindle(Display *disp, int ws_index);
void UpdateCurrentMonitor(Display *disp, Window root);
void MoveWindowToMonitor(Display *disp, Window root);
unsigned long GetXColor(Display *disp, unsigned long hex_color);
void CycleFocus(Display *disp, int direction);
void FocusWindow(Display *disp, Window w);
void ToggleFullscreen(Display *disp, Window root);
void SwapDwindleDirectional(Display *disp, int direction);
int XErrorHandlerImpl(Display *disp, XErrorEvent *ee);

#endif
