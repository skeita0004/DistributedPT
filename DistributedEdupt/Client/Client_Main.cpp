#include <iostream>
#include <vector>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// 通信用ピクセル構造体
struct Pixel {
	float r,g,b;
};

//struct（鯖と合わせるので変わるかも）
struct RenderTask {
	int taskId;
	int startY,endY,width,height;
};

// 自分のIPを表示する関数
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
			struct sockaddr_in* addr = (struct sockaddr_in*)p->ai_addr;
			inet_ntop(AF_INET,&addr->sin_addr,ipStr,sizeof(ipStr));
			cout << " >> " << ipStr << endl;
		}
		freeaddrinfo(res);
	}
}

int main() {
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2,2),&wsaData) != 0) return 1;

	ShowMyIPAddresses();

	while(true) {
		string ip;
		cout << "\n接続先サーバーIP: ";
		cin >> ip;

		SOCKET sock = socket(AF_INET,SOCK_STREAM,0);
		if(sock == INVALID_SOCKET) continue;

		SOCKADDR_IN addr;
		memset(&addr,0,sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(8888);
		inet_pton(AF_INET,ip.c_str(),&addr.sin_addr.s_addr);

		if(connect(sock,(SOCKADDR*)&addr,sizeof(addr)) == SOCKET_ERROR) {
			cout << "接続失敗" << endl;
			closesocket(sock);
			continue;
		}

		cout << "Connected! タスク待機中..." << endl;

		while(true) {
			//recv(sock, buf, 8)のやつ
			//ヘッダ（size:4byte + state:4byte）を受信
			char headBuf[8];
			int hRet = recv(sock,headBuf,8,0);
			if(hRet <= 0) break;

			//ネットワークオーダーからホストオーダーへ変換
			int dataSize;
			int state;
			memcpy(&dataSize,headBuf,4);
			memcpy(&state,headBuf + 4,4);
			dataSize = ntohl(dataSize);
			state = ntohl(state);

			// headerのsize分だけ本体（RenderTask）を受信
			vector<char> bodyBuf(dataSize);
			int bRet = recv(sock,bodyBuf.data(),dataSize,0);
			if(bRet <= 0) break;

			// 受信データのパース（デシリアライズ）
			RenderTask task;
			memcpy(&task,bodyBuf.data(),sizeof(RenderTask));
			int tid = ntohl(task.taskId);
			int sY  = ntohl(task.startY);
			int eY  = ntohl(task.endY);
			int w   = ntohl(task.width);

			// 割り当てOKの返信 (ACK)
			int ackId = htonl(tid);
			send(sock,(char*)&ackId,sizeof(ackId),0);
			cout << "Task #" << tid << " 受信 (State:" << state << ") 計算開始..." << endl;

			// edupt計算処理（わからん）
			vector<Pixel> lineData(w);
			for(int x = 0; x < w; x++) {

				//ここにeduptの計算結果いれればいい？
				//lineData[x].r = (float)x / (float)w;
				//lineData[x].g = (float)x / (float)w;
				//lineData[x].b = (float)x / (float)w;
			}

			//Storeのぶぶん
			// 結果送信用のバッファ作成（size + state + data）
			int resDataSize = sizeof(Pixel) * w;
			int totalSize = 8 + resDataSize;
			vector<char> sendBuf(totalSize);

			int netSize = htonl(resDataSize);
			int netState = htonl(2); // COMPLETION状態

			// バッファへの書き込み
			memcpy(sendBuf.data(),&netSize,4);
			memcpy(sendBuf.data() + 4,&netState,4);
			memcpy(sendBuf.data() + 8,lineData.data(),resDataSize);

			// 送信
			send(sock,sendBuf.data(),totalSize,0);
			cout << "Task #" << tid << " 完了報告送信済" << endl;
		}
		closesocket(sock);
	}

	WSACleanup();
	return 0;
}