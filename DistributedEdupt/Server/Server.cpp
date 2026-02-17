#include <iostream>
#include <vector>
#include <string>
#include <conio.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <cstdlib>
#include "render_data.h"

#pragma comment(lib, "ws2_32.lib")

const unsigned short SERVER_PORT = 8888;
const int WIN_WIDTH = 800;
const int WIN_HEIGHT = 600;
const int LINE_STEP = 4;

enum State : int8_t
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

// 内部管理用
struct Tile
{
	Tile(int _id,edupt::RenderData _renderData): 
		id(_id),
		renderData(_renderData)
	{}

	int id;
	edupt::RenderData renderData;
};

// 送信用
struct JobData
{
	int64_t mySize;
	State status;
	Tile tile;
};

// 受信用
struct RecvData
{
	int id;
	edupt::Color renderResult;
};

struct RenderTask
{
	int taskId;
	int startY;
	int endY;
	State status;
	std::string ip;
};

namespace
{
	WSADATA wsaData;
	SOCKET listenSock;
	SOCKADDR_IN serverAddr;

	int imageWidth;
	int imageHeight;
	int superSampleNum;
	int sampleNum;
}

int Initialize();

// サーバー自身のIPアドレスを取得する関数
void ShowServerIP();

void DisplayMessage(const std::vector<ClientInfo>& clients);


int main(int argc, char** argv)
{
    // 引数チェック(解像度、スーパーサンプル数、サンプル数)
	if(argc != 5)
	{
		std::cerr << "Invalid argment." << std::endl;
		std::cerr << "Example: ./DEdupt.exe width height supersample sample" << std::endl;
		return -1;
	}

	imageWidth     = atoi(argv[1]);
	imageHeight    = atoi(argv[2]);
	superSampleNum = atoi(argv[3]);
	sampleNum      = atoi(argv[4]);

	if(Initialize() != 0)
	{
		std::cerr << "Initialize failed." << std::endl;
		return -1;
	}

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

    // タイル分割処理
    // 指定された画像の解像度から、タイルの分割サイズを割り出す
    // 大体、64x64程度の正方タイルに分割、
    // 大抵の解像度は、正方タイルで割り切ることができないので、
    // 端っこの方は、正方タイルを諦める。

	// 処理データの用意
	std::vector<Tile> renderDatas;
	int loopNum = imageWidth / 64 * imageHeight / 64;
	int tileWidth = imageWidth / 64;
	int tileHeight = imageHeight / 64;

	for(int i = 0; i < loopNum; i++)
	{
		edupt::RenderData tmp{};
		tmp.width = imageWidth;
		tmp.height = imageHeight;

		tmp.tileWidth = 64;
		tmp.tileHeight = 64;
		
		tmp.offsetX = 64 * i % tileWidth;
		tmp.offsetY = 64 * i / tileWidth;
		
		tmp.sample = sampleNum;
		tmp.superSample = superSampleNum;

		Tile tile(i, tmp);

		renderDatas.push_back(tile);
	}

	// 処理データの変換・送信
	for(int i = 0; i < renderDatas.size(); i++)
	{
		char data[sizeof(JobData)]{};
		int64_t jobDataSize{sizeof(JobData) - sizeof(int64_t)};
		int8_t  state{(int8_t)STATE_QUOTA};
		int index{0};

		memcpy(&data[index],&jobDataSize,sizeof(int64_t));
		memcpy(&data[index += sizeof(int64_t)],&state,sizeof(int8_t));
		//memcpy(&data[index += sizeof(int8_t)],, );


		send(connectedClients[i % connectedClients.size()].sock, data, sizeof(JobData), 0);
	}

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

	// 受信(待機)、画像合成、形式変換(ffmpeg)
	while(true)
	{

	}

    for (auto& c : connectedClients) closesocket(c.sock);
    closesocket(listenSock);
    WSACleanup();

    std::cout << "Press any key to exit." << std::endl;
    _getch();

    return 0;
}

int Initialize()
{
	// 1. WinSock2.2 初期化

	if(WSAStartup(MAKEWORD(2,2),&wsaData) != 0)
	{
		std::cerr << "WSAStartup() failed." << std::endl;
		return -1;
	}

	// 2. リスンソケットの作成
	listenSock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(listenSock == INVALID_SOCKET)
	{
		WSACleanup();
		std::cerr << "socket() failed." << std::endl;
		return -1;
	}

	// ノンブロッキング設定
	unsigned long arg = 0x01;
	if(ioctlsocket(listenSock,FIONBIO,&arg) != 0)
	{
		WSACleanup();
		closesocket(listenSock);
		std::cerr << "ioctlsocket() failed." << std::endl;
		return -1;
	}

	// アドレスの作成
	serverAddr = {0};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(listenSock,(SOCKADDR*)&serverAddr,sizeof(serverAddr)) != 0)
	{
		WSACleanup();
		closesocket(listenSock);
		std::cerr << "bind() failed." << std::endl;
		return -1;
	}

	if(listen(listenSock,5) != 0)
	{
		WSACleanup();
		closesocket(listenSock);
		std::cerr << "listen() failed." << std::endl;
		return -1;
	}

	return 0;
}

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