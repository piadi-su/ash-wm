#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

#include <X11/extensions/Xinerama.h>

#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

//miei file
#include "config.h"
#include  "func.h"

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




//strcut per client/finestre
typedef struct Client {
	Window id;
	int monitor_id;

    int is_floating;   // 1 se la muovi col mouse, 0 se segue il tiling
    int is_fullscreen; // 1 se è a tutto schermo


    int x, y; 
	unsigned int w, h; 
	

	// Geometria di backup (Fullscreen / Floating)
    // Per salvare le dimensioni originali quando la stacchi in floating o fullscreen
    int old_x, old_y; 
    unsigned int old_w, old_h;

	struct Client *next;
	struct Client *prev;

}Client;



//struct per workspace
typedef struct{
	int id;
	int monitor_id;
	Client *list_Cl;
}Workspace ;

static Monitors monitors[MAX_MONITORS];
static int monitors_count = {0};
static Workspace workspaces[WORKSPACES];



//-----------------------------//



//func to change worksapce focus
void 
FocusWindow(Display *disp, Window w) {
    if (w != None) {
        XSetInputFocus(disp, w, RevertToParent, CurrentTime);
        // Opzionale: porta la finestra in cima visivamente
        XRaiseWindow(disp, w); 
    }
}

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
	
	XSelectInput(disp, w, EnterWindowMask | PropertyChangeMask);

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


	Dwindle(disp, active_ws);
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

	int ws = monitors[mon_idx].current_ws;

	// esce se volgio cambiare worksapce ma ci sono gia dentro
	if(ws == new_ws)
		return;
		
	printf("[ASH-WM] Monitor %d: cambio da WS %d a WS %d\n", mon_idx, ws, new_ws);

	Client *cursor = workspaces[ws].list_Cl;
	if(cursor != NULL)
	{
		do{
			
			XUnmapWindow(disp, cursor->id); //toglie le window dallo schermo
			cursor = cursor->next; // cambio il puntaore del cursore

		}while(cursor != workspaces[ws].list_Cl);
	}

	workspaces[ws].monitor_id = -1; //nascondiamo il vecchio ws togliando ache l'id che il monitor usava prima
	
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


	if(workspaces[new_ws].list_Cl != NULL)
    {
        FocusWindow(disp, workspaces[new_ws].list_Cl->id);
    }

	//svuoto il buffer che uso per alloracare spostamenti
	XSync(disp, False);
}


//func per muove una window ad un workspace
void 
MoveToWorkspace(Display *disp, Window root, int ws_target)
{
	
	Window focused_win;
	int revert_to;

	XGetInputFocus(disp, &focused_win, &revert_to);

	if(focused_win == None || focused_win == root)
		return;
	
	int source_ws = -1;
	Client *cursor = NULL;
	Client *found = NULL;


	for(int i = 0; i < WORKSPACES; i++)
	{
		cursor = workspaces[i].list_Cl;
		
		if(cursor != NULL)
		{
			do{
				if(cursor->id == focused_win)
				{
					source_ws = i;
					found = cursor;
					break;
				}
				
				cursor = cursor->next; // come se fosse un i++
			}while(cursor != workspaces[i].list_Cl);

		}

		if(source_ws != -1)
			break;

	}

	if(source_ws == -1 || source_ws == ws_target)
		return;

	printf("[ASH-WM] Sposto la finestra %lu dal WS %d al WS %d\n", focused_win, source_ws, ws_target);

	if(found->next == found)
	{
		workspaces[source_ws].list_Cl = NULL;// era l'unica del ws
	}

	else
	{
		found->prev->next = found->next;
		found->next->prev = found->prev;

		if(workspaces[source_ws].list_Cl == found)
		{
			workspaces[source_ws].list_Cl = found->next;
		}

		FocusWindow(disp, workspaces[source_ws].list_Cl->id);
	}

	if(workspaces[ws_target].monitor_id == -1)
	{
		XUnmapWindow(disp, focused_win);
	}

	if(workspaces[ws_target].list_Cl == NULL)
	{
		workspaces[ws_target].list_Cl = found;
		found->next = found;
		found->prev = found;

	}
	else
	{
		Client *head = workspaces[ws_target].list_Cl;
		Client *tail = head->prev;
		
		found->next = head;
		found->prev = head;
		tail->next = found;
		head->prev = found;

	}

	if(workspaces[ws_target].monitor_id != -1) {
        FocusWindow(disp, found->id);
    } else {
        XSetInputFocus(disp, root, RevertToParent, CurrentTime);
    }

	XSync(disp, False);

}

// server praticamente per freeare la memory quando una finestra o crusha o code simili
void 
RemoveWindowList(Display *disp, Window w)
{
	Client *found = NULL;
	int ws_index = -1;
	
	//cerca in quale worksapce sta la finestra
	for(int i = 0; i < WORKSPACES; i++)
	{
		Client *cursor = workspaces[i].list_Cl;

		if(cursor != NULL){

			do{
				if(cursor->id == w)
				{
					found = cursor;
					ws_index = i;
				}

				cursor = cursor->next;
			}while(cursor != workspaces[i].list_Cl);

		}

		if(found != NULL)
			break;


	}

	//se non sta la finestra esci
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
	}

	printf("[-] Finestra %lu rimossa (WS %d)\n", w, ws_index);
	free(found);

	// Ricalcola il layout del workspace dopo la rimozione!
	Dwindle(disp, ws_index);

}

// kill di window alla mod + q
void
KillWindow(Display  *disp, Window root)
{

	Window focused_win;
	int revert_to;

	XGetInputFocus(disp, &focused_win, &revert_to);

	if (focused_win == None || focused_win == root)
        return;

    printf("[ASH-WM] Uccido la finestra hardware: %lu\n", focused_win);
    
    // Ordiniamo a X11 di distruggere la finestra
    XDestroyWindow(disp, focused_win);
}


//func per layout dwindle autotyling
void
Dwindle(Display *disp, int ws_index)
{
    Client *head = workspaces[ws_index].list_Cl;

    if(head == NULL)
        return;

    int count_ws = 0;
    Client *cursor = head;
    do {
        count_ws++;
        cursor = cursor->next;
    } while(cursor != head);

    int mod_index = workspaces[ws_index].monitor_id;
    if(mod_index == -1)
        return;

    int mx = monitors[mod_index].x;
    int my = monitors[mod_index].y;
    int mw = monitors[mod_index].width;
    int mh = monitors[mod_index].height;

    if(count_ws == 1)
    {
        head->x = mx + GAPS;
        head->y = my + GAPS;
        head->w = mw - (GAPS * 2);
        head->h = mh - (GAPS * 2);

        XMoveResizeWindow(disp, head->id, head->x, head->y, head->w, head->h);
        return;
    }

    cursor = head;
    int wx = mx;
    int wy = my;
    int ww = mw;
    int wh = mh;

    for(int i = 0; i < count_ws; i++)
    {
        if(i == count_ws - 1)
        {
            cursor->x = wx + GAPS;
            cursor->y = wy + GAPS;
            cursor->w = ww - (GAPS * 2);
            cursor->h = wh - (GAPS * 2);
        }
        else
        {
            if(i % 2 == 0)
            {
                ww /= 2;
                cursor->x = wx + GAPS;
                cursor->y = wy + GAPS;
                cursor->w = ww - (GAPS * 1.5);
                cursor->h = wh - (GAPS * 2);
                wx += ww;
            }
            else
            {
                wh /= 2;
                cursor->x = wx + GAPS;
                cursor->y = wy + GAPS;
                cursor->w = ww - (GAPS * 2);
                cursor->h = wh - (GAPS * 1.5);
                wy += wh;
            }
        }

        // CORRETTO: Applica le geometrie calcolate a 'cursor->id', non a 'head->id'
        XMoveResizeWindow(disp, cursor->id, cursor->x, cursor->y, cursor->w, cursor->h);
        cursor = cursor->next;
    }
}










int main(void)
{
	
	XEvent Ev;
	unsigned int clean_state;

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
				FocusWindow(disp, Ev.xmaprequest.window); // focussa la window
				break;

				
			
			case KeyPress:
				// Puliamo lo stato da NumLock (Mod2Mask) o CapsLock (LockMask) se attivi
				clean_state = Ev.xkey.state & ~(LockMask | Mod2Mask);

				for(int i = 0 ; i < num_keys; i++)
				{
					// Controlliamo prima se il tasto premuto corrisponde ESATTAMENTE a quello registrato
					if((Ev.xkey.keycode == XKeysymToKeycode(disp, keys[i].keysym)) 
							&& (clean_state == keys[i].mod))
					{
						// Se corrisponde, separiamo i due comportamenti possibili:
						if(keys[i].cmd != NULL)
						{
							// Comando di sistema (Term, Rofi, ecc.)
							if(fork() == 0)
							{
								execvp(keys[i].cmd[0], keys[i].cmd);
								exit(0);
							}
						}
						else
						{	
							
							if(keys[i].arg == -1)
							{
								KillWindow(disp, root);
							}

							else{
								// Gestione Workspace (cmd è NULL)
								int ws_target = keys[i].arg;

								if(keys[i].mod & WS_MODIFIER)
								{
									printf("[ASH-WM] Richiesto spostamento finestra nel WS %d\n", ws_target);
									MoveToWorkspace(disp, root, ws_target);
								}
								else
								{
									ChangeWorksapce(disp, root, ws_target);
								}
							}
						}
						break; // Abbiamo trovato la chiave, interrompiamo il ciclo for
					}
				}
				break;


			
			//gestisce sopostamento mouse
			case EnterNotify:
				// Evitiamo di cambiare focus se stiamo entrando in sotto-finestre di X11 strane
				if (Ev.xcrossing.mode != NotifyNormal || Ev.xcrossing.detail == NotifyInferior)
					break;

				printf("[ASH-WM] Il mouse è entrato nella finestra %lu. Cambio focus.\n", Ev.xcrossing.window);
				FocusWindow(disp, Ev.xcrossing.window);
				break;


			case DestroyNotify:	 // questo é Window w
				RemoveWindowList(disp ,Ev.xdestroywindow.window);
				break;

		
			default:
				break;
		
		}

	}


	XCloseDisplay(disp);
	return 0;
}
