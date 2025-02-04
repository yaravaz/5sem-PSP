#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "stdafx.h"
#include <iostream>
#include <ctime>
#include "Winsock2.h"  
#pragma comment(lib, "WS2_32.lib")

using namespace std;

bool GetServer(char* name, short port, sockaddr* from, int* flen);

SOCKET sock;

int main()
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    WSADATA wsaData;
    SOCKADDR_IN from;
    char name[] = "Hello";
    memset(&from, 0, sizeof(from));
    int lfrom = sizeof(from);
    int optval = 1;
    try
    {
        if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
            throw SetErrorMsgText("Startup: ", WSAGetLastError());
        cout << "CLIENT" << endl;
        if ((sock = socket(AF_INET, SOCK_DGRAM, NULL)) == INVALID_SOCKET)
            throw SetErrorMsgText("socket: ", WSAGetLastError());
        if (GetServer(name, 2000, (sockaddr*)&from, &lfrom))
        {
            cout << "Connected server: " << endl;
            cout << "IP: " << inet_ntoa(from.sin_addr) << endl;
            cout << "Port: " << htons(from.sin_port) << endl;
        }

        if (closesocket(sock) == SOCKET_ERROR)
            throw SetErrorMsgText("closesocket: ", WSAGetLastError());
        if (WSACleanup() == SOCKET_ERROR)
            throw SetErrorMsgText("Cleanup: ", WSAGetLastError());
    }
    catch (string errorMsgText)
    {
        cout << "WSAGetLastError: " << errorMsgText << endl;
    }


}

bool GetServer(char* name, short port, sockaddr* from, int* flen)
{

    ((SOCKADDR_IN*)from)->sin_family = AF_INET;
    ((SOCKADDR_IN*)from)->sin_port = htons(port);
    ((SOCKADDR_IN*)from)->sin_addr.s_addr = inet_addr("127.255.255.255");
    // 127.255.255.255
    // 192.168.53.255
    // 192.168.84.255
    int lbuf;
    char ibuf[50];
    int optval = 1;
    int timeout = 5000;

    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&optval, sizeof(optval)) == SOCKET_ERROR)
        throw SetErrorMsgText("opt: ", WSAGetLastError());

    if (sendto(sock, name, strlen(name) + 1, NULL, from, *flen) == SOCKET_ERROR)
        throw SetErrorMsgText("sendto: ", WSAGetLastError());

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    while (true) {
        int result = recvfrom(sock, ibuf, sizeof(ibuf), NULL, from, flen);
        if (result == SOCKET_ERROR)
        {
            if (WSAGetLastError() == WSAETIMEDOUT) {
                throw SetErrorMsgText("recvfrom: ", WSAGetLastError());
                return false;
            }
                
            else
                throw SetErrorMsgText("recvfrom: ", WSAGetLastError());
        }

        cout << "Received: " << ibuf << endl;
    }
    return true;

}