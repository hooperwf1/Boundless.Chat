#include "communication.h"

int init_comms(){
	return 1;
}

int com_startConnection(char *hostname, int port);

// man getaddrinfo(3) for help with getaddrinfo
int com_connectSocket(char *hostname, int port){
	int sock;
	struct addrinfo hints = {0};
	struct addrinfo *res, *rp;
	
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	char portStr[7];
	snprintf(portStr, ARRAY_SIZE(portStr), "%d", port);

	if(getaddrinfo(hostname, portStr, &hints, &res) != 0){
		log_logError("Error with getaddrinfo", ERROR);
		return -1;
	}

	for(rp = res; rp != NULL; rp = rp->ai_next){
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if(sock == -1)
			continue;

		if(connect(sock, rp->ai_addr, rp->ai_addrlen) != -1)
			break; // Succesful

		close(sock);
	}

	freeaddrinfo(res);

	if(rp == NULL) { // Failure
		log_logMessage("Unable to connect", WARNING);
		return -1;
	}

	return sock;
}
