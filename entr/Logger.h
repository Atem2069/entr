#pragma once

#include<iostream>
#include<string>
#include<queue>
#include<fstream>
#include<source_location>
#include<format>
enum class LoggerSeverity
{
	Error,
	Warn,
	Info
};

class Logger
{
public:
	static void msg(LoggerSeverity severity, std::string msg, const std::source_location location = std::source_location::current());
};