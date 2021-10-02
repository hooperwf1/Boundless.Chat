#ifndef commands_h
#define commands_h

#include <string.h>
#include "chat.h"
#include "communication.h"

typedef struct {
	char prefix[100];
	char command[100];
	char params[15][100];
	int paramCount;

	CONNECTION *origin;
} COMMAND;

COMMAND *cmd_parseCommand(char *str);

int cmd_runCommandFromString(char *str);

int cmd_runCommand(COMMAND *cmd);

void cmd_freeCommand(COMMAND *cmd);

#endif
