#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <thread>
#include <chrono>
#include <winsock2.h>
#include "stdafx.h"

#define IP_SERVER "127.0.0.1"
#define PORT 2000

int main() {
    WSADATA wsaData;
    SOCKET cS;
    SOCKADDR_IN serv;

    setlocale(0, "rus");

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        throw SetErrorMsgText("Startup:", WSAGetLastError());

    if ((cS = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) 
        throw SetErrorMsgText("socket:", WSAGetLastError());

    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT);
    serv.sin_addr.s_addr = inet_addr(IP_SERVER);

    while (true) {
        if (connect(cS, (sockaddr*)&serv, sizeof(serv)) == SOCKET_ERROR) 
            throw SetErrorMsgText("connect:", WSAGetLastError());

        printf("client connected\n");

        if (closesocket(cS) == SOCKET_ERROR) {
            throw SetErrorMsgText("closesocket:", WSAGetLastError());
        }
        else {
            printf("client disconnected\n");
        }

        this_thread::sleep_for(chrono::seconds(2));

        cS = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (cS == INVALID_SOCKET) {
            throw SetErrorMsgText("socket:", WSAGetLastError());
            break;
        }
    }

    WSACleanup();
    return 0;
}