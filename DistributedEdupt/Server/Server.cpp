#define _CRT_SECURE_NO_WARNINGS

#include "Server.hpp"
#include "SubProcess.hpp"

#include "ppm.h"

#include <conio.h>
#include <sstream>
#include <system_error>

Server::Server() :
	imageWidth_(0),
	imageHeight_(0),
	superSampleNum_(0),
	sampleNum_(0),
	totalTileNum_(0),
	listenSock_(INVALID_SOCKET),
	serverAddr_(),
	connectedClients_(),
	renderData_(),
	localClient_(nullptr),
	viewer_(nullptr)
{
}

Server::~Server()
{
	for (auto& c : connectedClients_)
	{
		closesocket(c.sock);
	}

	closesocket(listenSock_);

	if (localClient_)
	{
		delete localClient_;
	}

	if (viewer_)
	{
		delete viewer_;
	}
}

RunState Server::Run(const std::string& _args)
{
	if (Initialize(_args) != 0)
	{
		std::cerr << "Initialize に失敗。" << std::endl;
		return RunState::FAIL;
	}

	// --- クライアント接続受付フェーズ ---
	std::cout << "クライアントの接続を待機中..." << std::endl;
	while (true) // このループはJoinClientの中で行うようにする
	{
		JoinClient();

		if (_kbhit() == TRUE and _getch() == '\n')
		{
			break;
		}

		Sleep(100);
	}
	std::cout << "\n接続を締め切りました。" << std::endl;
	std::cout << "計算フェーズへ移行します..." << std::endl;

	PreparationSendData();
	SendData();

	std::cout << "全てのレンダリングデータの送信が完了しました" << std::endl;

	// 受信(待機)、画像合成、形式変換(ffmpeg)
	while (true)
	{
		Sleep(10);
		// 送信したタイルの数分、タイルを受信できるまで受信処理
		RecvData();

		if (renderResult_.size() >= totalTileNum_)
		{
			break;
		}
	}

	// 2.1.が終わったら、画像を合成し、リサイズ、形式変換
	system(ffmpegArgs_.c_str());


	std::cout << "いずれかのキーを押すと終了します。" << std::endl;
	_getch();

	return RunState::SUCCESS;
}

int Server::Initialize(const std::string& _args)
{
	if (ParseArgs(_args) != 0)
	{
		std::cerr << "エラー！不正な引数です。" << std::endl;
	}

	// リスンソケットの作成
	listenSock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock_ == INVALID_SOCKET)
	{
		int errorCode{WSAGetLastError()};
		std::cerr << "socket() 失敗。" << std::endl;
		std::cerr << "エラーコード : " << errorCode << std::endl;
		std::cerr << std::system_category().message(errorCode) << std::endl;
		return -1;
	}

	// ノンブロッキング設定
	unsigned long arg = 0x01;
	if (ioctlsocket(listenSock_, FIONBIO, &arg) != 0)
	{
		int errorCode{WSAGetLastError()};
		std::cerr << "ioctlsocket() failed." << std::endl;
		std::cerr << "Error code : " << errorCode << std::endl;
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
		std::cerr << "bind() failed." << std::endl;
		std::cerr << "Error code : " << errorCode << std::endl;
		closesocket(listenSock_);
		return -1;
	}

	if (listen(listenSock_, 5) != 0)
	{
		int errorCode{WSAGetLastError()};
		std::cerr << "listen() failed." << std::endl;
		std::cerr << "Error code : " << errorCode << std::endl;
		closesocket(listenSock_);
		return -1;
	}

	ShowServerIP();
	
	localClient_ = new SubProcess();
	JoinLocalClient();

	viewer_ = new SubProcess();

	return 0;
}

int Server::ParseArgs(const std::string& _args)
{
	// 引数文字列を解析する
	std::istringstream iss{_args};
	std::string arg{};

	std::vector<std::string> args{};

	// 空白ごとに分解
	while (std::getline(iss, arg, ' '))
	{
		args.push_back(arg);
	}
	
	// 引数の個数チェック
	if (args.size() < ARG_NUM_)
	{
		std::cerr << "ERROR! Insufficient arguments" << std::endl;
		std::cerr << "Default num of arguments : 3, your : " << args.size() << std::endl;
		return -1;
	}

	size_t pos = args[0].find("x");

	// 区切り文字 'x' が、一つだけかチェック
	if (pos == std::string::npos and args[0].find('x', pos + 1) == std::string::npos)
	{
		std::cerr << "エラー！ 解像度の指定が不正です。" << std::endl;
		std::cerr << args[0] << std::endl;
	}

	imageWidth_  = stoi(args[0].substr(0, pos));
	imageHeight_ = stoi(args[0].substr(pos + 1));

	superSampleNum_ = stoi(args[1]);
	sampleNum_      = stoi(args[2]);

	return 0;
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
	tileSize_ = 64;

	tileNumX_ = (int)std::ceil(imageWidth_  / (float)tileSize_);
	tileNumY_ = (int)std::ceil(imageHeight_ / (float)tileSize_);

	int loopNum = tileNumX_ * tileNumY_;
	totalTileNum_ = loopNum;

	ffmpegPath_ = ".\\resource\\ffmpeg.exe";

#ifdef _DEBUG
	// ffmpegのパスが通っていること前提
	ffmpegPath_ = "ffmpeg";
#endif

	ffmpegArgs_ = ffmpegPath_ + " -y -i ./out%d.ppm -vf \"tile=";
	ffmpegArgs_ += std::to_string(tileNumX_) + "x" + std::to_string(tileNumY_);
	ffmpegArgs_ += ",hflip,vflip,crop=" + std::to_string(imageWidth_) + ":" + std::to_string(imageHeight_);
	ffmpegArgs_ += ":0:0\" ./render_result.png";

	std::vector<edupt::Color> initImage(tileSize_ * tileSize_, 0.0);

	for (int i = 0; i < loopNum; i++)
	{
		// レンダリング可視化用に、レンダリング前の画像を用意
		edupt::save_ppm_file("out" + std::to_string(i) + ".ppm", initImage.data(), tileSize_, tileSize_);

		std::string ppmToPng{ffmpegPath_ + " -loglevel quiet -y -i " + "out" + std::to_string(i) + ".ppm" + " out" + std::to_string(i) + ".png"};
		system(ppmToPng.c_str());

		edupt::RenderData tmp{};
		tmp.width       = imageWidth_;
		tmp.height      = imageHeight_;

		tmp.tileWidth   = tileSize_;
		tmp.tileHeight  = tileSize_;

		tmp.offsetX     = tileSize_ * (i % tileNumX_);
		tmp.offsetY     = tileSize_ * (i / tileNumX_);

		tmp.sample      = sampleNum_;
		tmp.superSample = superSampleNum_;

		Tile tile(i, tmp);

		renderData_.push_back(tile);
	}

	// このタイミングでビューワ起動
	LaunchViewer();
}

void Server::SendData()
{
	if (connectedClients_.empty())
	{
		std::cerr << "クライアントが1人もいません。" << std::endl;
		return;
	}

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

		memcpy(&data[index], sendData.tile.renderData.ChangeEndianHtoN(pRenderData), sizeof(edupt::RenderData));

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

		memcpy(&data[index], sendData.tile.renderData.ChangeEndianHtoN(pRenderData), sizeof(edupt::RenderData));

		send(client.sock, data, sizeof(JobData), 0);
	}
}

void Server::RecvData()
{
	for (auto client = connectedClients_.begin(); client != connectedClients_.end();)
	{
		// レンダリング済みタイルの要素数
		int IMAGE_ARRAY_SIZE{tileSize_ * tileSize_};

		// 受信するデータ本体のサイズ
		int RECV_BUF_SIZE{(int)sizeof(int) + (int)sizeof(edupt::Color) * IMAGE_ARRAY_SIZE};
		
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

			edupt::save_ppm_file("out" + std::to_string(tmp.id) + ".ppm",
								 tmp.renderResult.data(), 
								 tileSize_,
								 tileSize_);

			std::string ppmToPng{ffmpegPath_ + " -loglevel quiet -y -i " + "out" + std::to_string(tmp.id) + ".ppm" + " out" + std::to_string(tmp.id) + ".png"};
			system(ppmToPng.c_str());

			client->bodySize = 0;
			client->headReceivedSize = 0;
			client->bodyReceivedSize = 0;
			client->state = ClientInfo::State::HEAD_WAITING;
		}

		++client;
	}
}

void Server::JoinLocalClient()
{
	std::cout << "ローカルクライアント起動" << std::endl;
	
	std::string localClientPath{LOCAL_CLIENT_EXEPATH_};
	std::string ipAddress{LOCAL_CLIENT_IP_};
	std::string portNum{std::to_string(PORT_)};

	std::string cmdLine{"-c " + localClientPath + " " + ipAddress + " " + portNum};

#ifdef _DEBUG
	localClientPath = ".\\..\\x64\\Debug\\Server.exe";
#endif

	if (localClient_->Launch(cmdLine, CREATE_NEW_CONSOLE) == FALSE)
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
		Sleep(10);
	}
}

void Server::LaunchViewer()
{
	std::cout << "ビューワ起動" << std::endl;

	std::string viewerPath{VIEWER_EXEPATH_};

	std::string cmdLine{viewerPath + " " +
		std::to_string(imageWidth_)+ " " +
		std::to_string(imageHeight_) + " " +
		std::to_string(tileNumX_) + " " +
		std::to_string(tileNumY_) + " " +
		std::to_string(tileSize_)};

	#ifdef _DEBUG
	viewerPath = ".\\..\\x64\\Debug\\Viewer.exe";
	#endif

	if (viewer_->Launch(cmdLine, 0) == FALSE)
	{
		std::cerr << "ビューワの起動に失敗" << std::endl;
		return;
	}
	std::cout << "ビューワの起動完了、サーバに戻って操作を進めてください。" << std::endl;
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

void Server::ShowServerIP()
{
	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)) == 0)
	{
		struct addrinfo hints = {}, *res = nullptr;
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
