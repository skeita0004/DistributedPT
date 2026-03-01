#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>

#include <WinSock2.h>
#include <WS2tcpip.h>

#include "render_data.h"
#include "LocalClient.h"

#pragma comment(lib, "ws2_32.lib")

class LocalClient;

class Server
{
public:
	// 受信用
	struct RenderResult
	{
		int id;
		std::vector<edupt::Color> renderResult;
	};

	Server();
	~Server();

	/// @brief サーバの初期化
	/// @param _argv main関数が受け取ったコマンドライン引数
	/// @return 正常終了: 0, 異常終了: -1
	int Initialize(char** _argv);

	/// @brief 解放処理
	/// @return 正常終了: 0, 異常終了: -1
	int Release();

	void JoinClient();
	void PreparationSendData();
	void SendData();
	void RecvData();
	const std::vector<RenderResult>& GetRenderResult();

	void SendDataStab();

	int GetTotalTileNum()
	{
		return totalTileNum_;
	}

	int GetRecvDataNum()
	{
		return renderResult_.size();
	}

	const std::string& GetffmpegArgs()
	{
		return ffmpegArgs_;
	}
private:
	enum State : uint8_t
	{
		STATE_NONE,
		STATE_QUOTA,
		STATE_COMPLETE_SEND,
		STATE_COMPLETION,
		STATE_MAX
	};

	struct ClientInfo
	{
		SOCKET sock;
		std::string ip;
		std::vector<char> headBuf;
		std::vector<char> bodyBuf;
		uint32_t bodySize;
		size_t headReceivedSize;
		size_t bodyReceivedSize;

		enum class State
		{
			HEAD_WAITING,
			HEAD_RECEIVENG,
			BODY_WAITING,
			BODY_RECEIVENG,
			ALL_COMPLETE,
		} state;
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

		int id;
		edupt::RenderData renderData;
	};

	// 送信用
	struct JobData
	{
		JobData() :
			mySize(0),
			status(STATE_NONE),
			tile()
		{
		}

		JobData(uint64_t _mySize, State _status, Tile _tile) :
			mySize(_mySize),
			status(_status),
			tile(_tile)
		{
		}

		uint64_t mySize;
		State status;
		Tile tile;
	};

	/// @brief テスト用スタブ
	struct RenderTask
	{
		int taskId;
		int startY;
		int endY;
		State status;
		std::string ip;
	};

	void AcceptLocalClient();

	// サーバー自身のIPアドレスを取得する関数
	void ShowServerIP();

	void DisplayMessage(const std::vector<ClientInfo>& clients);

	// いらないかも～ TODO:消す
	void GetCommandLineArgs(char** _argv);


	inline static const uint16_t PORT_{8888};
	inline static const std::string LOCAL_CLIENT_IP_{"127.0.0.1"};

	inline static const int TILE_SIZE_{64};

	int imageWidth_;
	int imageHeight_;
	int superSampleNum_;
	int sampleNum_;

	int totalTileNum_;

	WSADATA wsaData_;
	SOCKET listenSock_;
	SOCKADDR_IN serverAddr_;

	std::vector<ClientInfo> connectedClients_;
	std::vector<Tile> renderData_;
	std::vector<RenderResult> renderResult_;

	std::string ffmpegArgs_;

	LocalClient* localClient_;
};