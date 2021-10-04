#ifndef chat_h
#define chat_h

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hstring.h"
#include "linkedlist.h"
#include "arraylist.h"
#include "logging.h"
#include "communication.h"

struct _connection;
typedef struct _connection CONNECTION;

typedef struct {
	char *name;

	// Private messages
	ARRAYLIST *messages;
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
	ARRAYLIST *messages, *users;
} CHANNEL;

typedef struct {
	char *name; 
	ARRAYLIST *channels, *users;
} GROUP;

GROUP *grp_createGroup(char *name);
void grp_deleteGroup(void *gptr);
GROUP *grp_getGroup(CONNECTION *c, char *name);
int grp_addChannel(GROUP *g, CHANNEL *c);

CHANNEL *chan_createChannel(char *name);
void chan_deleteChannel(void *cptr);

#endif
