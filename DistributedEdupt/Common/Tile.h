#pragma once

struct Tile
{
	Tile() :
		id(0),
		renderData()
	{
	}

	Tile(int _id, edupt::RenderData _renderData) :
		id(_id),
		renderData(_renderData)
	{
	}

	int id;
	edupt::RenderData renderData;
};