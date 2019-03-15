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
#	define MAKEWORD(a, b) ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#endif // ASNET_H_INCLUDED

DWORD asyiowork(void* lpvoid);

int regaccept();

int regread();

int regwrite();

// build windows socket
int buildnet();

// clear windows socket
int destnet();




