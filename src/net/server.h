#pragma once

#include "net/net.h"
#include "net/chan.h"

#define CLIENT_UNKNOWN -1
typedef i32 clientid_t;

typedef enum {
    CL_FREE = 0,
    CL_WAITING_HNDSHK_ACK,
    CL_ZOMBIE,
    CL_CONNECTED,
    CL_ACTIVE,
} netclientstate_t;

typedef struct {
    char name[NET_MAX_STR];
    netchan_t chan;
    u32 ticks_elapsed;
    netclientstate_t state;
} net_svclient_t;

typedef struct {
    net_svclient_t* clients;
    uint32_t client_count; // Note: always iterate client_limit? (clients can have gaps?)
    uint32_t client_limit;

    // Receive functions
    void (*func_process_netcmd)(netcmd_t cmd);
    
    // Other functions that run within pre-defined functions 
    void (*func_run)(void);
    // NOTE: THESE ADD FUNCTIONALITY, THEY DO NOT REPLACE
    int  (*func_client_init)(net_svclient_t* client);
    void (*func_client_remove)(net_svclient_t* client);
    void (*func_shutdown)(void);

    // Net data
    netaddr_t net_addr;
    netaddr_t broadcast_addr;
    netsock_t socket_udp;
    netsock_t socket_broadcast;
    
    // Configurables
    uint32_t tickrate;

    double broadcast_interval;

} netserver_t;

netserver_t* NetServer_Init(int client_limit, uint32_t tickrate, u16 port, u16 broadcast_port);
void NetServer_Shutdown(netserver_t* server);
void NetServer_Run(netserver_t* server);



netresult_size_t NetServer_Broadcast(netserver_t* server, void* data, size_t datalen);
