#include "commands.h"

COMMAND *cmd_parseStr(char *str){
	COMMAND *cmd = calloc(1, sizeof(COMMAND));
	if(cmd == NULL){
		log_logError("Error allocating command", ERROR);
		return NULL;
	}

	if(str[0] == ':')
		return cmd;

	return cmd;
}

int cmd_runCommandFromString(char *str){
	COMMAND *cmd = cmd_parseStr(str);
	if(cmd == NULL)
		return -1;

	int ret = cmd_runCommand(cmd);
	cmd_freeCommand(cmd);
	return ret;
}

int cmd_runCommand(COMMAND *cmd){
	return cmd->paramCount;	
}

void cmd_freeCommand(COMMAND *cmd){
	free(cmd);
}
