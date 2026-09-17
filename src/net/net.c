#include "net/net.h"
#include "net/platform/netplatform.h"
#include "net/readwrite.h"
#include <errno.h>
#include <netinet/in.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#endif

#define ADDRCAST(sockaddrin) (struct sockaddr*)&sockaddrin

netaddr_t netaddr_new(char* ip, u16 port){
    netaddr_t addr = {0};
    inet_pton(AF_INET, ip, &addr.ip);
    addr.ip = ntohl(addr.ip);
    addr.port = port;
    return addr;
}


static inline 
netaddr_t _sockaddr_to_netaddr(struct sockaddr_in addr){
    return (netaddr_t){.port = ntohs(addr.sin_port), .ip = ntohl(addr.sin_addr.s_addr)};
}

static inline
struct sockaddr_in _netaddr_to_sockaddr(netaddr_t addr){
    return (struct sockaddr_in){
        .sin_port = htons(addr.port),
        .sin_addr.s_addr = htonl(addr.ip),
        .sin_family = AF_INET
    };
}


netsock_t netsock_create_udp(void){
#ifdef _WIN32
    printf("Fuck windows\n");
    return NETSOCK_INVALID;
#else
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) 
        return NETSOCK_INVALID;
    return sock;
#endif
}


void netsock_close(netsock_t sock){
    if (sock == NETSOCK_INVALID)
        return;

#ifdef _WIN32
    reutrn;
#else
    close(sock);
#endif
}



netresult_t netsock_bind(netsock_t sock, netaddr_t addr){
#ifdef _WIN32
    return NET_FAILURE;
#else
    struct sockaddr_in in = _netaddr_to_sockaddr(addr);
    int res = bind(sock, (struct sockaddr*)&in, sizeof(in));
    if (res < 0) 
        return NET_FAILURE;
    return NET_SUCCESS;
#endif
}


netresult_t netsock_set_broadcast(netsock_t sock)
{
    int enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&enable, sizeof(enable)) != 0) {
        return NET_FAILURE;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable)) != 0) {
        return NET_FAILURE;
    }
    return NET_SUCCESS;
}


// For use on clients
netresult_t netsock_connect(netsock_t sock, netaddr_t addr){
#ifdef _WIN32
    return NET_FAILURE;
#else
    /*
    struct sockaddr_in in = _netaddr_to_sockaddr(addr);
    int res = connect(sock, (struct sockaddr*)&in, sizeof(in));
    if (res < 0)
        return NET_FAILURE;
    */
    return NET_SUCCESS;
#endif
}

netresult_size_t netsock_senddata(netsock_t sock, netaddr_t dest, char* data, size_t n){
    if (NETSOCK_ISNULL(sock)){
        fprintf(stderr, "NetSend: Invalid socket\n");
        return 0;
    }
    struct sockaddr_in destaddr = _netaddr_to_sockaddr(dest);
    ssize_t size = sendto(sock, data, n, 0, ADDRCAST(destaddr), sizeof(destaddr));
    if (size < 0){
        perror("sendto");
    }
    return (netresult_size_t)size;
}

netresult_size_t netsock_sendpacket(
        netsock_t sock, 
        netaddr_t dest, 
        char* data, 
        size_t n, 
        netpacktype_t type){
    netpkthdr_t header = {.size = n, .type = type, .sequence = 0};
    size_t buffsize = NETPKT_HDR_SIZE + n;
    char buff[buffsize];
    size_t pos = 0;
    _write_header(buff, &pos, &header);
    memcpy(buff + pos, data, n);
    struct sockaddr_in saddr = _netaddr_to_sockaddr(dest);
    size_t sent = sendto(
            sock, 
            buff, 
            buffsize, 
            0, 
            ADDRCAST(saddr), 
            sizeof(saddr));
    return (netresult_size_t)sent;
}

/*
netsize_t netsock_receive(netsock_t sock, char* output, netsize_t n, netaddr_t* who){
    if (NETSOCK_ISNULL(sock)){
        return 0;
    }
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);

    netsize_t in_size = 0;
    char buff[n];
    while ((in_size = recvfrom(sock, buff, n, MSG_DONTWAIT, ADDRCAST(from), &fromlen)) > 0){
       // What do i do here?  
    }
    memcpy(output, buff, n);
    return in_size;
    *who = _sockaddr_to_netaddr(from);
}
*/


netresult_size_t netsock_receive(
    netsock_t sock,
    char* output,
    size_t n,
    netaddr_t* who,
    netpacket_t* outpkt
){
    if (NETSOCK_ISNULL(sock))
        return 0;

    struct sockaddr_in from = {0};
    socklen_t fromlen = sizeof(from);

    ssize_t received = recvfrom(
        sock,
        output,
        n,
        MSG_DONTWAIT,
        (struct sockaddr*)&from,
        &fromlen
    );

    if (received < 0){
        if (errno == EAGAIN || errno == EWOULDBLOCK){
            return 0;
        }
        perror("Recvfrom");
        return -1;
    }

    if (who)
        *who = _sockaddr_to_netaddr(from);

    netpkthdr_t hdr = {0};
    size_t pos = 0;
    hdr = _read_header(output, &pos);
    
    outpkt->sequence = hdr.sequence;
    outpkt->size = hdr.size;
    outpkt->type = hdr.type;
    memcpy(outpkt->data, output + pos, received);

    return (netresult_size_t)received;
}


