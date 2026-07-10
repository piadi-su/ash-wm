#ifndef CONFIG_H
#define CONFIG_H

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

// struct che definisce i KeyBinds
typedef struct {
	unsigned int mod;
	KeySym keysym;
	char **cmd;
	int arg;
}KeyBinds;

// #define MODIFIER Mod4Mask
#define MODIFIER Mod1Mask
#define WS_MODIFIER ShiftMask

//macro per genereare i workaspce nel chill
#define WORKSPACE_KEYS(KEY, WS_INDEX) \
    { MODIFIER,             KEY, NULL, WS_INDEX }, \
    { MODIFIER|WS_MODIFIER,   KEY, NULL, WS_INDEX }


#define GAPS 10

//tutti i conandi
static char *term_cmd[] = {"alacritty", NULL};
static char *clock_cmd[] = {"xclock", NULL};
// static char *firefox_cmd[] = {"firefox", NULL};
static char *rofi_cmd[] = {"rofi", "-show", "drun", NULL};
static char *close_wm[] = {"killall", "ash-wm", NULL};


static KeyBinds keys[] = {
// Modificatore | Tasto | Comando

	{MODIFIER, XK_Return, term_cmd, -1},
	// {MODIFIER, XK_o, firefox_cmd},
	{MODIFIER, XK_o, clock_cmd, -1},
	{MODIFIER, XK_d, rofi_cmd, -1},
	{MODIFIER, XK_u, close_wm, -1},

	{MODIFIER, XK_q, NULL, -1},


	//bind per tutti i workspace
	WORKSPACE_KEYS(XK_1, 0),
    WORKSPACE_KEYS(XK_2, 1),
    WORKSPACE_KEYS(XK_3, 2),
    WORKSPACE_KEYS(XK_4, 3),
    WORKSPACE_KEYS(XK_5, 4),
    WORKSPACE_KEYS(XK_6, 5),
    WORKSPACE_KEYS(XK_7, 6),
    WORKSPACE_KEYS(XK_8, 7),
    WORKSPACE_KEYS(XK_9, 8),
    WORKSPACE_KEYS(XK_0, 9),

};

// calcola il numero totale
static const int num_keys = sizeof(keys) / sizeof(keys[0]);


#endif
