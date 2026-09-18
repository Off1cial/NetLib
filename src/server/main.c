#include <stdio.h>
#include <signal.h>
#include "net/server/server.h"

#define MAX_CLIENTS 2
#define TICKRATE 20
#define PORT 27015
#define BROADCAST_PORT (PORT + 1)

#define BROADCASTMSG "Hello all, im active rn n' shieeett\0"
#define BROADCASTMSG_SIZE (strlen(BROADCASTMSG) + 1)

netserver_t* server = NET_NULL;


// INTERRUPT DETECTION
static volatile sig_atomic_t g_should_quit = 0;

static void _handle_term(int sig) {
    (void)sig;
    g_should_quit = 1;
}

static void install_signal_handlers(void) {
    struct sigaction sa = {0};
    sa.sa_handler = _handle_term;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;   // no SA_RESTART
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);
}


void is_running(void){
    //printf("Is running\n");
    netresult_size_t size = NetServer_Broadcast(server, NULL,0);
    if (size > 0)
        printf("Broadcasted %dB\n", size);
    else if (size < 0)
     printf("Broadcast error: %d\n", size);
}

void sv_shutdown(void){
    printf("Goodbye\n");
}

int main(void){
    install_signal_handlers();
    server = NetServer_Init(MAX_CLIENTS, TICKRATE, PORT, BROADCAST_PORT);
    if (!server){
        printf("Failed to initialise server\n");
        exit(1);
    }
    server->func_run = &is_running;
    server->func_shutdown = &sv_shutdown;

    while(!g_should_quit){
        NetServer_Run(server);
    }

    NetServer_Shutdown(server);
    return 0;
}
