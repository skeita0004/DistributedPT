#pragma once

#include <WS2tcpip.h>

#include <string>
#include <vector>

struct ClientInfo
{
	enum class State
	{
		HEAD_WAITING,
		HEAD_RECEIVENG,
		BODY_WAITING,
		BODY_RECEIVENG,
		ALL_COMPLETE,
	} state;

	ClientInfo() :
		sock(INVALID_SOCKET),
		ip(),
		headBuf(),
		bodyBuf(),
		bodySize(0),
		headReceivedSize(0),
		bodyReceivedSize(0),
		state(State::HEAD_WAITING)
	{}

	ClientInfo(SOCKET _sock,
			   std::string _ip,
			   std::vector<char> _headBuf,
			   std::vector<char> _bodyBuf,
			   uint32_t _bodySize,
			   size_t _headReceivedSize,
			   size_t _bodyReceivedSize,
			   State  _state) :
		sock(_sock),
		ip(_ip),
		headBuf(_headBuf),
		bodyBuf(_bodyBuf),
		bodySize(_bodySize),
		headReceivedSize(_headReceivedSize),
		bodyReceivedSize(_bodyReceivedSize),
		state(_state)
	{}

	SOCKET sock;
	std::string ip;
	std::vector<char> headBuf;
	std::vector<char> bodyBuf;
	uint32_t bodySize;
	size_t headReceivedSize;
	size_t bodyReceivedSize;
};
