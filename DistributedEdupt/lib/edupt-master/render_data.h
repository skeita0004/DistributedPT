#pragma once

#include "material.h"
#include <WinSock2.h>

namespace edupt
{
struct RenderData
{
	RenderData() = default;
	RenderData(int32_t _width, int32_t _height,
			   int32_t _tileWidth, int32_t _tileHeight,
			   int32_t _offsetX, int32_t _offsetY,
			   int32_t _sample, int32_t _superSample) :
		width(_width), height(_height),
		tileWidth(_tileWidth), tileHeight(_tileHeight),
		offsetX(_offsetX), offsetY(_offsetY),
		sample(_sample), superSample(_superSample)
	{
	}

	const char* Store(char* _p)
	{
		int32_t tmp[] =
		{
			static_cast<int32_t>(htonl(width)),
			static_cast<int32_t>(htonl(height)),
			static_cast<int32_t>(htonl(tileWidth)),
			static_cast<int32_t>(htonl(tileHeight)),
			static_cast<int32_t>(htonl(offsetX)),
			static_cast<int32_t>(htonl(offsetY)),
			static_cast<int32_t>(htonl(sample)),
			static_cast<int32_t>(htonl(superSample))
		};

		memcpy(_p, tmp, sizeof(tmp));

		return _p;
	}

	/// @brief 構造体の中身をネットワークエンディアンからホストエンディアンへの変換
	/// @return 変換済み構造体
	RenderData Load()
	{
		RenderData tmp{};

		tmp.width       = ntohl(width);
		tmp.height      = ntohl(height);
		tmp.tileWidth   = ntohl(tileWidth);
		tmp.tileHeight  = ntohl(tileHeight);
		tmp.offsetX     = ntohl(offsetX);
		tmp.offsetY     = ntohl(offsetY);
		tmp.sample      = ntohl(sample);
		tmp.superSample = ntohl(superSample);

		return tmp;
	}

	int32_t width;
	int32_t height;

	int32_t tileWidth;
	int32_t tileHeight;

	int32_t offsetX;
	int32_t offsetY;

	int32_t sample;
	int32_t superSample;
};
}