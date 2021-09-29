#include "main.h"

void cleanup(){
	log_logMessage("Exiting process", INFO);	
	tui_close();
}

int main(){
	atexit(cleanup);

	if(init_logging(0, "/var/log/boundless-client/", 0) == -1)
		return -1;

	init_tui();

	// Connect to boundless.chat at port 6667
	struct com_ConnectionList *conList = init_connectionList();
	if(conList == NULL)
		return -1;

	struct com_Connection *con = com_openConnection("boundless.chat", 6667);	
	if(con == NULL)
		return -1;

	com_listenToConnection(conList, con);

	com_startPolling(conList);	

	tui_close();

	return 1;
}
