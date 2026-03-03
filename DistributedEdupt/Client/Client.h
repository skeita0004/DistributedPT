#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <queue> 

#include <ws2tcpip.h>

#include "render_data.h"

#include "../Common/JobData.h"

#pragma comment(lib, "ws2_32.lib")

class Client
{
public:
	enum class RunState
	{
		RUN_ERROR_INITIALIZE = 0,
		RUN_ERROR_CONNECT_SERVER,
		RUN_ERROR_RECVDATA,
		RUN_ERROR_SENDDATA,
		RUN_ERROR_RELEASE,

		RUN_RETRY,

		RUN_COMPLETE,

		RUN_STATE_MAX
	};

	struct RenderResult
	{
		uint32_t id;
		edupt::Color* image;
	};

	Client();
	~Client();

	RunState Run(int _argc, char** _argv);

private:
	int Initialize();
	int Release();

	bool ConnectServer(int _argc, char** _argv);
	int RecvData();
	int SendData();
	
	void ShowMyIPAddresses() const;

	/// @brief クライアントのソケット
	SOCKET  sock_;

	/// @brief 届いたタスクを貯めておくキュー
	std::queue<JobData> taskQueue_;

	/// @brief サーバIPアドレス
	std::string serverIP_;

	/// @brief サーバポート番号
	uint16_t    serverPort_;

	/// @brief サーバへの接続を試みるのは初めてか
	bool firstTry_;
};