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
	~Client() noexcept;

	RunState Run(const std::string& _args);

private:
	int Initialize();
	int Release();

	bool ConnectServer(const std::string& _args);
	int RecvData();
	int SendData();

	// boolで良くない?
	int ParseArgs(const std::string& _args);
	void ShowMyIPAddresses() const;

	inline static const size_t ARG_NUM_{2};

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