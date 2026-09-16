#include <stdio.h>

#include "net/server.h"

netserver_t server = {0};
#define MAX_CLIENTS 10
#define TICKRATE 1
#define PORT 27015

void is_running(void){
    printf("Is running\n");
}

void shutdown(void){
    printf("Goodbye\n");
}

int main(void){
    printf("Server\n");
    if (!NetServer_Init(&server, MAX_CLIENTS, TICKRATE, PORT)){
        printf("Failed to initialise server\n");
        exit(1);
    }else{
        printf("Server initialised\n");
    }

    server.func_run = &is_running;
    server.func_shutdown = &shutdown;

    while(1){
        NetServer_Run(&server);
    }

    NetServer_Shutdown(&server);
    return 0;
}
