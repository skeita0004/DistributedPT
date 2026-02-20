#include <iostream>
#include <vector>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <queue> 

//#include "scene.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

//通信用ピクセルデータ（Store用）
struct Pixel {
	float r,g,b;
};

//サーバーから届くタスク構造体（Load用）
struct RenderTask {
	int taskId;
	int startY,endY,width,height;
};

void ShowMyIPAddresses()
{
	char hostname[256];
	if(gethostname(hostname,sizeof(hostname)) == SOCKET_ERROR) return;
	ADDRINFOA hints,* res = NULL;
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET;
	if(getaddrinfo(hostname,NULL,&hints,&res) == 0) {
		cout << "Worker IP List:" << endl;
		for(ADDRINFOA* p = res; p != NULL; p = p->ai_next) {
			char ipStr[INET_ADDRSTRLEN];
			struct sockaddr_in* addr = (struct sockaddr_in*)p->ai_addr;
			inet_ntop(AF_INET,&addr->sin_addr,ipStr,sizeof(ipStr));
			cout << " >> " << ipStr << endl;
		}
		freeaddrinfo(res);
	}
}

int main(int argc,char* argv[])
{
	string serverIP;
	unsigned short serverPort;
	WSADATA wsaData;

	//WinSock初期化
	if(WSAStartup(MAKEWORD(2,2),&wsaData) != 0) return 1;

	//届いたタスクを一時的に貯めておくキュー
	queue<RenderTask> taskQueue;

	//接続ループ
	SOCKET sock = INVALID_SOCKET;
	while(true)
	{
		//argv[0]がコマンド名、argv[1]がIP、argv[2]がポート番号の想定
		//引数があるか、2回目以降の再試行かで分岐
		if(argc == 3) {
			//初回かつ引数がある場合
			serverIP = argv[1];
			serverPort = (unsigned short)atoi(argv[2]);
			cout << "コマンドライン引数を使用します: " << serverIP << ":" << serverPort << endl;
			//一度試行したら、失敗時に手入力へ移行するためargcをリセット
			argc = 0;
		} else {
			//引数がない、または引数での接続に失敗した場合は手入力
			ShowMyIPAddresses();
			string portStr;
			cout << "\n接続先サーバーIP: "; cin >> serverIP;
			cout << "ポート番号: ";      cin >> portStr;
			serverPort = (unsigned short)stoi(portStr);
		}

		//ソケット作成
		sock = socket(AF_INET,SOCK_STREAM,0);
		if(sock == INVALID_SOCKET) {
			cout << "ソケット作成失敗" << endl;
			WSACleanup();
			return 1;
		}

		//接続設定
		SOCKADDR_IN addr = {0};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(serverPort);
		inet_pton(AF_INET,serverIP.c_str(),&addr.sin_addr.s_addr);

		//接続実行
		if(connect(sock,(SOCKADDR*)&addr,sizeof(addr)) == SOCKET_ERROR)
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

	while(true)
	{
		//string ip,portStr;
		//cout << "\n接続先サーバーIP: "; cin >> ip;
		//cout << "ポート番号: "; cin >> portStr;

		//SOCKET sock = socket(AF_INET,SOCK_STREAM,0);
		//SOCKADDR_IN addr = {0};
		//addr.sin_family = AF_INET;
		//addr.sin_port = htons((unsigned short)stoi(portStr));
		//inet_pton(AF_INET,ip.c_str(),&addr.sin_addr.s_addr);

		//if(connect(sock,(SOCKADDR*)&addr,sizeof(addr)) == SOCKET_ERROR)
		//{
		//	cout << "接続失敗" << endl;
		//	closesocket(sock);
		//	continue;
		//}

		//cout << "Connected! サーバーからのタスクを待機します。" << endl;

		while(true)
		{
			//ここから12バイトに合わせた変更
			char taskBuf[12];
			int recvRet = recv(sock,taskBuf,sizeof(taskBuf),0);

			if(recvRet > 0) {
				const char* _p = taskBuf; // 解析用ポインタ
				int nID,nStartY,nEndY;
				//ポインタをずらしながら3つ分取り出す
				int rawTaskId,rawStartY,rawEndY;
				memcpy(&nID,_p,4);     _p += 4;
				memcpy(&nStartY,_p,4); _p += 4;
				memcpy(&nEndY,_p,4);

				//ネットワークオーダーからホストオーダー変換
				RenderTask task;
				task.taskId = ntohl(nID);
				task.startY = ntohl(nStartY);
				task.endY   = ntohl(nEndY);

				//サーバーから送られてこない情報はクライアント側の定数で補完
				task.width  = 800; // Server.cppのWIN_WIDTH
				task.height = 600; // Server.cppのWIN_HEIGHT

				cout << "タスク #" << task.taskId << " 受信完了。キューへ追加（現在：" << taskQueue.size() + 1 << "件）" << endl;

				taskQueue.push(task);
			} else if(recvRet == 0)
			{
				cout << "サーバーが接続を閉じました。" << endl;
				break;
			} else
			{
				//何も届いていない場合はループの先頭に戻って待機
				continue;
			}

			//計算・報告フェーズ
			//キューに溜まったタスクを順次処理
			while(!taskQueue.empty())
			{
				RenderTask current = taskQueue.front();
				taskQueue.pop();

				//割り当て確認の返信(ACK)
				int ackId = htonl(current.taskId);
				send(sock,(char*)&ackId,sizeof(ackId),0);

				cout << "タスク #" << current.taskId << " を処理中..." << endl;
				
				//edupt計算はわからんので、ここではダミーデータを生成
				vector<Pixel> lineData(current.width);
				for(int x = 0; x < current.width; x++) {
					lineData[x].r = (float)x / (float)current.width;
					lineData[x].g = (float)x / (float)current.width;
					lineData[x].b = (float)x / (float)current.width;
				}

				//Store工程、送信データの作成
				int pixelDataSize = (int)(sizeof(Pixel) * current.width);
				vector<char> sendBuf(8 + pixelDataSize);
				char* _sendP = sendBuf.data();

				//最初にエンディアンをhostからnetに変換
				int sSize  = htonl(pixelDataSize);
				int sState = htonl(2);//COMPLETION

				//memcpyで詰め込み、ポインタをずらす
				memcpy(_sendP,&sSize,sizeof(int));  _sendP += sizeof(int);
				memcpy(_sendP,&sState,sizeof(int)); _sendP += sizeof(int);

				//最後に計算データ本体をコピー
				memcpy(_sendP,lineData.data(),pixelDataSize);

				//送信実行
				send(sock,sendBuf.data(),(int)sendBuf.size(),0);
				cout << "タスク #" << current.taskId << " の計算結果を送信しました。" << endl;
			}
		}
		closesocket(sock);
		cout << "-------------------------------------------" << endl;
		cout << "サーバーとの通信を終了しました。" << endl;
		cout << "別のサーバーに接続するか、再試行する場合はIPを入力してください。" << endl;
	}
	WSACleanup();
	return 0;
}