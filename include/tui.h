#ifndef tui_h
#define tui_h

#include <curses.h>
#include <menu.h>
#include <stdio.h>
#include <string.h>
#include <hstring.h>
#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>
#include "linkedlist.h"
#include "logging.h"
#include "communication.h"

#define UNUSED(x) x __attribute__((unused))
#define ARRAY_SIZE(arr) (int)(sizeof(arr)/sizeof(arr[0]))

typedef struct {
	WINDOW *border;
	WINDOW *content;
	char title[10];

	// VLINE, HLINE, UL, UR, LL, LR
	int borderChars[6];
	int startx, starty;
	int endx, endy;

	// Misc data (chat msg, groups, etc)
	union {
		struct link_List data; // For saved messages
		struct { // For text box
			char chars[BUFSIZ];
			int index;
		};
	};
} SECTION;

typedef struct {
	SECTION *sidebar; //Servers, dms, etc
	SECTION *chat; // Messages
	SECTION *text; // Text box

	SECTION *active; // Which one is active
	struct com_ConnectionList *cList;
} TUI;

TUI *init_tui(struct com_ConnectionList *cList);

void tui_close();

void handleUserInput(TUI *t);

SECTION *createSection(char *title, int borderChars[]);

void drawSidebar(SECTION *sidebar);
void drawChatbox(SECTION *chat, TUI *t);
void drawTextbox(SECTION *text, TUI *t, int height);

int resizeSection(SECTION *s, int starty, int startx, int height, int width);

// Make sure to supply newline yourself
int printChatMessage(char *msg);

int drawMessages(TUI *t);

// Type character into the text box
int typeCharacter(TUI *t, int ch);

// Handle keyboard input, highlight etc
int setActiveSection(TUI *t, SECTION *s);

int sectionContainsPoint(SECTION *s, int y, int x);

int drawBorderTitle(SECTION *s);

int drawBorder(SECTION *s, int bBuffer[LINES][COLS]);
int drawBorders(TUI *t);

//Place all windows
int setupWindows(TUI *t);

void handleResize(int sig);

#endif
