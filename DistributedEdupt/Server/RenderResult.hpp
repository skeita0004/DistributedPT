#pragma once
#include <cstdint>
#include <vector>
#include "render_data.h"

struct RenderResult
{
	uint32_t id;
	std::vector<edupt::Color> renderResult;
};