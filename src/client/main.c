#include <stdio.h>
#include <string.h>
#include "net/client.h"
#include "net/net.h"

netclient_t* client = NET_NULL;

#define NAME "redw0od0-client\0"
#define NAMELEN strlen(NAME)

#define SERVER_PORT 27015
#define SERVER_BROADCAST_PORT (SERVER_PORT + 1)
#define SERVER_IP "192.168.1.161" // Just so happens?

void runfunc(void){
    if (client->connection.chan.state != NETCHAN_CONNECTED)
        return;
    char data[] = NAME;
    //printf("Sending: %s\n", data);
    /*
    netresult_size_t size = netchan_send(
            &client->connection.chan, 
            client->connection.socket_udp, 
            NET_PACKET_NETCMD, 
            data, 
            NAMELEN);
            */



    //printf("%d: Sent %dB\n", client->connection.chan.out_sequence, size);
}

int main(void){
    printf("Client\n");

    client = NetClient_Init(NAME, NAMELEN, SERVER_BROADCAST_PORT);
    client->func_run = &runfunc;
    client->update_rate = 4;

    NetClient_ConnectServer(client, netaddr_new(SERVER_IP, SERVER_PORT));

    while(1){
        NetClient_Run(client);
    }

    return 0;
}




