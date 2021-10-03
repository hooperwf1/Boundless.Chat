#include "hstring.h"

void lowerString(char *str){
	for(size_t i = 0; i < strlen(str); i++){
		str[i] = tolower(str[i]);
	}
}

size_t strhcpy(char *dst, char *src, size_t size){
	size_t i;
	for(i = 0; i < size-1; i++){
		dst[i] = src[i];

		if(src[i] == '\0')
			break;
	}

	dst[size-1] = '\0';
	return i+1;
}

size_t strhcat(char *dst, char *src, size_t size){
	size_t n = strlen(dst);
	dst += n;
	return strhcpy(dst, src, size-n);
}

// General character location
int findCharacter(char *str, int size, char key){
	for(int i = 0; i < size && str[i] != '\0'; i++){
		if(str[i] == key)
			return i;
	}

	return -1;
}

int numChar(char *str, int size, char key){
	int num = 0;
	for (int i = 0; i < size && str[i] != '\0'; i++){
		if(str[i] == key)
			num++;
	}

	return num;
}

void freeStrSplit(char **split){
	int i = 0;
	char *str = split[i];
	while(str != NULL){
		str = split[i];
		free(str);
		i++;
	}

	free(split);
}

char **splitString(char *str, int size, char key){
	int num = numChar(str, size, key) + 1;
	char **strArray = calloc(num + 1, sizeof(char *));
	if(strArray == NULL)
		return NULL;

	int loc = 0;	
	for(int i = 0; i < num; i++){
		// Locate next char
		int end = findCharacter(str+loc, size - (loc), key);
		if(end == -1)
			end = strlen(str) + 1;
		else
			end += loc;

		int len = end - loc;

		strArray[i] = calloc(len + 1, sizeof(char));
		if(strArray[i] == NULL){
			freeStrSplit(strArray);
			return NULL;
		}

		strhcpy(strArray[i], str + loc, len + 1);
		loc = end + 1;
	}

	return strArray;
}



