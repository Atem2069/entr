#pragma once

//I will remove this at some point
//Just a quick hack while the ARM9 still uses the 3-stage pipeline (it won't in the future)

#include<iostream>

enum class PipelineState
{
	UNFILLED,
	FILLED
};

struct Pipeline
{
	PipelineState state;
	uint32_t opcode;
};

