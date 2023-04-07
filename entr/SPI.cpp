#include"SPI.h"

SPI::SPI(std::shared_ptr<InterruptManager> interruptManager)
{
	m_firmware = std::make_shared<Firmware>();
	m_interruptManager = interruptManager;
}

SPI::~SPI()
{

}

uint8_t SPI::read(uint32_t address)
{
	//return 0;
	switch (address)
	{
	case 0x040001C0:
		return SPICNT & 0x83;
	case 0x040001C1:
		return ((SPICNT >> 8) & 0xCF);
	case 0x040001C2:
		if (!enabled)
			return 0;
		//Logger::getInstance()->msg(LoggerSeverity::Info, std::format("SPI read: {:#x}", m_SPIData));
		return m_SPIData;
	}
}

void SPI::write(uint32_t address, uint8_t value)
{
	//return;
	switch (address)
	{
	case 0x040001C0:
		SPICNT &= 0xFF00; SPICNT |= value;
		break;
	case 0x040001C1:
	{
		bool enable_old = ((SPICNT >> 15) & 0b1);
		uint8_t oldDeviceSelect = ((SPICNT >> 8) & 0b11);
		bool oldChipSelectHold = ((SPICNT >> 11) & 0b1);
		SPICNT &= 0x00FF; SPICNT |= (value << 8);
		irq = ((SPICNT >> 14) & 0b1);
		enabled = ((SPICNT >> 15) & 0b1);
		chipSelectHold = ((SPICNT >> 11) & 0b1);
		deviceSelect = ((SPICNT >> 8) & 0b11);
		if (enabled && !enable_old || (enabled && !enable_old) || (deviceSelect!=oldDeviceSelect))
		{
			if (deviceSelect == 1)
				m_firmware->deselect();
		}
		Logger::getInstance()->msg(LoggerSeverity::Info, std::format("New SPI settings. enabled={} irq={} chipselect hold = {} device={}", enabled, irq, chipSelectHold, deviceSelect));
		break;
	}
	case 0x040001C2:
		writeSPIData(value);
		break;
	}
}

void SPI::writeSPIData(uint8_t value)
{
	if (!enabled)
		return;

	//Logger::getInstance()->msg(LoggerSeverity::Info, std::format("SPI write: {:#x}", value));

	switch (deviceSelect)
	{
	case 1:
		m_SPIData = m_firmware->sendCommand(value);
		if (!chipSelectHold)
			m_firmware->deselect();
		break;
	default:
		m_SPIData = 0xFF;
	}
	
	if (irq)
		m_interruptManager->NDS7_requestInterrupt(InterruptType::SPI);

	SPICNT &= ~(1 << 7);
}