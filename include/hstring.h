#ifndef hstring_h
#define hstring_h

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

void lowerString(char *str);

// Safe version of strncpy, always NULL terminated and faster (no unneeded NULL writing)
size_t strhcpy(char *dst, char *src, size_t size);

// Same as strhcpy, but starts copying at the end of dst
size_t strhcat(char *dst, char *src, size_t size);

// General character location
int findCharacter(char *str, int size, char key);

// Number of times a characters appears in a string
int numChar(char *str, int size, char key);

char **splitString(char *str, int size, char key);
void freeStrSplit(char **split);

#endif
