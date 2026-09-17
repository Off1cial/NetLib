#ifndef NET_H
#define NET_H

#include "common/common.h"
#include "net/platform/netplatform.h"
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#define NET_MAX_PACKET 512
#define NET_MAX_STR 256

typedef u16 netpacktype_t;

#define NET_PACKET_HNDSHK_REQ 1
#define NET_PACKET_HNDSHK_ACC 2
#define NET_PACKET_HNDSHK_DEN 3
#define NET_PACKET_HNDSHK_ACK 4
#define NET_PACKET_NETCMD 5
#define NET_PACKET_NETSNAPSHOT 6
#define NET_PACKET_BROADCAST 7

typedef i16 netresult_size_t;
typedef u16 netlen_t;
#define NETSOCK_ISNULL(sock) (sock == NETSOCK_INVALID)

#define DOFUNC(func, ...) \
    do { \
        if ((func)) \
            (func)(__VA_ARGS__); \
    } while (0)


// Tells a client/server - "Receiving a set of data, the interesting section is of size 'netsize_t size'

#define NETERROR_UNKNOWNPEER -1
#define NETERROR_INVALIDSIZE -2
#define NETERROR_NULLDATA -3
#define NETERROR_INVALIDSOCKET -4

#define NETPORT_ANY 0

typedef struct {
    u32 sequence;
    netlen_t size; // size of corresponding data in the buffer
    netpacktype_t type;
}netpkthdr_t;

// Used on receiving end
typedef struct {
    netpacktype_t type;
    u32 sequence;

    //void* data;
    char data[NET_MAX_PACKET];
    netlen_t size;
} netpacket_t;

// Host side address data - automatically converted to network-side when used
typedef struct {
    u32 ip;
    u16 port;
} netaddr_t;

// Encapsulates a client's input to be sent to the server
typedef struct netcmd_t{
    size_t size;
    void* data;
} netcmd_t;

typedef struct netsnapshot_t{
    void *state;
    size_t size;

    u32 tick;
    u32 ack;
} netsnapshot_t;


static inline bool netaddr_equal(netaddr_t a, netaddr_t b){
    return (a.ip == b.ip) && (a.port == b.port);
}

static inline bool netaddr_isbroadcast(netaddr_t addr){
    return (addr.ip == INADDR_BROADCAST);
}

static inline const char* netaddr_to_string(netaddr_t addr, char* buff, size_t buffn){
    struct in_addr ia;
    ia.s_addr = htonl(addr.ip);
    snprintf(buff, buffn, "%s:%u", inet_ntoa(ia), addr.port);
    return buff;
}

netaddr_t netaddr_new(char* ip, u16 port);

static inline netaddr_t netaddr_newbroadcast(u16 port){
    return (netaddr_t){.ip = INADDR_BROADCAST, .port = port};
}

netsock_t netsock_create_udp(void);
netsock_t netsock_create_tcp(void);
void netsock_close(netsock_t sock);

netresult_t netsock_bind(netsock_t sock, netaddr_t addr);
netresult_t netsock_set_broadcast(netsock_t sock);

// For use on clients
netresult_t netsock_connect(netsock_t sock, netaddr_t addr);


/**
 *
 * @brief Sends 'n' bytes of data to the destination address
 *
 * @param sock source socket
 * @param dest host-side destination address
 */
netresult_size_t netsock_senddata(netsock_t sock, netaddr_t dest, char* data, size_t n);
/*
 * @brief Read 'n' bytes of data from 'sock' into output
 * @param who Pointer to the source address to fill
 */
netresult_size_t netsock_receive(netsock_t sock, char* output, size_t n, netaddr_t* who, netpacket_t* outpkt);


netresult_size_t netsock_sendpacket(
        netsock_t sock, 
        netaddr_t dest, 
        char* data, 
        size_t n, 
        netpacktype_t type);

#endif  
