#ifndef NETCLIENT_H
#define NETCLIENT_H

#include "net/net.h"
#include "net/chan.h"

#define CON_TIMER 5.0F // Time to wait between handshake attempts
#define CON_ATTEMPTS 5 // Total number of handshake attempts to make, 0 - dont giveup until manual intervention (NickEh30)

typedef enum netclientstate_t
{
    NETC_STATE_IDLE, // Internet access, not trying to join a server
    NETC_STATE_ATTEMPTING, // Sending join handshakes
    NETC_STATE_JOINING, // Handshake accepted, waiting for server data / loading
    NETC_STATE_ACTIVE, // Playing, sending commands, receiving snapshots
} netclientstate_t;

typedef struct {
    netchan_t chan;
    netsock_t socket_udp;
} netconnection_t;

typedef struct netclient_t
{
    char name[NET_MAX_STR];
    netconnection_t connection;
    netclientstate_t cstate;
    
    // Configurables
    u32 update_rate; // 0 Until connected -> received server info packet(s)
    float interp;

    // Addon funcs, they add to default lib functionality, do not replace. (Client run loop calls this amongst its other routines)
    void (*func_run)(void);

    // timers
    double attempt_timer; // Time left for this handshake attempt
    double attempt_lasttime; // milliseconds
    u32 attempts_made;

} netclient_t;


netclient_t* NetClient_Init(const char* name, size_t namelen);


// Forms the connection for communication - unrelated to joining a game server
//netresult_t NetClient_ConnectAttempt(netclient_t* client, netaddr_t server_addr);
void NetClient_ConnectServer(netclient_t* client, netaddr_t server_addr);
void NetClient_Run(netclient_t* client);
#endif
