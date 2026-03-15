#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include "Application.hpp"
#include "WSAContext.hpp"

#include "Server.hpp"
#include "Client.hpp"
#include "RunState.hpp"

#include <iostream>
#include <cstring>
#include <system_error>
#include <vector>

Application::Application()
{

}

Application::~Application()
{

}

int Application::Run(std::vector<std::string> _args)
{
	// クライアントのとき：
	// DPT.exe -c "IP PORT" (3)
	// 
	// サーバのとき：
	// DPT.exe -s "WIDxHEI SS S" (3)

	// 引数チェック
	if (_args.size() != 3)
	{
		std::cerr << "不正な引数です。" << std::endl;
		std::cerr << "使用例: ./DPT.exe -c|-s (\"serverIP serverPort\")|(\"widthxheight supersample sample\")" << std::endl;
		return -1;
	}

	try
	{
		WSAContext wsaContext{};
	}
	catch (std::system_error e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << "WSAContext Create failed." << std::endl;
		return -1;
	}

	// 実行ファイル名、動作形式を除いたものを各Run関数へと渡す
	// こうやると、イテレータで勝手にコピーしてくれる
	std::vector<std::string> trimedArgs(_args[_args.size() - 1]);

	if (_args[1] == "-c")
	{
		Client client{};
		while (true)
		{
			if (client.Run(trimedArgs) != RunState::RETRY)
			{
				break;
			}
		}
	}
	else if (_args[1] == "-s")
	{
		Server server{};
		while (true)
		{
			if (server.Run(trimedArgs) != RunState::RETRY)
			{
				break;
			}
		}
	}
	else
	{
		std::cerr << "不正な引数です : " << _args[1] << std::endl;
		return -1;
	}

	return 0;
}