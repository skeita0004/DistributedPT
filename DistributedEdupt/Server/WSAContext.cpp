#include "WSAContext.hpp"
#include <iostream>
#include <stdexcept>
#include <system_error>

WSAContext::WSAContext()
{

	int result = WSAStartup(MAKEWORD(2, 2), &wsaData_);
	if (result != 0)
	{
		std::cerr << "WSAStartup failed. Error code : ";
		std::error_code ec(result, std::system_category());

		throw std::system_error(ec);
	}
}

WSAContext::~WSAContext() noexcept
{
	int result = WSACleanup();
	if (result != 0)
	{
		std::cerr << "WSACleanup failed. Error code : " << result << std::endl;
		std::cerr << std::system_category().message(result) << std::endl;
	}
}
