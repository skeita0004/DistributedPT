#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <vector>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <queue> 
#include "render.h"


//#include "scene.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

//通信用ピクセルデータ（Store用）
struct Pixel
{
	float r, g, b;
};

//サーバーから届くタスク構造体（Load用）
struct RenderTask
{
	int taskId;
	int startY, endY, width, height;
};

void ShowMyIPAddresses()
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
		cout << "Worker IP List:" << endl;
		for (ADDRINFOA* p = res; p != NULL; p = p->ai_next)
		{
			char ipStr[INET_ADDRSTRLEN];
			struct sockaddr_in* addr = (struct sockaddr_in*)p->ai_addr;
			inet_ntop(AF_INET, &addr->sin_addr, ipStr, sizeof(ipStr));
			cout << " >> " << ipStr << endl;
		}
		freeaddrinfo(res);
	}
}

int main(int argc, char* argv[])
{
	enum State : uint8_t
	{
		STATE_NONE,
		STATE_QUOTA,
		STATE_COMPLETE_SEND,
		STATE_COMPLETION,
		STATE_MAX
	};

	// 内部管理用
	struct Tile
	{
		Tile() :
			id(0),
			renderData()
		{
		}

		Tile(int _id, edupt::RenderData _renderData) :
			id(_id),
			renderData(_renderData)
		{
		}

		void ChangeEndianNtoH()
		{
			id = ntohl(id);
			renderData = renderData.Load();
		}

		void ChangeEndianHtoN()
		{

		}

		int id;
		edupt::RenderData renderData;
	};

	struct JobData
	{
		JobData() :
			mySize(0),
			status(STATE_NONE),
			tile()
		{
		}

		int64_t mySize;
		State status;
		Tile tile;
	};

	struct RenderResult
	{
		uint32_t id;
		edupt::Color* image;
	};

	string serverIP;
	unsigned short serverPort;
	WSADATA wsaData;


	//WinSock初期化
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;

	//届いたタスクを一時的に貯めておくキュー
	queue<JobData> taskQueue;

	//接続ループ
	SOCKET sock = INVALID_SOCKET;
	while (true)
	{
		//argv[0]がコマンド名、argv[1]がIP、argv[2]がポート番号の想定
		//引数があるか、2回目以降の再試行かで分岐
		if (argc == 3)
		{
			//初回かつ引数がある場合
			serverIP = argv[1];
			serverPort = (unsigned short)atoi(argv[2]);
			cout << "コマンドライン引数を使用します: " << serverIP << ":" << serverPort << endl;
			//一度試行したら、失敗時に手入力へ移行するためargcをリセット
			argc = 0;
		}
		else
		{
			//引数がない、または引数での接続に失敗した場合は手入力
			ShowMyIPAddresses();
			string portStr;
			cout << "\n接続先サーバーIP: ";
			cin >> serverIP;

			cout << "ポート番号: ";
			cin >> portStr;

			serverPort = (unsigned short)stoi(portStr);
		}

		//ソケット作成
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == INVALID_SOCKET)
		{
			cout << "ソケット作成失敗" << endl;
			WSACleanup();
			return 1;
		}

		//接続設定
		SOCKADDR_IN addr = {0};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(serverPort);
		inet_pton(AF_INET, serverIP.c_str(), &addr.sin_addr.s_addr);

		//接続実行
		if (connect(sock, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
		{
			cout << "接続失敗 IP: " << serverIP << " Port: " << serverPort << " に繋げませんでした。" << endl;
			closesocket(sock);
			// ループの最初（入力待ち）に戻る
			continue;
		}

		//接続成功して初めてこのメッセージを出す
		cout << "Connected!  サーバーからのタスクを待機します。" << endl;
		break; //接続できたので入力ループを脱出
	}

	while (true)
	{
		while (true)
		{
			//ここから12バイトに合わせた変更

			// 受信データのサイズを取得する
			// 受信バッファのサイズを取得したサイズでリサイズする

			// 受信データ格納用
			char recvRawData[sizeof(JobData)];

			int recvRet = recv(sock, recvRawData, sizeof(JobData), 0);
			if (recvRet > 0)
			{
				const char* _p = recvRawData; // 解析用ポインタ

				//int nID, nStartY, nEndY;
				JobData tmp{};
				int index{0};

				//ポインタをずらしながら3つ分取り出す
				//int rawTaskId, rawStartY, rawEndY;
				//memcpy( &nID, _p, 4 );     _p += 4;
				//memcpy( &nStartY, _p, 4 ); _p += 4;
				//memcpy( &nEndY, _p, 4 );
				memcpy(&tmp.mySize, &_p[index], sizeof(tmp.mySize));
				index += sizeof(tmp.mySize);

				memcpy(&tmp.status, &_p[index], sizeof(tmp.status));
				index += sizeof(tmp.status);

				memcpy(&tmp.tile, &_p[index], sizeof(tmp.tile));

				//ネットワークオーダーからホストオーダー変換
				//RenderTask task;
				//task.taskId = ntohl( nID );
				//task.startY = ntohl( nStartY );
				//task.endY = ntohl( nEndY );
				tmp.mySize = ntohl(tmp.mySize);
				tmp.tile.ChangeEndianNtoH();

				////サーバーから送られてこない情報はクライアント側の定数で補完
				//task.width = 800; // Server.cppのWIN_WIDTH
				//task.height = 600; // Server.cppのWIN_HEIGHT

				cout << "タスク #" << tmp.tile.id << " 受信完了。キューへ追加（現在：" << taskQueue.size() + 1 << "件）" << endl;

				if (tmp.status == STATE_COMPLETE_SEND)
				{
					break;
				}

				taskQueue.push(tmp);
			}
			else if (recvRet == 0)
			{
				cout << "サーバーが接続を閉じました。" << endl;
				break;
			}
			else
			{
				//何も届いていない場合はループの先頭に戻って待機
				continue;
			}
		}

		//計算・報告フェーズ
		//キューに溜まったタスクを順次処理
		while (!taskQueue.empty())
		{
			JobData current = taskQueue.front();
			taskQueue.pop();
			const int IMAGE_ARRAY_SIZE{current.tile.renderData.tileWidth * current.tile.renderData.tileHeight};

			// レンダリング処理
			cout << "タスク #" << current.tile.id << " を処理中..." << endl;
			std::vector<edupt::Color> image{};
			image.reserve(IMAGE_ARRAY_SIZE);
			image.resize(IMAGE_ARRAY_SIZE);
			edupt::render(current.tile.renderData, image);
			cout << "タスク #" << current.tile.id << " の処理が完了" << endl;


			// 割り当て確認の返信(ACK)
			// いらないかもしれない

			//Store工程、送信データの作成
			const size_t SEND_BUF_SIZE{sizeof(edupt::Color) * IMAGE_ARRAY_SIZE + sizeof(uint32_t) + sizeof(uint32_t)};

			std::vector<char> sendBuf{};
			sendBuf.reserve(SEND_BUF_SIZE);
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

			////memcpyで詰め込み、ポインタをずらす
			//memcpy(_sendP, &sSize, sizeof(int));  _sendP += sizeof(int);
			//memcpy(_sendP, &sState, sizeof(int)); _sendP += sizeof(int);
			int offset{0};
			memcpy(sendBuf.data(), &sendBufSize, sizeof(sendBufSize));
			offset += sizeof(sendBufSize);
			memcpy(sendBuf.data() + offset, &id, sizeof(id));
			offset += sizeof(id);
			memcpy(sendBuf.data() + offset, image_int.data(),
				   sizeof(edupt::NetVec) * image_int.size());

			//最後に計算データ本体をコピー
			//memcpy(_sendP, lineData.data(), pixelDataSize);

			//送信実行
			int ret = send(sock, sendBuf.data(), sendBuf.size(), 0);
			cout << "タスク #" << current.tile.id << " の計算結果を送信しました。" << endl;

#ifdef _DEBUG
			cout << "サイズ：" << ret << endl;
#endif
		}

		shutdown(sock, SD_BOTH);
		closesocket(sock);
		cout << "-------------------------------------------" << endl;
		cout << "割り当てられた全ての計算が完了したため、サーバーとの通信を終了しました。" << endl;
		cout << "別のサーバーに接続するか、再試行する場合はIPを入力してください。" << endl;
	}
	WSACleanup();
	return 0;
}