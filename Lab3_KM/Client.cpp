#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

DWORD WINAPI ReceiveThread(LPVOID lpParam) {
    SOCKET s = (SOCKET)lpParam;
    char buf[1024];
    while (true) {
        int res = recv(s, buf, 1024, 0);
        if (res > 0) {
            buf[res] = '\0';
            cout << "\r[Forum]: " << buf << "\nYou: ";
        }
        else break;
    }
    return 0;
}

int main() {
    system("title CLIENT");
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    cout << "Searching for server." << endl;
    SOCKET udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in udpAddr = { AF_INET, htons(8888), INADDR_ANY };
    bind(udp, (sockaddr*)&udpAddr, sizeof(udpAddr));

    char discoveryBuf[1024];
    sockaddr_in fromAddr;
    int fromLen = sizeof(fromAddr);
    recvfrom(udp, discoveryBuf, 1024, 0, (sockaddr*)&fromAddr, &fromLen);

    char* serverIP = inet_ntoa(fromAddr.sin_addr);
    cout << "Server found at: " << serverIP << endl;
    closesocket(udp);

    SOCKET tcp = socket(AF_INET, SOCK_STREAM, 0);
    fromAddr.sin_port = htons(9999);
    if (connect(tcp, (sockaddr*)&fromAddr, sizeof(fromAddr)) == SOCKET_ERROR) {
        cout << "Connection failed." << endl;
        return 1;
    }

    cout << "Connected. Type messages:\n" << endl;
    CreateThread(NULL, 0, ReceiveThread, (LPVOID)tcp, 0, NULL);

    string msg;
    while (true) {
        cout << "You: ";
        getline(cin, msg);
        if (msg == "exit") break;
        send(tcp, msg.c_str(), msg.length(), 0);
    }

    closesocket(tcp);
    WSACleanup();
    return 0;
}