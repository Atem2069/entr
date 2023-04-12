#include<iostream>
#include<thread>

#include"Display.h"
#include"NDS.h"
#include"Logger.h"

std::shared_ptr<NDS> m_nds;
std::shared_ptr<InputState> m_inputState;

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

		//blarg. key input
		Input::inputState.A = m_display.getPressed(GLFW_KEY_X);
		Input::inputState.B = m_display.getPressed(GLFW_KEY_Z);
		Input::inputState.L = m_display.getPressed(GLFW_KEY_A);
		Input::inputState.R = m_display.getPressed(GLFW_KEY_S);
		Input::inputState.Start = m_display.getPressed(GLFW_KEY_ENTER);
		Input::inputState.Select = m_display.getPressed(GLFW_KEY_RIGHT_SHIFT);
		Input::inputState.Up = m_display.getPressed(GLFW_KEY_UP);
		Input::inputState.Down = m_display.getPressed(GLFW_KEY_DOWN);
		Input::inputState.Left = m_display.getPressed(GLFW_KEY_LEFT);
		Input::inputState.Right = m_display.getPressed(GLFW_KEY_RIGHT);
	}

	m_nds->notifyDetach();

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