#ifndef commands_h
#define commands_h

#include <stdio.h>
#include <string.h>
#include "hstring.h"
#include "chat.h"
#include "communication.h"

#define ARRAY_SIZE(arr) (int)(sizeof(arr)/sizeof(arr[0]))

typedef struct {
	char prefix[100];
	char command[100];
	char params[15][100];
	int paramCount;

	CONNECTION *origin;
} COMMAND;

COMMAND *cmd_parseStr(char *str, CONNECTION *origin);

int cmd_runCommandFromString(char *str, CONNECTION *origin);

int cmd_runCommand(COMMAND *cmd);

void cmd_freeCommand(COMMAND *cmd);

#endif
