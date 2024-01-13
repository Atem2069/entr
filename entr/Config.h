#pragma once

#include<iostream>

struct SystemConfig
{
	std::string exePath;
	std::string RomName;
	bool shouldReset;
	bool directBoot;
	bool disableFrameSync;
	int saveType;
	int saveSizeOverride;
	int numGPUThreads;
	double fps;
};

enum class SystemState
{
	Off,
	Running,
	PowerOn,
	Shutdown,
	Reset
};

struct Config
{
	static SystemConfig NDS;
	static SystemState state;
};