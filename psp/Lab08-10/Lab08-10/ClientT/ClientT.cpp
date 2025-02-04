#include "stdafx.h"
#include <thread>
#include <chrono>

//#define IP_SERVER "127.0.0.1"
//#define IP_SERVER "192.168.77.52"
//#define IP_SERVER "192.168.157.129"
#define SERVER_NAME "asusvika"

atomic_bool input_received = false;

void checkTimeout(SOCKET cS) {
    char message[50];
    int libuf = 0;

    for (int i = 0; i < 10; ++i) {
        if (input_received) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (!input_received) {
        if ((libuf = recv(cS, message, sizeof(message), NULL)) == SOCKET_ERROR)
            throw SetErrorMsgText("error recv: ", WSAGetLastError());

        if (!strcmp(message, "TimeOUT")) {
            puts("time out");

            if (closesocket(cS) == SOCKET_ERROR)
                std::cerr << "error closesocket: " << WSAGetLastError() << std::endl;
            if (WSACleanup() == SOCKET_ERROR)
                std::cerr << "error Cleanup: " << WSAGetLastError() << std::endl;

            exit(0);
        }
    }

}


char* get_message(int msg) {
    switch (msg) {
    case 1: return (char*)"echo";
    case 2: return (char*)"time";
    case 3: return (char*)"rand";
    case 4: return (char*)"close";
    default: return (char*)"wrong";
    }
}

int _tmain(int argc, char* argv[]) {
    char* error = (char*)"close";
    int count = 0;
    SOCKET cS;
    WSADATA wsaData;
    setlocale(0, "rus");
    try {

        if (WSAStartup(MAKEWORD(2, 1), &wsaData) != 0)
            throw SetErrorMsgText("Startup:", WSAGetLastError());
        if ((cS = socket(AF_INET, SOCK_STREAM, NULL)) == INVALID_SOCKET)
            throw SetErrorMsgText("socket:", WSAGetLastError());

        SOCKADDR_IN serv;


        /*serv.sin_family = AF_INET;
        serv.sin_port = htons(2000);
        serv.sin_addr.s_addr = inet_addr(IP_SERVER);*/

        hostent* host = gethostbyname(SERVER_NAME);
        if (!host)
            throw SetErrorMsgText("gethostbyname:", WSAGetLastError());

        char* ip_addr = inet_ntoa(*(in_addr*)(host->h_addr));

        serv.sin_family = AF_INET;
        serv.sin_port = htons(2000);

        serv.sin_addr.s_addr = inet_addr(ip_addr);

        if ((connect(cS, (sockaddr*)&serv, sizeof(serv))) == SOCKET_ERROR)
            throw SetErrorMsgText("connect:", WSAGetLastError());


        while (true) {
            char message[50];
            int libuf = 0, lobuf = 0;

            int service;
            thread input_thread(checkTimeout, cS);
            input_thread.detach();
            puts("write option(echo ~ 1, time ~ 2, rand ~ 3, close ~ 4)");
            scanf("%d", &service);
            input_received = true;
            char* outMessage = new char[5];
            strcpy(outMessage, get_message(service));

            if ((lobuf = send(cS, outMessage, strlen(outMessage) + 1, NULL)) == SOCKET_ERROR)
                throw SetErrorMsgText("send:", WSAGetLastError());

            printf("sended: %s\n", outMessage);

            if (service != 1) {
                if ((libuf = recv(cS, message, sizeof(message), NULL)) == SOCKET_ERROR)
                    throw SetErrorMsgText("recv:", WSAGetLastError());

                if (!strcmp(message, "TimeOUT")) {
                    puts("time out");

                    if (closesocket(cS) == SOCKET_ERROR)
                        throw SetErrorMsgText("closesocket:", WSAGetLastError());
                    if (WSACleanup() == SOCKET_ERROR)
                        throw SetErrorMsgText("Cleanup:", WSAGetLastError());

                    return 0;
                }
            }
                

            if (service != 1 && service != 2 && service != 3 && service != 4) {
                printf(message);
                break;
            }
            if (service == 4) break;

            if (service == 1) {
                for (int j = 10; j >= 0; --j) {
                    Sleep(1000);
                    sprintf(outMessage, "%d", j);
                    if ((lobuf = send(cS, outMessage, strlen(outMessage) + 1, NULL)) == SOCKET_ERROR)
                        throw SetErrorMsgText("send:", WSAGetLastError());
                    printf("send: %s\n", outMessage);
                    if ((libuf = recv(cS, message, sizeof(message), NULL)) == SOCKET_ERROR)
                        throw SetErrorMsgText("recv:", WSAGetLastError());

                    if (!strcmp(message, "TimeOUT")) {
                        puts("time out");

                        if (closesocket(cS) == SOCKET_ERROR)
                            throw SetErrorMsgText("closesocket:", WSAGetLastError());
                        if (WSACleanup() == SOCKET_ERROR)
                            throw SetErrorMsgText("Cleanup:", WSAGetLastError());

                        return 0;
                    }

                    printf("receive: %s\n", message);
                }
            }
            else if (service == 2 || service == 3) {

                printf("receive: %s\n", message);

            }
            input_received = false;
        }

        if (closesocket(cS) == SOCKET_ERROR)
            throw SetErrorMsgText("closesocket:", WSAGetLastError());
        if (WSACleanup() == SOCKET_ERROR)
            throw SetErrorMsgText("Cleanup:", WSAGetLastError());
    }
    catch (string errorMsgText) {
        printf("\n%s\n", errorMsgText.c_str());
    }
    printf("\n");
    system("pause");
    return 0;
}
