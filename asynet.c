#include "asynet.h"

int CALLBACK ConditionAcceptFunc(\
    LPWSABUF lpCallerId, \
    LPWSABUF lpCallerData, \
    LPQOS pQos, \
    LPQOS lpGQOS, \
    LPWSABUF lpCalleeId,\
    LPWSABUF lpCalleeData,\
    GROUP FAR * g,\
    DWORD_PTR dwCallbackData \
    ){
    return CF_ACCEPT;
}

DWORD asyiowork(LPVOID lpvoid){
    HANDLE hIocp = (HANDLE)lpvoid;
    DWORD dwNumberOfBytes = 1460;
    LPPER_OVERLAPPED lpOverlapped;
    lpOverlapped =(LPPER_OVERLAPPED) malloc(sizeof(PER_OVERLAPPED));
    PER_HANDLE_DATA completionkey;
    int end_loop = 1;
    while (end_loop)
    {
        int bRet = GetQueuedCompletionStatus(hIocp, &dwNumberOfBytes, (PULONG_PTR)&completionkey, (LPOVERLAPPED*) lpOverlapped, INFINITE);
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


int loadnet(){

    return 0;
}

int unload(){

   return 0;
}
