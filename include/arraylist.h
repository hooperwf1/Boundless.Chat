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

// Shortcut to insert item at the end of list
int arrl_addItem(ARRAYLIST *l, void *ptr);

void *arrl_getItem(ARRAYLIST *l, int index);

// Will increase memory used by inc
int arrl_enlargeData(ARRAYLIST *l, int inc);

#endif
