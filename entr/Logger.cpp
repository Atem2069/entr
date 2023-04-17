#include"Logger.h"

void Logger::msg(LoggerSeverity severity, std::string msg, const std::source_location location)
{
	std::string funcOrigin = location.function_name();
	std::string prefix = "[" + funcOrigin + "]";
	switch (severity)
	{
	case LoggerSeverity::Error: prefix += "[ERROR]"; break;
	case LoggerSeverity::Warn: prefix += "[WARN]"; break;
	case LoggerSeverity::Info:prefix += "[INFO]"; break;
	}

	prefix += msg;
	std::cout << prefix << '\n';

}
