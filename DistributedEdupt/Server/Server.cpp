#include <iostream>
#include <vector>
#include <string>
#include <conio.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

const unsigned short SERVER_PORT = 8888;

struct ClientInfo
{
    SOCKET sock;
    std::string ip;
};

enum State
{
    NONE,
    QUOTA,
    COMPLETION,
    STATE_MAX
};

// サーバー自身のIPアドレスを取得する関数
void ShowServerIP() 
{
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0)
    {
        struct addrinfo hints = {}, * res = nullptr;
        hints.ai_family = AF_INET; // IPv4
        if (getaddrinfo(hostname, nullptr, &hints, &res) == 0)
        {
            std::cout << "   Server IP Addresses:" << std::endl;
            for (auto curr = res; curr != nullptr; curr = curr->ai_next) 
            {
                char ipStr[INET_ADDRSTRLEN];
                struct sockaddr_in* addr = (struct sockaddr_in*)curr->ai_addr;
                inet_ntop(AF_INET, &addr->sin_addr, ipStr, sizeof(ipStr));
                std::cout << "     >> " << ipStr << std::endl;
            }
            freeaddrinfo(res);
        }
    }
}

void DisplayMessage(const std::vector<ClientInfo>& clients)
{
    system("cls");
    std::cout << "Server Waiting..." << std::endl;
    ShowServerIP();
    std::cout << "------------------" << std::endl;
    
    if (clients.empty())
    {
        std::cout << "クライアントを待っています..." << std::endl;
    }
    else
    {
        std::cout << "クライアントが接続しました:" << clients.size() << std::endl;
        for (size_t i = 0; i < clients.size(); ++i)
        {
            std::cout << " [" << i << "] IPアドレス: " << clients[i].ip << std::endl;
        }
    }
    std::cout << "Enterでクライアントの受付を終了..." << std::endl;
}

int main() {
    // 1. WinSock2.2 初期化
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed." << std::endl;
        return -1;
    }

    // 2. リスンソケットの作成
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) 
    {
        WSACleanup();
        return -1;
    }

    // ノンブロッキング設定
    unsigned long arg = 0x01;
    ioctlsocket(listenSock, FIONBIO, &arg);
    
    // アドレスの作成
    SOCKADDR_IN addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(listenSock, (SOCKADDR*)&addr, sizeof(addr));
    listen(listenSock, 5);

    std::vector<ClientInfo> connectedClients;

    DisplayMessage(connectedClients);

    // --- クライアント接続受付フェーズ ---
    while (true) 
    {
        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET newSock = accept(listenSock, (SOCKADDR*)&clientAddr, &addrLen);

        if (newSock != INVALID_SOCKET) 
        {
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));

            ClientInfo info = { newSock, std::string(ipStr) };
            connectedClients.push_back(info);

            DisplayMessage(connectedClients); // 接続があったので表示更新
        }

        if (_kbhit()) 
        {
            if (_getch() == 13) break;
        }

        Sleep(100);
    }

    std::cout << "\n接続を締め切りました..." << std::endl;
    std::cout << "計算フェーズへ移行します..." << std::endl;

    // データ送信するところ



    for (auto& c : connectedClients) closesocket(c.sock);
    closesocket(listenSock);
    WSACleanup();

    std::cout << "Press any key to exit." << std::endl;
    _getch();
    return 0;
}