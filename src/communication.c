#include "communication.h"

struct com_ConnectionList *init_connectionList(){
	// Allocate struct
	struct com_ConnectionList *comList = calloc(1, sizeof(struct com_ConnectionList));
	if(comList == NULL){
		log_logError("Error initalizing connection list", FATAL);
		return NULL;
	}

	// Init mutex
	int ret = pthread_mutex_init(&comList->mutex, NULL);
	if(ret < 0){
		log_logError("Error initalizing mutex", FATAL);
		free(comList);
		return NULL;
	}

	// Set all fds to -1 to prevent them from being polled
	for(int i = 0; i < ARRAY_SIZE(comList->pfds); i++){
		comList->pfds[i].fd = -1;
	}

	comList->numConnected = 0;

	return comList;
}

pthread_t com_startPolling(struct com_ConnectionList *cList){
	pthread_t thread;
	if(pthread_create(&thread, NULL, com_pollConnections, cList) != 0){
		log_logError("Error starting thread", ERROR);
		return -1;
	}

	return thread;
}

void *com_pollConnections(void *ptr){
	int ret;
	char buff[BUFSIZ];
	struct com_ConnectionList *comList = (struct com_ConnectionList *) ptr;

	while(comList->numConnected > 0){
		ret = poll(comList->pfds, ARRAY_SIZE(comList->pfds), -1);
		if(ret == -1){
			log_logError("Error polling", ERROR);
			return NULL;
		}

		pthread_mutex_lock(&comList->mutex);
		for(int i = 0; i < ARRAY_SIZE(comList->pfds); i++){
			if(comList->pfds[i].revents != 0){
				if(comList->pfds[i].revents & POLLIN){ // Data to read
					int bytes = read(comList->pfds[i].fd, buff, sizeof(buff));
					if(bytes == -1){
						log_logMessage("Error reading from socket", WARNING);
						com_deleteConnection(comList, NULL, i); 
						continue;
					} else if (bytes == 0) { // Close
						log_logMessage("Connection disconnect", INFO);
						com_deleteConnection(comList, NULL, i); 
						continue;

					}

					printf("%s\n", buff);
				}
			}
		}
		pthread_mutex_unlock(&comList->mutex);
	}

	log_logMessage("All connections are closed, stopping polling", INFO);
	return NULL;
}

int com_deleteConnection(struct com_ConnectionList *cList, struct com_Connection *con, int pos){
	int loc = pos;

	if(con != NULL){ // Find it manually
		pthread_mutex_lock(&cList->mutex);
		loc = findArrayItem(cList->conns, sizeof(struct com_Connection), ARRAY_SIZE(cList->conns), con);
		pthread_mutex_unlock(&cList->mutex);
	}

	if(loc == -1)
		return -1;

	// Close socket and set fd to -1 to mark as open
	pthread_mutex_lock(&cList->mutex);
	cList->pfds[loc].fd = -1;
	close(cList->conns[loc].socket);
	pthread_mutex_unlock(&cList->mutex);

	return 1;
}

int com_listenToConnection(struct com_ConnectionList *cList, struct com_Connection *con){
	if(cList == NULL || con == NULL)
		return -1;

	int ret = -1;
	pthread_mutex_lock(&cList->mutex);
	for (int i = 0; i < ARRAY_SIZE(cList->pfds); i++){
		if(cList->pfds[i].fd == -1){ // Unused spot
			// Fill out pollfd
			cList->pfds[i].fd = con->socket;
			cList->pfds[i].events = POLLIN|POLLOUT;

			// Put com_Connection in conns in the same index
			memcpy(&cList->conns[i], con, sizeof(struct com_Connection));

			cList->numConnected++;
			ret = i;
			break;
		}
	}
	pthread_mutex_unlock(&cList->mutex);
	
	return ret;
}

struct com_Connection *com_openConnection(char *hostname, int port){
	int sock = com_connectSocket(hostname, port);
	if(sock < 0)
		return NULL;

	struct com_Connection *c = malloc(sizeof(struct com_Connection));
	if(c == NULL){
		close(sock);
		log_logError("Error allocating connection", ERROR);
		return NULL;
	}

	// Fill out connection
	c->type = TYPE_SERV;
	c->socket = sock;
	strhcpy(c->name, hostname, ARRAY_SIZE(c->name));

	return c;
}

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
