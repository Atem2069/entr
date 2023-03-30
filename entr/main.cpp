#include<iostream>
#include<thread>

#include"Display.h"
#include"NDS.h"
#include"Logger.h"

std::shared_ptr<NDS> m_nds;

void emuWorkerThread();


int main()
{
	Logger::getInstance()->msg(LoggerSeverity::Info, "Hello world!");

	Display m_display(2);

	m_nds = std::make_shared<NDS>();
	std::thread m_workerThread(&emuWorkerThread);

	while (!m_display.getShouldClose())
	{
		void* data = m_nds->getPPUData();
		if (data != nullptr)
			m_display.update(data);
		m_display.draw();

		//todo: update input
	}

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