#include "main.h"

void cleanup(){
	log_logMessage("Exiting process", INFO);	
}

int main(){
	atexit(cleanup);

	if(init_logging(0, "/var/log/boundless-client/") == -1)
		return -1;
	if(init_comms() == -1)
		return -1;

	char buff[1024] = "PING\n";
	int sock = com_connectSocket("boundless.chat", 6667);
	write(sock, buff, strlen(buff));
	read(sock, buff, ARRAY_SIZE(buff));
	printf("%s\n", buff);

	return 1;
}
