#ifndef communication_h
#define communication_h

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdatomic.h>
#include <pthread.h>
#include <poll.h>
#include "hstring.h"
#include "linkedlist.h"
#include "array.h"
#include "main.h"
#include "tui.h"

#define POLL_SIZE 1024

// For connection type
#define TYPE_SERV 0
#define TYPE_USER 1

struct com_Connection {
	atomic_int type;
	atomic_int socket;
	char name[1024];
};

struct com_ConnectionList {
	struct com_Connection conns[POLL_SIZE]; // com_Connection
	struct pollfd pfds[POLL_SIZE]; // poll
	int numConnected;
	pthread_mutex_t mutex;
};

struct com_ConnectionList *init_connectionList();

// Starts and returns a thread that is polling the sockets
pthread_t com_startPolling(struct com_ConnectionList *cList);

// use poll
void *com_pollConnections(void *comList);

int com_sendMessage(struct com_Connection *con, char *msg);

// closes a socket and removes it from polling
// leave con NULL to use pos, otherwise pos unused
int com_deleteConnection(struct com_ConnectionList *cList, struct com_Connection *con, int pos);

// Takes a connection and adds it to a list for polling
// con should be freed after if it isnt needed
int com_listenToConnection(struct com_ConnectionList *cList, struct com_Connection *con);

// Gets socket and fills out a com_Connection struct
struct com_Connection *com_openConnection(char *hostname, int port);

// Will connect to the given destination and return socket
int com_connectSocket(char *hostname, int port);

#endif
