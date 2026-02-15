#pragma once

#include "material.h"

namespace edupt
{
	struct RenderData
	{
		RenderData(int _width,     int _height,
				   int _tileWidth, int _tileHeight,
				   int _offsetX,   int _offsetY,
				   int _sample,    int _superSample,
				   int* _imageArray, int _imageArraySize) :
				   width(_width), height(_height),
				   tileWidth(_tileWidth), tileHeight(_tileHeight),
				   offsetX(_offsetX), offsetY(_offsetY),
				   sample(_sample), superSample(_superSample),
				   imageArray(_imageArray), imageArraySize(_imageArraySize) {}

		int width;
		int height;

		int tileWidth;
		int tileHeight;

		int offsetX;
		int offsetY;

		int sample;
		int superSample;
	
		Color* imageArray;
		int  imageArraySize;
	};
}