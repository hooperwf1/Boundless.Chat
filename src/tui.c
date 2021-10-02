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

	setlocale(LC_ALL, "");
	if(initscr() == NULL)
		return NULL;

	if(has_colors()){
		start_color();
		init_pair(1, COLOR_BLUE, COLOR_BLACK);
	}

	raw();
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
	item->ptr = ptr;
	strhcpy(item->text, text, size);

	return item;
}

void freeMenu(MENU *m){
	link_clear(&m->items, freeMenuItem);

	free(m);
}

void freeMenuItem(void *iptr){
	MENUITEM *i = (MENUITEM *) iptr;
	link_clear(&i->subitems, freeMenuItem);

	free(i->text);
	free(i);
}

int addItemToMenu(MENU *m, MENUITEM *i){
	pthread_mutex_lock(&m->mutex);
	// Make this item selected if it is the first
	if(link_isEmpty(&m->items) == 1)
		m->selected = i;

	link_add(&m->items, i);
	pthread_mutex_unlock(&m->mutex);

	return 1;
}

int addSubitem(MENUITEM *item, MENUITEM *sub){
	pthread_mutex_lock(&item->mutex);
	link_add(&item->subitems, sub);
	pthread_mutex_unlock(&item->mutex);

	return 1;
}

int drawMenu(MENU *m, SECTION *s){
	wclear(s->content);
	wmove(s->content, 0, 0);

	pthread_mutex_lock(&m->mutex);
	struct link_Node *n;
	for(n = m->items.head; n != NULL; n = n->next){
		drawMenuItem(n->data, m, s);
	}
	pthread_mutex_unlock(&m->mutex);

	wrefresh(s->content);

	return 1;
}

int drawMenuItem(MENUITEM *item, MENU *m, SECTION *s){
	pthread_mutex_lock(&item->mutex);
	//Draw item
	if(item == m->selected)
		wattron(s->content, A_BOLD);
	waddstr(s->content, item->text);
	wprintw(s->content, "\n");
	wattroff(s->content, A_BOLD);

	// Draw any subitems if applicable
	if(item->enableSubitems == ENABLE){
		struct link_Node *n;
		for(n = item->subitems.head; n != NULL; n = n->next){
			//Branch before subitem
			if(n->next != NULL)
				waddch(s->content, ACS_LTEE);
			else
				waddch(s->content, ACS_LLCORNER);

			drawMenuItem(n->data, m, s);
		}
	}
	pthread_mutex_unlock(&item->mutex);

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

	link_add(chat->data, save);
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
	struct link_Node *n;
	for(n = chat->data->head; n != NULL; n = n->next){
		wprintw(chat->content, "%s", n->data);
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

int typeCharacter(TUI *t, int ch){
	SECTION *text = t->text;

	// Enter
	if((ch == KEY_ENTER || ch == '\n') && text->index > 0){
		text->chars[text->index] = '\n';
		text->index++;
		text->chars[text->index] = '\0';

		printChatMessage(text->chars);
		text->index = 0;

		com_sendMessage(&t->cList->conns[0], text->chars);

		text->chars[0] = '\0';
		wclear(text->content);
		wrefresh(text->content);

		return 1;
	} else if (ch == KEY_BACKSPACE){
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
	mvwprintw(s->border, 0, (width-strlen(s->title))/2, s->title);
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

	wrefresh(s->content);

	return 1;
}

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

	return 1;
}

/*	MUST be done in this order
	1. Sidebar
	2. Textbox
	3. Chatbox
*/
int setupWindows(TUI *t){
	MENU *menu = createMenu();
	MENUITEM *item1 = createMenuItem("&Boundless", NULL);
	MENUITEM *item2 = createMenuItem("&Programming", NULL);
	MENUITEM *item3 = createMenuItem("#general", NULL);
	MENUITEM *item4 = createMenuItem("#no-terroriz-plz", NULL);

	addItemToMenu(menu, item1);
	addItemToMenu(menu, item2);
	addSubitem(item1, item3);
	addSubitem(item1, item4);
	item1->enableSubitems = ENABLE;

	//Sidebar
	int borderChars[8] = {ACS_VLINE, ACS_HLINE, ACS_ULCORNER, ACS_TTEE, ACS_LLCORNER, ACS_BTEE};
	t->sidebar = createSection("Groups", borderChars);
	drawSidebar(t->sidebar);
	scrollok(t->sidebar->content, TRUE);

	// Textbox
	int borderChars1[] = {ACS_VLINE, ACS_HLINE, ACS_LTEE, ACS_RTEE, ACS_BTEE, ACS_LRCORNER};
	t->text = createSection("", borderChars1);
	drawTextbox(t->text, t, 3);

	//Chatbox
	int borderChars2[] = {ACS_VLINE, ACS_HLINE, ACS_TTEE, ACS_URCORNER, ACS_LTEE, ACS_RTEE};
	t->chat = createSection("Chat", borderChars2);
	drawChatbox(t->chat, t);
	scrollok(t->chat->content, TRUE);

	// Default active section
	setActiveSection(t, t->sidebar);

	drawBorders(t);
	drawMenu(menu, t->sidebar);
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
