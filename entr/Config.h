#pragma once

#include<iostream>

struct SystemConfig
{
	std::string exePath;
	std::string RomName;
	bool shouldReset;
	bool directBoot;
	int saveType;
	double fps;
};

struct Config
{
	static SystemConfig NDS;
};