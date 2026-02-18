#pragma once

#include "material.h"
#include <WinSock2.h>

namespace edupt
{
	struct RenderData
	{
		RenderData();
		RenderData(int32_t _width,     int32_t _height,
				   int32_t _tileWidth, int32_t _tileHeight,
				   int32_t _offsetX,   int32_t _offsetY,
				   int32_t _sample,    int32_t _superSample) :
				   width(_width), height(_height),
				   tileWidth(_tileWidth), tileHeight(_tileHeight),
				   offsetX(_offsetX), offsetY(_offsetY),
				   sample(_sample), superSample(_superSample)
				   {}

		const char* Store(char* _p)
		{
			int32_t tmp[] =
			{
				htonl( width ),
				htonl( height ),
				htonl( tileWidth ),
				htonl( tileHeight ),
				htonl( offsetX ),
				htonl( offsetY ),
				htonl( sample ),
				htonl( superSample )
			};

			memcpy( _p, tmp, sizeof( tmp ) );

			return _p;
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