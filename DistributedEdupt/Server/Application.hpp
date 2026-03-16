#pragma once
#include <vector>
#include <string>

class WSAContext;

class Application
{
public:
	Application();
	~Application() noexcept;

	int Run(std::vector<std::string> _args);

private:
	WSAContext* pWSAContext_;
};