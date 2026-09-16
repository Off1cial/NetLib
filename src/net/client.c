#include "net/client.h"
#include "net/net.h"
#include "net/platform/netplatform.h"
#include "common/plt_time.h"
#include <stdio.h>
#include <string.h>

// Timing variables for consistent update rate
static double accum = 0.0;
static double previous = 0.0;

static void cl_recv(netclient_t* client);

netclient_t* NetClient_Init(const char* name, size_t namelen){
    netclient_t* client = calloc(1, sizeof(netclient_t)); 
    if (!client)
        return NET_NULL;
    client->connection.socket_udp = -1;
    client->connection.state = CON_UNITIALISED;
    strncpy(client->name, name, namelen + 1);

    client->connection.socket_udp = netsock_create_udp();

    previous = plt_timemillis();
    return client;
}

// Forms the connection for communication - unrelated to joining a game server
netresult_t NetClient_ConnectAttempt(netclient_t* client, netaddr_t server_addr){
        
    memset(&client->connection.chan, 0, sizeof(netchan_t));
    client->connection.state = CON_FREE;
    if (NETSOCK_ISNULL(client->connection.socket_udp)){
        client->connection.socket_udp = netsock_create_udp();
    }
    /*
    netresult_t res = netsock_connect(client->connection.socket_udp, server_addr);
    if (!res)
        return NET_FAILURE;
    client->connection.chan.remote = server_addr;
    client->connection.chan.state = NETCHAN_CONNECTED;
    */
    netresult_t res = netchan_connect(
            &client->connection.chan, 
            client->connection.socket_udp,
            client->name, strlen(client->name),
            server_addr);
    if (!res) return NET_FAILURE;

    client->connection.state = CON_CONNECTED;
    return NET_SUCCESS;
}


void NetClient_Run(netclient_t* client){
    double now = plt_timemillis();
    double dt = (now - previous) / 1000.0f;
    previous = now;
    accum += dt;
    while (accum >= (1.0f / client->update_rate)){
        cl_recv(client);
        DOFUNC(client->func_run);
        accum -= (1.0f / client->update_rate);
    }
}





static void _handle_handshake_acc(netclient_t* client){
    client->connection.state = CON_CONNECTED;
    client->connection.chan.state = NETCHAN_CONNECTED;
}

static void cl_recv(netclient_t* client){
    char buff[NET_MAX_PACKET];

    for (;;){
        netpacket_t incoming = {0};
        netresult_size_t recvsize = 
            netchan_recv(
                    &client->connection.chan, 
    client->connection.socket_udp, 
                    buff, NET_MAX_PACKET,
                    &incoming);
        
        if (recvsize <= 0) break; // Error occured, add code to handle individually 
        
        switch(incoming.type){
            case NET_PACKET_HNDSHK_ACC:
                printf("Handshake accepted: %s\n", incoming.data);
                _handle_handshake_acc(client);
                break;

            default: break;
        }
    }
}
