#include <iostream>
#include <vector>
#include <string>
#include <conio.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

const unsigned short SERVER_PORT = 8888;
const int WIN_WIDTH = 800;
const int WIN_HEIGHT = 600;
const int LINE_STEP = 4;

enum State
{
    STATE_NONE,
    STATE_QUOTA,
    STATE_COMPLETION,
    STATE_MAX
};

struct ClientInfo
{
	SOCKET sock;
	std::string ip;
};

struct RenderTask
{
	int taskId;
	int startY;
	int endY;
	State status;
	std::string ip;
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
        std::cout << "Waiting Client..." << std::endl;
    }
    else
    {
        std::cout << "Client Connected:" << clients.size() << std::endl;
        for (size_t i = 0; i < clients.size(); ++i)
        {
            std::cout << " [" << i << "] IP Address: " << clients[i].ip << std::endl;
        }
    }
    std::cout << "Press Enter to stop accepting client connections..." << std::endl;
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

    std::cout << "\nStop Accept!" << std::endl;
    std::cout << "Transitioning to the calculation phase..." << std::endl;


	// タスクの初期化
	std::vector<RenderTask> taskTable;
	int Counter = 0;
	for(int y = 0; y < WIN_HEIGHT; y += LINE_STEP)
	{
		RenderTask t;
		t.taskId = Counter++;
		t.startY = y;
		t.endY = y + LINE_STEP;
		t.status = STATE_NONE;
		t.ip = "";
		taskTable.push_back(t);
	}

	std::cout << "Total Tasks: " << taskTable.size() << " created." << std::endl;

	// データ送信するところ
	size_t clientIndex = 0;
	for(auto& task : taskTable)
	{
		if(connectedClients.empty())
		{
			break;
		}

		ClientInfo& targetClient = connectedClients[clientIndex];

		task.status = STATE_QUOTA;
		task.ip = targetClient.ip;

		struct SendPacket
		{
			int pTaskId;
			int pStartY;
			int pEndY;
		}packet;

		packet.pTaskId = htonl(task.taskId);
		packet.pStartY = htonl(task.startY);
		packet.pEndY = htonl(task.endY);

		send(targetClient.sock,(char*)&packet,sizeof(packet),0);

		std::cout << "[Assign] Task " << task.taskId << "(" << task.startY << " - " << task.endY
				  << ") -> Client " << targetClient.ip << std::endl;
	}

    for (auto& c : connectedClients) closesocket(c.sock);
    closesocket(listenSock);
    WSACleanup();

    std::cout << "Press any key to exit." << std::endl;
    _getch();
    return 0;
}