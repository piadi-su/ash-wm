#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

#include <X11/extensions/Xinerama.h>

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

//miei file
#include "config.h"


//miei define

#define MAX_MONITORS 4
#define WORKSPACES 10




// strcut per definere monitors,
typedef struct {
	int id;
	int x,y;
	int width,height;
	int current_ws;
} Monitors ;

static Monitors monitors[MAX_MONITORS];
static int monitors_count = {0};


//strcut per client/finestre
typedef struct Client {
	Window id;
	int monitor_id;
	struct Client *next;
	struct Client *prev;

}Client;



//struct per workspace
typedef struct{
	int id;
	int monitor_id;
	Client *list_Cl;
}Workspace ;


static Workspace workspaces[WORKSPACES];


/*
 * add a new window to the list 
 * from the Ev Ev.xmaprequest.window
 * */
void 
AddWindowList(Display *disp, Window w, Window root)
{
	//creo nuovo client e tolgo il garbage
	Client *new_window = calloc(1, sizeof(Client));
	if (new_window == NULL)
		return;
	
	//aggiungimo la window nel id della strct 
	new_window->id = w;
	new_window->monitor_id = 0;
	
	
	//le usiamo per capire dove sta il mouse sullo schemro
	Window dummy_win;
    int dummy_int;
    unsigned int dummy_mask;
    int mouse_x, mouse_y;
	int target_monitor = 0; //def monitor = 0

	//chiediamo a x11 le cordinate del cursore
	XQueryPointer(disp, root, &dummy_win, &dummy_win, &mouse_x, &mouse_y, &dummy_int, &dummy_int, &dummy_mask);


	//trova la postizione del mouse e ritorna il numero del monitor dov'e sta tipo 0 1 2 3
	for (int i = 0; i < monitors_count; i++) {
        if (mouse_x >= monitors[i].x && mouse_x < (monitors[i].x + monitors[i].width) &&
            mouse_y >= monitors[i].y && mouse_y < (monitors[i].y + monitors[i].height)) {
            target_monitor = i;
            break;
        }
    }
	
	//controlla quale worksapce é aperto ora su quel monitor
	int active_ws = monitors[target_monitor].current_ws;


	printf("[ASH-WM] Finestra %lu spawnata sul Monitor %d -> Salvata nel Workspace %d\n", 
           w, target_monitor, active_ws);

	
	//-------------------------------------------//

	/*
	 *  / c \
	 *  b    d
	 *  \ a /
     *
	 * */
		
	//-------------------------------------------//
	

	//se non sta un cazzo prendiamo la finestra e la mettiamo nella lista e come prev é next a sole lei
	if (workspaces[active_ws].list_Cl == NULL) {
		workspaces[active_ws].list_Cl = new_window;
		workspaces[active_ws].list_Cl->next = workspaces[active_ws].list_Cl;
		workspaces[active_ws].list_Cl->prev = workspaces[active_ws].list_Cl;

	}

	// se ce ne stanno altre di finestre le concateniamo a cerchio
	else {
		Client *head = workspaces[active_ws].list_Cl;
		new_window->next = head;
		new_window->prev = head->prev;
		head->prev->next = new_window;
		head->prev = new_window;
	}


}

//cambia il worksapce che vediamo
void
ChangeWorksapce(Display *disp, Window root, int new_ws)
{
	Window dummy_win;
    int dummy_int;
    unsigned int dummy_mask;
    int mouse_x, mouse_y;
    int mon_idx = 0;


	//chiediamo a x11 le cordinate del cursore
	XQueryPointer(disp, root, &dummy_win, &dummy_win, &mouse_x, &mouse_y, &dummy_int, &dummy_int, &dummy_mask);

	for (int i = 0; i < monitors_count; i++) {
        if (mouse_x >= monitors[i].x && mouse_x < (monitors[i].x + monitors[i].width) &&
            mouse_y >= monitors[i].y && mouse_y < (monitors[i].y + monitors[i].height)) {
            mon_idx = i;
            break;
        }
    }

	int old_ws = monitors[mon_idx].current_ws;

	// esce se volgio cambiare worksapce ma ci sono gia dentro
	if(old_ws == monitors[mon_idx].current_ws)
		return;
		
	printf("[ASH-WM] Monitor %d: cambio da WS %d a WS %d\n", mon_idx, old_ws, new_ws);

	Client *cursor = workspaces[old_ws].list_Cl;
	if(cursor != NULL)
	{
		do{
			
			XUnmapWindow(disp, cursor->id); //toglie le window dallo schermo
			cursor = cursor->next; // cambio il puntaore del cursore

		}while(cursor != workspaces[old_ws].list_Cl);
	}

	workspaces[old_ws].monitor_id = -1; //nascondiamo il vecchio ws togliando ache l'id che il monitor usava prima
	
	//aggiorno il nuovo worksapce
	
	monitors[mon_idx].current_ws = new_ws; //metto nel monitor l'index del workspace
	workspaces[new_ws].monitor_id = mon_idx; //metto nel current workapce l'id del monitor che usa

	cursor = workspaces[new_ws].list_Cl;
	if(cursor != NULL)
	{
		do{
			
			XMapWindow(disp, cursor->id); //mostra le window allo schermo
			cursor = cursor->next; // cambio il puntaore del cursore

		}while(cursor != workspaces[new_ws].list_Cl);
	}

	//svuoto il buffer che uso per alloracare spostamenti
	XSync(disp, False);
}	



int main(void)
{
	
	XEvent Ev;

	Display *disp = XOpenDisplay(NULL);
	if(disp == NULL){
		fprintf(stderr, "error opening the display");
		return 1;
	}
	
	int sc = DefaultScreen(disp);

	//create root window/desktop
	Window root = RootWindow(disp, sc);


	//serve ad un programma a registrarsi in coda 
	XSelectInput(disp, root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask);

	// svuota il buffer dove stanno le finestre in coda perche x11 e asincrono 
	XSync(disp, False);

	
	//for per leggere tasti da config.h
	for(int i = 0 ; i < num_keys; i++)
	{
		// immagino prenda il codice di ogni keybinds nel array di struct KeYBinds
		KeyCode code = XKeysymToKeycode(disp, keys[i].keysym);
		XGrabKey(disp, code, keys[i].mod, root, True, GrabModeAsync, GrabModeAsync); // prende i tasti/sequesta dalla root win
	}
	
	
	//get monitors
	if(XineramaIsActive(disp))
	{
		int n;
		XineramaScreenInfo *info = XineramaQueryScreens(disp, &n);

		monitors_count = n < MAX_MONITORS ? n : MAX_MONITORS;

		
		for(int i = 0; i < monitors_count; i++)
		{
			monitors[i].id = info[i].screen_number;
			monitors[i].x = info[i].x_org;
			monitors[i].y = info[i].y_org;
			monitors[i].width = info[i].width;
			monitors[i].height = info[i].height;


			printf("[ASH-WM] Salvato Monitor %d: X=%d, Y=%d, W=%d, H=%d\n", 
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
	

	
	//inizialize all 10 worksapce
	for(int i = 0 ; i < WORKSPACES; i++)
	{
		workspaces[i].id = 0;
		workspaces[i].list_Cl = NULL;
		workspaces[i].monitor_id = -1;
	}
	for(int i = 0 ; i < monitors_count; i++)
	{
		monitors[i].current_ws = i;
		workspaces[i].monitor_id = i;
	}

	

	while(1)
	{

		XNextEvent(disp, &Ev);
		
		switch (Ev.type) {

			case MapRequest:	 
				AddWindowList(disp, Ev.xmaprequest.window, root );

				printf("[+] nuova finestra id: %lu\n", Ev.xmaprequest.window);

				                    // questo é Window w
				XMapWindow(disp, Ev.xmaprequest.window); //mappa/ fa vedere la finestra intercettata 
				break;

				
			
			case KeyPress:
				for(int i = 0 ; i < num_keys; i++)
				{
					if(	(Ev.xkey.keycode == XKeysymToKeycode(disp, keys[i].keysym)) 
							&& (Ev.xkey.state & keys[i].mod))
					{
						if(fork() == 0)
						{
							execvp(keys[i].cmd[0], keys[i].cmd);
							exit(0);
						}
					}

					else
					{
						int ws_target = keys[i].arg;

						if(Ev.xkey.state & ShiftMask)
						{
							printf("[ASH-WM] Richiesto spostamento finestra nel WS %d (da fare)\n", ws_target);
							//temporaneo
						}
						else
						{
							ChangeWorksapce(disp, root, ws_target);
						}

					}
				}

				break;


			case DestroyNotify:	 // questo é Window w
				break;


			default:
				break;
		
		}

	}


	XCloseDisplay(disp);
	return 0;
}
