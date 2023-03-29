#include<iostream>

#include"NDS.h"
#include"Logger.h"

int main()
{
	Logger::getInstance()->msg(LoggerSeverity::Info, "Hello world!");

	NDS m_nds;
	m_nds.run();

	return 0;
}