#ifndef commands_h
#define commands_h

#include <stdio.h>
#include <string.h>
#include "hstring.h"
#include "chat.h"
#include "communication.h"
#include "tui.h"

#define ARRAY_SIZE(arr) (int)(sizeof(arr)/sizeof(arr[0]))

typedef struct {
	char prefix[100];
	char command[100];
	char params[15][100];
	int paramCount;

	CONNECTION *origin;
} COMMAND;

COMMAND *cmd_parseStr(char *str, CONNECTION *origin);

// Shortcut: combines cmd_parseStr and cmd_runCommand
int cmd_runCommandFromString(char *str, CONNECTION *origin);

int cmd_runCommand(COMMAND *cmd);

void cmd_freeCommand(COMMAND *cmd);

// Command functions
int cmd_join(COMMAND *cmd);
int cmd_ping(COMMAND *cmd);

#endif
