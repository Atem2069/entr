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

struct Config
{
	static SystemConfig NDS;
};