#include"NDS.h"

NDS::NDS()
{
	//should init scheduler here (when fully implemented, i.e. i'm not lazy)
	m_initialise();
}

NDS::~NDS()
{

}

void NDS::run()
{
	//for now, just load in nds file and parse header - and print out ARM9/ARM7 load points, entry points, code size etc.
	//then need to map that data, could just use the bus's read/write functions lol.
	//also have to set R15 for both the ARM9/ARM7 so they start executing at the right place.
	std::vector<uint8_t> romData = readFile("rom\\armwrestler.nds");
	uint32_t ARM9Offs = romData[0x020] | (romData[0x021] << 8) | (romData[0x022] << 16) | (romData[0x023] << 24);
	uint32_t ARM9Entry = romData[0x024] | (romData[0x025] << 8) | (romData[0x026] << 16) | (romData[0x027] << 24);
	uint32_t ARM9LoadAddr = romData[0x028] | (romData[0x029] << 8) | (romData[0x02A] << 16) | (romData[0x02B] << 24);
	uint32_t ARM9Size = romData[0x02C] | (romData[0x02D] << 8) | (romData[0x02E] << 16) | (romData[0x02F] << 24);

	uint32_t ARM7Offs = romData[0x030] | (romData[0x031] << 8) | (romData[0x032] << 16) | (romData[0x033] << 24);
	uint32_t ARM7Entry = romData[0x034] | (romData[0x035] << 8) | (romData[0x036] << 16) | (romData[0x037] << 24);
	uint32_t ARM7LoadAddr = romData[0x038] | (romData[0x039] << 8) | (romData[0x03A] << 16) | (romData[0x03B] << 24);
	uint32_t ARM7Size = romData[0x03C] | (romData[0x03D] << 8) | (romData[0x03E] << 16) | (romData[0x03F] << 24);

	Logger::getInstance()->msg(LoggerSeverity::Info, std::format("ARM9 ROM offset={:#x} entry={:#x} load={:#x} size={:#x}", ARM9Offs, ARM9Entry, ARM9LoadAddr, ARM9Size));
	Logger::getInstance()->msg(LoggerSeverity::Info, std::format("ARM7 ROM offset={:#x} entry={:#x} load={:#x} size={:#x}", ARM7Offs, ARM7Entry, ARM7LoadAddr, ARM7Size));

	m_bus = std::make_shared<Bus>();
	//load arm9/arm7 binaries
	for (int i = 0; i < ARM9Size; i++)
	{
		uint8_t curByte = romData[ARM9Offs+i];
		m_bus->NDS9_write8(ARM9LoadAddr+i, curByte);
	}
	for (int i = 0; i < ARM7Size; i++)
	{
		uint8_t curByte = romData[ARM7Offs + i];
		m_bus->NDS7_write8(ARM7LoadAddr + i, curByte);
	}
	Logger::getInstance()->msg(LoggerSeverity::Info, "Mapped ARM9/ARM7 binaries into memory!");

	ARM9 = std::make_shared<ARM946E>(ARM9Entry, m_bus, m_interruptManager, m_scheduler);
	ARM7 = std::make_shared<ARM7TDMI>(ARM7Entry, m_bus, m_interruptManager, m_scheduler);

	while (true)
	{
		ARM9->run(32);	//ARM9 runs twice the no. cycles as the ARM7, as it runs at twice the clock speed
		ARM7->run(16);
		//todo: tick scheduler 16 cycles here. most hardware runs at ~33MHz so we're using that as a base clock
	}
}

void NDS::notifyDetach()
{
	//todo
}

void NDS::m_initialise()
{
	//todo
}

void NDS::m_destroy()
{
	//todo
}

std::vector<uint8_t> NDS::readFile(const char* name)
{
	// open the file:
	std::ifstream file(name, std::ios::binary);

	// Stop eating new lines in binary mode!!!
	file.unsetf(std::ios::skipws);

	// get its size:
	std::streampos fileSize;

	file.seekg(0, std::ios::end);
	fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	// reserve capacity
	std::vector<uint8_t> vec;
	vec.reserve(fileSize);

	// read the data:
	vec.insert(vec.begin(),
		std::istream_iterator<uint8_t>(file),
		std::istream_iterator<uint8_t>());

	file.close();

	return vec;
}