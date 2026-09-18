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

/*
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
}*/

void remove_client(netserver_t* server, net_svclient_t* client){ 
    if (!server || !client) 
        return;

    //server->func_client_remove(client);
    DOFUNC(server->func_client_remove, client);
    memset(client, 0, sizeof(net_svclient_t));
    client->state = CL_FREE;
    server->client_count--;

}

net_svclient_t* alloc_client(netserver_t* server){
    if (server->client_count >= server->client_limit) return NULL;

    for (u32 i = 0; i < server->client_limit; i++){
        net_svclient_t* cl = &server->clients[i];
        if (cl->state == CL_FREE) return cl;
    }
    return NULL;
}

static void _send_hndshk_denial(netserver_t* server, netaddr_t addr){
    char msg[] = "Connection refused: server is full\0";
    size_t len = strlen(msg) + 1;
    netsock_sendpacket(
            server->socket_udp,
            addr,
            msg, len, NET_PACKET_HNDSHK_DEN
            );
}


void _add_client(
    netserver_t* server,
    net_svclient_t* client,
    char* name,
    size_t namelen,
    netaddr_t addr)
{
    client->state = CL_CONNECTED;
    client->chan.state = NETCHAN_CONNECTED;
    client->chan.remote = addr;
    client->ticks_elapsed = 0;

    size_t len = namelen;
    if (len >= sizeof(client->name))
        len = sizeof(client->name) - 1;

    memcpy(client->name, name, len);
    client->name[len] = '\0';

    DOFUNC(server->func_client_init, client);
    server->client_count++;
}

static void _handle_client_unknown(netserver_t* server, char* name, size_t n, netaddr_t addr){
    net_svclient_t* client = alloc_client(server); // Potential slot

    if (!client){
        printf("Failed to add client, server full\n");
        // Send handshake denial packet
        _send_hndshk_denial(server, addr);
        return;
    }
    // Send acception packet
    _add_client(server, client, name, n, addr);
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
static double previous_tick = 0.0;
void sv_run(netserver_t *server){
    double now =  plt_timemillis();
    double dt = (now - previous_tick) / 1000.0f;
    previous_tick = now;
    accum += dt;
    
    while (accum >= (1.0f / server->tickrate)){
        //if(server->func_run) server->func_run();
        sv_recv(server);
        DOFUNC(server->func_run);
        accum -= (1.0f / server->tickrate);
    }
}

static double previous_broadcast = 0.0;
netresult_size_t NetServer_Broadcast(netserver_t* server, void* data, size_t datalen){
    double now = plt_timemillis();
    double since_broadcast = (now - previous_broadcast) / 1000.0f;
    if (since_broadcast < server->broadcast_interval)
        return 0;
    
    previous_broadcast = now;
    size_t metasize = NETPKT_HDR_SIZE + NETADDR_SIZE;
    if (NET_MAX_PACKET - datalen < metasize)
        return NETERROR_INVALIDSIZE;
    size_t buffsize = datalen + NETPKT_HDR_SIZE;
    char buff[buffsize];
   
    netpkthdr_t header = {
        .size = datalen,
        .type = NET_PACKET_BROADCAST,
        .sequence = 0
    };
    size_t pos = 0;
    _write_header(buff, &pos, &header);
    _write_netaddr(buff, &pos, server->net_addr);
    memcpy(buff + pos, data,  datalen);
    return netsock_senddata(server->socket_broadcast,server->broadcast_addr, buff, buffsize);
}


netserver_t* NetServer_Init(int client_limit, uint32_t tickrate, u16 port, u16 broadcast_port){
    netserver_t* server = calloc(1, sizeof(netserver_t));
    memset(server, 0, sizeof(netserver_t));
    server->clients = calloc(client_limit, sizeof(net_svclient_t)); 
    server->client_limit = client_limit;
    server->tickrate = tickrate;
    server->broadcast_addr =  netaddr_newmulticast(broadcast_port);
    server->net_addr = netaddr_getnet(port);
    server->socket_udp = netsock_create_udp(); 
    server->socket_broadcast = netsock_create_udp();
    server->broadcast_interval = 3.0f;


    int one = 1;
    struct in_addr mcast_iface;
    mcast_iface.s_addr = htonl(server->net_addr.ip);

    netsock_setopt_ip(server->socket_broadcast, NETSOCKOPT_MULTICAST_IF,
                  &mcast_iface, sizeof(mcast_iface));
    netsock_setopt_ip(server->socket_broadcast, NETSOCKOPT_MULTICAST_LOOP,
                  &one, sizeof(one));

    // Socket-level options take an int.
    netsock_setopt(server->socket_broadcast, NETSOCKOPT_BROADCAST,
               &one, sizeof(one));
    netsock_setopt(server->socket_broadcast, NETSOCKOPT_REUSEADDR,
               &one, sizeof(one));

    if (!netsock_bind(server->socket_udp, netaddr_newany(port))){
        fprintf(stderr, "Failed to bind server socket\n");
        netsock_close(server->socket_udp);
        netsock_close(server->socket_broadcast);
        free(server->clients);
        free(server);
        return NULL;
    }


    previous_tick = plt_timemillis();
    previous_broadcast = previous_tick;
    char hostip[256], broadcastip[256];
    netaddr_to_string(server->net_addr, hostip, 256);
    netaddr_to_string(server->broadcast_addr, broadcastip, 256);


    printf("[NET]: %dHz Server %s\n", server->tickrate, hostip);
    printf("[NET]: Broadcast %s\n", broadcastip);
    return server;
}

void NetServer_Shutdown(netserver_t* server){
    // Broadcast closing packet with msg
    DOFUNC(server->func_shutdown);
    free(server->clients);
    netsock_close(server->socket_udp);
    memset(server, 0, sizeof(netserver_t));
    free(server);
}

void NetServer_Run(netserver_t* server){
    sv_run(server);
}
