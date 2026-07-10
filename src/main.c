#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

#include <X11/extensions/Xinerama.h>

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>


//miei file
#include "config.h"
#include  "func.h"

//miei define
#define WORKSPACES_X_MONITOR 10
#define WORKSPACES (WORKSPACES_X_MONITOR * N_MONITORS)



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

static Monitors monitors[N_MONITORS];
static int monitors_count = {0};
static Workspace workspaces[WORKSPACES];



//-----------------------------//



//func to change worksapce focus
// Funzione corretta per gestire il focus e aggiornare i bordi
void FocusWindow(Display *disp, Window w) {
    if (w == None) return;

    // 1. Assegna il focus reale di X11
    XSetInputFocus(disp, w, RevertToParent, CurrentTime);
    XRaiseWindow(disp, w);

    // 2. Trova in quale workspace si trova la finestra per aggiornare i bordi di quel WS
    int target_ws = -1;
    Client *found = NULL;

    for (int i = 0; i < WORKSPACES; i++) {
        Client *cursor = workspaces[i].list_Cl;
        if (cursor != NULL) {
            do {
                if (cursor->id == w) {
                    target_ws = i;
                    found = cursor;
                    break;
                }
                cursor = cursor->next;
            } while (cursor != workspaces[i].list_Cl);
        }
        if (target_ws != -1) break;
    }

    // Se non abbiamo trovato la finestra nei nostri workspace, usciamo
    if (target_ws == -1 || found == NULL) return;

    // 3. Aggiorna visivamente i bordi di tutte le finestre SOLO nel workspace della finestra focalizzata
    Client *curr = workspaces[target_ws].list_Cl;
    if (curr == NULL) return;

    do {
        // Applica lo spessore del bordo prima di colorarlo
        XSetWindowBorderWidth(disp, curr->id, BORDER_WIDTH);
        
        if (curr->id == w) {
            // Colore Focus
            XSetWindowBorder(disp, curr->id, GetXColor(disp, COLOR_FOCUS));
        } else {
            // Colore Unfocus
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
ChangeWorkspace(Display *disp, Window root, int target_local_id) // riceve l'indice del tasto (0-9)
{
    Window dummy_win;
    int dummy_int;
    unsigned int dummy_mask;
    int mouse_x, mouse_y;
    int mon_idx = 0;

    // Trova il monitor in cui si trova il mouse
    XQueryPointer(disp, root, &dummy_win, &dummy_win, &mouse_x, &mouse_y, &dummy_int, &dummy_int, &dummy_mask);

    for (int i = 0; i < monitors_count; i++) {
        if (mouse_x >= monitors[i].x && mouse_x < (monitors[i].x + monitors[i].width) &&
            mouse_y >= monitors[i].y && mouse_y < (monitors[i].y + monitors[i].height)) {
            mon_idx = i;
            break;
        }
    }

    int old_ws = monitors[mon_idx].current_ws;
    
    // Calcola il workspace REALE del monitor corrente (0-9 per mon 0, 10-19 per mon 1)
    int new_ws = (mon_idx * WORKSPACES_X_MONITOR) + (target_local_id % WORKSPACES_X_MONITOR);

    if(old_ws == new_ws)
        return;

    printf("[ASH-WM] Monitor %d cambia da WS %d a WS %d\n", mon_idx, old_ws, new_ws);

    // Nascondi le finestre del vecchio workspace
    Client *cursor = workspaces[old_ws].list_Cl;
    if(cursor != NULL) {
        do {
            XUnmapWindow(disp, cursor->id);
            cursor = cursor->next;
        } while(cursor != workspaces[old_ws].list_Cl);
    }
    workspaces[old_ws].monitor_id = -1; 

    // Attiva il nuovo workspace sul monitor corrente
    monitors[mon_idx].current_ws = new_ws;
    workspaces[new_ws].monitor_id = mon_idx;

    // Mostra le finestre del nuovo workspace
    cursor = workspaces[new_ws].list_Cl;
    if(cursor != NULL) {
        do {
            XMapWindow(disp, cursor->id);
            cursor = cursor->next;
        } while(cursor != workspaces[new_ws].list_Cl);
        
        Dwindle(disp, new_ws);
        FocusWindow(disp, workspaces[new_ws].list_Cl->id);
    } else {
        XSetInputFocus(disp, root, RevertToParent, CurrentTime);
    }

    XSync(disp, False);
}

//func per muove una window ad un workspace (anche cross-monitor)
void 
MoveToWorkspace(Display *disp, Window root, int target_local_id) // target_local_id riceve il tasto premuto (es. 0-9)
{
    Window focused_win;
    int revert_to;

    XGetInputFocus(disp, &focused_win, &revert_to);

    if(focused_win == None || focused_win == root)
        return;
    
    int source_ws = -1;
    Client *cursor = NULL;
    Client *found = NULL;

    // 1. Trova in quale workspace si trova attualmente la finestra
    for(int i = 0; i < WORKSPACES; i++)
    {
        cursor = workspaces[i].list_Cl;
        if(cursor != NULL)
        {
            do {
                if(cursor->id == focused_win)
                {
                    source_ws = i;
                    found = cursor;
                    break;
                }
                cursor = cursor->next;
            } while(cursor != workspaces[i].list_Cl);
        }
        if(source_ws != -1)
            break;
    }

    if(source_ws == -1)
        return;

    // 2. Trova il monitor ATTUALE in cui si trova la finestra basandoti sul source_ws
    int current_monitor_of_window = source_ws / WORKSPACES_X_MONITOR;

    // 3. Calcola il workspace di destinazione REALE
    // Vogliamo mandarla sul monitor OPPOSTO? Oppure su quello indicato dal mouse?
    // Per fare il salto diretto 0-9 <-> 10-19, decidiamo il monitor di destinazione:
    int target_monitor = current_monitor_of_window;
    
    // Se hai più di un monitor, mandala sull'altro!
    if (monitors_count > 1) {
        // Se era sul Monitor 0 va al Monitor 1, se era sul Monitor 1 va al Monitor 0
        target_monitor = (current_monitor_of_window == 0) ? 1 : 0;
    }

    // Calcolo del workspace assoluto finale (es: Monitor 1 * 10 + 3 = 13)
    int ws_target = (target_monitor * WORKSPACES_X_MONITOR) + (target_local_id % WORKSPACES_X_MONITOR);

    if(source_ws == ws_target)
        return;

    printf("[ASH-WM] Sposto la finestra %lu dal WS %d al WS %d (Monitor %d)\n", focused_win, source_ws, ws_target, target_monitor);

    // 4. Rimuovi dal vecchio workspace
    if(found->next == found)
    {
        workspaces[source_ws].list_Cl = NULL; // Era l'unica
    }
    else
    {
        found->prev->next = found->next;
        found->next->prev = found->prev;

        if(workspaces[source_ws].list_Cl == found)
        {
            workspaces[source_ws].list_Cl = found->next;
        }
    }

    // 5. Gestione visibilità: se il workspace di destinazione NON è visibile, nascondi la finestra
    if(workspaces[ws_target].monitor_id == -1)
    {
        XUnmapWindow(disp, focused_win);
    }
    else
    {
        // Altrimenti se l'altro monitor sta mostrando quel workspace, rendila visibile immediatamente
        XMapWindow(disp, focused_win);
    }

    // 6. Inserisci nel nuovo workspace
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
        found->prev = tail;
        tail->next = found;
        head->prev = found;
    }

    // 7. Gestione Focus protetta
    if(workspaces[ws_target].monitor_id != -1) {
        FocusWindow(disp, found->id);
    } else {
        if (workspaces[source_ws].list_Cl != NULL) {
            FocusWindow(disp, workspaces[source_ws].list_Cl->id);
        } else {
            XSetInputFocus(disp, root, RevertToParent, CurrentTime);
        }
    }
    
    // 8. Ricalcola i layout geometrici per entrambi i workspace
    if(workspaces[ws_target].monitor_id != -1) {
        Dwindle(disp, ws_target);
    }
    Dwindle(disp, source_ws);

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
					break;
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

		if (workspaces[ws_index].monitor_id != -1) {
            FocusWindow(disp, workspaces[ws_index].list_Cl->id);
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

    // Dimensione minima assoluta per non far crashare X11
    const unsigned int MIN_WIDTH = 60;
    const unsigned int MIN_HEIGHT = 60;

    // Se c'è una sola finestra, prende tutto il monitor (meno i gap)
    if(count_ws == 1)
    {
        int target_w = mw - (GAPS * 2);
        int target_h = mh - (GAPS * 2);

        head->x = mx + GAPS;
        head->y = my + GAPS;
        head->w = target_w < (int)MIN_WIDTH ? MIN_WIDTH : (unsigned int)target_w;
        head->h = target_h < (int)MIN_HEIGHT ? MIN_HEIGHT : (unsigned int)target_h;

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
            // Ultima finestra: si prende tutto lo spazio rimasto
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
            if(i % 2 == 0)
            {
                ww /= 2;
                // Previene il collasso dello spazio disponibile al giro successivo
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
                // Previene il collasso dello spazio disponibile al giro successivo
                if (wh < (int)MIN_HEIGHT) wh = MIN_HEIGHT;

                int target_h = wh - (GAPS * 2);
                cursor->x = wx + GAPS;
                cursor->y = wy + GAPS;
                cursor->w = (ww - (GAPS * 2)) < (int)MIN_WIDTH ? MIN_WIDTH : (unsigned int)(ww - (GAPS * 2));
                cursor->h = target_h < (int)MIN_HEIGHT ? MIN_HEIGHT : (unsigned int)target_h;
                wy += wh;
            }
        }

        // Controllo finale di allineamento sui bordi fisici del monitor
        if (cursor->x + (int)cursor->w > mx + mw) cursor->x = (mx + mw) - (int)cursor->w;
        if (cursor->y + (int)cursor->h > my + mh) cursor->y = (my + mh) - (int)cursor->h;

        // Sicurezza estrema: costringiamo le coordinate a non andare oltre l'origine del monitor
        if (cursor->x < mx) cursor->x = mx;
        if (cursor->y < my) cursor->y = my;

        XMoveResizeWindow(disp, cursor->id, cursor->x, cursor->y, cursor->w, cursor->h);
        cursor = cursor->next;
    }
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
	
	//cerco mouse nei monitors
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
		printf("[ASH-WM] Sincronizzo: Il mouse ha attivato il Monitor %d (WS %d)\n", target_monitor, active_ws);
		workspaces[active_ws].monitor_id = target_monitor;
	}
}



// Sposta la finestra focalizzata sul monitor successivo/opposto
void 
MoveWindowToMonitor(Display *disp, Window root)
{
    if (monitors_count <= 1)
        return;

    Window focused_win;
    int revert_to;
    XGetInputFocus(disp, &focused_win, &revert_to);

    if (focused_win == None || focused_win == root)
        return;

    int source_ws = -1;
    Client *found = NULL;

    // 1. Trova in quale workspace si trova attualmente la finestra focalizzata
    for (int i = 0; i < WORKSPACES; i++) {
        Client *cursor = workspaces[i].list_Cl;
        if (cursor != NULL) {
            do {
                if (cursor->id == focused_win) {
                    source_ws = i;
                    found = cursor;
                    break;
                }
                cursor = cursor->next;
            } while (cursor != workspaces[i].list_Cl);
        }
        if (source_ws != -1)
            break;
    }

    if (source_ws == -1 || found == NULL)
        return;

    // 2. Determina il monitor di origine e destinazione
    int current_monitor = source_ws / WORKSPACES_X_MONITOR;
    int target_monitor = (current_monitor == 0) ? 1 : 0; // Alterna tra 0 e 1 per dual-monitor
    int ws_target = monitors[target_monitor].current_ws;

    printf("[ASH-WM] Sposto la finestra %lu dal Monitor %d (WS %d) al Monitor %d (WS %d)\n", 
           focused_win, current_monitor, source_ws, target_monitor, ws_target);

    // 3. Rimuovi la finestra dal vecchio workspace (gestione lista circolare)
    if (found->next == found) {
        workspaces[source_ws].list_Cl = NULL; 
    } else {
        found->prev->next = found->next;
        found->next->prev = found->prev;
        if (workspaces[source_ws].list_Cl == found) {
            workspaces[source_ws].list_Cl = found->next;
        }
    }

    // 4. Inserisci la finestra nella lista del nuovo workspace
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

    // 5. Ricalcola i layout geometrici (Dwindle aggiornerà x, y, w, h sul nuovo monitor)
    Dwindle(disp, ws_target);
    Dwindle(disp, source_ws);

    // 6. Muovi il mouse al centro della finestra spostata per stabilizzare il focus-follows-mouse
    XWarpPointer(disp, None, found->id, 0, 0, 0, 0, found->w / 2, found->h / 2);
    FocusWindow(disp, found->id);

    // 7. Se il vecchio monitor è rimasto vuoto, pulisci il focus sulla root
    if (workspaces[source_ws].list_Cl == NULL) {
        XSetInputFocus(disp, root, RevertToParent, CurrentTime);
    }

    XSync(disp, False);
}

unsigned long GetXColor(Display *disp, unsigned long hex_color)
{
	XColor col;
	col.red   = ((hex_color >> 16) & 0xFF) * 257;
    col.green = ((hex_color >> 8)  & 0xFF) * 257;
    col.blue  = (hex_color         & 0xFF) * 257;
    col.flags = DoRed | DoGreen | DoBlue;
    XAllocColor(disp, DefaultColormap(disp, DefaultScreen(disp)), &col);
    return col.pixel;
}


void CycleFocus(Display *disp, int direction) {
    Window dummy_win;
    int dummy_int;
    unsigned int dummy_mask;
    int mouse_x, mouse_y;
    int mon_idx = 0;

    // FIX: Troviamo il monitor attivo tramite puntatore per ottenere il Workspace corrente
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
        curr = cursor->next; // Nota: avevi usato curr in precedenza, uniformato qui
    } while (curr != head);

    if (target == NULL) {
        FocusWindow(disp, head->id);
        return;
    }

    Client *next_node = (direction == 1) ? target->next : target->prev;

    if (next_node != NULL) {
        FocusWindow(disp, next_node->id);
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
	XSelectInput(disp, root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask | PointerMotionMask);

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

		monitors_count = n < N_MONITORS ? n : N_MONITORS;

		
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

	
	signal(SIGCHLD, SIG_IGN);
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

							else if (keys[i].arg == -2) { // Immaginando -2 come identificativo per spostare monitor
								MoveWindowToMonitor(disp, root);
							}

							else {
								// Prende il tasto premuto (0-9)
								int ws_target = keys[i].arg % WORKSPACES_X_MONITOR; 

								if(keys[i].mod & WS_MODIFIER)
								{
									// Questa farà saltare la finestra nell'altro range (es. da 3 a 13)
									MoveToWorkspace(disp, root, ws_target);
								}
								else
								{
									// Questa cambierà workspace restando nel range del monitor corrente
									ChangeWorkspace(disp, root, ws_target);
								}
							}
						}
						break; // Abbiamo trovato la chiave, interrompiamo il ciclo for
					}
				}
				break;

			
			case MotionNotify:
				// Il mouse si sta muovendo sullo sfondo o tra i monitor: aggiorna la mappa dei monitor
				UpdateCurrentMonitor(disp, root);
				break;
			
			//gestisce sopostamento mouse
			case EnterNotify:
				if (Ev.xcrossing.mode != NotifyNormal || Ev.xcrossing.detail == NotifyInferior)
					break;

				UpdateCurrentMonitor(disp, root);

				// FIX DI SICUREZZA: Controlla se la finestra è effettivamente visibile a schermo prima di darle il focus
				XWindowAttributes wa;
				XGetWindowAttributes(disp, Ev.xcrossing.window, &wa);
				if (wa.map_state == IsViewable) {
					printf("[ASH-WM] Il mouse è entrato nella finestra %lu. Cambio focus.\n", Ev.xcrossing.window);
					FocusWindow(disp, Ev.xcrossing.window);
				}
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
