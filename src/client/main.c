#include <stdio.h>
#include <string.h>
#include "net/client.h"
#include "net/net.h"

netclient_t* client = NET_NULL;

#define NAME "redw0od0-client\0"
#define NAMELEN strlen(NAME)

#define SERVER_PORT 27015
#define SERVER_IP "192.168.1.161" // Just so happens?

void runfunc(void){
    if (client->connection.chan.state != NETCHAN_CONNECTED)
        return;
    char data[] = NAME;
    printf("Sending: %s\n", data);
    netresult_size_t size = netchan_send(
            &client->connection.chan, 
            client->connection.socket_udp, 
            NET_PACKET_NETCMD, 
            data, 
            NAMELEN);



    printf("%d: Sent %dB\n", client->connection.chan.out_sequence, size);
}

int main(void){
    printf("Client\n");

    client = NetClient_Init(NAME, NAMELEN);
    client->func_run = &runfunc;
    client->update_rate = 1;

    if (!NetClient_ConnectAttempt(client, netaddr_new(SERVER_IP, SERVER_PORT))){
        printf("Failed to connect\n");
        exit(1);
    }else{
        printf("Attempting connection to %s:%d...\n", SERVER_IP, SERVER_PORT);
    }
    
    while(1){
        NetClient_Run(client);
    }

    return 0;
}




