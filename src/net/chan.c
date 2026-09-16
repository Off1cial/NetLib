#include "net/chan.h"
#include "net/net.h"
#include "net/readwrite.h"
#include "net/platform/netplatform.h"
#include <string.h>
#include <stdio.h>


netresult_size_t netchan_send(
        netchan_t* chan, 
        netsock_t sock,
        netpacktype_t type, 
        const void* data, 
        size_t size){
    if (!data || !chan || (size <= 0)) return NETERROR_NULLDATA;
    if (sock == NETSOCK_INVALID) return NETERROR_INVALIDSOCKET;
    

    netpkthdr_t header = {
        .size = size,
        .type = type,
        .sequence = chan->out_sequence++
    };
    
    size_t buffsize = NETPKT_HDR_SIZE + size;
    char buff[buffsize]; 
    size_t pos = 0;
    _write_header(buff, &pos, &header);
    memcpy(buff + pos, data, size);

    return netsock_senddata(sock, chan->remote, buff, buffsize);
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
   if (!netaddr_equal(from, chan->remote)) return NETERROR_UNKNOWNPEER;

   /* Performed by netsock_receive()
    size_t pos = 0;
    netpkthdr_t hdr = _read_header(buff, &pos);
    pkt->type = hdr.type;
    pkt->size = hdr.size;
    pkt->sequence = hdr.sequence;
   */

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
    printf("Sent handshake (%dB), waiting..\n", rs);
    chan->state = NETCHAN_WAITING;
    return NET_SUCCESS;
}

