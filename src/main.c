#include "main.h"

void cleanup(){
	log_logMessage("Exiting process", INFO);	
	tui_close();
}

int main(){
	atexit(cleanup);

	if(init_logging(0, "/var/log/boundless-client/", 0) == -1)
		return -1;

	// Connect to boundless.chat at port 6667
	struct com_ConnectionList *conList = init_connectionList();
	if(conList == NULL)
		return -1;

	TUI *tui = init_tui(conList);

	struct com_Connection *con = com_openConnection("boundless.chat", 6667);	
	if(con != NULL){
		com_listenToConnection(conList, con);
	}

	com_startPolling(conList);	

	handleUserInput(tui);

	return 1;
}
