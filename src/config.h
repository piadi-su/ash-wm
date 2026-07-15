#ifndef CONFIG_H
#define CONFIG_H

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

// command wnum
typedef enum {
    ACTION_NONE = 0,
    ACTION_KILL,
    ACTION_MOVE_MONITOR,
    ACTION_FOCUS_NEXT,
    ACTION_FOCUS_PREV,
    ACTION_TOGGLE_FULLSCREEN,
    ACTION_SWAP_NEXT,
    ACTION_SWAP_PREV,
    ACTION_TOGGLE_FLOATING,
	ACTION_WORKSPACE
} WMAction;

typedef struct {
    unsigned int mod;
    KeySym keysym;
    char **cmd;
    WMAction action; 
    int arg;         
} KeyBinds;

//macro to define workspace
#define WORKSPACE_KEYS(KEY, WS_INDEX) \
    { MODIFIER,             KEY, NULL, ACTION_WORKSPACE, WS_INDEX }, \
    { MODIFIER|WS_MODIFIER,   KEY, NULL, ACTION_WORKSPACE, WS_INDEX }



//MOD key
#define MODIFIER Mod4Mask //win
// #define MODIFIER Mod1Mask //alt



#define WS_MODIFIER ShiftMask



#define N_MONITORS 2



#define GAPS 6



#define BORDER_WIDTH 2
// #define COLOR_FOCUS   0x5294E2      


#define COLOR_FOCUS   0xff0000
#define COLOR_UNFOCUS 0x000000




//ALL COMMANDS
static char *term_cmd[] = {"alacritty", NULL};
static char *firefox_cmd[] = {"firefox", NULL};
static char *files_cmd[] = {"xclock", NULL};
static char *rofi_cmd[] = {"rofi", "-show", "drun", NULL};


static KeyBinds keys[] = {
    // MOD | KEY | CMD | WM ACTION | ARGS (if needed)
	
    {MODIFIER, XK_Return, term_cmd,  ACTION_NONE,              0},
    {MODIFIER, XK_o,      firefox_cmd, ACTION_NONE,            0},
    {MODIFIER, XK_g,      files_cmd, ACTION_NONE,            0},
    {MODIFIER, XK_d,      rofi_cmd,  ACTION_NONE,              0},

    {MODIFIER,            XK_q,      NULL, ACTION_KILL,              0},
    {MODIFIER|ShiftMask,  XK_m,      NULL, ACTION_MOVE_MONITOR,      0},
    {MODIFIER,            XK_j,      NULL, ACTION_FOCUS_NEXT,        0},
    {MODIFIER,            XK_k,      NULL, ACTION_FOCUS_PREV,        0},
    {MODIFIER,            XK_f,      NULL, ACTION_TOGGLE_FULLSCREEN, 0},
    {MODIFIER|ShiftMask,  XK_j,      NULL, ACTION_SWAP_NEXT,         0},
    {MODIFIER|ShiftMask,  XK_k,      NULL, ACTION_SWAP_PREV,         0},
    {MODIFIER|ShiftMask,  XK_space,  NULL, ACTION_TOGGLE_FLOATING,   0},

    // Workspacebinds from macro
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




static const int num_keys = sizeof(keys) / sizeof(keys[0]);


#endif
