#include <iostream>
#include <vector>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// edupt関連のヘッダ（プロジェクトの構成に合わせてパスを調整してください）
// #include "edupt.h" 

const unsigned short SERVER_PORT = 8888;

// レンダリングタスクの構造体
struct RenderTask {
    int taskId;      // タスク番号
    int startY;      // レンダリング開始行
    int endY;        // レンダリング終了行
    int width;       // 画像幅
    int height;      // 画像高さ
};

// 計算結果を受け取る構造体
struct RenderResult {
    int taskId;
    // ここにeduptで計算された色データや座標などを含める
};

int main() {
    // 1. WinSock2.2 初期化
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return -1;
    }

    // 2. リスンソケットの作成
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        WSACleanup();
        return -1;
    }

    // ノンブロッキング設定
    /*
    unsigned long arg = 0x01;
    ioctlsocket(listenSock, FIONBIO, &arg);
    */

    // 3. アドレスの割り当て
    SOCKADDR_IN bindSockAddress = { 0 };
    bindSockAddress.sin_family = AF_INET;
    bindSockAddress.sin_port = htons(SERVER_PORT);
    bindSockAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenSock, (SOCKADDR*)&bindSockAddress, sizeof(bindSockAddress)) == SOCKET_ERROR) {
        closesocket(listenSock);
        WSACleanup();
        return -1;
    }

    // 4. 接続待機
    if (listen(listenSock, 5) == SOCKET_ERROR) {
        closesocket(listenSock);
        WSACleanup();
        return -1;
    }

    std::cout << "[Server] Rendering Server Started. Port: " << SERVER_PORT << std::endl;
    std::cout << "[Server] Waiting for Worker PCs..." << std::endl;

    // クライアント管理用
    SOCKET clientSock;
    SOCKADDR_IN clientAddr;
    int addrLen = sizeof(clientAddr);

    // 5. メインループ
    bool running = true;
    while (running) {
        clientSock = accept(listenSock, (SOCKADDR*)&clientAddr, &addrLen);
        if (clientSock != INVALID_SOCKET) {
            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
            std::cout << "[Connect] Worker PC connected: " << clientIP << std::endl;

            // --- ここでeduptを利用したタスク割り当て ---
            RenderTask task = { 1, 0, 100, 800, 600 }; // 例：最初の100行を依頼

            // ネットワークバイトオーダーへの変換（重要：マルチプラットフォーム/PC間通信のため）
            RenderTask netTask = task;
            netTask.taskId = htonl(task.taskId);
            // ... 他のメンバもhtonl/htonsする ...

            send(clientSock, (char*)&netTask, sizeof(netTask), 0);
            std::cout << "[Task] Sent rendering task #" << task.taskId << " to worker." << std::endl;

            // 結果の受信待ち
            RenderResult result;
            int ret = recv(clientSock, (char*)&result, sizeof(result), 0);
            if (ret > 0) {
                std::cout << "[Result] Received rendered data from task #" << ntohl(result.taskId) << std::endl;
                // ここで受け取ったデータを1枚の画像メモリに書き込む等の処理
            }

            closesocket(clientSock); // 今回は1回ごとに終了する簡易例
        }

        // サーバー終了条件（例として1回処理したら終了。実際はキー入力などで制御）
        running = false;
    }

    // 6. 終了処理
    closesocket(listenSock);
    WSACleanup();
    std::cout << "[Server] Shutdown." << std::endl;

    return 0;
}