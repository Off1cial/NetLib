#ifndef NETCLIENT_H
#define NETCLIENT_H

#include "net/net.h"

typedef enum {
    CON_UNITIALISED = 0,
    CON_FREE,
    CON_CONNECTED,
    CON_WAITINGACK,
    CON_ACTIVE,

} netconnstate_t;

typedef struct {
    netconnstate_t state;
    netaddr_t remote;
    netsock_t socket_udp;
} netconnection_t;

typedef struct netclient_t
{
    char name[NET_MAX_STR];
    netconnection_t connection;
    
} netclient_t;


netclient_t* NetClient_Init(const char* name, size_t namelen);


// Forms the connection for communication - unrelated to joining a game server
netresult_t NetClient_Connect(netclient_t* client, netaddr_t server_addr);
#endif
