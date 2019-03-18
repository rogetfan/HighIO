#ifndef ASYNET_H_INCLUDED
#define ASYNET_H_INCLUDED
#endif

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <mswsock.h>
#include <stdio.h>
#ifndef MAKEWORD
#define MAKEWORD(a, b) ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#endif // ASNET_H_INCLUDED
#define DEFAULT_BUFLEN 1460


typedef struct _tag_MY_OVERLAPPED
{
    OVERLAPPED m_overlapped;
    SOCKET m_sClient;
    long m_lEvent;
    DWORD m_dwNumberOfBytesRecv;
    DWORD m_dwFlags;
    char *m_pszBuf;
    LONG m_dwBufSize;
} PER_OVERLAPPED, *LPPER_OVERLAPPED;

typedef struct _PER_HANDLE_DATA {
    SOCKET socket;
    SOCKADDR_STORAGE  client_addr;
} PER_HANDLE_DATA, * LPPER_HANDLE_DATA;

int CALLBACK ConditionAcceptFunc(\
    LPWSABUF lpCallerId, \
    LPWSABUF lpCallerData, \
    LPQOS pQos, \
    LPQOS lpGQOS, \
    LPWSABUF lpCalleeId,\
    LPWSABUF lpCalleeData,\
    GROUP FAR * g,\
    DWORD_PTR dwCallbackData \
    );

DWORD asyiowork(void* lpvoid);

int regaccept();

int regread();

int regwrite();

// build windows socket
int buildnet();

// clear windows socket
int destnet();




