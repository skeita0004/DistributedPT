#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>

#include <WinSock2.h>
#include <WS2tcpip.h>

#include "render_data.h"
#include "ClientInfo.h"

#include "../Common/JobData.h"

#include "RunState.hpp"
#include "RenderResult.hpp"

#pragma comment(lib, "ws2_32.lib")

class SubProcess;

class Server
{
public:
	Server();
	~Server() noexcept;

	/// @brief 
	/// @param _argv 
	/// @return 
	RunState Run(const std::string& _args);

private:
	/// @brief サーバの初期化
	/// @param _argv main関数が受け取ったコマンドライン引数
	/// @return 正常終了: 0, 異常終了: -1
	int Initialize(const std::string& _args);

	/// @brief 引数の解析とメンバへの代入
	/// @param _args 
	/// @return 
	int ParseArgs(const std::string& _args);

	/// @brief クライアントの接続待機
	void JoinClient();

	/// @brief クライアントへ送信するデータの前処理
	void PreparationSendData();

	/// @brief データを送信する
	void SendData();

	/// @brief データを受信する
	void RecvData();

	/// @brief ローカルクライアントの参加を待機
	void JoinLocalClient();
	
	/// @brief ビューワの起動
	void LaunchViewer();

	/// @brief サーバー自身のIPアドレスを取得する関数
	void ShowServerIP();

	/// @brief 接続済みクライアント一覧の表示
	void DisplayMessage(const std::vector<ClientInfo>& clients);

	inline static const size_t ARG_NUM_{3};
	inline static const uint16_t PORT_{8888};
	inline static const std::string LOCAL_CLIENT_IP_{"127.0.0.1"};
	inline static const std::string LOCAL_CLIENT_EXEPATH_{"./resource/DPT.exe"};
	inline static const std::string VIEWER_EXEPATH_{"./resource/Viewer.exe"};

	int imageWidth_;
	int imageHeight_;
	int superSampleNum_;
	int sampleNum_;

	int tileSize_;
	int tileNumX_;
	int tileNumY_;
	int totalTileNum_;

	SOCKET listenSock_;
	SOCKADDR_IN serverAddr_;
	std::vector<ClientInfo> connectedClients_;

	std::vector<Tile> renderData_;
	std::vector<RenderResult> renderResult_;

	std::string ffmpegPath_;
	std::string ffmpegArgs_;

	SubProcess* localClient_;
	SubProcess* viewer_;
};