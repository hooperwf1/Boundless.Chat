#include "chat.h"

GROUP *grp_createGroup(char *name){
	GROUP *g = calloc(1, sizeof(GROUP));
	if(g == NULL){
		log_logError("Error allocating GROUP", ERROR);
		return NULL;
	}

	g->channels = arrl_createArrayList(20);
	g->users = arrl_createArrayList(20);
	if(g->channels == NULL || g->users == NULL){
		grp_deleteGroup(g);
		log_logMessage("Error allocating group", ERROR);
		return NULL;
	}

	g->name = malloc(strlen(name) + 1);
	if(g->name == NULL){
		log_logMessage("Error allocating GROUP name", ERROR);
		grp_deleteGroup(g);
		return NULL;
	}
	strhcpy(g->name, name, strlen(name) + 1);

	return g;
}

void grp_deleteGroup(void *gptr){
	GROUP *g = (GROUP *) gptr;

	free(g->name);
	arrl_deleteArrayList(g->channels, chan_deleteChannel);
	arrl_deleteArrayList(g->users, NULL);
	free(g);
}

GROUP *grp_getGroup(CONNECTION *c, char *name){
	char realName[1024];

	int divide = findCharacter(name, strlen(name), '/');
	if(divide == -1) // No divide in name
		strhcpy(realName, name, strlen(name));
	else
		strhcpy(realName, name, divide + 1);
		
	pthread_mutex_lock(&c->mutex);
	GROUP *g;
	for(int i = 0; i < c->groups->length; i++){
		g = arrl_getItem(c->groups, i);
		if(strncmp(realName, g->name, ARRAY_SIZE(realName)) == 0)
			break;

		g = NULL;
	}
	pthread_mutex_unlock(&c->mutex);

	return g;
}

int grp_addChannel(GROUP *g, CHANNEL *c){
	return arrl_addItem(g->channels, c);
}

CHANNEL *chan_createChannel(char *name){
	CHANNEL *c = calloc(1, sizeof(CHANNEL));
	if(c == NULL){
		log_logError("Error allocating CHANNEL", ERROR);
		return NULL;
	}

	c->messages = arrl_createArrayList(50);
	c->users = arrl_createArrayList(20);
	if(c->messages == NULL || c->users == NULL){
		chan_deleteChannel(c);
		log_logMessage("Error allocating channel", ERROR);
		return NULL;
	}

	c->name = malloc(strlen(name) + 1);
	if(c->name == NULL){
		log_logMessage("Error allocating CHANNEL name", ERROR);
		chan_deleteChannel(c);
		return NULL;
	}
	strhcpy(c->name, name, strlen(name) + 1);

	return c;
}

void chan_deleteChannel(void *cptr){
	CHANNEL *c = (CHANNEL *) cptr;

	free(c->name);
	arrl_deleteArrayList(c->messages, free);
	arrl_deleteArrayList(c->users, NULL);
	free(c);
}
