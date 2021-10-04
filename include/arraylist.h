#ifndef arraylist_h
#define arraylist_h

#include <stdio.h>
#include <stdlib.h>
#include "logging.h"

typedef struct {
	int size, length;
	void **data;
} ARRAYLIST;

ARRAYLIST *arrl_createArrayList(int size);

// Callback is action to do for each item in the list, NULL to do nothing
void arrl_deleteArrayList(ARRAYLIST *l, void callback(void *ptr));

int arrl_insertItem(ARRAYLIST *l, int index, void *ptr);
void *arrl_deleteItem(ARRAYLIST *l, int index);

// Shortcut to insert item at the end of list
int arrl_addItem(ARRAYLIST *l, void *ptr);

void *arrl_getItem(ARRAYLIST *l, int index);

/*	Leave callback NULL to just compare pointers
	otherwise, callback checks for equality
	1 = equal, anything else = false */
int arrl_contains(ARRAYLIST *l, void *ptr, int callback(void *d, void *p));

// Will increase memory used by inc
int arrl_enlargeData(ARRAYLIST *l, int inc);

#endif
