#include "Client.hpp"

#include "render.h"

#include <cstdio>
#include <sstream>

Client::Client() :
	sock_(INVALID_SOCKET),
	taskQueue_(),
	serverIP_(),
	serverPort_(),
	firstTry_(true)
{
}

Client::~Client()
{
}


RunState Client::Run(const std::string& _args)
{
	if (Initialize() != 0)
	{
		std::cerr << "Initialize failed." << std::endl;
		return RunState::FAIL;
	}

	ConnectServer(_args);

	RecvData();
	SendData();

	Release();

	std::cout << "-------------------------------------------" << std::endl;
	std::cout << "割り当てられた全ての計算が完了したため、サーバーとの通信を終了しました。" << std::endl;
	std::cout << "別のサーバーに接続するか、再試行する場合はIPを入力してください。" << std::endl;
	std::cout << "再試行しますか？ : Y/n" << std::endl;

	switch (getchar()) // ブロッキングしない？
	{
	case 'Y':
	case 'y':
	system("cls");
	return RunState::RETRY;

	case 'N':
	case 'n':
	break;

	default:
	break;
	}

	return RunState::SUCCESS;
}

int Client::Initialize()
{
	//ソケット作成
	sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_ == INVALID_SOCKET)
	{
		int errorCode{WSAGetLastError()};
		std::cerr << "socket() failed." << std::endl;
		std::cerr << "Error code : " << errorCode << std::endl;
		return -1;
	}
	return 0;
}

int Client::Release()
{
	shutdown(sock_, SD_BOTH);
	closesocket(sock_);
	sock_ = INVALID_SOCKET;

	taskQueue_= {};

	return 0;
}

bool Client::ConnectServer(const std::string& _args)
{
	while (true)
	{
		//argv[0]がコマンド名、argv[1]がIP、argv[2]がポート番号
		//引数があるか、2回目以降の再試行かで分岐
		if (firstTry_)
		{
			//初回かつ引数がある場合
			if (ParseArgs(_args) != 0)
			{
				continue;
			}

			std::cout << "コマンドライン引数を使用します: " << serverIP_ << ":" << serverPort_ << std::endl;

			firstTry_ = false;
		}
		else
		{
			//引数がない、または引数での接続に失敗した場合は手入力
			ShowMyIPAddresses();
			std::cout << "\n接続先サーバーIP: ";
			std::cin >> serverIP_;

			std::string portStr{};
			std::cout << "ポート番号: ";
			std::cin >> portStr;

			serverPort_ = (uint16_t)stoi(portStr);
		}

		//接続設定
		SOCKADDR_IN addr = {0};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(serverPort_);
		inet_pton(AF_INET, serverIP_.c_str(), &addr.sin_addr.s_addr);

		//接続実行
		if (connect(sock_, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
		{
			std::cout << "接続失敗 IP: " << serverIP_ << " Port: " << serverPort_ << " へ接続できませんでした。" << std::endl;
			std::cout << "コマンドライン引数、もしくは、接続先IPアドレス・ポート番号が誤っている可能性があります。" << std::endl;

			continue;
		}

		std::cout << "Connected!  サーバーからのタスクを待機します。" << std::endl;
		break;
	}

	return true;
}

int Client::RecvData()
{
	while (true)
	{
		// 受信データ格納用
		char recvRawData[sizeof(JobData)];

		int recvRet = recv(sock_, recvRawData, sizeof(JobData), 0);
		if (recvRet > 0)
		{
			// 解析用ポインタ
			const char* _p = recvRawData;

			JobData tmp{};
			int index{0};

			//ポインタをずらしながら3つ分取り出す
			memcpy(&tmp.mySize, &_p[index], sizeof(tmp.mySize));
			index += sizeof(tmp.mySize);

			memcpy(&tmp.status, &_p[index], sizeof(tmp.status));
			index += sizeof(tmp.status);

			memcpy(&tmp.tile, &_p[index], sizeof(tmp.tile));

			//ネットワークオーダーからホストオーダー変換
			tmp.mySize = ntohll(tmp.mySize);
			tmp.tile.id = ntohl(tmp.tile.id);
			tmp.tile.renderData = tmp.tile.renderData.ChangeEndianNtoH();

			// STATE_COMPLETE_SENDを受け取ったら受信処理を終了
			if (tmp.status == STATE_COMPLETE_SEND)
			{
				return 0;
			}

			std::cout << "タイル #" << tmp.tile.id << " 受信完了。キューへ追加（現在：" << taskQueue_.size() + 1 << "件）" << std::endl;

			taskQueue_.push(tmp);
		}
		else if (recvRet == 0)
		{
			std::cout << "サーバーが接続を閉じました。" << std::endl;
			return 0;
		}
	}
}

int Client::SendData()
{
	while (true)
	{
		if (taskQueue_.empty())
		{
			return 0;
		}

		JobData current = taskQueue_.front();
		taskQueue_.pop();

		const int IMAGE_ARRAY_SIZE{current.tile.renderData.tileWidth * current.tile.renderData.tileHeight};

		// レンダリング処理
		std::cout << "タイル #" << current.tile.id << " を処理中..." << std::endl;
		std::vector<edupt::Color> image{};
		image.resize(IMAGE_ARRAY_SIZE);
		edupt::render(current.tile.renderData, image);
		std::cout << "タイル #" << current.tile.id << " の処理が完了" << std::endl;

		//Store工程、送信データの作成
		const size_t SEND_BUF_SIZE{sizeof(edupt::Color) * IMAGE_ARRAY_SIZE + sizeof(uint32_t) + sizeof(uint32_t)};

		std::vector<char> sendBuf{};
		sendBuf.resize(SEND_BUF_SIZE);

		// エンディアン変換
		uint32_t sendBufSize{htonl(static_cast<uint32_t>(SEND_BUF_SIZE - sizeof(uint32_t)))};

		uint32_t id = htonl(current.tile.id);

		std::vector<edupt::NetVec> image_int{};
		image_int.reserve(IMAGE_ARRAY_SIZE);
		image_int.resize(IMAGE_ARRAY_SIZE);

		if (not(image.empty()))
		{
			for (int i = 0; i < IMAGE_ARRAY_SIZE; i++)
			{
				image_int[i] = image[i].ChangeEndianHtoN();
			}
		}

		//memcpyで詰め込み、ポインタをずらす
		int offset{0};
		memcpy(sendBuf.data(), &sendBufSize, sizeof(sendBufSize));
		offset += sizeof(sendBufSize);
		memcpy(sendBuf.data() + offset, &id, sizeof(id));
		offset += sizeof(id);
		memcpy(sendBuf.data() + offset, image_int.data(),
			   sizeof(edupt::NetVec) * image_int.size());

		//送信実行
		int ret = send(sock_, sendBuf.data(), static_cast<int>(sendBuf.size()), 0);
		std::cout << "タスク #" << current.tile.id << " の計算結果を送信しました。" << std::endl;

		#ifdef _DEBUG
		std::cout << "サイズ：" << ret << std::endl;
		#endif
	}
}

int Client::ParseArgs(const std::string & _args)
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
		std::cerr << "エラー！ 引数の数が不正です。" << std::endl;
		std::cerr << "規定の引数の個数 : 2, your : " << args.size() << std::endl;
		return -1;
	}

	serverIP_   = args[0];
	serverPort_ = stoi(args[1]);

	return 0;
}

void Client::ShowMyIPAddresses() const
{
	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR)
	{
		return;
	}

	ADDRINFOA hints, * res = NULL;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;

	if (getaddrinfo(hostname, NULL, &hints, &res) == 0)
	{
		std::cout << "このPCのIPアドレス:" << std::endl;
		for (ADDRINFOA* p = res; p != NULL; p = p->ai_next)
		{
			char ipStr[INET_ADDRSTRLEN];
			sockaddr_in* addr = (sockaddr_in*)p->ai_addr;
			inet_ntop(AF_INET, &addr->sin_addr, ipStr, sizeof(ipStr));

			std::cout << " >> " << ipStr << std::endl;
		}
		freeaddrinfo(res);
	}
}