#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <queue> 

#include <ws2tcpip.h>

#include "render_data.h"

#include "../Common/JobData.h"
#include "RunState.hpp"

#pragma comment(lib, "ws2_32.lib")

class Client
{
public:
	Client();
	~Client();

	RunState Run(const std::vector<std::string>& _argv);

private:
	int Initialize();
	int Release();

	bool ConnectServer(const std::vector<std::string>& _argv);
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