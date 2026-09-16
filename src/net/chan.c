#include "net/chan.h"
#include "net/net.h"
#include "net/platform/netplatform.h"
#include <string.h>
// Dereference increment
#define DEREFINC(p) (*p)++

#define _WRITE_INT(buff, pos, value) \
    _write_intgeneric(buff, pos, (uintmax_t)(value), sizeof(value))

#define HDRSIZE sizeof(netpkthdr_t)

// Read little endian
static inline u32 _read_u32(char* buff, size_t* pos){
    u32 value = (
            (buff[DEREFINC(pos)] << 24) |
            (buff[DEREFINC(pos)] << 16) |
            (buff[DEREFINC(pos)] << 8)  |
            (buff[DEREFINC(pos)])
            );
    return value;
}

static inline u32 _read_u16(char* buff, size_t* pos){
    u16 val = 
        (buff[DEREFINC(pos)] << 16) |
        (buff[DEREFINC(pos)]);
    return val;
}
// Writes in big endian
/*
    u32 u = 0x12345678;
    buff[pos++] = u >> 24; // 0x12
    buff[pos++] = u >> 16; // 0x34
    buff[pos++] = u >> 8;  // 0x56
    buff[pos++] = u;       // 0x78
 */
static inline void _write_u32(char* buff, size_t* pos, u32 value){
    buff[DEREFINC(pos)] = value >> 24;
    buff[DEREFINC(pos)] = value >> 16;
    buff[DEREFINC(pos)] = value >> 8;
    buff[DEREFINC(pos)] = value;
}

static inline void _write_u16(char* buff, size_t* pos, u16 value){
    buff[DEREFINC(pos)] = value >> 16;
    buff[DEREFINC(pos)] = value;
}



// Big endian write
static inline void _write_intgeneric(
    char* buff,
    size_t* pos,
    uintmax_t value,
    size_t bytes)
{
    for (size_t i = 0; i < bytes; i++)
        buff[DEREFINC(pos)] =
            (value >> (8 * (bytes - 1 - i))) & 0xff;
}

static inline void _write_header(char* buff, size_t* pos, const netpkthdr_t* header){
    _WRITE_INT(buff, pos, header->sequence);
    _WRITE_INT(buff, pos, header->size);
    _WRITE_INT(buff, pos, header->type);
}

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
    
    size_t buffsize = HDRSIZE + size;
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

   size_t recsize = netsock_receive(sock, buff, buffsize, &from);
   if (!netaddr_equal(from, chan->remote)) return NETERROR_WRONGPEER;

   return recsize;
}

netresult_t netchan_connect(
        netchan_t* chan,
        netsock_t sock,
        netaddr_t destination
        )
{
    if (!chan) return NETERROR_NULLDATA;    
    if (sock == NETSOCK_INVALID) return NETERROR_INVALIDSOCKET;

    netresult_t res = netsock_connect(sock, destination);
    if (!res) return NET_FAILURE;

    chan->state = NETCHAN_CONNECTED;
    chan->remote = destination;
    chan->out_sequence = 0;
    chan->in_sequence = 0;
    chan->ack = 0;
    return res;
}

