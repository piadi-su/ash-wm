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



// strcut per definere monitors,
typedef struct {
	int id;
	int x,y;
	int width,height;
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

static Client *list_Cl = NULL;





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

	//chiediamo a x11 le cordinate del cursore
	XQueryPointer(disp, root, &dummy_win, &dummy_win, &mouse_x, &mouse_y, &dummy_int, &dummy_int, &dummy_mask);


	for (int i = 0; i < monitors_count; i++) {
		if (mouse_x >= monitors[i].x && mouse_x < (monitors[i].x + monitors[i].width) &&
				mouse_y >= monitors[i].y && mouse_y < (monitors[i].y + monitors[i].height)) {
			new_window->monitor_id = i; // Trovato!
			break;
		}
	}

	printf("[ASH-WM] Finestra %lu assegnata al Monitor %d\n", w, new_window->monitor_id);

		
	//se la lista é nulla la inizializzo con il nuovo client
	if(list_Cl == NULL)
	{
		/*
		 prendo il pointer nullo lista_Cl
		 e lo faccio puntare a new window 
		 che contiene l'id della finestra
		  */

		list_Cl = new_window;
		list_Cl->next = list_Cl;
		list_Cl->prev = list_Cl;
	}

	else
	{
		new_window->next = list_Cl; // l'ultima finesta ha come next la prima
		new_window->prev = list_Cl->prev; // l'ultima  ha come prev la penutlima
		list_Cl->prev->next = new_window; // dicimao alla pen ultima che era il suo next é la ultima creata
		list_Cl->prev = new_window; // diciamo alla prima che come precendeta ha l'ultima
	}


	/*
	 *  / c \
	 *  b    d
	 *  \ a /
     *
	 * */
		


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
