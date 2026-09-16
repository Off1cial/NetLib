#include "net/client.h"
#include "net/net.h"
#include "net/platform/netplatform.h"
#include "common/plt_time.h"
#include <stdio.h>
#include <string.h>

// Timing variables for consistent update rate
static double accum = 0.0;
static double previous = 0.0;


netclient_t* NetClient_Init(const char* name, size_t namelen){
    netclient_t* client = calloc(1, sizeof(netclient_t)); 
    if (!client)
        return NET_NULL;
    client->connection.socket_udp = -1;
    client->connection.state = CON_UNITIALISED;
    strncpy(client->name, name, namelen);

    client->connection.socket_udp = netsock_create_udp();

    previous = plt_timemillis();
    return client;
}

// Forms the connection for communication - unrelated to joining a game server
netresult_t NetClient_Connect(netclient_t* client, netaddr_t server_addr){
    if (NETSOCK_ISNULL(client->connection.socket_udp)){
        client->connection.socket_udp = netsock_create_udp();
    }
    netresult_t res = netsock_connect(client->connection.socket_udp, server_addr);
    if (!res)
        return NET_FAILURE;
    client->connection.remote = server_addr;
    client->connection.state = CON_CONNECTED;
    return NET_SUCCESS;
}


void NetClient_Run(netclient_t* client){
    double now = plt_timemillis();
    double dt = (now - previous) / 1000.0f;
    previous = now;
    accum += dt;
    while (accum >= (1.0f / client->update_rate)){
        DOFUNC(client->func_run);
        accum -= (1.0f / client->update_rate);
    }
}

