#pragma once

#include "net/net.h"

typedef enum{
    NETCHAN_DISCONNECTED,
    NETCHAN_WAITING, // Handshake attempted, waiting for response
    NETCHAN_CONNECTED,
} netchanstate_t;

typedef struct {
    netchanstate_t state;
    netaddr_t remote;
    u32 out_sequence; // Increments on outgoing data
    u32 in_sequence; // Increments on incoming data
    u32 ack;        // Sequence number of last acknowledged packet
} netchan_t; // Net channel?


netresult_t netchan_connect(
        netchan_t* chan,
        netsock_t sock,
        netaddr_t destination
        );

netresult_size_t netchan_send(
        netchan_t* chan, 
        netsock_t sock, 
        netpacktype_t type, 
        const void* data, 
        size_t size);

netresult_size_t netchan_recv(
        netchan_t* chan, 
        netsock_t sock, 
        void* buff, 
        size_t buffsize, 
        netpacket_t* pkt);
