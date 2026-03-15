#pragma once
#include <WinSock2.h>

class WSAContext
{
public:
	WSAContext();
	~WSAContext() noexcept;

private:
	WSADATA wsaData_;
};