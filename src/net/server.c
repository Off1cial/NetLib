#include "net/server.h"
#include "net/chan.h"
#include "net/net.h"
#include "common/plt_time.h"
#include "net/platform/netplatform.h"
#include "net/readwrite.h"
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

clientid_t _id_clientaddr(netserver_t *server, netaddr_t addr){
    for (u32 i = 0; i < server->client_count; i++){
        if (netaddr_equal(addr, server->clients[i].chan.remote)){
            return i;
        }
    }
    return CLIENT_UNKNOWN;
}   

int _extract_netcmd(char* buff, size_t buff_size, netcmd_t* out){
    if (buff_size < sizeof(netcmd_t)){
        return 0;
    }
    netcmd_t cmd = ((netcmd_t*)buff)[0];
    *out = cmd;
    return 1;
}


net_svclient_t* add_client(netserver_t *server, char* name, netaddr_t addr){
    if (server->client_count >= server->client_limit){
        return NULL;
    }
    net_svclient_t client = {0};
    client.chan.remote = addr;
    client.chan.state = NETCHAN_CONNECTED;
    //strncpy(client.name, name, NET_MAX_STR);
    
    u32 id = server->client_count;
    server->clients[id] = client;
    client.state = CL_CONNECTED;
    //server->func_client_init(&server->clients[id]);
    DOFUNC(server->func_client_init, &server->clients[id]);
    server->client_count++;
    return &server->clients[id];
}

void remove_client(netserver_t* server, net_svclient_t* client){ 
    if (!server || !client) 
        return;

    //server->func_client_remove(client);
    DOFUNC(server->func_client_remove, client);
    memset(client, 0, sizeof(net_svclient_t));
    server->client_count--;

}



static void _handle_client_unknown(netserver_t* server, char* name, size_t n, netaddr_t addr){
    net_svclient_t* client = add_client(server, name, addr);

    if (!client){
        printf("Failed to add client\n");
        // Send handshake denial packet
        return;
    }
    // Send acception packet
    char data[] = "Hello, client!\0";
    size_t len = strlen(data) + 1;
    netchan_send(
        &client->chan, 
        server->socket_udp, 
        NET_PACKET_HNDSHK_ACC, 
        data, len
    );
    printf("Client '%s' added\n", name);
}

static void sv_recv(netserver_t* server){
    char buff[NET_MAX_PACKET];
    for (;;){
        
        netaddr_t fromaddr = {0};
        netpacket_t packet = {0};
        clientid_t client_id = -1;
    
        netresult_size_t recvsize = 
            netsock_receive(server->socket_udp, buff, NET_MAX_PACKET, &fromaddr, &packet);
        if (recvsize <= 0) break;
        
        printf("Received %dB, type %d: ", recvsize, packet.type);
        // Identify client
        client_id = _id_clientaddr(server, fromaddr);
        if (client_id == CLIENT_UNKNOWN){
            // New client
            printf("New client\n");
            if (packet.type == NET_PACKET_HNDSHK_REQ){
                _handle_client_unknown(server, packet.data, packet.size, fromaddr);
            }
            continue;
        } 

        // Known client
        printf("Known client\n");
        continue;
    }

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

netresult_size_t NetServer_Broadcast(netserver_t* server, void* data, size_t datalen){
    size_t buffsize = datalen + NETPKT_HDR_SIZE;
    char buff[buffsize];
   
    netpkthdr_t header = {
        .size = datalen,
        .type = NET_PACKET_BROADCAST,
        .sequence = 0
    };
    size_t pos = 0;
    _write_header(buff, &pos, &header);
    memcpy(buff + pos, data,  datalen);
    return netsock_senddata(server->socket_broadcast, server->broadcast_addr, buff, buffsize);
}


netserver_t* NetServer_Init(int client_limit, uint32_t tickrate, u16 port, u16 broadcast_port){
    netserver_t* server = calloc(1, sizeof(netserver_t));
    memset(server, 0, sizeof(netserver_t));
    server->clients = calloc(client_limit, sizeof(net_svclient_t)); 
    server->client_limit = client_limit;
    server->tickrate = tickrate;
    //server->local_addr.port = port;
    //server->local_addr.ip = 0;
    server->local_addr = netaddr_new("0.0.0.0", port);
    //server->broadcast_addr = netaddr_newbroadcast(broadcast_port);
    server->broadcast_addr = netaddr_new("127.0.0.1", broadcast_port);
    server->socket_udp = netsock_create_udp(); 
    server->socket_broadcast = netsock_create_udp();
    netsock_set_broadcast(server->socket_broadcast);
    if (!netsock_bind(server->socket_udp, server->local_addr)){
        fprintf(stderr, "Failed to bind server socket\n");
        netsock_close(server->socket_udp);
        netsock_close(server->socket_broadcast);
        free(server->clients);
        free(server);
        return NULL;
    }
    /*
    if (!netsock_bind(server->socket_broadcast, server->broadcast_addr)){
        fprintf(stderr, "Failed to bind server broadcast socket\n");
        netsock_close(server->socket_udp);
        netsock_close(server->socket_broadcast);
        free(server->clients);
        free(server);
        return NULL;
    }
    */


    previous = plt_timemillis();
    char hostname[256];
    char hostip[256];
    gethostname(hostname, 256);
    struct hostent* host = gethostbyname(hostname);
    strcpy(hostip, inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));
    printf("[NET]: %dHz Server %s:%d\n", server->tickrate, hostip, port);
    return server;
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
