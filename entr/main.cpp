#include<iostream>
#include<thread>

//go fuck yourself winapi.
#define NOMINMAX

#include"Display.h"
#include"NDS.h"
#include"Logger.h"
#include"Config.h"

std::shared_ptr<NDS> m_nds;
InputState* m_inputState;

void emuWorkerThread();


int main()
{
	Config::NDS.shouldReset = false;
	Config::NDS.numGPUThreads = 4;
	Logger::msg(LoggerSeverity::Info, "Hello world!");

	Display m_display(2);
	std::thread m_workerThread;

	while (!m_display.getShouldClose())
	{
		switch (Config::state)
		{
		case SystemState::PowerOn:
		{
			m_nds = std::make_shared<NDS>();
			if (m_nds->initialise())
			{
				m_workerThread = std::thread(&emuWorkerThread);
				Config::state = SystemState::Running;
				Logger::msg(LoggerSeverity::Info, "PowerOn->Running");
			}
			else
			{
				m_nds.reset();
				Config::state = SystemState::Off;
				Logger::msg(LoggerSeverity::Error, "Couldn't initialize new NDS instance.");
			}
			break;
		}
		case SystemState::Shutdown:
		{
			m_nds->notifyDetach();
			m_workerThread.join();
			m_nds.reset();
			Config::state = SystemState::Off;
			Logger::msg(LoggerSeverity::Info, "Shut down NDS instance.");
			memset(PPU::m_safeDisplayBuffer, 0, 256 * 384 * sizeof(uint32_t));
			break;
		}
		case SystemState::Reset:
		{
			m_nds->notifyDetach();
			m_workerThread.join();
			m_nds.reset();
			Logger::msg(LoggerSeverity::Info, "Reset NDS instance.");
			Config::state = SystemState::PowerOn;
		}
		}

		void* data = PPU::m_safeDisplayBuffer;
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

		int x = 0, y = 0;
		m_display.getCursorPos(x, y);
		if ((x >= 0 && x < 512) && (y >= 384 && y < 768) && m_display.getLeftMouseClick())
		{
			Touchscreen::screenX = x/2;
			Touchscreen::screenY = (y/2)-192;
			Input::extInputState.penDown = true;
		}
		else
		{
			Input::extInputState.penDown = false;
		}
	}

	if (Config::state == SystemState::Running)
	{
		m_nds->notifyDetach();
		m_workerThread.join();
	}
	return 0;
}

void emuWorkerThread()
{
	Logger::msg(LoggerSeverity::Info, "Entered worker thread!");
	m_nds->run();
	Logger::msg(LoggerSeverity::Info, "Exited worker thread!");
}