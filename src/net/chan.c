#include "net/chan.h"
#include "common/plt_time.h"
#include "net/net.h"
#include "net/readwrite.h"
#include "net/platform/netplatform.h"
#include "common/common.h"
#include <string.h>
#include <stdio.h>

netchan_t netchan_new(netaddr_t remote){
    netchan_t chan = {0};
    chan.remote = remote;
    return chan;
}

void netchan_setremote(netchan_t* chan, netaddr_t remote){
    ASSERT(NULL != chan, "Attempted to set the remote of a null channel");
    chan->remote = remote;
}

void netchan_updatetime(netchan_t* chan, double* time_ms){
    ASSERT(chan && time_ms, "Attempted to set the time of a null channel/time");
    *time_ms = plt_timemillis();
}


void netchan_updatesequence_in(netchan_t* chan){
    ASSERT(NULL != chan, "Attempted to update the sequence of a null channel");
    chan->in_sequence_bits *= 2;
    chan->in_sequence++;
    netchan_updatetime(chan, &chan->t_lastrecv_ms);
}

void netchan_updatesequence_out(netchan_t* chan){
    ASSERT(NULL != chan, "Attempted to update the sequence of a null channel");
    chan->out_sequence++;
    netchan_updatetime(chan, &chan->t_lastsend_ms);
}

netresult_size_t netchan_send(
        netchan_t* chan, 
        netsock_t sock,
        netpacktype_t type, 
        const void* data, 
        size_t size){
    if (!data || !chan || (size <= 0)) return NETERROR_NULLDATA;
    if (sock == NETSOCK_INVALID) return NETERROR_INVALIDSOCKET;
    
    size_t buffsize = NETPKT_HDR_SIZE + size;
    if (buffsize > NET_MAX_PACKET) return NETERROR_INVALIDSIZE;

    netpkthdr_t header = {
        .size = size,
        .type = type,
        .sequence = chan->out_sequence + 1
    };

    char buff[buffsize]; 
    size_t pos = 0;
    _write_header(buff, &pos, &header);
    memcpy(buff + pos, data, size);

    size_t sent = netsock_senddata(sock, chan->remote, buff, buffsize);
    if (sent <= 0) return sent;


    netchan_updatesequence_out(chan);
    return sent;
}

netresult_size_t netchan_recv(
        netchan_t* chan, 
        netsock_t sock,
        void* buff, 
        size_t buffsize, 
        netpacket_t* pkt){
    if (!chan || !buff || !pkt) return NETERROR_NULLDATA;
    if (buffsize <= 0) return NETERROR_INVALIDSIZE;
    if (sock == NETSOCK_INVALID) return NETERROR_INVALIDSOCKET;
    
    netaddr_t from;

    size_t recsize = netsock_receive(sock, buff, buffsize, &from, pkt);
    if (recsize <= 0 )
        return recsize;

    if ((pkt->type != NET_PACKET_BROADCAST) && !netaddr_equal(from, chan->remote)){
        return NETERROR_UNKNOWNPEER;
    } 
    netchan_updatesequence_in(chan);
    
    return recsize;
}

netresult_t netchan_connect(
        netchan_t* chan,
        netsock_t sock,
        void* intro_data,
        size_t datasize,
        netaddr_t destination
        )
{
    if (!chan) return NETERROR_NULLDATA;    
    if (sock == NETSOCK_INVALID) return NETERROR_INVALIDSOCKET;

    //netresult_t res = netsock_connect(sock, destination);
    //if (!res) return NET_FAILURE;
    chan->remote = destination;
    chan->ack = 0;
    chan->in_sequence = 0;
    netresult_size_t rs = netchan_send(
            chan, 
            sock, 
            NET_PACKET_HNDSHK_REQ, 
            intro_data, 
            datasize);
    if (rs <= 0){
        chan->state = NETCHAN_DISCONNECTED;
        return NET_FAILURE;
    }
    //printf("Sent handshake (%dB), waiting..\n", rs);
    chan->state = NETCHAN_WAITING;
    return NET_SUCCESS;
}

