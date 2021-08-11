#ifndef communication_h
#define communication_h

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdatomic.h>
#include "hstring.h"
#include "main.h"

struct com_Connection {
	atomic_int type;
	atomic_int socket;
};

int init_comms();

// Gets socket and fills out a com_Connection struct
int com_startConnection(char *hostname, int port);

// Will connect to the given destination and return socket
int com_connectSocket(char *hostname, int port);

#endif
