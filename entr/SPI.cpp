#include"SPI.h"

SPI::SPI()
{

}

void SPI::init(InterruptManager* interruptManager)
{
	m_SPIDevices[0] = std::make_unique<PowerManager>();
	m_SPIDevices[1] = std::make_unique<Flash>("rom\\firmware.bin");
	m_SPIDevices[2] = std::make_unique<Touchscreen>();
	m_interruptManager = interruptManager;
}

SPI::~SPI()
{
	m_SPIDevices[0].reset();
	m_SPIDevices[1].reset();
	m_SPIDevices[2].reset();
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
		//Logger::msg(LoggerSeverity::Info, std::format("SPI read: {:#x}", m_SPIData));
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
		if (oldDeviceSelect != deviceSelect)
			m_SPIDevices[oldDeviceSelect]->deselect();
		//Logger::msg(LoggerSeverity::Info, std::format("New SPI settings. enabled={} irq={} chipselect hold = {} device={}", enabled, irq, chipSelectHold, deviceSelect));
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

	//Logger::msg(LoggerSeverity::Info, std::format("SPI write: {:#x}", value));

	m_SPIData = m_SPIDevices[deviceSelect]->sendCommand(value);
	if (!chipSelectHold)
		m_SPIDevices[deviceSelect]->deselect();
	
	if (irq)
		m_interruptManager->NDS7_requestInterrupt(InterruptType::SPI);

	SPICNT &= ~(1 << 7);
}