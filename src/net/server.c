#include "net/server.h"
#include "net/net.h"
#include "common/plt_time.h"
#include "net/platform/netplatform.h"
#include <netinet/in.h> 
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>


static inline 
netaddr_t _sockaddr_to_netaddr(struct sockaddr_in addr){
    return (netaddr_t){.port = addr.sin_port, .ip = addr.sin_addr.s_addr};
}

int _id_clientaddr(netserver_t *server, netaddr_t addr){
    for (u32 i = 0; i < server->client_count; i++){
        if (netaddr_equal(addr, server->clients[i].chan.remote)){
            return i;
        }

    }
    return -1;
}   

int _extract_netcmd(char* buff, size_t buff_size, netcmd_t* out){
    if (buff_size < sizeof(netcmd_t)){
        out->valid = 0;
        return 0;
    }
    netcmd_t cmd = ((netcmd_t*)buff)[0];
    cmd.valid = 1;
    *out = cmd;
    return 1;
}


int add_client(netserver_t *server, char* name, netaddr_t addr){
    if (server->client_count >= server->client_limit){
        return 0;
    }
    net_svclient_t client = {0};
    client.chan.remote = addr;
    strncpy(client.name, name, NET_MAX_STR);
    
    u32 id = server->client_count;
    server->clients[id] = client;
    client.state = CL_CONNECTED;
    //server->func_client_init(&server->clients[id]);
    DOFUNC(server->func_client_init, &server->clients[id]);
    server->client_count++;

    return 1;
}

void remove_client(netserver_t* server, net_svclient_t* client){ 
    if (!server || !client) 
        return;

    //server->func_client_remove(client);
    DOFUNC(server->func_client_remove, client);
    memset(client, 0, sizeof(net_svclient_t));
    server->client_count--;

}


static void sv_recv(netserver_t* server){

    char buff[NET_MAX_PACKET];
    netsize_t size = 0;

    netaddr_t from;

    size = netsock_receive(server->socket_udp, buff, NET_MAX_PACKET, &from);
    printf("Received %zd byte(s)\n", size);
    
}

static double accum = 0.0;
static double previous = 0.0;
void sv_run(netserver_t *server){
    double now =  plt_timemillis();
    double dt = (now - previous) / 1000.0f;
    previous = now;
    accum += dt;
    
    while (accum >= (1.0f / server->tickrate)){
        //if(server->func_run) server->func_run();
        sv_recv(server);
        DOFUNC(server->func_run);
        accum -= (1.0f / server->tickrate);
    }
}


int NetServer_Init(netserver_t* server, int client_limit, uint32_t tickrate, u16 port){
    if (!server) 
        return NET_SUCCESS;
    memset(server, 0, sizeof(netserver_t));
    server->clients = calloc(client_limit, sizeof(net_svclient_t)); 
    server->tickrate = tickrate;
    server->local_addr.port = port;
    server->local_addr.ip = 0;
    server->socket_udp = netsock_create_udp(); 
    if (!netsock_bind(server->socket_udp, server->local_addr)){
        fprintf(stderr, "Failed to bind server socket\n");
        return NET_FAILURE;
    }
    previous = plt_timemillis();
    char hostname[256];
    char hostip[256];
    gethostname(hostname, 256);
    struct hostent* host = gethostbyname(hostname);
    strcpy(hostip, inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));
    printf("[NET]: %dHz Server %s:%d\n", server->tickrate, hostip, port);

    return NET_SUCCESS;
}

void NetServer_Shutdown(netserver_t* server){
    // Broadcast closing packet with msg
    DOFUNC(server->func_shutdown);
    free(server->clients);
    netsock_close(server->socket_udp);
    memset(server, 0, sizeof(netserver_t));
}

void NetServer_Run(netserver_t* server){
    sv_run(server);
}
