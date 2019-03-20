#include "server.h"

static DWORD accpet_listener(LPVOID lpvoid);

static DWORD dwFLAG = 0;
DWORD asyiowork(LPVOID lpvoid){
    HANDLE hIocp = (HANDLE)lpvoid;
    DWORD dwNumberOfBytes = 0;
    LPPER_OVERLAPPED lpOverlapped = NULL;
    PER_HANDLE_DATA completionkey;
    int end_loop = 1;
    char* s_result;
    WSABUF r_result;
    //WSABUF w_result;
    while (end_loop)
    {
        int bRet = GetQueuedCompletionStatus(hIocp, &dwNumberOfBytes, (PULONG_PTR)&completionkey, (LPOVERLAPPED *) &lpOverlapped, 1000);
        if (bRet == 0)
        {
            continue;
        }
        switch (lpOverlapped->p_iostate){
            case AS_ACCEPT:
                _do_accept(lpOverlapped);
                break;
            case AS_READ:
                if(dwNumberOfBytes == 0){
                    memset(lpOverlapped->p_rz_buff,0,DEFAULT_BUFLEN);
                    lpOverlapped->p_iostate = AS_CLOSE;
                    WSARecv(lpOverlapped->p_client_socket, &lpOverlapped->p_wsa_buf, 1, &dwNumberOfBytes, &dwFLAG, &lpOverlapped->p_Overlapped, NULL);
                }else{
                    r_result = _do_read(lpOverlapped,dwNumberOfBytes);
                    s_result = (char *)malloc(r_result.len+1);
                    memcpy(s_result,r_result.buf,r_result.len);
                    *(s_result+r_result.len)='\0';
                    printf("receive data %ld bytes,Data: %s\n\r",r_result.len,s_result);
                    free(s_result);
                    free(r_result.buf);
                }
                break;
            case AS_WRITE:
                // printf("client>%s", lpOverlapped->m_pszBuf);
                // lpOverlapped->m_lEvent = FD_WRITE;
                // WSABUF buf = {0};
                // buf.buf = lpOverlapped->m_pszBuf;
                // buf.len = dwNumberOfBytes;
                // WSASend(lpOverlapped->m_sClient, &buf, 1, &lpOverlapped->m_dwNumberOfBytesRecv, lpOverlapped->m_dwFlags, &lpOverlapped->m_overlapped, NULL);
                break;
            case AS_CLOSE:
                printf("Closing network socket ...\r\n");
                shutdown(lpOverlapped->p_client_socket, SD_BOTH);
                closesocket(lpOverlapped->p_client_socket);
                lpOverlapped->p_client_socket = AS_CLOSE;
                free(lpOverlapped);
                printf("Socket has been closed ...\r\n");
                break;
            case AS_CONNECT:
                break;
        }

    }
    return (DWORD)0;
}

int _do_accept(LPPER_OVERLAPPED lpOverlapped){
    DWORD dwNumberOfBytes = 0;
    lpOverlapped->p_wsa_buf.buf = lpOverlapped->p_rz_buff;
    lpOverlapped->p_wsa_buf.len = DEFAULT_BUFLEN;
    lpOverlapped->p_iostate = AS_READ;
    WSARecv(lpOverlapped->p_client_socket, &lpOverlapped->p_wsa_buf, 1, &dwNumberOfBytes, &dwFLAG, &lpOverlapped->p_Overlapped, NULL);
    return (int)dwNumberOfBytes;
}

WSABUF _do_read(LPPER_OVERLAPPED lpOverlapped,DWORD dwNumberOfBytes){
    WSABUF r_result = {0};
    r_result.len = dwNumberOfBytes;
    r_result.buf = (char *)malloc(dwNumberOfBytes);
    memcpy(r_result.buf,lpOverlapped->p_rz_buff,r_result.len);
    memset(lpOverlapped->p_rz_buff,0,DEFAULT_BUFLEN);
    lpOverlapped->p_wsa_buf.buf = lpOverlapped->p_rz_buff;
    lpOverlapped->p_wsa_buf.len = DEFAULT_BUFLEN;
    lpOverlapped->p_iostate = AS_READ;
    WSARecv(lpOverlapped->p_client_socket, &lpOverlapped->p_wsa_buf, 1, &dwNumberOfBytes, &dwFLAG, &lpOverlapped->p_Overlapped, NULL);
    return r_result;
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
    printf("there are %d cores in your machine , io thread count will be %d%s",server_in->io_core,thread_len,ONE_LINE);
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
    SOCKET client_socket = INVALID_SOCKET;
    GUID GuidAcceptEx = WSAID_ACCEPTEX;
    //funcion pointer
    LPFN_ACCEPTEX lpfn_accept_ex = NULL;
    // WSAOVERLAPPED ol_overlap;
    LPPER_OVERLAPPED p_overlap = NULL;
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
        client_socket = WSASocket(result->ai_family, result->ai_socktype, result->ai_protocol,0,0,WSA_FLAG_OVERLAPPED);
        if (client_socket == INVALID_SOCKET) {
            printf("Create accept socket failed with error: %d\n\r", WSAGetLastError());
            closesocket(listener_in->listen_socket);
            WSACleanup();
            return (DWORD)1;
        }
        p_overlap = (LPPER_OVERLAPPED) malloc(sizeof(PER_OVERLAPPED));
        // Empty our overlapped structure and accept connections.
        memset(&p_overlap->p_Overlapped, 0, sizeof(WSAOVERLAPPED));
        p_overlap->p_client_socket = client_socket;
        p_overlap->p_iostate = AS_ACCEPT;
        //Get count of thread associated with CPUs
        if(lpfn_accept_ex(listener_in->listen_socket, client_socket, lp_output_buf,\
                 out_buf_len - 2*(sizeof(SOCKADDR_IN) + 16),\
                 sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN)+16,\
                 &dwBytes, (WSAOVERLAPPED *)p_overlap ) == 0 && WSAGetLastError() != 997){
            printf("AcceptEx failed with error: %d\n", WSAGetLastError());
            closesocket(client_socket);
            closesocket(listener_in->listen_socket);
            WSACleanup();
            return (DWORD)1;
        }
        CreateIoCompletionPort((HANDLE) client_socket, listener_in->init_iocp, (u_long) 0, 0); 
        WSAResetEvent(e[0]);
    }
    printf("listener is stopped by main thread\n\r");
    closesocket(listener_in->listen_socket);
    WSACleanup();
    free(listener_in);
    return (DWORD)0;
}