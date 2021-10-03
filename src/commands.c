#include "commands.h"

COMMAND *cmd_parseStr(char *str, CONNECTION *origin){
	COMMAND *cmd = calloc(1, sizeof(COMMAND));
	if(cmd == NULL){
		log_logError("Error allocating command", ERROR);
		return NULL;
	}
	cmd->origin = origin;
	cmd->paramCount = 0;

	// Make a copy
	char cmdStr[strlen(str) + 1];
	strhcpy(cmdStr, str, ARRAY_SIZE(cmdStr));

	// Find end of command
	int len = findCharacter(str, strlen(str), '\n');
	int len2 = findCharacter(str, strlen(str), '\r');
	if(len == -1 && len2 == -1) // no \n or \r, just go off of \0
		len = strlen(str) + 1;
	else if(len == -1) // no \n just \r
		len = len2;
	cmdStr[len] = '\0';

	char **words = splitString(cmdStr, len, ' '); 
	if(words == NULL){
		cmd_freeCommand(cmd);
		return NULL;
	}

	int i = 0;
	while(words[i] != NULL){
		if(i == 0 && words[i][0] == ':'){ //check for prefix
			strhcpy(cmd->prefix, words[i], ARRAY_SIZE(cmd->command));
			i++;
			continue;
		}

		if(cmd->command[0] == '\0'){ // Fill in command next
			strhcpy(cmd->command, words[i], ARRAY_SIZE(cmd->command));
			i++;
			continue;
		}

		// The rest is in one param
		if(words[i][0] == ':'){
			while(words[i] != NULL){
				strhcat(cmd->params[cmd->paramCount], words[i], ARRAY_SIZE(cmd->params[0]));
				strhcat(cmd->params[cmd->paramCount], " ", ARRAY_SIZE(cmd->params[0]));
				i++;
			}
			// Null byte to overwrite space at the end
			cmd->params[cmd->paramCount][strlen(cmd->params[cmd->paramCount])] = '\0';

			cmd->paramCount++;
			break;
		}

		// The rest go to params
		strhcpy(cmd->params[cmd->paramCount], words[i], ARRAY_SIZE(cmd->params[0]));
		cmd->paramCount++;
		i++;
	}

	freeStrSplit(words);

	return cmd;
}

int cmd_runCommandFromString(char *str, CONNECTION *origin){
	COMMAND *cmd = cmd_parseStr(str, origin);
	if(cmd == NULL)
		return -1;

	int ret = cmd_runCommand(cmd);
	cmd_freeCommand(cmd);
	return ret;
}

int cmd_runCommand(COMMAND *cmd){
	printf("%s %s\n", cmd->prefix, cmd->command);
	for(int i = 0; i < cmd->paramCount; i++){
		printf("%s\n", cmd->params[i]);
	}
	return cmd->paramCount;	
}

void cmd_freeCommand(COMMAND *cmd){
	free(cmd);
}
