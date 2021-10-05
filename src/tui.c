#include "tui.h"

TUI *tui;

TUI *init_tui(CONLIST *cList){
	TUI *t = calloc(1, sizeof(TUI));
	if(t == NULL){
		log_logError("Error allocation TUI", ERROR);
		return NULL;
	}
	tui = t;
	t->cList = cList;

	putenv("NCURSES_NO_UTF8_ACS=1");

	setlocale(LC_ALL, "");
	if(initscr() == NULL)
		return NULL;

	if(has_colors()){
		start_color();
		init_pair(1, COLOR_BLUE, COLOR_BLACK);
	}

	#ifdef DEV
		cbreak();
	#else
		raw();
	#endif

	noecho();
	keypad(stdscr, TRUE); //Enable F1, arrows keys, etc
	box(stdscr, 0, 0);
	curs_set(0);
	refresh();

	// Handle resizing
	signal(SIGWINCH, handleResize);

	setupWindows(t);
	
	return t;
}

void tui_close(){
	endwin();
}

MENU *createMenu(){
	MENU *m = calloc(1, sizeof(MENU));
	if(m == NULL){
		log_logError("Error allocating MENU", ERROR);
		return NULL;
	}

	m->items = arrl_createArrayList(20);
	if(m->items == NULL){
		free(m);
		return NULL;
	}

	// Init mutex
	int ret = pthread_mutex_init(&m->mutex, NULL);
	if(ret < 0){
		log_logError("Error initalizing mutex", ERROR);
		free(m);
		return NULL;
	}

	return m;
}

MENUITEM *createMenuItem(char *text, void *ptr){
	MENUITEM *item = calloc(1, sizeof(MENUITEM));
	if(item == NULL){
		log_logError("Error allocating MENUITEM", ERROR);
		return NULL;
	}

	item->subitems = arrl_createArrayList(5);
	if(item->subitems == NULL){
		free(item);
		return NULL;
	}

	int size = strlen(text)+1;
	item->text = malloc(size);
	if(item->text == NULL){
		log_logError("Error allocating MENUITEM string", ERROR);
		free(item);
		return NULL;
	}

	// Init mutex
	int ret = pthread_mutex_init(&item->mutex, NULL);
	if(ret < 0){
		log_logError("Error initalizing mutex", ERROR);
		free(item);
		free(item->text);
		return NULL;
	}

	item->enableSubitems = DISABLE;
	item->parent = NULL;
	item->ptr = ptr;
	strhcpy(item->text, text, size);

	return item;
}

void freeMenu(MENU *m){
	if(m == NULL)
		return;

	arrl_deleteArrayList(m->items, freeMenuItem);
	free(m);
}

void freeMenuItem(void *iptr){
	if(iptr == NULL)
		return;

	MENUITEM *i = (MENUITEM *) iptr;
	arrl_deleteArrayList(i->subitems, freeMenuItem);

	free(i->text);
	free(i);
}

int addItemToMenu(MENU *m, MENUITEM *i){
	if(i == NULL)
		return -1;

	pthread_mutex_lock(&m->mutex);
	// Make this item selected if it is the first
	if(m->items->length == 0)
		m->selected = i;

	arrl_addItem(m->items, i);
	pthread_mutex_unlock(&m->mutex);

	return 1;
}

int addSubitem(MENUITEM *item, MENUITEM *sub){
	if(item == NULL || sub == NULL)
		return -1;

	pthread_mutex_lock(&item->mutex);
	sub->parent = item;
	arrl_addItem(item->subitems, sub);
	pthread_mutex_unlock(&item->mutex);

	return 1;
}

int drawMenu(SECTION *s){
	MENU *m = s->menu;
	wclear(s->content);
	wmove(s->content, 0, 0);

	pthread_mutex_lock(&m->mutex);
	ARRAYLIST *l = m->items;
	for(int i = 0; i < l->length; i++){
		MENUITEM *data = arrl_getItem(l, i);

		drawMenuItem(data, s);
	}
	pthread_mutex_unlock(&m->mutex);

	wrefresh(s->content);

	return 1;
}

int drawMenuItem(MENUITEM *item, SECTION *s){
	MENU *m = s->menu;

	pthread_mutex_lock(&item->mutex);
	//Draw item
	if(item == m->selected)
		wattron(s->content, A_BOLD);
	waddstr(s->content, item->text);
	wprintw(s->content, "\n");
	wattroff(s->content, A_BOLD);

	// Draw any subitems if applicable
	if(item->enableSubitems == ENABLE){
		ARRAYLIST *l = item->subitems;
		for(int i = 0; i < l->length; i++){
			MENUITEM *data = arrl_getItem(l, i);

			if(i < l->length - 1)
				waddch(s->content, ACS_LTEE);
			else
				waddch(s->content, ACS_LLCORNER);

			drawMenuItem(data, s);
		}
	}
	pthread_mutex_unlock(&item->mutex);

	return 1;
}

MENUITEM *selectMenuItem(MENU *m, int dir){
	if(dir == 1)
		return nextMenuItem(m);
	else if(dir == -1)
		return prevMenuItem(m);

	return NULL;
}

/* Steps in order, until one works
	1. First subitem
	2. Next item in this list
	3. Up levels until there is a next item
	4. Failure
*/
MENUITEM *nextMenuItem(MENU *m){
	MENUITEM *newS;

	pthread_mutex_lock(&m->mutex);
	newS = m->selected;

	pthread_mutex_lock(&newS->mutex);
	// First subitem
	if(newS->enableSubitems == ENABLE && arrl_getItem(newS->subitems, 0) != NULL){
		MENUITEM *item = arrl_getItem(newS->subitems, 0);
		m->selected = item;
		pthread_mutex_unlock(&newS->mutex);
		pthread_mutex_unlock(&m->mutex);
		return item;
	} 
	
	// Next item
	ARRAYLIST *pList = m->items;
	if(newS->parent != NULL)
		pList = newS->parent->subitems;

	int loc = arrl_contains(pList, newS, NULL);
	if(loc < pList->length - 1){
		MENUITEM *item = arrl_getItem(pList, loc + 1);
		m->selected = item;
		pthread_mutex_unlock(&newS->mutex);
		pthread_mutex_unlock(&m->mutex);
		return item;
	}

	// Up levels
	pthread_mutex_unlock(&newS->mutex);
	while(1){
		pList = m->items;
		if(newS->parent != NULL)
			pList = newS->parent->subitems;

		loc = arrl_contains(pList, newS, NULL);
		if(loc < pList->length - 1){
			MENUITEM *item = arrl_getItem(pList, loc + 1);
			m->selected = item;
			pthread_mutex_unlock(&newS->mutex);
			pthread_mutex_unlock(&m->mutex);
			return item;
		}

		// Failed all the way up to the top of list
		if(newS->parent == NULL)
			break;

		newS = newS->parent;
	}

	pthread_mutex_unlock(&m->mutex);

	return NULL;
}

/* Steps in order, until one works
	1. Last subitem of previous item
	2. Previous item
	3. Up a level, unless it is the FIRST item of the MENU
	4. Failure
*/
MENUITEM *prevMenuItem(MENU *m){
	MENUITEM *newS;

	pthread_mutex_lock(&m->mutex);
	newS = m->selected;
	pthread_mutex_lock(&newS->mutex);

	// Setup: get previous item
	MENUITEM *prev = NULL;
	ARRAYLIST *pList = m->items;
	if(newS->parent != NULL)
		pList = newS->parent->subitems;

	int loc = arrl_contains(pList, newS, NULL);
	if(loc > 0) {
		prev = arrl_getItem(pList, loc-1);
	}
	
	pthread_mutex_unlock(&newS->mutex);

	if(prev != NULL) {
		// Find last subitem
		pthread_mutex_lock(&prev->mutex);
		newS = prev;
		while(newS->enableSubitems != DISABLE){
			MENUITEM *temp = arrl_getItem(newS->subitems, newS->subitems->length-1);
			if(temp == NULL) // No subitems
				break;

			newS = temp;
		}
		pthread_mutex_unlock(&prev->mutex);

		m->selected = newS;
		pthread_mutex_unlock(&m->mutex);
		return newS;
	}

	// Parent item if in front
	if(newS->parent != NULL)
		m->selected = newS->parent;	

	pthread_mutex_unlock(&m->mutex);

	return newS->parent;
}

int cmpClusAndMenuItem(void *g, void *i){
	MENUITEM *item = (MENUITEM *) i;
	if(item->ptr == g)
		return 1;

	return -1;
}

int updateSidebar(TUI *t, CONNECTION *c){
	SECTION *s = t->sidebar;
	MENU *m = s->menu;
	ARRAYLIST *cl = c->groups, *ml = m->items;

	pthread_mutex_lock(&c->mutex);

	// Go thru connection to see what is missing	
	for(int i = 0; i < cl->length; i++){
		GROUP *g = arrl_getItem(cl, i);
		MENUITEM *item;

		pthread_mutex_lock(&m->mutex);
		int loc = arrl_contains(ml, g, cmpClusAndMenuItem);
		pthread_mutex_unlock(&m->mutex);
		if(loc == -1){
			item = createMenuItem(g->name, g);
			addItemToMenu(m, item);
		} else {
			item = arrl_getItem(ml, loc);
		}
		item->enableSubitems = ENABLE;

		// Check for missing channels/subitems
		for(int x = 0; x < g->channels->length; x++){
			CHANNEL *c = arrl_getItem(g->channels, x);

			int subloc = arrl_contains(item->subitems, c, cmpClusAndMenuItem);
			if(subloc == -1){
				MENUITEM *subitem = createMenuItem(c->name, c);
				addSubitem(item, subitem);
			}
		}
	}

	// TODO - remove leftovers

	pthread_mutex_unlock(&c->mutex);

	drawMenu(s);

	return 1;
}

int selectSidebarItem(TUI *t){
	SECTION *side = t->sidebar;
	SECTION *chat = t->chat;
	MENU *m = side->menu;

	if(m->selected == NULL)
		return -1;

	pthread_mutex_lock(&m->mutex);

	// Group, just toggle subitems
	if(m->selected->text[0] == '&') // XOR to toggle 1 to 0 and vice versa
		m->selected->enableSubitems ^= ENABLE;
	else // Channel
		strhcpy(chat->title, m->selected->text, ARRAY_SIZE(chat->title));

	pthread_mutex_unlock(&m->mutex);
	
	drawBorders(t);

	return 1;
}

void handleUserInput(TUI *t){
	while(1){
		int c = getch();
		switch(c){
			case KEY_F(1): //Switch active panel
				if(t->active == t->text){
					setActiveSection(t, t->sidebar);
				} else if (t->active == t->sidebar){
					setActiveSection(t, t->chat);
				} else {
					setActiveSection(t, t->text);
				}

				break;
				
			case 'q':
				if(t->active == t->text)
					goto typing;
				exit(1);

			case KEY_ENTER:
			case '\n':
				if(t->active == t->text)
					sendTextBuffer(t);
				else if(t->active == t->sidebar)
					selectSidebarItem(t);	
				break;

			case KEY_DOWN:
				if(t->active == t->sidebar){
					selectMenuItem(t->sidebar->menu, 1);
					drawMenu(t->sidebar);
				}
				break;

			case KEY_UP:
				if(t->active == t->sidebar){
					selectMenuItem(t->sidebar->menu, -1);
					drawMenu(t->sidebar);
				}
				break;

			// Type into the text box
			default:
			typing:
				if(t->active == t->text)
					typeCharacter(t, c);
				break;
		}
	}
}

SECTION *createSection(char *title, int borderChars[]){
	SECTION *section = calloc(1, sizeof(SECTION));
	if(section == NULL){
		log_logError("Error allocating sidebar", ERROR);
		return NULL;
	}
	strhcpy(section->title, title, ARRAY_SIZE(section->title));

	WINDOW *border = newwin(0, 0, 0, 0);
	WINDOW *content = newwin(0, 0, 0, 0);
	section->border = border;
	section->content = content;

	memcpy(section->borderChars, borderChars, sizeof(section->borderChars));

	return section;
}

void drawSidebar(SECTION *sidebar){
	int width = 26 < COLS/3 ? 26 : COLS/3;
	resizeSection(sidebar, 1, 0, LINES-2, width);

	WINDOW *border = sidebar->border;
	wrefresh(border);
}

void drawChatbox(SECTION *chat, TUI *t){
	int startx = getbegx(t->text->border);
	int height = getbegy(t->text->border);
	int width = COLS-startx;

	resizeSection(chat, 1, startx, height, width);
	wmove(chat->content, 0, 0);

	WINDOW *border = chat->border;
	wrefresh(border);
	wrefresh(chat->content);
}

void drawTextbox(SECTION *text, TUI *t, int height){
	int startx = getmaxx(t->sidebar->border)-1;

	resizeSection(text, LINES-(height+1), startx, height, COLS-startx);
	wmove(text->content, 0, 0);

	WINDOW *border = text->border;
	wrefresh(border);
}

int resizeSection(SECTION *s, int starty, int startx, int height, int width){
	WINDOW *border = s->border;
	wresize(border, height, width);
	mvwin(border, starty, startx);

	WINDOW *content = s->content;
	wresize(content, height-2, width-2);
	mvwin(content, starty+1, startx+1);

	s->starty = starty;
	s->startx = startx;
	s->endx = startx + width - 1;
	s->endy = starty + height - 1;

	return 1;
}

int printChatMessage(char *msg){
	TUI *t = tui;
	SECTION *chat = t->chat;
	if(chat->data == NULL)
		return -1;

	int size = strlen(msg) + 1;

	char *save = malloc(size);
	strhcpy(save, msg, size);

	arrl_addItem(chat->data, save);
	wprintw(chat->content, "%s", save);
	wrefresh(chat->content);

	return 1;
}

int drawMessages(TUI *t){
	SECTION *chat = t->chat;
	wclear(chat->content);
	wmove(chat->content, 0, 0);

	if(chat->data == NULL)
		return -1;

	// Print
	for(int i = 0; i < chat->data->length; i++){
		wprintw(chat->content, "%s", arrl_getItem(chat->data, i));
		wrefresh(chat->content);
	}

	return 1;
}

int drawTextinput(TUI *t){
	SECTION *text = t->text;

	wclear(text->content);
	wmove(text->content, 0, 0);
	wprintw(text->content, text->chars);
	wrefresh(text->content);

	return 1;
}

int sendTextBuffer(TUI *t){
	SECTION *text = t->text;

	if(text->index <= 0)
		return -1;

	text->chars[text->index] = '\n';
	text->index++;
	text->chars[text->index] = '\0';

	printChatMessage(text->chars);
	text->index = 0;

	com_sendMessage(text->c, text->chars);

	text->chars[0] = '\0';
	wclear(text->content);
	wrefresh(text->content);

	return 1;
}

int typeCharacter(TUI *t, int ch){
	SECTION *text = t->text;

	// Enter
	if (ch == KEY_BACKSPACE){
		int cur = getcurx(text->content);
		if(cur > 0){ // Is at beginning of line?
			text->index--;
			text->chars[text->index] = '\0';

			// Remove the deleted character
			drawTextinput(t);
		}

		return 1;
	}
	
	if(iscntrl(ch) != 0) // Is not printable
		return -1;

	text->chars[text->index] = (char) ch;
	text->index++;
	text->chars[text->index] = '\0';

	drawTextinput(t);

	return 1;
}

int setActiveSection(TUI *t, SECTION *s){
	if(t->active != NULL)  // Remove active color from old active
		wattroff(t->active->border, COLOR_PAIR(1));

	t->active = s;	
	wattron(t->active->border, COLOR_PAIR(1));
	drawBorders(t);

	return 1;
}

int sectionContainsPoint(SECTION *s, int y, int x){
	if(y >= s->starty && y <= s->endy && x >= s->startx && x <= s->endx)
		return 1;

	return -1;
}

int drawBorderTitle(SECTION *s){
	int width = s->endx - s->startx + 1;
	wattron(s->border, A_BOLD);
	mvwprintw(s->border, 0, (width-strlen(s->title))/2, s->title);
	wattroff(s->border, A_BOLD);
	wrefresh(s->border);

	return 1;
}

int drawBorder(SECTION *s, int bBuffer[LINES][COLS]){
	int *chars = s->borderChars;

	// Insert top and bottom
	for(int i = s->startx + 1; i < s->endx; i++){
		bBuffer[s->starty][i] = chars[1];
		bBuffer[s->endy][i] = chars[1];
	}

	// Insert left and right
	for(int i = s->starty + 1; i < s->endy; i++){
		bBuffer[i][s->startx] = chars[0];
		bBuffer[i][s->endx] = chars[0];
	}

	// Insert corners
	bBuffer[s->starty][s->startx] = chars[2];
	bBuffer[s->starty][s->endx] = chars[3];
	bBuffer[s->endy][s->startx] = chars[4];
	bBuffer[s->endy][s->endx] = chars[5];

	return 1;
}

/*	MUST be done in this order
	1. Sidebar
	2. Textbox
	3. Chatbox
*/
int drawBorders(TUI *t){
	int bBuffer[LINES][COLS]; // Buffer to store border for later drawing
	memset(bBuffer, 0, LINES * COLS * sizeof(int));

	drawBorder(t->sidebar, bBuffer);
	drawBorder(t->text, bBuffer);
	drawBorder(t->chat, bBuffer);

	//Highlight focused one
	SECTION *a = t->active;
	for(int y = 0; y < LINES; y++){
		for(int x = 0; x < COLS; x++){
			if(bBuffer[y][x] == 0)
				continue;

			attron(A_BOLD);
			if(sectionContainsPoint(a, y, x) == 1){
				attron(COLOR_PAIR(1));
			} else {
				attroff(COLOR_PAIR(1));
			}

			mvaddch(y, x, bBuffer[y][x]);

			attron(A_NORMAL);
		}
	}

	refresh();

	drawBorderTitle(t->sidebar);
	drawBorderTitle(t->chat);

	drawMessages(t);
	drawTextinput(t);
	drawMenu(t->sidebar);

	return 1;
}

int setupSidebar(TUI *t){
	MENU *menu = createMenu();
	MENUITEM *i1 = createMenuItem("&bruh", NULL);
	MENUITEM *i2 = createMenuItem("&bruv", NULL);
	MENUITEM *i3 = createMenuItem("bruk", NULL);
	MENUITEM *i4 = createMenuItem("brum", NULL);
	MENUITEM *i5 = createMenuItem("brua", NULL);
	addItemToMenu(menu, i1);
	addItemToMenu(menu, i2);
	addSubitem(i1, i3);
	addSubitem(i1, i4);
	addSubitem(i2, i5);
	i1->enableSubitems = ENABLE;
	menu->selected = i3;

	int borderChars[8] = {ACS_VLINE, ACS_HLINE, ACS_ULCORNER, ACS_TTEE, ACS_LLCORNER, ACS_BTEE};
	t->sidebar = createSection("No Connection", borderChars);
	drawSidebar(t->sidebar);
	t->sidebar->menu = menu;
	scrollok(t->sidebar->content, TRUE);

	return 1;
}

int setupTextbox(TUI *t){
	int borderChars1[] = {ACS_VLINE, ACS_HLINE, ACS_LTEE, ACS_RTEE, ACS_BTEE, ACS_LRCORNER};
	t->text = createSection("", borderChars1);
	t->text->c = &t->cList->conns[0];
	drawTextbox(t->text, t, 3);

	return 1;
}
	
int setupChatbox(TUI *t){
	//Chatbox
	int borderChars2[] = {ACS_VLINE, ACS_HLINE, ACS_TTEE, ACS_URCORNER, ACS_LTEE, ACS_RTEE};
	t->chat = createSection("Chat", borderChars2);
	drawChatbox(t->chat, t);
	scrollok(t->chat->content, TRUE);

	return 1;
}

int setupWindows(TUI *t){
	setupSidebar(t);
	setupTextbox(t);
	setupChatbox(t);

	// Default active section
	setActiveSection(t, t->sidebar);

	drawBorders(t);
	//wscrl

	return 1;
}

void handleResize(UNUSED(int sig)){
	endwin(); // Reinitalize size
	refresh();
	clear();

	setupWindows(tui);
	drawMessages(tui);
}
