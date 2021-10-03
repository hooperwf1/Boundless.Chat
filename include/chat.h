#ifndef chat_h
#define chat_h

#include "linkedlist.h"
#include "arraylist.h"

typedef struct {
	char *name;

	// Private messages
	struct link_List messages;
} USER;

typedef struct {
	char *name;
	int color[3];
} ROLE;

typedef struct {
	USER *user;
	ROLE *role;
} GRP_USER;

typedef struct {
	char *name;
	USER *users[512];

	ARRAYLIST messages;
} CHANNEL;

typedef struct {
	USER *users[512];
	CHANNEL channels[100];	
} GROUP;

#endif
