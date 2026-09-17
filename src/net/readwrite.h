#pragma once

#include "net/net.h"
// Dereference increment
#define DEREFINC(p) (*p)++

#define _WRITE_INT(buff, pos, value) \
    _write_intgeneric(buff, pos, (uintmax_t)(value), sizeof(value))

//#define HDRSIZE sizeof(netpkthdr_t)


// Change with netpkthdr_t, this is to avoid struct padding
#define NETPKT_HDR_SIZE (sizeof(u32) + sizeof(netlen_t) + sizeof(netpacktype_t))
#define NETADDR_SIZE (sizeof(u32) + sizeof(u16))
// Read little endian
static inline u32 _read_u32(const char* buff, size_t* pos)
{
    u32 value = 0;

    value |= (u32)(u8)buff[DEREFINC(pos)] << 24;
    value |= (u32)(u8)buff[DEREFINC(pos)] << 16;
    value |= (u32)(u8)buff[DEREFINC(pos)] << 8;
    value |= (u32)(u8)buff[DEREFINC(pos)];

    return value;
}


static inline u32 _read_u16(char* buff, size_t* pos){
    u16 val = 0;

    val |=  (u16)(u8)buff[DEREFINC(pos)] << 8;
    val |=  (u16)(u8)buff[DEREFINC(pos)];
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
    buff[DEREFINC(pos)] = value >> 8;
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
            (value >> (8 * (bytes - 1 - i))) & 0xFF;
}

static inline uintmax_t _read_intgeneric(
    char* buff,
    size_t* pos,
    size_t bytes)
{
    uintmax_t value = 0;
    for (size_t i = 0; i < bytes; i++){
        value |= (uintmax_t)(u8)    buff[DEREFINC(pos)] << (8 * (bytes - 1 - i));
    }
    return value;
}

static inline void _write_header(char* buff, size_t* pos, const netpkthdr_t* header){
    _WRITE_INT(buff, pos, header->sequence);
    _WRITE_INT(buff, pos, header->size);
    _WRITE_INT(buff, pos, header->type);
}


#define TYPEMATCH(match, item) (typeof(match))item 
static inline netpkthdr_t _read_header(char* buff, size_t* pos){
    netpkthdr_t header = {0};
    header.sequence = (typeof(header.sequence))_read_intgeneric(buff, pos, sizeof(header.sequence));
    header.size = (typeof(header.sequence))_read_intgeneric(buff, pos, sizeof(header.size));
    header.type = (typeof(header.type))_read_intgeneric(buff, pos, sizeof(header.type));
    return header;
}

// Writes in host byte form
static inline void _write_netaddr(char* buff, size_t* pos, netaddr_t addr){
    _WRITE_INT(buff, pos, addr.ip);
    _WRITE_INT(buff, pos, addr.port);
}

static inline netaddr_t _read_netaddr(char* buff, size_t* pos){
    netaddr_t addr = {0};
    addr.ip = TYPEMATCH(addr.ip, _read_intgeneric(buff, pos, sizeof(addr.ip)));

    addr.port = 27015; 
    return addr;
}
