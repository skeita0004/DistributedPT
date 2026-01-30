#include <iostream>
#include <vector>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

//サーバーと共通のタスク設定構造体
struct RenderTask {
	int taskId;
	int startY,endY,width,height;
};

//通信用のピクセルデータ構造体
struct Pixel {
	float r,g,b;
};

//サーバーへの結果報告用ヘッダ
struct RenderResultHeader {
	int taskId;
	int width;
};

//ローカルIPアドレスを表示する関数
void ShowMyIPAddresses() {
	char hostname[256];
	if(gethostname(hostname,sizeof(hostname)) == SOCKET_ERROR) return;

	ADDRINFOA hints,* res = NULL;
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if(getaddrinfo(hostname,NULL,&hints,&res) == 0) {
		cout << "Worker IP List:" << endl;
		for(ADDRINFOA* p = res; p != NULL; p = p->ai_next) {
			char ipStr[INET_ADDRSTRLEN];
			struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
			inet_ntop(AF_INET,&(ipv4->sin_addr),ipStr,sizeof(ipStr));
			cout << " >> " << ipStr << endl;
		}
		freeaddrinfo(res);
	}
}

int main() {
	//WinSock初期化
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2,2),&wsaData) != 0) return 1;

	ShowMyIPAddresses();

	while(true) {
		string ip;
		cout << "\n接続先サーバーIPを入力してください: ";
		cin >> ip;

		SOCKET sock = socket(AF_INET,SOCK_STREAM,0);
		if(sock == INVALID_SOCKET) continue;

		SOCKADDR_IN addr;
		memset(&addr,0,sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(8888); //サーバー側のSERVER_PORTと一致させる
		inet_pton(AF_INET,ip.c_str(),&addr.sin_addr.s_addr);

		//サーバーへの接続試行
		cout << "接続中..." << endl;
		if(connect(sock,(SOCKADDR*)&addr,sizeof(addr)) == SOCKET_ERROR) {
			cout << "接続失敗。サーバーが受付状態であることを確認してください。" << endl;
			closesocket(sock);
			continue;
		}

		cout << "Connection OK! サーバーが計算フェーズへ移行するのを待機します..." << endl;

		//計算タスク処理ループ
		while(true) {
			RenderTask netTask;
			//サーバーからタスクデータが届くまで待機
			int ret = recv(sock,(char*)&netTask,sizeof(netTask),0);

			if(ret <= 0) {
				cout << "サーバーから切断されました。" << endl;
				break;
			}

			//バイトオーダーをネットワーク用からホスト用に変換
			int tid = ntohl(netTask.taskId);
			int sY = ntohl(netTask.startY);
			int eY = ntohl(netTask.endY);
			int w = ntohl(netTask.width);

			//割り当て確認（ACK）の返信
			int ackId = htonl(tid);
			send(sock,(char*)&ackId,sizeof(ackId),0);

			cout << "タスク受信: #" << tid << " (" << sY << "-" << eY << "行) 計算中..." << endl;

			//指定された幅に合わせて色データ配列を確保
			vector<Pixel> lineData(w);
			for(int x = 0; x < w; x++) {
				//ここにeduptの計算処理を組み込む
				//計算結果をPixel構造体に代入する
				lineData[x].r = 1.0f;
				lineData[x].g = 1.0f;
				lineData[x].b = 1.0f;
			}

			//計算結果の送信準備
			RenderResultHeader resHeader;
			resHeader.taskId = htonl(tid);
			resHeader.width = htonl(w);

			//ヘッダ情報の送信
			send(sock,(char*)&resHeader,sizeof(resHeader),0);
			//続けて1行分のピクセルデータを一括送信
			send(sock,(char*)lineData.data(),(int)(sizeof(Pixel) * w),0);

			cout << "タスク #" << tid << " の計算結果を送信しました。" << endl;
		}

		//接続が切れた場合はソケットを閉じて再接続待ちへ
		closesocket(sock);
	}

	WSACleanup();
	return 0;
}