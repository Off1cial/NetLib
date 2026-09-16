#include "net/client.h"
#include "net/net.h"
#include "net/platform/netplatform.h"
#include <stdio.h>
#include <string.h>

netclient_t* NetClient_Init(const char* name, size_t namelen){
    netclient_t* client = calloc(1, sizeof(netclient_t)); 
    if (!client)
        return NET_NULL;
    client->connection.socket_udp = -1;
    client->connection.state = CON_UNITIALISED;
    strncpy(client->name, name, namelen);
    return client;
}

// Forms the connection for communication - unrelated to joining a game server
netresult_t NetClient_Connect(netclient_t* client, netaddr_t server_addr){
    if (NETSOCK_ISNULL(client->connection.socket_udp)){
        client->connection.socket_udp = netsock_create_udp();
    }
    netresult_t res = netsock_connect(client->connection.socket_udp, server_addr);
    if (res) client->connection.state = CON_CONNECTED;
    return res;
}



