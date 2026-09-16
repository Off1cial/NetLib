#ifndef NETCLIENT_H
#define NETCLIENT_H

#include "net/net.h"
#include "net/chan.h"

typedef enum {
    CON_UNITIALISED = 0,
    CON_FREE,
    CON_CONNECTED,
    CON_WAITINGACK,
    CON_ACTIVE,

} netconnstate_t;

typedef struct {
    netconnstate_t state;
    netchan_t chan;
    netsock_t socket_udp;
} netconnection_t;

typedef struct netclient_t
{
    char name[NET_MAX_STR];
    netconnection_t connection;
    
    // Configurables
    u32 update_rate; // 0 Until connected -> received server info packet(s)
    float interp;

    // Addon funcs, they add to default lib functionality, do not replace. (Client run loop calls this amongst its other routines)
    void (*func_run)(void);

} netclient_t;


netclient_t* NetClient_Init(const char* name, size_t namelen);


// Forms the connection for communication - unrelated to joining a game server
netresult_t NetClient_ConnectAttempt(netclient_t* client, netaddr_t server_addr);
void NetClient_Run(netclient_t* client);
#endif
