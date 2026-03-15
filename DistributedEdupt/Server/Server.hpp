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

#pragma comment(lib, "ws2_32.lib")
#include "RunState.hpp"

class SubProcess;

class Server
{
public:
	Server();
	~Server();

	RunState Run(const std::vector<std::string>& _argv);

private:
	/// @brief サーバの初期化
	/// @param _argv main関数が受け取ったコマンドライン引数
	/// @return 正常終了: 0, 異常終了: -1
	int Initialize(const std::vector<std::string>& _argv);

	/// @brief 解放処理
	/// @return 正常終了: 0, 異常終了: -1
	int Release();

	/// @brief クライアントの接続待機
	void JoinClient();

	/// @brief クライアントへ送信するデータの前処理
	void PreparationSendData();

	/// @brief データを送信する
	void SendData();

	/// @brief データを受信する
	void RecvData();

	// 受信用
	struct RenderResult
	{
		uint32_t id;
		std::vector<edupt::Color> renderResult;
	};

	const std::vector<RenderResult>& GetRenderResult()
	{
		return renderResult_;
	}

	int GetTotalTileNum()
	{
		return totalTileNum_;
	}

	const std::string& GetffmpegArgs()
	{
		return ffmpegArgs_;
	}

	/// @brief 従来の送信処理（現在未使用）
	void SendDataStab();
	/// @brief テスト用スタブ
	struct RenderTask
	{
		int taskId;
		int startY;
		int endY;
		State status;
		std::string ip;
	};

	void JoinLocalClient();
	void LaunchViewer();

	// サーバー自身のIPアドレスを取得する関数
	void ShowServerIP();

	// 接続済みクライアント一覧の表示
	void DisplayMessage(const std::vector<ClientInfo>& clients);

	inline static const uint16_t PORT_{8888};
	inline static const std::string LOCAL_CLIENT_IP_{"127.0.0.1"};
	inline static const std::string LOCAL_CLIENT_EXEPATH_{"./resource/Client.exe"};
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