#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501

#include <winsock2.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_BUFLEN 1460
#define DEFAULT_PORT 9003           // REMOTE PORT
#define DEFAULT_ADDR "127.0.0.1"    // server address

#ifndef MAKEWORD
#	define MAKEWORD(a, b) ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#endif // ~MAKEWORD

typedef char byte;
byte recvbuf [DEFAULT_BUFLEN];

int main()
{
    printf("Hello world!\n");
    return 0;
}
