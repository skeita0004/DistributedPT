#include "Server.h"


Server::Server() :
	imageWidth_(0),
	imageHeight_(0),
	superSampleNum_(0),
	sampleNum_(0),
	wsaData_(),
	listenSock_(0),
	serverAddr_(),
	connectedClients_(),
	renderData_(),
	localClient_(nullptr)
{
}

Server::~Server()
{
}

int Server::Initialize(char** _argv)
{
	GetCommandLineArgs(_argv);

	// 1. WinSock2.2 初期化
	if (WSAStartup(MAKEWORD(2, 2), &wsaData_) != 0)
	{
		int errorCode{WSAGetLastError()};
		std::cerr << "Error code : " << errorCode << std::endl;
		std::cerr << "WSAStartup() failed." << std::endl;
		return -1;
	}

	// 2. リスンソケットの作成
	listenSock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock_ == INVALID_SOCKET)
	{
		int errorCode{WSAGetLastError()};
		std::cerr << "Error code : " << errorCode << std::endl;
		std::cerr << "socket() failed." << std::endl;
		WSACleanup();
		return -1;
	}

	// ノンブロッキング設定
	unsigned long arg = 0x01;
	if (ioctlsocket(listenSock_, FIONBIO, &arg) != 0)
	{
		int errorCode{WSAGetLastError()};
		std::cerr << "Error code : " << errorCode << std::endl;
		std::cerr << "ioctlsocket() failed." << std::endl;
		WSACleanup();
		closesocket(listenSock_);
		return -1;
	}

	// アドレスの作成
	serverAddr_ = {0};
	serverAddr_.sin_family = AF_INET;
	serverAddr_.sin_port = htons(PORT_);
	serverAddr_.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenSock_, (SOCKADDR*)&serverAddr_, sizeof(serverAddr_)) != 0)
	{
		int errorCode{WSAGetLastError()};
		std::cerr << "Error code : " << errorCode << std::endl;
		std::cerr << "bind() failed." << std::endl;
		WSACleanup();
		closesocket(listenSock_);
		return -1;
	}

	if (listen(listenSock_, 5) != 0)
	{
		int errorCode{WSAGetLastError()};
		std::cerr << "Error code : " << errorCode << std::endl;
		std::cerr << "listen() failed." << std::endl;
		WSACleanup();
		closesocket(listenSock_);
		return -1;
	}

	ShowServerIP();
	localClient_ = new LocalClient();
	AcceptLocalClient();

	return 0;
}

int Server::Release()
{
	for (auto& c : connectedClients_)
	{
		closesocket(c.sock);
	}

	closesocket(listenSock_);
	WSACleanup();
	localClient_->Terminate();
	delete localClient_;

	return 0;

}

void Server::AcceptLocalClient()
{
	std::cout << "ローカルクライアント起動" << std::endl;
	// ローカルクライアント用に後でループ分ける
	std::string localClientPath{"./resource/Client.exe"};
	std::string ipAddress{"127.0.0.1"};

#ifdef _DEBUG
	//localClientPath = "D:\\GE2A22\\home\\PG\\repos\\DistributedEdupt\\DistributedEdupt\\x64\\Debug\\Client.exe";
	localClientPath = "C:/Users/saito/source/repos/DistributedEdupt/DistributedEdupt/x64/Debug/Client.exe";
#endif

	if (localClient_->Launch(localClientPath,
		"127.0.0.1",
		std::to_string(PORT_)) == FALSE)
	{
		std::cerr << "ローカルクライアントの起動に失敗" << std::endl;
		return;
	}
	std::cout << "ローカルクライアントの起動完了、接続を待機します。" << std::endl;

	while (true)
	{
		JoinClient();
		
		if (connectedClients_.empty() == false)
		{
			// 一番最初に接続するのはローカルクライアントなので、beginで問題なし
			if (connectedClients_.begin()->ip == ipAddress)
			{
				std::cout << "ローカルクライアントからの接続を受け付けました" << std::endl;
				break;
			}
		}
	}
}

void Server::ShowServerIP()
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

void Server::DisplayMessage(const std::vector<ClientInfo>& clients)
{
	system("cls");
	std::cout << "Server Waiting..." << std::endl;
	ShowServerIP();
	std::cout << "------------------" << std::endl;

	std::cout << "Client Connected:" << clients.size() << std::endl;
	for (size_t i = 0; i < clients.size(); ++i)
	{
		std::cout << " [" << i << "] IP Address: " << clients[i].ip << std::endl;
	}
	std::cout << "Press Enter to stop accepting client connections..." << std::endl;
}

void Server::GetCommandLineArgs(char** _argv)
{
	imageWidth_ = atoi(_argv[1]);
	imageHeight_ = atoi(_argv[2]);
	superSampleNum_ = atoi(_argv[3]);
	sampleNum_ = atoi(_argv[4]);
}

void Server::RecvData()
{
	for (auto client = connectedClients_.begin(); client != connectedClients_.end();)
	{
		// レンダリング済みタイルの要素数
		const int IMAGE_ARRAY_SIZE{TILE_SIZE_ * TILE_SIZE_};

		// 受信するデータ本体のサイズ
		const int RECV_BUF_SIZE{sizeof(int) + sizeof(edupt::Color) * IMAGE_ARRAY_SIZE};
		
		// 合計受信データサイズ
		int totalRet{0};
		
		// ヘッダ読み込み
		if (client->state == ClientInfo::State::HEAD_WAITING or 
			client->state == ClientInfo::State::HEAD_RECEIVENG)
		{
			int ret = recv(client->sock,
						   client->headBuf.data() + client->headReceivedSize,
						   sizeof(client->bodySize) - client->headReceivedSize,
						   0);
			if (ret > 0)
			{
				client->headReceivedSize += ret;
				client->state = ClientInfo::State::HEAD_RECEIVENG;
			}
			else if (ret == 0)
			{
				std::cout << "クライアント: " << client->ip << "が接続を解除しました" << std::endl;
				closesocket(client->sock);
				client = connectedClients_.erase(client);
				continue;
			}
			else
			{
				if (WSAGetLastError() == WSAEWOULDBLOCK)
				{
					++client;
					continue;
				}
				else
				{
					// エラーメッセージ表示
					std::cout << WSAGetLastError() << std::endl;

					++client;
					continue;

				}
			}

			if (client->headReceivedSize == sizeof(client->bodySize))
			{
				client->state = ClientInfo::State::BODY_WAITING;
				memcpy(&client->bodySize, client->headBuf.data(), sizeof(client->bodySize));
				client->bodySize = ntohl(client->bodySize);
				client->bodyBuf.resize(client->bodySize);
			}
		}

		// ボデー読み込み
		if (client->state == ClientInfo::State::BODY_WAITING or
			client->state == ClientInfo::State::BODY_RECEIVENG)
		{
			int ret = recv(client->sock,
			   client->bodyBuf.data() + client->bodyReceivedSize,
			   client->bodySize - client->bodyReceivedSize,
			   0);
			if (ret > 0)
			{
				client->bodyReceivedSize += ret;
				client->state = ClientInfo::State::BODY_RECEIVENG;
			}
			else if (ret == 0)
			{
				std::cout << "クライアント: " << client->ip << "が接続を解除しました" << std::endl;
				closesocket(client->sock);
				client = connectedClients_.erase(client);
				continue;
			}
			else
			{
				if (WSAGetLastError() == WSAEWOULDBLOCK)
				{
					++client;
					continue;
				}
				else
				{
					// エラーメッセージ表示
					++client;
					continue;
				}
			}

			if (client->bodyReceivedSize == client->bodySize)
			{
				client->state = ClientInfo::State::ALL_COMPLETE;
			}
		}

		if (client->state == ClientInfo::State::ALL_COMPLETE)
		{
			// ポインタ演算用
			int index{0};

			// 受信用一時オブジェクト
			RenderResult tmp;
			tmp.id = 0;

			memcpy(&tmp.id, client->bodyBuf.data(), sizeof(tmp.id));
			index += sizeof(tmp.id);

			tmp.renderResult.resize(IMAGE_ARRAY_SIZE);

			// 配列のエンディアン変換処理
			std::vector<edupt::NetVec> imageInt{};
			imageInt.resize(IMAGE_ARRAY_SIZE);
			memcpy(imageInt.data(), client->bodyBuf.data() + index, IMAGE_ARRAY_SIZE * sizeof(edupt::Color));

			tmp.id = ntohl(tmp.id);

			for (int i = 0; i < IMAGE_ARRAY_SIZE; i++)
			{
				tmp.renderResult[i] = edupt::Vec().ChangeEndianNtoH(imageInt[i]);
			}

			std::cout << "レンダリング済みデータ(id: " << tmp.id << ")を受信しました" << std::endl;
			renderResult_.push_back(tmp);

			//client->headBuf.clear();
			//client->bodyBuf.clear();
			client->bodySize = 0;
			client->headReceivedSize = 0;
			client->bodyReceivedSize = 0;
			client->state = ClientInfo::State::HEAD_WAITING;
		}

		++client;
	}
}

const std::vector<Server::RenderResult>& Server::GetRenderResult()
{
	return renderResult_;
}

void Server::JoinClient()
{
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(clientAddr);
	SOCKET newSock = accept(listenSock_, (SOCKADDR*)&clientAddr, &addrLen);

	if (newSock != INVALID_SOCKET)
	{
		char ipStr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));

		ClientInfo info = {newSock,
						   std::string(ipStr),
						   std::vector<char>{},
						   std::vector<char>{},
						   0,
						   0,
						   0,
						   ClientInfo::State::HEAD_WAITING};
		info.headBuf.resize(sizeof(uint32_t));

		connectedClients_.push_back(info);

		// TODO:シーンデータ送信(余力があれば)

		DisplayMessage(connectedClients_); // 接続があったので表示更新
	}
}

void Server::PreparationSendData()
{
	// 処理データの用意
	int tileNumWidth  = (int)std::ceil(imageWidth_  / (float)TILE_SIZE_);
	int tileNumHeight = (int)std::ceil(imageHeight_ / (float)TILE_SIZE_);

	int loopNum = tileNumWidth * tileNumHeight;
	totalTileNum_ = loopNum;

	std::string ffmpegPath{"./resource/ffmpeg.exe"};

#ifdef _DEBUG
	ffmpegPath = "ffmpeg";
#endif

	ffmpegArgs_ = ffmpegPath + "-y -i ./out%d.ppm -vf \"tile=";
	ffmpegArgs_ += std::to_string(tileNumWidth) + "x" + std::to_string(tileNumHeight);
	ffmpegArgs_ += ",hflip,vflip,crop=" + std::to_string(imageWidth_) + ":" + std::to_string(imageHeight_);
	ffmpegArgs_ += ":0:0\" ./render_result.png";

	for (int i = 0; i < loopNum; i++)
	{
		edupt::RenderData tmp{};
		tmp.width       = imageWidth_;
		tmp.height      = imageHeight_;

		tmp.tileWidth   = TILE_SIZE_;
		tmp.tileHeight  = TILE_SIZE_;

		tmp.offsetX     = TILE_SIZE_ * (i % tileNumWidth);
		tmp.offsetY     = TILE_SIZE_ * (i / tileNumWidth);

		tmp.sample      = sampleNum_;
		tmp.superSample = superSampleNum_;

		Tile tile(i, tmp);

		renderData_.push_back(tile);
	}
}

void Server::SendData()
{
	// クライアントが0であれば終了

	// 処理データの変換・送信
	for (int i = 0; i < renderData_.size(); i++)
	{
		JobData sendData{
			sizeof(JobData) - sizeof(int64_t),
			STATE_NONE,
			renderData_[i]};

		char data[sizeof(JobData)]{};

		uint64_t jobDataSize{htonll(sendData.mySize)};
		uint8_t  state{(int8_t)STATE_QUOTA};
		uint32_t id{htonl(sendData.tile.id)};

		char pRenderData[sizeof(edupt::RenderData)]{};
		int index{0};


		memcpy(&data[index], &jobDataSize, sizeof(jobDataSize));
		index += sizeof(jobDataSize);

		memcpy(&data[index], &state, sizeof(state));
		index += sizeof(state);

		memcpy(&data[index], &id, sizeof(id));
		index += sizeof(id);

		memcpy(&data[index], sendData.tile.renderData.Store(pRenderData), sizeof(edupt::RenderData));

		// クライアントに対し、順番に送信
		send(connectedClients_[i % connectedClients_.size()].sock, data, sizeof(JobData), 0);
	}

	// タイル情報を全て送り終えたら、全てのクライアントに対して、計算命令を出す
	for (auto& client : connectedClients_)
	{
		JobData sendData{};

		sendData.status = STATE_COMPLETE_SEND;

		char data[sizeof(JobData)]{};

		uint64_t jobDataSize{htonll(sendData.mySize)};
		uint8_t  state{sendData.status};
		uint32_t id{htonl(sendData.tile.id)};

		char pRenderData[sizeof(edupt::RenderData)]{};
		int index{0};


		memcpy(&data[index], &jobDataSize, sizeof(jobDataSize));
		index += sizeof(jobDataSize);

		memcpy(&data[index], &state, sizeof(state));
		index += sizeof(state);

		memcpy(&data[index], &id, sizeof(id));
		index += sizeof(id);

		memcpy(&data[index], sendData.tile.renderData.Store(pRenderData), sizeof(edupt::RenderData));

		send(client.sock, data, sizeof(JobData), 0);
	}
}

// 現在未使用
void Server::SendDataStab()
{
	const int WIN_WIDTH = 800;
	const int WIN_HEIGHT = 600;
	const int LINE_STEP = 4;

	std::vector<RenderTask> taskTable;
	int Counter = 0;
	for (int y = 0; y < WIN_HEIGHT; y += LINE_STEP)
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
	for (auto& task : taskTable)
	{
		if (connectedClients_.empty())
		{
			break;
		}

		ClientInfo& targetClient = connectedClients_[clientIndex];

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

		int sendResult = send(targetClient.sock, (char*)&packet, sizeof(packet), 0);

		if (sendResult != SOCKET_ERROR)
		{
			std::cout << "[Assign] Task " << task.taskId << "(" << task.startY << " - " << task.endY
				<< ") -> Client " << targetClient.ip << std::endl;
		}
		else
		{
			std::cout << "[Error] Failed to send task " << task.taskId << " to " << targetClient.ip << std::endl;
		}

		// タスクを送るクライアントを更新
		clientIndex = (clientIndex + 1) % connectedClients_.size();
	}
}
