#include "main.h"

void cleanup(){
	log_logMessage("Exiting process", INFO);	
}

int main(){
	atexit(cleanup);

	if(init_logging(0, "/var/log/boundless-client/") == -1)
		return -1;

	// Connect to boundless.chat at port 6667
	struct com_ConnectionList *conList = init_connectionList();
	if(conList == NULL)
		return -1;

	struct com_Connection *con = com_openConnection("boundless.chat", 6667);	
	if(con == NULL)
		return -1;

	com_listenToConnection(conList, con);

	pthread_t pollThread = com_startPolling(conList);	
	printf("Thread id = %ld\n", pollThread);
	pthread_join(pollThread, NULL);

	return 1;
}
