#ifndef NET_H
#define NET_H

#include "common/common.h"
#include "net/platform/netplatform.h"

#define NET_MAX_PACKET 512
#define NET_MAX_STR 256

typedef uint16_t netpacktype_t;
#define NET_PACKET_NETCMD 1
#define NET_PACKET_NETSNAPSHOT 2

#define NETSOCK_ISNULL(sock) (sock == NETSOCK_INVALID)

typedef size_t netsize_t;

// Host side address data - automatically converted to network-side when used
typedef struct {
    u32 ip;
    u16 port;
} netaddr_t;

// Encapsulates a client's input to be sent to the server
typedef struct netcmd_t{
    size_t size;
    void* data;
    u32 sequence;
    u8 valid; // Server-side validation
} netcmd_t;

typedef struct netsnapshot_t{
    void *state;
    size_t size;

    u32 tick;
    u32 ack;
} netsnapshot_t;

typedef struct {
   netaddr_t remote;
} netchan_t; // Net channel?

static inline bool netaddr_equal(netaddr_t a, netaddr_t b){
    return (a.ip == b.ip) && (a.port == b.port);
}

netsock_t netsock_create_udp(void);
netsock_t netsock_create_tcp(void);
void netsock_close(netsock_t sock);

netresult_t netsock_bind(netsock_t sock, netaddr_t addr);

// For use on clients
netresult_t netsock_connect(netsock_t sock, netaddr_t addr);


/**
 *
 * @brief Sends 'n' bytes of data to the destination address
 *
 * @param sock source socket
 * @param dest host-side destination address
 */
netsize_t netsock_senddata(netsock_t sock, netaddr_t dest, char* data, netsize_t n);
/*
 * @brief Read 'n' bytes of data from 'sock' into output
 * @param who Pointer to the source address to fill
 */
netsize_t netsock_receive(netsock_t sock, char* output, netsize_t n, netaddr_t* who);

#endif  
