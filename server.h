#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED
#endif

#include "asynet.h"
#define ONE_LINE "\n\r"
#define DEFAULT_BUFLEN 1460

enum IO_STATE{
    AS_ACCEPT,
    AS_READ,
    AS_WRITE,
    AS_CLOSE,
    AS_CONNECT
};

typedef struct _MY_OVERLAPPED
{
   OVERLAPPED   p_Overlapped;                      
   SOCKET       p_client_socket;          
   WSABUF       p_wsa_buf;              
   char         p_sz_buff[DEFAULT_BUFLEN]; 
   enum IO_STATE     p_iostate;               
} PER_OVERLAPPED, *LPPER_OVERLAPPED;

typedef struct _PER_HANDLE_DATA {
    SOCKET socket;
    SOCKADDR_STORAGE  client_addr;
} PER_HANDLE_DATA, * LPPER_HANDLE_DATA;


typedef struct _SERVER_INSTANCE{
    char *hostname;
    char *port;
    int  ai_flags;
    int  ai_family;
    int  ai_socktype;
    int  ai_protocol;
    int  is_fin;  // 1 means running, 0 means stop
    int  io_core; // count of io_worker thread

} SERVER_INSTANCE,*LPSERVER_INSTANCE;

typedef struct _LISTENER_INSTANCE{
    SOCKET listen_socket;
    HANDLE init_iocp;
    struct addrinfo *result;
    int  *is_fin;
} LISTENER_INSTANCE,*LPLISTENER_INSTANCE;

HANDLE create_server(LPSERVER_INSTANCE instance);

DWORD asyiowork(void* lpvoid);

int _do_accept();

int regread();

int regwrite();

// build windows socket
int buildnet();

// clear windows socket
int destnet();

int loadnet();

int unload();
