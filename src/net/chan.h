#pragma once

#include "net/net.h"

typedef enum{ // Preserve this order
    NETCHAN_DISCONNECTED = 0,
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

netresult_t netchan_connect(
        netchan_t* chan,
        netsock_t sock,
        void* intro_data,
        size_t datasize,
        netaddr_t destination);

