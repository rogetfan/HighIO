#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501

#include "server.h"
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_PORT "8847"       // listen port
#define DEFAULT_ADDR "0.0.0.0"    // server address


int main(int argc,char **argv)
{

    LPSERVER_INSTANCE server_in = NULL;
    server_in = (LPSERVER_INSTANCE) malloc(sizeof(SERVER_INSTANCE));
    memset(server_in,0,sizeof(SERVER_INSTANCE));
    server_in->hostname = DEFAULT_ADDR;
    server_in->port = DEFAULT_PORT;
    server_in->ai_flags = AI_PASSIVE;
    server_in->ai_family = AF_INET;
    server_in->ai_socktype = SOCK_STREAM;
    server_in->ai_protocol = IPPROTO_TCP;
    server_in->is_fin = 1;
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    int n_process = si.dwNumberOfProcessors;
    server_in->io_core = 2 * n_process; // count of io_worker thread
    create_server(server_in);
    puts("Press ESC to stop...");
    while(1){
        if(GetAsyncKeyState(VK_ESCAPE)){
            server_in->is_fin = 0;
            break;
        }
    }
    Sleep(2000);
    free(server_in);
    puts("Press Any Key to exit...");
    getchar();
    exit(0);
}


