#include <iostream>
#include <vector>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>
#include <queue> 

#include "scene.h"

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

int main()
{
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2,2),&wsaData) != 0) return 1;

	ShowMyIPAddresses();

	//届いたタスクを一時的に貯めておくキュー
	queue<RenderTask> taskQueue;

	while(true)
	{
		string ip,portStr;
		cout << "\n接続先サーバーIP: "; cin >> ip;
		cout << "ポート番号: "; cin >> portStr;

		SOCKET sock = socket(AF_INET,SOCK_STREAM,0);
		SOCKADDR_IN addr = {0};
		addr.sin_family = AF_INET;
		addr.sin_port = htons((unsigned short)stoi(portStr));
		inet_pton(AF_INET,ip.c_str(),&addr.sin_addr.s_addr);

		if(connect(sock,(SOCKADDR*)&addr,sizeof(addr)) == SOCKET_ERROR)
		{
			cout << "接続失敗" << endl;
			closesocket(sock);
			continue;
		}

		cout << "Connected! サーバーからのタスクを待機します。" << endl;

		while(true)
		{
			//Load工程、ヘッダ受信
			//まずはsizeとstateの合計8バイトを受信
			char headBuf[8];
			int hRet = recv(sock,headBuf,8,0);
			if(hRet <= 0)
			{
				cout << "サーバーが接続を閉じました" << endl;
				break;
			} else if(hRet < 0)
			{
				cout << "ネットワークエラーが発生しました。" << endl;
				break;
			}

			//ポインタを使って受信データを解析
			const char* _recvP = headBuf;
			int netSize,netState;

			//memcopyでバイトデータを取り出し、ポインタを進める
			memcpy(&netSize,_recvP,sizeof(int));
			_recvP += sizeof(int);//指す先を次に進める

			memcpy(&netState,_recvP,sizeof(int));//最後は進めない

			//バイトオーダーをnetからhostへ変換
			int dataSize = ntohl(netSize);
			int state    = ntohl(netState);

			//本体受信
			//判明したdataSize分だけ、後続の構造体データを受信
			vector<char> bodyBuf(dataSize);
			int bRet = recv(sock,bodyBuf.data(),dataSize,0);
			if(bRet <= 0)
			{
				cout << "タスクデータ受け取り中に切断されました" << endl;
				break;
			}

			//PDFのLoad関数例に倣い、tmp変数を使用して復元
			RenderTask tmp;
			const char* _bodyP = bodyBuf.data();

			//順番にmemcpyしてポインタをずらす
			memcpy(&tmp.taskId,_bodyP,sizeof(int)); _bodyP += sizeof(int);
			memcpy(&tmp.startY,_bodyP,sizeof(int)); _bodyP += sizeof(int);
			memcpy(&tmp.endY,_bodyP,sizeof(int)); _bodyP += sizeof(int);
			memcpy(&tmp.width,_bodyP,sizeof(int)); _bodyP += sizeof(int);
			memcpy(&tmp.height,_bodyP,sizeof(int));//最後は進めない

			//全ての項目をホスト用に変換して確定
			RenderTask task;
			task.taskId = ntohl(tmp.taskId);
			task.startY = ntohl(tmp.startY);
			task.endY   = ntohl(tmp.endY);
			task.width  = ntohl(tmp.width);
			task.height = ntohl(tmp.height);

			//キューに突っ込む
			taskQueue.push(task);
			cout << "タスク #" << task.taskId << " 受信完了。キューへ追加（現在: " << taskQueue.size() << "件）" << endl;

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