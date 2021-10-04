#ifndef communication_h
#define communication_h

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdatomic.h>
#include <pthread.h>
#include <poll.h>
#include "chat.h"
#include "linkedlist.h"
#include "arraylist.h"

struct _connection {
	atomic_int type;
	atomic_int socket;
	char name[1024];

	ARRAYLIST *groups, *users;
	pthread_mutex_t mutex;
};
typedef struct _connection CONNECTION;

typedef struct {
	CONNECTION *conns;
	struct pollfd *pfds; // poll
	int numConnected, size;
	pthread_mutex_t mutex;
} CONLIST;

#include "hstring.h"
#include "array.h"
#include "main.h"
#include "commands.h"
#include "chat.h"

// For connection type
#define TYPE_SERV 0
#define TYPE_USER 1

CONLIST *init_connectionList();

// Starts and returns a thread that is polling the sockets
pthread_t com_startPolling(CONLIST *cList);

// use poll
void *com_pollConnections(void *comList);

int com_sendMessage(CONNECTION *con, char *msg);

// closes a socket and removes it from polling
// leave con NULL to use pos, otherwise pos unused
int com_deleteConnection(CONLIST *cList, CONNECTION *con, int pos);

// Takes a connection and adds it to a list for polling
// con should be freed after if it isnt needed
int com_listenToConnection(CONLIST *cList, CONNECTION *con);

// Gets socket and fills out a com_Connection struct
CONNECTION *com_openConnection(char *hostname, int port);

// Will connect to the given destination and return socket
int com_connectSocket(char *hostname, int port);

#endif
