#include"GuiRenderer.h"

void GuiRenderer::init(GLFWwindow* window)
{
	ImGui::CreateContext();
	ImGui::StyleColorsClassic();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	ImGui::GetIO().IniFilename = NULL;
}

void GuiRenderer::prepareFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void GuiRenderer::render()
{
	ImVec2 cursorPos = ImGui::GetIO().MousePos;
	bool showMenuOverride = (cursorPos.x >= 0 && cursorPos.y >= 0) && (cursorPos.y <= 25);
	if ((m_autoHideMenu && showMenuOverride) || !m_autoHideMenu || m_menuItemSelected)
	{
		if (ImGui::BeginMainMenuBar())
		{
			m_menuItemSelected = false;
			if (ImGui::BeginMenu("File"))
			{
				m_menuItemSelected = true;
				ImGui::MenuItem("Open...", nullptr, &m_openFileDialog);
				ImGui::MenuItem("Exit", nullptr, nullptr);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("System"))
			{
				m_menuItemSelected = true;
				ImGui::MenuItem("Direct Boot", nullptr, &Config::NDS.directBoot);
				ImGui::MenuItem("Disable Video Sync", nullptr, &Config::NDS.disableFrameSync);
				ImGui::MenuItem("Emulation settings", nullptr, &m_showSaveTypeDialog);
				ImGui::EndMenu();
			}
		}
		ImGui::EndMainMenuBar();
	}

	if (m_openFileDialog)
	{
		//this is all legacy garbage, should ideally use IFileOpenDialog at some point
		OPENFILENAMEA ofn = {};
		CHAR szFile[255] = { 0 };

		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = NULL;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = "Nintendo DS ROM Files (*.nds)\0*.nds\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
		{
			std::string filename = szFile;
			Config::NDS.RomName = filename;
			switch (Config::state)
			{
			case SystemState::Off: Config::state = SystemState::PowerOn; break;
			case SystemState::Running: Config::state = SystemState::Reset; break;
			}
		}

		m_openFileDialog = false;
	}

	if (m_showSaveTypeDialog)
	{
		ImGui::Begin("Emulation settings", &m_showSaveTypeDialog, ImGuiWindowFlags_NoCollapse);
		//should account for different save sizes (e.g. can have small EEPROM variants)
		ImGui::Text("Cartridge Savetype");
		if (ImGui::RadioButton("EEPROM 8K", Config::NDS.saveType == 0 && Config::NDS.saveSizeOverride == 8192)) { Config::NDS.saveType = 0; Config::NDS.saveSizeOverride = 8192; }
		if (ImGui::RadioButton("EEPROM 64K", Config::NDS.saveType == 0 && Config::NDS.saveSizeOverride==65536)) { Config::NDS.saveType = 0; Config::NDS.saveSizeOverride = 65536; }
		if (ImGui::RadioButton("EEPROM 128K", Config::NDS.saveType == 0 && Config::NDS.saveSizeOverride==131072)) { Config::NDS.saveType = 0; Config::NDS.saveSizeOverride = 131072;}
		if (ImGui::RadioButton("Flash 256K", Config::NDS.saveType == 1 && Config::NDS.saveSizeOverride==256*1024)) { Config::NDS.saveType = 1; Config::NDS.saveSizeOverride = 256*1024;}
		if (ImGui::RadioButton("Flash 512K", Config::NDS.saveType == 1 && Config::NDS.saveSizeOverride==512*1024)) { Config::NDS.saveType = 1; Config::NDS.saveSizeOverride = 512*1024;}
		if (ImGui::RadioButton("Flash 1024K", Config::NDS.saveType == 1 && Config::NDS.saveSizeOverride==1024*1024)) { Config::NDS.saveType = 1; Config::NDS.saveSizeOverride = 1024*1024;}
		if (ImGui::RadioButton("Flash 8192K", Config::NDS.saveType == 1 && Config::NDS.saveSizeOverride==8192*1024)) { Config::NDS.saveType = 1; Config::NDS.saveSizeOverride = 8192*1024;}
		ImGui::SliderInt("GPU Threads", &Config::NDS.numGPUThreads, 1, 16);
		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool GuiRenderer::m_openFileDialog = false;
bool GuiRenderer::m_autoHideMenu = true;
bool GuiRenderer::m_menuItemSelected = false;
bool GuiRenderer::m_showSaveTypeDialog = false;