#include"NDS.h"

NDS::NDS()
{

}

NDS::~NDS()
{

}

void NDS::run()
{
	while (!m_shouldStop)
	{
		ARM9.run(64);	//ARM9 runs twice the no. cycles as the ARM7, as it runs at twice the clock speed
		ARM7.run(32);
		m_scheduler.addCycles(32);
		m_scheduler.tick();	//<--should probably remove this 'tick' logic, remnant from agbe
	}

	if (m_bus.ARM7_halt && ARM9.getHalted())
		m_scheduler.jumpToNextEvent();
}

void NDS::frameEventHandler()
{
	//handle video sync at some point..
	m_ppu.updateDisplayOutput();
	m_scheduler.addEvent(Event::Frame, &NDS::onEvent, (void*)this, m_scheduler.getEventTime()+560190);
	m_input.tick();
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

	uint32_t ARM9Offs = romData[0x020] | (romData[0x021] << 8) | (romData[0x022] << 16) | (romData[0x023] << 24);
	uint32_t ARM9Entry = romData[0x024] | (romData[0x025] << 8) | (romData[0x026] << 16) | (romData[0x027] << 24);
	uint32_t ARM9LoadAddr = romData[0x028] | (romData[0x029] << 8) | (romData[0x02A] << 16) | (romData[0x02B] << 24);
	uint32_t ARM9Size = romData[0x02C] | (romData[0x02D] << 8) | (romData[0x02E] << 16) | (romData[0x02F] << 24);

	uint32_t ARM7Offs = romData[0x030] | (romData[0x031] << 8) | (romData[0x032] << 16) | (romData[0x033] << 24);
	uint32_t ARM7Entry = romData[0x034] | (romData[0x035] << 8) | (romData[0x036] << 16) | (romData[0x037] << 24);
	uint32_t ARM7LoadAddr = romData[0x038] | (romData[0x039] << 8) | (romData[0x03A] << 16) | (romData[0x03B] << 24);
	uint32_t ARM7Size = romData[0x03C] | (romData[0x03D] << 8) | (romData[0x03E] << 16) | (romData[0x03F] << 24);

	Logger::msg(LoggerSeverity::Info, std::format("ARM9 ROM offset={:#x} entry={:#x} load={:#x} size={:#x}", ARM9Offs, ARM9Entry, ARM9LoadAddr, ARM9Size));
	Logger::msg(LoggerSeverity::Info, std::format("ARM7 ROM offset={:#x} entry={:#x} load={:#x} size={:#x}", ARM7Offs, ARM7Entry, ARM7LoadAddr, ARM7Size));

	m_cartridge.init(romData, &m_interruptManager);
	m_bus.init(nds7bios, nds9bios, &m_cartridge, &m_scheduler, &m_interruptManager, &m_ppu, &m_input);

	if (Config::NDS.directBoot)
	{
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
	ARM9.init(ARM9Entry, &m_bus, &m_interruptManager, &m_scheduler);
	ARM7.init(ARM7Entry, &m_bus, &m_interruptManager, &m_scheduler);
	m_scheduler.addEvent(Event::Frame, &NDS::onEvent, (void*)this, 560190);
	return true;
}

void NDS::m_destroy()
{
	m_scheduler.invalidateAll();
	m_scheduler.addEvent(Event::Frame, &NDS::onEvent, (void*)this, 560190);
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