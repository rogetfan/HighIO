#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501

#include "asynet.h"
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_BUFLEN 1460
#define DEFAULT_PORT "8847"       // listen port
#define DEFAULT_ADDR "0.0.0.0"    // server address
#define ONE_LINE "\n\r"




int main(int argc,char **argv)
{
    WSADATA wsaData;
    SOCKET listen_socket = INVALID_SOCKET;
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed: %s",ONE_LINE);
        return 1;
    }else{
        printf("Windows Socket Asynchronous start successfully!%s",ONE_LINE);
    }

     //Get count of thread associated with CPUs
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    int n_process = si.dwNumberOfProcessors;
    int const n_thread = 2 * n_process;
    printf("there are %d cores%s",n_process,ONE_LINE);
    HANDLE init_completion_port = CreateIoCompletionPort(\
    INVALID_HANDLE_VALUE, NULL, 0, 0 );
    HANDLE worker_threads[n_thread];
    for(int i=0;i<n_thread;i++){
        DWORD thread_id = (DWORD)i;
        worker_threads[i] = CreateThread(NULL,0,&asyiowork,init_completion_port,0,&thread_id);
        CloseHandle(worker_threads[i]);
    }
    struct addrinfo *result = NULL,hints;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(DEFAULT_ADDR, DEFAULT_PORT, &hints, &result) != 0) {
        printf("getaddrinfo failed: %s",ONE_LINE);
        WSACleanup();
        return 1;
    }

    listen_socket = WSASocket(result->ai_family, result->ai_socktype, result->ai_protocol,0,0,WSA_FLAG_OVERLAPPED);
    if (listen_socket == INVALID_SOCKET) {
        printf("Error at socket(): %d%s", WSAGetLastError(),ONE_LINE);
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    if (bind(listen_socket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR){
        printf("bind failed with error: %d%s", WSAGetLastError(),ONE_LINE);
        freeaddrinfo(result);
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }else{
        printf("bind %s successfully!%s",DEFAULT_ADDR,ONE_LINE);
    }
    // freeaddrinfo(result);
    if(SOCKET_ERROR==listen(listen_socket, SOMAXCONN)){
        printf("listen failed with error: %d%s", WSAGetLastError(),ONE_LINE);
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }else{
        printf("Listen to %s successfully%s",DEFAULT_PORT,ONE_LINE);
    }
    puts("Press ESC to exit");
    while(1){
        PER_HANDLE_DATA *handle_data = NULL;
        SOCKADDR_IN sa_remote;
        SOCKET accept;
        int remote_len = sizeof(sa_remote);
        accept = WSAAccept(listen_socket, (SOCKADDR *)&sa_remote,&(remote_len),&ConditionAcceptFunc,(DWORD_PTR) NULL);
        if(accept == INVALID_SOCKET){
            printf("Error at socket(): %d%s", WSAGetLastError(),ONE_LINE);
            perror("INVALID SOCKET When get accept socket");
        }
        else{
            printf("Socket number %d connected\n", (int)accept);
            handle_data = (LPPER_HANDLE_DATA) malloc(sizeof(PER_HANDLE_DATA));
            memset(handle_data,0,sizeof(PER_HANDLE_DATA));
            handle_data->socket = accept;
            memcpy(&handle_data->client_addr, &sa_remote, remote_len);
            CreateIoCompletionPort((HANDLE) accept,init_completion_port, (ULONG_PTR)handle_data, 0);
        }
        if(GetAsyncKeyState(VK_ESCAPE)){
            break;
        }
    }
    closesocket(listen_socket);
    WSACleanup();
    return 0;
}


