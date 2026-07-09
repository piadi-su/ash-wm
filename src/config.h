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
}KeyBinds;

#define MODIFIER Mod4Mask

//tutti i conandi
static char *term_cmd[] = {"alacritty", NULL};
static char *clock_cmd[] = {"xclock", NULL};
static char *firefox_cmd[] = {"firefox", NULL};
static char *rofi_cmd[] = {"rofi", "-show", "drun", NULL};
static char *close_wm[] = {"killall", "ash-wm", NULL};


static KeyBinds keys[] = {
// Modificatore | Tasto | Comando

	{MODIFIER, XK_Return, term_cmd},
	// {MODIFIER, XK_o, firefox_cmd},
	{MODIFIER, XK_o, clock_cmd},
	{MODIFIER, XK_d, rofi_cmd},
	{MODIFIER, XK_u, close_wm},

};

// calcola il numero totale
static const int num_keys = sizeof(keys) / sizeof(keys[0]);


#endif
