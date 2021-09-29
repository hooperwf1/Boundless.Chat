#include "tui.h"

TUI *tui;

TUI *init_tui(){
	TUI *t = calloc(1, sizeof(TUI));
	if(t == NULL){
		log_logError("Error allocation TUI", ERROR);
		return NULL;
	}
	tui = t;

	setlocale(LC_ALL, "");
	if(initscr() == NULL)
		return NULL;

	if(has_colors())
		start_color();

	raw();
	noecho();
	keypad(stdscr, TRUE); //Enable F1, arrows keys, etc
	box(stdscr, 0, 0);
	refresh();

	// Handle resizing
	signal(SIGWINCH, handleResize);

	setupWindows(t);
	
	return t;
}

void tui_close(){
	getch();
	endwin();
}

SECTION *createSection(char *title){
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

	return section;
}

void drawSidebar(SECTION *sidebar){
	int width = 26 < COLS/3 ? 26 : COLS/3;
	resizeSection(sidebar, 1, 0, LINES-2, width);

	WINDOW *border = sidebar->border;
	wborder(border, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_ULCORNER, ACS_TTEE, ACS_LLCORNER, ACS_BTEE);
	mvwprintw(border, 0, (width-strlen(sidebar->title))/2, sidebar->title);
	wrefresh(border);
}

void drawChatbox(SECTION *chat, TUI *t){
	int startx = getbegx(t->text->border);
	int height = getbegy(t->text->border);
	int width = COLS-startx;

	resizeSection(chat, 1, startx, height, width);
	wmove(chat->content, 0, 0);

	WINDOW *border = chat->border;
	wborder(border, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_TTEE, ACS_URCORNER, ACS_LTEE, ACS_RTEE);
	mvwprintw(border, 0, (width-strlen(chat->title))/2, chat->title);
	wrefresh(border);
	wrefresh(chat->content);
}

void drawTextbox(SECTION *text, TUI *t, int height){
	int startx = getmaxx(t->sidebar->border)-1;

	resizeSection(text, LINES-(height+1), startx, height, COLS-startx);

	WINDOW *border = text->border;
	wborder(border, ACS_VLINE, ACS_VLINE, ACS_HLINE, ACS_HLINE, ACS_LTEE, ACS_RTEE, ACS_BTEE, ACS_LRCORNER);
	wrefresh(border);
}

int resizeSection(SECTION *s, int starty, int startx, int height, int width){
	WINDOW *border = s->border;
	wresize(border, height, width);
	mvwin(border, starty, startx);

	WINDOW *content = s->content;
	wresize(content, height-2, width-2);
	mvwin(content, starty+1, startx+1);

	return 1;
}

int printChatMessage(char *msg){
	TUI *t = tui;
	SECTION *chat = t->chat;
	int size = strlen(msg) + 1;

	char *save = malloc(size);
	strhcpy(save, msg, size);

	link_add(&chat->data, save);
	wprintw(chat->content, "%s\n", save);
	wrefresh(chat->content);

	return 1;
}

int drawMessages(TUI *t){
	SECTION *chat = t->chat;
	wclear(chat->content);

	// Print
	struct link_Node *n;
	for(n = chat->data.head; n != NULL; n = n->next){
		wprintw(chat->content, "%s\n", n->data);
		wrefresh(chat->content);
	}

	return 1;
}

/*	MUST be done in this order
	1. Sidebar
	2. Textbox
	3. Chatbox
*/
int setupWindows(TUI *t){
	t->sidebar = createSection("Groups");
	drawSidebar(t->sidebar);
	t->text = createSection("");
	drawTextbox(t->text, t, 3);
	t->chat = createSection("Chat");
	drawChatbox(t->chat, t);
	scrollok(t->chat->content, TRUE);

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
