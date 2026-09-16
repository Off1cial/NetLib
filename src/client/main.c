#include <stdio.h>
#include <string.h>
#include "net/client.h"

netclient_t* client = NET_NULL;

#define NAME "redw0od0-client"
#define NAMELEN strlen(NAME)

#define SERVER_PORT 27015
#define SERVER_IP "192.168.1.161" // Just so happens?

void runfunc(void){
    char data[] = NAME;
    netsize_t size = netsock_senddata(
            client->connection.socket_udp, 
            client->connection.remote,
            data, NAMELEN
            );

    printf("Sent %zd\n", size);
}

int main(void){
    printf("Client\n");

    client = NetClient_Init(NAME, NAMELEN);
    client->func_run = &runfunc;
    client->update_rate = 20;

    if (!NetClient_Connect(client, netaddr_new(SERVER_IP, SERVER_PORT))){
        printf("Failed to connect\n");
        exit(1);
    }else{
        printf("Connected to %s:%d\n", SERVER_IP, SERVER_PORT);
    }
    
    while(1){
        NetClient_Run(client);
    }

    return 0;
}




