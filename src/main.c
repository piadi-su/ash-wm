#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

//miei file
#include "config.h"


typedef struct Client {
	Window id;
	struct Client *next;
	struct Client *prev;

}Client;

static Client *list_Cl = NULL;

/*
 * add a new window to the list 
 * from the Ev Ev.xmaprequest.window
 * */
void 
AddWindowList(Window w)
{
	//creo nuovo client e tolgo il garbage
	Client *new_window = calloc(1, sizeof(Client));
	if (new_window == NULL)
		return;
	
	//aggiungimo la window nel id della strct 
	new_window->id = w;

		
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
		list_Cl->next = new_window; // diciamo alla prima che come precendeta ha l'ultima
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
	XSelectInput(disp, root, SubstructureRedirectMask | SubstructureNotifyMask);

	// svuota il buffer dove stanno le finestre in coda perche x11 e asincrono 
	XSync(disp, False);


	for(int i = 0 ; i < num_keys; i++)
	{
		// immagino prenda il codice di ogni keybinds nel array di struct KeYBinds
		KeyCode code = XKeysymToKeycode(disp, keys[i].keysym);
		XGrabKey(disp, code, keys[i].mod, root, True, GrabModeAsync, GrabModeAsync); // prende i tasti/sequesta dalla root win
	}

	while(1)
	{

		XNextEvent(disp, &Ev);
		
		switch (Ev.type) {

			case MapRequest:	 
				AddWindowList(Ev.xmaprequest.window);

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
