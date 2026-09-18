#pragma once

#include "net/net.h"
// Dereference increment
#define DEREFINC(p) (*p)++

#define _WRITE_INT(buff, pos, value) \
    _write_intgeneric(buff, pos, (uintmax_t)(value), sizeof(value))

//#define HDRSIZE sizeof(netpkthdr_t)



// Read little endian
static inline bool _read_u32(const char* buff, size_t bufflen, size_t* pos, u32* out)
{
    if (*pos + sizeof(u32) > bufflen) return false;
    u32 value = 0;

    value |= (u32)(u8)buff[DEREFINC(pos)] << 24;
    value |= (u32)(u8)buff[DEREFINC(pos)] << 16;
    value |= (u32)(u8)buff[DEREFINC(pos)] << 8;
    value |= (u32)(u8)buff[DEREFINC(pos)];

    *out = value;
    return true;
}


static inline bool _read_u16(char* buff, size_t bufflen, size_t* pos, u16* out){
    if (*pos + sizeof(u16) > bufflen) return false;
    u16 val = 0;
    val |=  (u16)(u8)buff[DEREFINC(pos)] << 8;
    val |=  (u16)(u8)buff[DEREFINC(pos)];
    *out = val;
    return true;
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

static inline bool _read_intgeneric(
    char* buff,
    size_t bufflen,
    size_t* pos,
    size_t bytes,
    uintmax_t* out)
{
    uintmax_t value = 0;
    if (*pos + bytes > bufflen) return false;
    for (size_t i = 0; i < bytes; i++){
        value |= (uintmax_t)(u8)    buff[DEREFINC(pos)] << (8 * (bytes - 1 - i));
    }
    *out = value;
    return true;
}

static inline void _write_header(char* buff, size_t* pos, const netpkthdr_t* header){
    _WRITE_INT(buff, pos, header->sequence);
    _WRITE_INT(buff, pos, header->size);
    _WRITE_INT(buff, pos, header->type);
}


#define TYPEMATCH(match, item) (typeof(match))item 
static inline bool _read_header(char* buff, size_t bufflen, size_t* pos, netpkthdr_t* out){
    if (*pos + NETPKT_HDR_SIZE > bufflen) return false;
    netpkthdr_t header = {0};
    uintmax_t tmp;
    if (!_read_intgeneric(buff, bufflen, pos, sizeof(header.sequence), &tmp)) return false;
    header.sequence = TYPEMATCH(header.sequence, tmp);
    if (!_read_intgeneric(buff, bufflen, pos, sizeof(header.size), &tmp)) return false;
    header.size = TYPEMATCH(header.size, tmp);
    if (!_read_intgeneric(buff, bufflen, pos, sizeof(header.type), &tmp)) return false;
    header.type = TYPEMATCH(header.type, tmp);
    *out = header;
    return true;
}

// Writes in host byte form
static inline void _write_netaddr(char* buff, size_t* pos, netaddr_t addr){
    _WRITE_INT(buff, pos, addr.ip);
    _WRITE_INT(buff, pos, addr.port);
}

static inline bool _read_netaddr(char* buff, size_t bufflen, size_t* pos, netaddr_t* out){
    ASSERT(buff && out, "Failed to read netaddr, null buff/out address"); 
    netaddr_t addr = {0};
    uintmax_t tmp;
    if (!_read_intgeneric(buff, bufflen, pos, sizeof(addr.ip), &tmp)) return false;
    addr.ip = TYPEMATCH(addr.ip, tmp); 
    if (!_read_intgeneric(buff, bufflen, pos, sizeof(addr.port), &tmp)) return false;
    addr.port = TYPEMATCH(addr.port, tmp); 
    *out = addr;
    return true;
}
