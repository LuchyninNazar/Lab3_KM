#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <iostream>
#include <vector>
#include <string>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

vector<SOCKET> clientSockets;
CRITICAL_SECTION cs;

DWORD WINAPI BroadcastThread(LPVOID lpParam) {
    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) return 1;

    BOOL broadcastEnable = TRUE;
    setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastEnable, sizeof(broadcastEnable));

    sockaddr_in broadcastAddr;
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(8888);
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    const char* msg = "SERVER_DISCOVERY";
    while (true) {
        sendto(udpSocket, msg, (int)strlen(msg), 0, (sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
        Sleep(2000);
    }
    return 0;
}

DWORD WINAPI ClientThread(LPVOID lpParam) {
    SOCKET myNode = (SOCKET)lpParam;
    char buf[1024];

    while (true) {
        int bytes = recv(myNode, buf, 1024, 0);
        if (bytes <= 0) break;

        buf[bytes] = '\0';
        cout << "Log: " << buf << endl;

        EnterCriticalSection(&cs);
        for (SOCKET s : clientSockets) {
            if (s != myNode) {
                send(s, buf, bytes, 0);
            }
        }
        LeaveCriticalSection(&cs);
    }

    EnterCriticalSection(&cs);
    for (auto it = clientSockets.begin(); it != clientSockets.end(); ++it) {
        if (*it == myNode) {
            clientSockets.erase(it);
            break;
        }
    }
    LeaveCriticalSection(&cs);

    closesocket(myNode);
    return 0;
}

int main() {
    HANDLE hMutex = CreateMutexA(NULL, FALSE, "Local\\Lab3_Server_Singleton");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    system("title SERVER");

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cout << "WSAStartup failed." << endl;
        return 1;
    }

    InitializeCriticalSection(&cs);

    HANDLE hBroadcast = CreateThread(NULL, 0, BroadcastThread, NULL, 0, NULL);
    if (hBroadcast) CloseHandle(hBroadcast);

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET) {
        cout << "Socket creation failed: " << GetLastError() << endl;
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9999);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Bind failed. Port 9999 might be busy." << endl;
        WSACleanup();
        return 1;
    }

    listen(server, SOMAXCONN);

    cout << "SERVER STATUS: ON" << endl;


    while (true) {
        SOCKET client = accept(server, NULL, NULL);
        if (client != INVALID_SOCKET) {
            EnterCriticalSection(&cs);
            clientSockets.push_back(client);
            LeaveCriticalSection(&cs);

            HANDLE hThread = CreateThread(NULL, 0, ClientThread, (LPVOID)client, 0, NULL);
            if (hThread) {
                CloseHandle(hThread);
                cout << "New connection accepted. Total: " << clientSockets.size() << endl;
            }
        }
    }

    DeleteCriticalSection(&cs);
    closesocket(server);
    WSACleanup();
    if (hMutex) CloseHandle(hMutex);

    return 0;
}