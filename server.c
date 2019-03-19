#include "server.h"

static DWORD accpet_listener(LPVOID lpvoid);


DWORD asyiowork(LPVOID lpvoid){
    HANDLE hIocp = (HANDLE)lpvoid;
    DWORD dwNumberOfBytes = 0;
    LPPER_OVERLAPPED lpOverlapped = NULL;
    PER_HANDLE_DATA completionkey;
    int end_loop = 1;
    while (end_loop)
    {
        int bRet = GetQueuedCompletionStatus(hIocp, &dwNumberOfBytes, (PULONG_PTR)&completionkey, (LPOVERLAPPED *) &lpOverlapped, INFINITE);
        printf("Number is %d\n\r",bRet);
        if (bRet == 0)
        {
            continue;
        }
        switch (lpOverlapped->m_lEvent)
        {
        case FD_ACCEPT:
            printf("12345");
            break;
        case FD_CLOSE: 
            end_loop = 0;
            printf("Thread %ld is waiting to exit\n", GetCurrentThreadId());
            break;
        case FD_WRITE:
            printf("Sending data finished......\n");
            shutdown(lpOverlapped->m_sClient, SD_BOTH);
            closesocket(lpOverlapped->m_sClient);
            break;
        case FD_READ:
            printf("client>%s", lpOverlapped->m_pszBuf);
            lpOverlapped->m_lEvent = FD_WRITE;
            WSABUF buf = {0};
            buf.buf = lpOverlapped->m_pszBuf;
            buf.len = dwNumberOfBytes;
            lpOverlapped->m_dwFlags = 0;
            WSASend(lpOverlapped->m_sClient, &buf, 1, &lpOverlapped->m_dwNumberOfBytesRecv, lpOverlapped->m_dwFlags, &lpOverlapped->m_overlapped, NULL);
        }

    }
    return (DWORD)0;
}


HANDLE create_server(LPSERVER_INSTANCE instance){
    LPSERVER_INSTANCE server_in = (LPSERVER_INSTANCE)instance;
    WSADATA wsaData;
    SOCKET listen_socket = INVALID_SOCKET;
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed: %s",ONE_LINE);
        return NULL;
    }else{
        printf("Windows Socket Asynchronous start successfully!%s",ONE_LINE);
    }
    // Initial Completion port
    HANDLE init_completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0 );
    if (init_completion_port == NULL) {
        printf("CreateIoCompletionPort failed with error: %d\n",WSAGetLastError());
        WSACleanup();
        return NULL;
    }

    struct addrinfo *result = NULL,hints;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = server_in->ai_family;
    hints.ai_socktype = server_in->ai_socktype;
    hints.ai_protocol = server_in->ai_protocol;
    hints.ai_flags = server_in->ai_flags;
    // check ip and address
    if (getaddrinfo(server_in->hostname, server_in->port, &hints, &result) != 0) {
        printf("getaddrinfo failed: %s",ONE_LINE);
        WSACleanup();
        return NULL;
    }

    listen_socket = WSASocket(result->ai_family, result->ai_socktype, result->ai_protocol,0,0,WSA_FLAG_OVERLAPPED);
    if (listen_socket == INVALID_SOCKET) {
        printf("Error at socket(): %d%s", WSAGetLastError(),ONE_LINE);
        freeaddrinfo(result);
        WSACleanup();
        return NULL;
    }
    // Associate the listening socket with the completion port
    CreateIoCompletionPort((HANDLE)listen_socket, init_completion_port, (ULONG_PTR)0, 0);

    if (bind(listen_socket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR){
        printf("bind failed with error: %d%s", WSAGetLastError(),ONE_LINE);
        freeaddrinfo(result);
        closesocket(listen_socket);
        WSACleanup();
        return NULL;
    }else{
        printf("bind %s successfully!%s",server_in->hostname,ONE_LINE);
    }
    // listen port
    if(SOCKET_ERROR==listen(listen_socket, SOMAXCONN)){
        printf("listen failed with error: %d%s", WSAGetLastError(),ONE_LINE);
        closesocket(listen_socket);
        WSACleanup();
        return NULL;
    }else{
        printf("Listen to %s successfully!%s",server_in->port,ONE_LINE);
    }
    int thread_len = server_in->io_core*2;
    printf("there are %d cores in your machine , io thread count will be %d%s",thread_len,thread_len,ONE_LINE);
    HANDLE worker_threads[thread_len];
    for(int i=0;i<thread_len;i++){
        DWORD thread_id = (DWORD)i;
        worker_threads[i] = CreateThread(NULL,0,&asyiowork,init_completion_port,0,&thread_id);
        CloseHandle(worker_threads[i]);
    }

    LPLISTENER_INSTANCE listen_in;
    listen_in = (LPLISTENER_INSTANCE) malloc(sizeof(LISTENER_INSTANCE));
    memset(listen_in,0,sizeof(LISTENER_INSTANCE));
    listen_in->listen_socket = listen_socket;
    listen_in->init_iocp = init_completion_port;
    listen_in->is_fin = &(server_in->is_fin);
    listen_in->result = result;
    DWORD lis_thread_id = server_in->io_core+1;
    HANDLE listener = CreateThread(NULL,0,&accpet_listener,listen_in,0,&lis_thread_id);
    return listener;
}

static DWORD accpet_listener(LPVOID lpvoid){
    LPLISTENER_INSTANCE listener_in = (LPLISTENER_INSTANCE)lpvoid;
    SOCKET accept_socket = INVALID_SOCKET;
    GUID GuidAcceptEx = WSAID_ACCEPTEX;
    //funcion pointer
    LPFN_ACCEPTEX lpfn_accept_ex = NULL;
    WSAOVERLAPPED ol_overlap;
    DWORD dwBytes;
    int out_buf_len = 2*(sizeof(SOCKADDR_IN) + 16);
    char lp_output_buf[out_buf_len];
    struct addrinfo *result = listener_in->result;
    int event_result;
    WSAEVENT e[1];

    e[0] = WSACreateEvent();
    WSAEventSelect(listener_in->listen_socket,e[0],FD_ACCEPT);
    if(WSAIoctl(listener_in->listen_socket,\
                SIO_GET_EXTENSION_FUNCTION_POINTER,\
                &GuidAcceptEx,\
                sizeof (GuidAcceptEx),\
                &lpfn_accept_ex,\
                sizeof (lpfn_accept_ex),
                &dwBytes, NULL, NULL)== SOCKET_ERROR){

            printf("WSAIoctl failed with error: %d\n\r", WSAGetLastError());
            closesocket(listener_in->listen_socket);
            WSACleanup();
            return (DWORD)1;
    }
    while(*(listener_in->is_fin)){
        event_result = WSAWaitForMultipleEvents(1, e, FALSE, 1000, FALSE);
        if(event_result == WSA_WAIT_TIMEOUT){
            Sleep(1);
            continue;
        }
        accept_socket = WSASocket(result->ai_family, result->ai_socktype, result->ai_protocol,0,0,WSA_FLAG_OVERLAPPED);
        if (accept_socket == INVALID_SOCKET) {
            printf("Create accept socket failed with error: %d\n\r", WSAGetLastError());
            closesocket(listener_in->listen_socket);
            WSACleanup();
            return (DWORD)1;
        }
        // Empty our overlapped structure and accept connections.
        memset(&ol_overlap, 0, sizeof(ol_overlap));
        //Get count of thread associated with CPUs
        if(lpfn_accept_ex(listener_in->listen_socket, accept_socket, lp_output_buf,\
                 out_buf_len - 2*(sizeof(SOCKADDR_IN) + 16),\
                 sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN)+16,\
                 &dwBytes, &ol_overlap ) == 0 && WSAGetLastError() != 997){
            printf("AcceptEx failed with error: %d\n", WSAGetLastError());
            closesocket(accept_socket);
            closesocket(listener_in->listen_socket);
            WSACleanup();
            return (DWORD)1;
        }
        CreateIoCompletionPort((HANDLE) accept_socket, listener_in->init_iocp, (u_long) 0, 0); 
        WSAResetEvent(e[0]);
    }
    printf("listener is stopped by main thread\n\r");
    closesocket(listener_in->listen_socket);
    WSACleanup();
    free(listener_in);
    return (DWORD)0;
}