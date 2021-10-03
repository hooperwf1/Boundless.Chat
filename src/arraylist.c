#include "arraylist.h"

ARRAYLIST *arrl_createArrayList(int size){
	ARRAYLIST *l = malloc(sizeof(ARRAYLIST));
	if(l == NULL){
		log_logError("error allocating arraylist", ERROR);
		return NULL;
	}

	l->size = size;
	l->length = 0;
	l->data = calloc(size, sizeof(void *));
	if(l->data == NULL){
		log_logError("error allocating arraylist", ERROR);
		free(l);
		return NULL;
	}
		
	return l;
}

void arrl_deleteArrayList(ARRAYLIST *l, void callback(void *ptr)){
	if(callback != NULL){
		for(int i = 0; i < l->length; i++){
			callback(arrl_getItem(l, i));
		}
	}
		
	free(l->data);
	free(l);
}

int arrl_insertItem(ARRAYLIST *l, int index, void *ptr){
	if(l->length == l->size){ // Full add more space	
		if(arrl_enlargeData(l, 10) != 1)
			return -1;
	}

	if(index > l->length || index < 0){
		return -1;
	}

	//Shift everything before it over
	for(int i = l->length; i > index; i++){
		l->data[i] = l->data[i-1];
	}

	l->data[index] = ptr;
	l->length++;

	return 1;
}

int arrl_addItem(ARRAYLIST *l, void *ptr){
	return arrl_insertItem(l, l->length, ptr);
}

void *arrl_getItem(ARRAYLIST *l, int index){
	if(index > l->length || index < 0){
		return NULL;
	}

	return l->data[index];
}

int arrl_enlargeData(ARRAYLIST *l, int inc){
	void *ptr = realloc(l->data, (l->size + inc) * sizeof(void *));
	if(ptr == NULL){
		log_logError("Unable to expand ArrayList", ERROR);
		return -1;
	}

	l->data = ptr;
	return 1;
}
