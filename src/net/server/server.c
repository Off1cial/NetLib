#include "net/server/server.h"
#include "common/common.h"
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


static double accum = 0.0;
static double previous_tick = 0.0;

clientid_t _id_clientaddr(netserver_t *server, netaddr_t addr){
    for (u32 i = 0; i < server->client_limit; i++){
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

void remove_client(netserver_t* server, net_svclient_t* client){
    if (!server || !client)
        return;

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
    char msg[] = "Connection refused: server is full";
    size_t len = strlen(msg) + 1;
    netsock_sendpacket(
            server->socket_udp,
            addr,
            msg, len, NET_PACKET_HNDSHK_DEN
            );
}


static void _send_hndshk_accept(netserver_t* server, netaddr_t addr){
    char msg[] = "Connection accepted, server is awaiting ack";
    size_t len = strlen(msg) + 1;
    netsock_sendpacket(
            server->socket_udp,
            addr,
            msg, len, NET_PACKET_HNDSHK_ACC
            );
}

// Promotes a temporary/known client into a fully-named connected client.
// (Not currently called anywhere yet -- wire this up once the handshake
// ACK path below actually has a name to promote with. Left in place so
// the fix compiles and the intent is preserved from the original file.)
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

static void _client_setname(net_svclient_t* client, char* name, size_t namelen){
    size_t len = (namelen >= NET_MAX_STR) ? (NET_MAX_STR - 1) : namelen; 
    memcpy(client->name, name, len);
    client->name[len] = '\0';
}

// A stripped 'add_client'
static net_svclient_t* _temp_client(netserver_t* server, char* name, size_t namelen, netaddr_t cl_addr){
    net_svclient_t* client = alloc_client(server);
    if (!client) return NET_NULL;
    _client_setname(client, name, namelen);
    client->state = CL_TEMPORARY;
    client->chan.state  = NETCHAN_CONNECTED;
    client->chan.remote = cl_addr;
    client->chan.t_lastrecv_ms = plt_timemillis();
    server->client_count++;
    return client;
}

// The temporary client responsed, let them join
static void _promote_client(netserver_t* server, net_svclient_t* client){
    ASSERT(NULL != server, "Cannot promote client without server");
    ASSERT(NULL != client, "Cannot promote null client\n");
    // Send server cvars, extra information that wasnt in the broadcast
    // ....
    client->state = CL_CONNECTED; // Not active yet, wait for the client to finish loading
}

static void _handle_client_unknown(
        netserver_t* server,
        char* name, size_t namelen,
        netaddr_t addr
        )
{
    // Temporarily allocate a client
    net_svclient_t* client = _temp_client(server, name, namelen, addr);
    if (!client){
        _send_hndshk_denial(server, addr);
        return;
    }
    // NOTE: 'name'/'namelen' aren't stored anywhere yet -- previously they
    // were silently dropped on the floor here too. Once the ACK path calls
    // _add_client, thread name/namelen through (e.g. stash them on the temp
    // client) so the promotion has something to copy in.
    (void)name;
    (void)namelen;
    _send_hndshk_accept(server, addr);
    // HANDLE A CHAN TIMEOUT ELSEWHERE
}

// Single reader of server->socket_udp. Handshakes for unknown addresses are
// handled inline; packets from already-known clients are dispatched here too
// (rather than being handed to a second function that reads the same socket
// again -- that was the bug: sv_recv_clients() used to call netchan_recv()
// on socket_udp expecting to see packets that sv_recv() had *already*
// drained and discarded this same tick, so known-client traffic, including
// the handshake ACK, was never actually processed).
static void sv_recv(netserver_t* server){
    char buff[NET_MAX_PACKET];
    for (;;){

        netaddr_t fromaddr = {0};
        netpacket_t packet = {0};
        clientid_t client_id = -1;

        netresult_size_t recvsize =
            netsock_receive(server->socket_udp, buff, NET_MAX_PACKET, &fromaddr, &packet);
        if (recvsize <= 0) break;

        printf("Received %ldB, type %d: ", (long)recvsize, packet.type);
        // Identify client
        client_id = _id_clientaddr(server, fromaddr);
        if (client_id == CLIENT_UNKNOWN){
            // New client
            printf("New client\n");
            switch(packet.type){
                case NET_PACKET_HNDSHK_REQ:
                    _handle_client_unknown(server, packet.data, packet.size, fromaddr);
                    break;
            }
            continue;
        }

        // Known client -- process the packet we just read instead of
        // discarding it and hoping something else reads it later.
        printf("Known client\n");
        net_svclient_t* client = &server->clients[client_id];
        client->chan.t_lastrecv_ms = plt_timemillis();
        client->chan.in_sequence = 1;

        switch (packet.type){
            case NET_PACKET_HNDSHK_ACK:
                printf("Received ack\n");
                _promote_client(server, client);
                break;
            default:
                break;
        }
    }
}

// Timeout / housekeeping pass only. Does NOT read from socket_udp -- all
// socket reads happen once, in sv_recv() above.
static void sv_handle_clients(netserver_t* server){
    // server->clients can have gaps, iterate limit
    for (u32 i = 0; i < server->client_limit; i++){
        net_svclient_t* client = &server->clients[i];
        if (client->state == CL_FREE) continue;

        double dt = (plt_timemillis() - client->chan.t_lastrecv_ms) / 1000.0f;

        // Temporary (mid-handshake) clients must be able to time out even
        // before any packet has flowed through in_sequence, otherwise a
        // client that never sends its ACK permanently occupies a slot.
        if (client->state == CL_TEMPORARY){
            if (dt >= server->client_timeout * 10){
                printf("Temp client %d timed out\n", i);
                remove_client(server, client);
            }
            continue;
        }
        if ((dt >= server->client_timeout) && client->chan.in_sequence){
            printf("Connected/active client timeout\n");
            remove_client(server, client);
            continue;
        }
    }
}


void sv_run(netserver_t *server){
    double now =  plt_timemillis();
    double dt = (now - previous_tick) / 1000.0f;
    previous_tick = now;
    accum += dt;

    while (accum >= (1.0f / server->tickrate)){
        sv_recv(server);
        sv_handle_clients(server);
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
    size_t metasize = NETPKT_HDR_SIZE + NETADDR_SIZE + sizeof(u32) * 3;

    // datalen is unbounded caller input. The old check
    // (NET_MAX_PACKET - datalen < metasize) underflows to a huge positive
    // number when datalen > NET_MAX_PACKET, silently passing validation,
    // right before a VLA sized by (datalen + NETPKT_HDR_SIZE) was stack
    // allocated -- i.e. a caller-controlled stack overflow. Rewritten to
    // avoid the unsigned underflow, and the VLA is replaced with a fixed
    // buffer.
    if (datalen > NET_MAX_PACKET || datalen > (size_t)(NET_MAX_PACKET) - metasize)
        return NETERROR_INVALIDSIZE;

    char buff[NET_MAX_PACKET];
    size_t buffsize = datalen + metasize;

    netpkthdr_t header = {
        .size = datalen,
        .type = NET_PACKET_BROADCAST,
        .sequence = 0
    };
    size_t pos = 0;
    _write_header(buff, &pos, &header);
    _write_netaddr(buff, &pos, server->net_addr);
    _write_u32(buff, &pos, server->tickrate);
    _write_u32(buff, &pos, server->client_count);
    _write_u32(buff, &pos, server->client_limit);
    memcpy(buff + pos, data, datalen);
    return netsock_senddata(server->socket_broadcast, server->broadcast_addr, buff, buffsize);
}


netserver_t* NetServer_Init(int client_limit, uint32_t tickrate, u16 port, u16 broadcast_port){
    netserver_t* server = calloc(1, sizeof(netserver_t));
    if (!server) return NULL;

    server->clients = calloc(client_limit, sizeof(net_svclient_t));
    if (!server->clients){
        free(server);
        return NULL;
    }
    server->client_limit = client_limit;
    server->tickrate = tickrate;
    server->broadcast_addr =  netaddr_newmulticast(broadcast_port);
    server->net_addr = netaddr_getnet(port);
    server->socket_udp = netsock_create_udp();
    server->socket_broadcast = netsock_create_udp();
    server->broadcast_interval = 3.0f;
    server->client_timeout = 10.0f;

    int one = 1;
    struct in_addr mcast_iface;
    mcast_iface.s_addr = htonl(server->net_addr.ip);

    if (!netsock_setopt_ip(server->socket_broadcast, NETSOCKOPT_MULTICAST_IF,
                  &mcast_iface, sizeof(mcast_iface))){
        fprintf(stderr, "Warning: failed to set multicast interface\n");
    }
    if (!netsock_setopt_ip(server->socket_broadcast, NETSOCKOPT_MULTICAST_LOOP,
                  &one, sizeof(one))){
        fprintf(stderr, "Warning: failed to set multicast loop\n");
    }

    // Socket-level options take an int.
    if (!netsock_setopt(server->socket_broadcast, NETSOCKOPT_BROADCAST,
               &one, sizeof(one))){
        fprintf(stderr, "Warning: failed to set SO_BROADCAST\n");
    }
    if (!netsock_setopt(server->socket_broadcast, NETSOCKOPT_REUSEADDR,
               &one, sizeof(one))){
        fprintf(stderr, "Warning: failed to set SO_REUSEADDR\n");
    }

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
    if (!server) return;
    // Broadcast closing packet with msg
    DOFUNC(server->func_shutdown);
    free(server->clients);
    netsock_close(server->socket_udp);
    netsock_close(server->socket_broadcast); // was previously leaked
    memset(server, 0, sizeof(netserver_t));
    free(server);
}

void NetServer_Run(netserver_t* server){
    sv_run(server);
}
