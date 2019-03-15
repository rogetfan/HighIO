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
    SOCKET ListenSocket = INVALID_SOCKET,ClientSocket = INVALID_SOCKET;
    char recvbuf[DEFAULT_BUFLEN];
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed: %s",ONE_LINE);
        return 1;
    }else{
        printf("Windows Socket Asynchronous start successfully!%s",ONE_LINE);
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

    ListenSocket = WSASocket(result->ai_family, result->ai_socktype, result->ai_protocol,0,0,WSA_FLAG_OVERLAPPED);
    if (ListenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %d%s", WSAGetLastError(),ONE_LINE);
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
     //Get count of thread associated with CPUs
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    int n_process = si.dwNumberOfProcessors;
    int n_thread = 2 * n_process;
    printf("there are %d cores",n_process);
    HANDLE init_completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0 );
    HANDLE worker_threads[n_thread];
    for(int i=0;i<n_thread;i++){
        DWORD thread_id = (DWORD)i;
        worker_threads[i] = CreateThread(NULL,0,&asyiowork,init_completion_port,0,&thread_id);
    }

    if (bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        printf("bind failed with error: %d%s", WSAGetLastError(),ONE_LINE);
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }else{
        printf("bind %s successfully!%s",DEFAULT_ADDR,ONE_LINE);
    }
    freeaddrinfo(result);
    if(SOCKET_ERROR==listen(ListenSocket, SOMAXCONN)){
        printf("listen failed with error: %d%s", WSAGetLastError(),ONE_LINE);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }else{
        printf("Listen to %s successfully%s",DEFAULT_PORT,ONE_LINE);
    }

    while(1){
        if(INVALID_SOCKET == (ClientSocket = accept(ListenSocket, NULL, NULL))) {
            printf("accept failed with error: %d%s", WSAGetLastError(),ONE_LINE);
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }
        int recv_len = 1;
        while(recv_len){
            recv_len = recv(ClientSocket,recvbuf,DEFAULT_BUFLEN,0);
            if(recv_len == 0){
                printf("Connection is closing%s",ONE_LINE);
                break;
            }
            if(recv_len < 0){
                perror("read socket error");
            }else{
                for(int i=0;i<recv_len;i++){
                    printf("char is %d%s",(int)recvbuf[i],ONE_LINE);
                }
                printf("receive length is %d%s",recv_len,ONE_LINE);
                printf("%s",recvbuf);

            }
        }
        if (SOCKET_ERROR == shutdown(ClientSocket, SD_SEND)) {
            printf("shutdown failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }else{
            closesocket(ClientSocket);
        }
    }

    closesocket(ListenSocket);
    WSACleanup();
    return 0;
}


