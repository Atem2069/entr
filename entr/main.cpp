#include<iostream>
#include<thread>

#include"NDS.h"
#include"Logger.h"

std::shared_ptr<NDS> m_nds;

void emuWorkerThread();


int main()
{
	Logger::getInstance()->msg(LoggerSeverity::Info, "Hello world!");

	m_nds = std::make_shared<NDS>();
	std::thread m_workerThread(&emuWorkerThread);

	if(m_workerThread.joinable())
		m_workerThread.join();
	return 0;
}

void emuWorkerThread()
{
	Logger::getInstance()->msg(LoggerSeverity::Info, "Entered worker thread!");
	m_nds->run();
	Logger::getInstance()->msg(LoggerSeverity::Info, "Exited worker thread!");
}