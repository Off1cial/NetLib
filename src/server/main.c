#include <stdio.h>

#include "net/server.h"

#define MAX_CLIENTS 10
#define TICKRATE 20
#define PORT 27015

netserver_t* server = NET_NULL;

void is_running(void){
    printf("Is running\n");
}

void shutdown(void){
    printf("Goodbye\n");
}

int main(void){
    server = NetServer_Init(MAX_CLIENTS, TICKRATE, PORT);
    if (!server){
        printf("Failed to initialise server\n");
        exit(1);
    }
    //server.func_run = &is_running;
    server->func_shutdown = &shutdown;

    while(1){
        NetServer_Run(server);
    }

    NetServer_Shutdown(server);
    return 0;
}
