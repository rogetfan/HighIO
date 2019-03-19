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
    SOCKET accept_socket = INVALID_SOCKET;
    GUID GuidAcceptEx = WSAID_ACCEPTEX;
    //funcion pointer
    LPFN_ACCEPTEX lpfn_accept_ex = NULL;
    WSAOVERLAPPED ol_overlap;
    DWORD dwBytes;
    char lp_output_buf[DEFAULT_BUFLEN];
    int out_buf_len = DEFAULT_BUFLEN;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed: %s",ONE_LINE);
        return 1;
    }else{
        printf("Windows Socket Asynchronous start successfully!%s",ONE_LINE);
    }
    // Initial Completion port
    HANDLE init_completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0 );
    if (init_completion_port == NULL) {
        printf("CreateIoCompletionPort failed with error: %d\n",WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct addrinfo *result = NULL,hints;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    // check ip and address
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
    // Associate the listening socket with the completion port
    CreateIoCompletionPort((HANDLE)listen_socket, init_completion_port, (ULONG_PTR)0, 0);

    if (bind(listen_socket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR){
        printf("bind failed with error: %d%s", WSAGetLastError(),ONE_LINE);
        freeaddrinfo(result);
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }else{
        printf("bind %s successfully!%s",DEFAULT_ADDR,ONE_LINE);
    }
    // listen port
    if(SOCKET_ERROR==listen(listen_socket, SOMAXCONN)){
        printf("listen failed with error: %d%s", WSAGetLastError(),ONE_LINE);
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }else{
        printf("Listen to %s successfully%s",DEFAULT_PORT,ONE_LINE);
    }
    for(int i =0 ;i < 10; ++i){
    if(WSAIoctl(listen_socket,\
                SIO_GET_EXTENSION_FUNCTION_POINTER,\
                &GuidAcceptEx,\
                sizeof (GuidAcceptEx),\
                &lpfn_accept_ex,\
                sizeof (lpfn_accept_ex),
                &dwBytes, NULL, NULL)== SOCKET_ERROR){

        printf("WSAIoctl failed with error: %d\n\r", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    accept_socket = WSASocket(result->ai_family, result->ai_socktype, result->ai_protocol,0,0,WSA_FLAG_OVERLAPPED);
    if (accept_socket == INVALID_SOCKET) {
        printf("Create accept socket failed with error: %d\n\r", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }
    // Empty our overlapped structure and accept connections.
    memset(&ol_overlap, 0, sizeof(ol_overlap));
    //Get count of thread associated with CPUs
    if(lpfn_accept_ex(listen_socket, accept_socket, lp_output_buf,\
                 out_buf_len - 2*(sizeof(SOCKADDR_IN) + 16),\
                 sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN)+16,\
                 &dwBytes, &ol_overlap ) == 0 && WSAGetLastError() != 997){
        printf("AcceptEx failed with error: %d\n", WSAGetLastError());
        closesocket(accept_socket);
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }
    CreateIoCompletionPort((HANDLE) accept_socket, init_completion_port, (u_long) 0, 0); 
    }
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    int n_process = si.dwNumberOfProcessors;
    //const int n_thread = 2 * n_process;
    const int n_thread = 1;
    printf("there are %d cores%s",n_process,ONE_LINE);
    HANDLE worker_threads[n_thread];
    for(int i=0;i<n_thread;i++){
        DWORD thread_id = (DWORD)i;
        worker_threads[i] = CreateThread(NULL,0,&asyiowork,init_completion_port,0,&thread_id);
        CloseHandle(worker_threads[i]);
    }
    puts("Press ESC to exit...");
    while(1){
        if(GetAsyncKeyState(VK_ESCAPE)){
            break;
        }
    }
    closesocket(listen_socket);
    WSACleanup();
    return 0;
}


