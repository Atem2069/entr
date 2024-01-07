#include"NDS.h"

NDS::NDS()
{

}

NDS::~NDS()
{

}

void NDS::run()
{
	m_lastTime = std::chrono::high_resolution_clock::now();
	while (!m_shouldStop)
	{
		ARM9.run(32);	//ARM9 runs twice the no. cycles as the ARM7, as it runs at twice the clock speed
		ARM7.run(16);
		m_scheduler.addCycles(16);
		m_scheduler.tick();	//<--should probably remove this 'tick' logic, remnant from agbe

		//if (m_bus.ARM7_halt && ARM9.getHalted())
		//	m_scheduler.jumpToNextEvent();
	}
}

void NDS::frameEventHandler()
{
	m_ppu.updateDisplayOutput();
	m_input.tick();

	auto curTime = std::chrono::high_resolution_clock::now();
	double timeDiff = std::chrono::duration<double, std::milli>(curTime - m_lastTime).count();	
	static constexpr double target = ((560190.0 / 33554432.0) * 1000.0);

	if (!Config::NDS.disableFrameSync)
	{
		while (timeDiff < target)
		{
			curTime = std::chrono::high_resolution_clock::now();
			timeDiff = std::chrono::duration<double, std::milli>(curTime - m_lastTime).count();
		}
	}
	Config::NDS.fps = 1.0 / (timeDiff / 1000);
	m_lastTime = curTime;
}

void NDS::notifyDetach()
{
	m_shouldStop = true;
}

bool NDS::initialise()
{
	Logger::msg(LoggerSeverity::Info, std::format("Create new NDS instance. directBoot={} ROM path={}",Config::NDS.directBoot, Config::NDS.RomName));
	m_scheduler.invalidateAll();
	std::vector<uint8_t> romData;
	std::vector<uint8_t> nds7bios;
	std::vector<uint8_t> nds9bios;
	std::vector<uint8_t> firmware;	//don't like this tbh.
	if (!readFile(romData, Config::NDS.RomName.c_str()))	//silently fail, not a huge issue
		return false;
	if (!readFile(nds7bios, "rom\\biosnds7.bin"))
	{
		Logger::msg(LoggerSeverity::Error, "Couldn't load NDS7 BIOS..");
		return false;
	}
	if (!readFile(nds9bios, "rom\\biosnds9.bin"))
	{
		Logger::msg(LoggerSeverity::Error, "Couldn't load NDS9 BIOS..");
		return false;
	}
	if (!readFile(firmware, "rom\\firmware.bin"))
		return false;

	uint32_t ARM9Offs = *(uint32_t*)&romData[0x20];
	uint32_t ARM9Entry = *(uint32_t*)&romData[0x24];
	uint32_t ARM9LoadAddr = *(uint32_t*)&romData[0x28];
	uint32_t ARM9Size = *(uint32_t*)&romData[0x2c];

	uint32_t ARM7Offs = *(uint32_t*)&romData[0x30];
	uint32_t ARM7Entry = *(uint32_t*)&romData[0x34];
	uint32_t ARM7LoadAddr = *(uint32_t*)&romData[0x38];
	uint32_t ARM7Size = *(uint32_t*)&romData[0x3c];


	Logger::msg(LoggerSeverity::Info, std::format("ARM9 ROM offset={:#x} entry={:#x} load={:#x} size={:#x}", ARM9Offs, ARM9Entry, ARM9LoadAddr, ARM9Size));
	Logger::msg(LoggerSeverity::Info, std::format("ARM7 ROM offset={:#x} entry={:#x} load={:#x} size={:#x}", ARM7Offs, ARM7Entry, ARM7LoadAddr, ARM7Size));

	m_cartridge.init(romData, &m_interruptManager,&m_scheduler);
	m_bus.init(nds7bios, nds9bios, &m_cartridge, &m_scheduler, &m_interruptManager, &m_ppu, &m_gpu, &m_input);

	if (Config::NDS.directBoot)
	{
		//misc values from gbatek (bios ram usage)
		m_bus.NDS7_write8(0x04000300, 1);
		m_bus.NDS9_write8(0x04000300, 1);
		m_bus.NDS9_write8(0x04000247, 0x03);
		m_bus.NDS9_write32(0x027FF800, 0x00001FC2);
		m_bus.NDS9_write32(0x027FF804, 0x00001FC2);
		m_bus.NDS9_write16(0x027FF850, 0x5835);
		m_bus.NDS9_write16(0x027FF880, 0x0007);
		m_bus.NDS9_write16(0x027FF884, 0x0006);
		m_bus.NDS9_write32(0x027FFC00, 0x00001FC2);
		m_bus.NDS9_write32(0x027FFC04, 0x00001FC2);
		m_bus.NDS9_write16(0x027FFC10, 0x5835);
		m_bus.NDS9_write16(0x027FFC40, 0x0001);

		m_cartridge.directBoot();
		//load arm9/arm7 binaries
		for (int i = 0; i < ARM9Size; i++)
		{
			uint8_t curByte = romData[ARM9Offs + i];
			m_bus.NDS9_write8(ARM9LoadAddr + i, curByte);
		}
		for (int i = 0; i < ARM7Size; i++)
		{
			uint8_t curByte = romData[ARM7Offs + i];
			m_bus.NDS7_write8(ARM7LoadAddr + i, curByte);
		}
		Logger::msg(LoggerSeverity::Info, "Mapped ARM9/ARM7 binaries into memory!");

		//copy over rom header
		for (int i = 0; i < 0x170; i++)
			m_bus.NDS9_write8(0x027FFE00 + i, romData[i]);
		//copy firmware user data (e.g. tsc calibration)
		for (int i = 0; i < 0xF0; i++)
			m_bus.NDS9_write8(0x027FFC80 + i, firmware[0x3FE00 + i]);
	}
	else
	{
		uint8_t keyBuf[0x1048];
		for (int i = 0; i < 0x1048; i++)
			keyBuf[i] = nds7bios[0x30 + i];
		m_cartridge.encryptSecureArea(keyBuf);
		ARM9Entry = 0xFFFF0000;
		ARM7Entry = 0x0;
	}

	m_ppu.init(&m_interruptManager, &m_scheduler);
	m_gpu.init(&m_interruptManager, &m_scheduler);
	ARM9.init(ARM9Entry, &m_bus, &m_interruptManager, &m_scheduler);
	ARM7.init(ARM7Entry, &m_bus, &m_interruptManager, &m_scheduler);
	m_ppu.registerFrameCallback((callbackFn)&NDS::onEvent, (void*)this);
	return true;
}

void NDS::m_destroy()
{
	m_scheduler.invalidateAll();
}

bool NDS::readFile(std::vector<uint8_t>& vec, const char* name)
{
	// open the file:
	std::ifstream file(name, std::ios::binary);
	if (!file)
		return false;

	// Stop eating new lines in binary mode!!!
	file.unsetf(std::ios::skipws);

	// get its size:
	std::streampos fileSize;

	file.seekg(0, std::ios::end);
	fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	// reserve capacity
	vec.reserve(fileSize);

	// read the data:
	vec.insert(vec.begin(),
		std::istream_iterator<uint8_t>(file),
		std::istream_iterator<uint8_t>());

	file.close();

	return true;
}

void NDS::onEvent(void* context)
{
	NDS* thisPtr = (NDS*)context;
	thisPtr->frameEventHandler();
}