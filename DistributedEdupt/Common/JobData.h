#pragma once

#include "State.h"
#include "Tile.h"

struct JobData
{
	JobData() :
		mySize(0),
		status(STATE_NONE),
		tile()
	{
	}

	JobData(uint64_t _mySize, State _status, Tile _tile) :
		mySize(_mySize),
		status(_status),
		tile(_tile)
	{
	}

	uint64_t mySize;
	State status;
	Tile tile;
};
