#include <WinSock2.h>

#include "Client.h"

int main(int argc, char* argv[])
{
	//WinSock初期化
	WSADATA wsaData{};
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		int errorCode{WSAGetLastError()};
		std::cerr << "WSAStartup() failed." << std::endl;
		std::cerr << "Error code : " << errorCode << std::endl;
		WSACleanup();
		return -1;
	}

	// メインループ
	Client  client{};
	while (true)
	{
		// RUN_RETRY以外が返ってきたら終了
		if (client.Run(argc, argv) != Client::RunState::RUN_RETRY)
		{
			break;
		}
	}

	// WSA終了
	if (WSACleanup() != 0)
	{
		int errorCode{WSAGetLastError()};
		std::cerr << "WSACleanup() failed." << std::endl;
		std::cerr << "Error code : " << errorCode << std::endl;
		return -1;
	}
	
	return 0;
}