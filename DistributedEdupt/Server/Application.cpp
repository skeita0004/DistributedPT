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

Application::Application() :
	pWSAContext_(nullptr)
{

}

Application::~Application()
{
	if (pWSAContext_)
	{
		delete pWSAContext_;
	}
}

int Application::Run(std::vector<std::string> _args)
{
	// クライアントのとき：
	// DPT.exe -c "IP PORT" (3)
	
	// サーバのとき：
	// DPT.exe -s "WIDxHEI SS S" (3)

	// 引数チェック
	if (_args.size() != 3)
	{
		std::cerr << "不正な引数です。" << std::endl;
		std::cerr << "使用例: ./DPT.exe -c|-s \"serverIP serverPort\"|\"widthxheight supersample sample\"" << std::endl;
		return -1;
	}

	try
	{
		pWSAContext_ = new WSAContext();
	}
	catch (std::system_error e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << "WSAContext のインスタンス化に失敗しました。" << std::endl;
		return -1;
	}

	std::string trimmedArgs(*(_args.begin() + 2));

	if (_args[1] == "-c")
	{
		Client client{};
		while (true)
		{
			if (client.Run(trimmedArgs) != RunState::RETRY)
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
			if (server.Run(trimmedArgs) != RunState::RETRY)
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