#include <stdio.h>

#include "net/server.h"

#define MAX_CLIENTS 2
#define TICKRATE 20
#define PORT 27015
#define BROADCAST_PORT (PORT + 1)

#define BROADCASTMSG "Hello all, im active rn n' shieeett\0"
#define BROADCASTMSG_SIZE (strlen(BROADCASTMSG) + 1)

netserver_t* server = NET_NULL;

void is_running(void){
    //printf("Is running\n");
    netresult_size_t size = NetServer_Broadcast(server, BROADCASTMSG, BROADCASTMSG_SIZE);
    if (size > 0)
        printf("Broadcasted %dB\n", size);
    else if (size < 0)
     printf("Broadcast error: %d\n", size);
}

void sv_shutdown(void){
    printf("Goodbye\n");
}

int main(void){
    server = NetServer_Init(MAX_CLIENTS, TICKRATE, PORT, BROADCAST_PORT);
    if (!server){
        printf("Failed to initialise server\n");
        exit(1);
    }
    server->func_run = &is_running;
    server->func_shutdown = &sv_shutdown;

    while(1){
        NetServer_Run(server);
    }

    NetServer_Shutdown(server);
    return 0;
}
