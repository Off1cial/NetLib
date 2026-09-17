#include "net/client.h"
#include "net/chan.h"
#include "net/net.h"
#include "net/platform/netplatform.h"
#include "common/plt_time.h"
#include "net/readwrite.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

// Timing variables for consistent update rate
static double accum = 0.0;
static double previous = 0.0;

static void cl_recv(netclient_t* client);
static void cl_recv_broadcast(netclient_t* client);

netclient_t* NetClient_Init(const char* name, size_t namelen, u16 broadcast_port){
    netclient_t* client = calloc(1, sizeof(netclient_t)); 
    if (!client){
        return NET_NULL;
    }
    client->connection.socket_udp = -1;
    strncpy(client->name, name, namelen + 1);

    client->connection.socket_udp = netsock_create_udp();
    client->socket_broadcast = netsock_create_udp();
    netsock_set_broadcast(client->socket_broadcast);
    netaddr_t broadcast_addr = (netaddr_t){.ip = 0, .port = broadcast_port};
    if (!netsock_bind(client->socket_broadcast, broadcast_addr)){
        netsock_close(client->connection.socket_udp);
        netsock_close(client->socket_broadcast);
        free(client);
        return NET_NULL;
    }

    previous = plt_timemillis();
    client->cstate = NETC_STATE_IDLE;
    return client;
}

// Forms the connection for communication - unrelated to joining a game server
netresult_t NetClient_ConnectAttempt(netclient_t* client, netaddr_t server_addr){
        
    memset(&client->connection.chan, 0, sizeof(netchan_t));
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
    
    char ipbuff[256];
    printf("Attempting connection to %s\n", netaddr_to_string(server_addr, ipbuff, 256));
    client->cstate = NETC_STATE_ATTEMPTING;
    return NET_SUCCESS;
}

static inline void _clear_channel(netchan_t* chan){
    if(chan) memset(chan, 0 , sizeof(netchan_t));
}

static void _handle_attempts(netclient_t* client, netaddr_t server_addr){
    if (client->attempts_made >= CON_ATTEMPTS){
        client->attempts_made = 0;
        client->attempt_timer = 0.0;
        client->cstate = NETC_STATE_IDLE;
        _clear_channel(&client->connection.chan);
        printf("Connection failed after %d retries\n", CON_ATTEMPTS - 1);
        return;
    }
    double now = plt_timemillis();
    // Seconds since last attempt
    double time_since_last = (now - client->attempt_lasttime) / 1000.0;
    if (time_since_last < CON_TIMER)
        return;
    
    NetClient_ConnectAttempt(client, server_addr);
    client->attempts_made++;
    client->attempt_lasttime = now;
}

void NetClient_ConnectServer(netclient_t* client, netaddr_t server_addr){
    client->attempt_lasttime = 0.0;
    client->attempts_made = 0;
    client->connection.chan.remote = server_addr;
    client->cstate = NETC_STATE_ATTEMPTING;
}

void NetClient_Run(netclient_t* client){
    double now = plt_timemillis();
    double dt = (now - previous) / 1000.0f;
    previous = now;
    accum += dt;
    while (accum >= (1.0f / client->update_rate)){
        printf("STATE= %d\n", client->cstate);
        if (client->cstate == NETC_STATE_IDLE){
            cl_recv_broadcast(client);
        }
        cl_recv(client);
        switch(client->cstate){
            case NETC_STATE_ATTEMPTING:
                _handle_attempts(client, client->connection.chan.remote);
                break;
            case NETC_STATE_JOINING:
                printf("Joining...\n");
                break;
            case NETC_STATE_ACTIVE:
                printf("Active\n");
                break;
            default: break;
        }

        DOFUNC(client->func_run);
        accum -= (1.0f / client->update_rate);
    }
}





static void _handle_handshake_acc(netclient_t* client){
    client->connection.chan.state = NETCHAN_CONNECTED;
    client->cstate = NETC_STATE_JOINING;
}



static void cl_recv_broadcast(netclient_t* client){
    char buff[NET_MAX_PACKET];
    printf("recvbroadcast\n");
    for (;;){
        netpacket_t brdcst = {0};
        netaddr_t from = {0};
        netresult_size_t recvsize = 
            netsock_receive(
                    client->socket_broadcast, 
                    buff, 
                    NET_MAX_PACKET, &from, &brdcst);
        if (recvsize <= 0) break;
    
        if (brdcst.type != NET_PACKET_BROADCAST) break;
        int i = 0;
        for(char *c=buff; i < recvsize; c++, i++){
            printf("%c", *c);
        }
        putchar(10);



        // For now, automate a join attempt to the server
        // Assuming the source is a game server?
        size_t ip_pos = 0;
        netaddr_t server_addr = _read_netaddr(buff, &ip_pos);
        NetClient_ConnectServer(client, server_addr); 
        break;
    }
}

static void cl_recv(netclient_t* client){
    char buff[NET_MAX_PACKET];
    printf("recv\n");
    for (;;){
        netpacket_t incoming = {0};
        netresult_size_t recvsize = 
            netchan_recv(
                    &client->connection.chan, 
                    client->connection.socket_udp, 
                    buff, NET_MAX_PACKET,
                    &incoming);
        
        if (recvsize <= 0) {
            break; // Error occured, add code to handle individually 
        }
        switch(incoming.type){
            case NET_PACKET_HNDSHK_ACC:
                printf("Handshake accepted: %s\n", incoming.data);
                _handle_handshake_acc(client);
                break;
            case NET_PACKET_HNDSHK_DEN:
                printf("Handshake denied: %s\n", incoming.data);
                break;

            default: break;
        }
    }
}
