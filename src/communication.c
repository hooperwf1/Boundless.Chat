#include "communication.h"

CONLIST *init_connectionList(){
	// Allocate struct
	CONLIST *cList = calloc(1, sizeof(CONLIST));
	if(cList == NULL){
		log_logError("Error initalizing connection list", ERROR);
		return NULL;
	}

	// Init mutex
	int ret = pthread_mutex_init(&cList->mutex, NULL);
	if(ret < 0){
		log_logError("Error initalizing mutex", ERROR);
		free(cList);
		return NULL;
	}

	// Allocate inital size for conns and pfds
	cList->size = 10;
	cList->conns = calloc(cList->size, sizeof(CONNECTION));
	if(cList->conns == NULL){
		log_logError("Error initalizing connection list", ERROR);
		free(cList);
		return NULL;
	}

	cList->pfds = calloc(cList->size, sizeof(struct pollfd));
	if(cList->pfds == NULL){
		log_logError("Error initalizing connection list", ERROR);
		free(cList);
		free(cList->conns);
		return NULL;
	}

	// Set all fds to -1 to prevent them from being polled
	for(int i = 0; i < cList->size; i++){
		cList->pfds[i].fd = -1;
	}

	cList->numConnected = 0;

	return cList;
}

pthread_t com_startPolling(CONLIST *cList){
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
	CONLIST *comList = (CONLIST *) ptr;
	
	// Disable SIGWINCH for this thread
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGWINCH);
	ret = pthread_sigmask(SIG_BLOCK, &set, NULL);
	if(ret != 0){
		log_logError("Error blocking SIGWINCH", ERROR);
		return NULL;
	}

	while(comList->numConnected > 0){
		ret = poll(comList->pfds, comList->size, -1);
		if(ret == -1){
			log_logError("Error polling", ERROR);
			return NULL;
		}

		pthread_mutex_lock(&comList->mutex);
		for(int i = 0; i < comList->size; i++){
			if(comList->pfds[i].revents != 0){
				if(comList->pfds[i].revents & POLLIN){ // Data to read
					int bytes = read(comList->pfds[i].fd, buff, sizeof(buff)-1);
					if(bytes == -1){
						log_logError("Error reading from socket", WARNING);
						com_deleteConnection(comList, NULL, i); 
						continue;
					} else if (bytes == 0) { // Close
						log_logMessage("Connection disconnect", INFO);
						com_deleteConnection(comList, NULL, i); 
						continue;
					}
					
					// Ensure null byte
					buff[bytes] = '\0';

					printChatMessage(buff);
				}
			}
		}
		pthread_mutex_unlock(&comList->mutex);
	}

	log_logMessage("All connections are closed, stopping polling", INFO);
	return NULL;
}

int com_sendMessage(CONNECTION *con, char *msg){
	if(con == NULL || msg == NULL)
		return -1;

	write(con->socket, msg, strlen(msg));

	return 1;
}

int com_deleteConnection(CONLIST *cList, CONNECTION *con, int pos){
	int loc = pos;

	if(con != NULL){ // Find it manually
		pthread_mutex_lock(&cList->mutex);
		loc = findArrayItem(cList->conns, sizeof(CONNECTION), cList->size, con);
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

int com_listenToConnection(CONLIST *cList, CONNECTION *con){
	if(cList == NULL || con == NULL)
		return -1;

	int ret = -1;
	pthread_mutex_lock(&cList->mutex);
	for (int i = 0; i < cList->size; i++){
		if(cList->pfds[i].fd == -1){ // Unused spot
			// Fill out pollfd
			cList->pfds[i].fd = con->socket;
			cList->pfds[i].events = POLLIN;

			// Put com_Connection in conns in the same index
			memcpy(&cList->conns[i], con, sizeof(CONNECTION));

			cList->numConnected++;
			ret = i;
			break;
		}
	}
	pthread_mutex_unlock(&cList->mutex);
	
	return ret;
}

CONNECTION *com_openConnection(char *hostname, int port){
	CONNECTION *c = malloc(sizeof(CONNECTION));
	if(c == NULL){
		log_logError("Error allocating connection", ERROR);
		return NULL;
	}

	// Init mutex
	int ret = pthread_mutex_init(&c->mutex, NULL);
	if(ret < 0){
		log_logError("Error initalizing mutex", ERROR);
		free(c);
		return NULL;
	}

	int sock = com_connectSocket(hostname, port);
	if(sock < 0)
		return NULL;

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
