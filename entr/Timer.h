#pragma once

#include"Logger.h"
#include"InterruptManager.h"
#include"Scheduler.h"

struct TimerRegister
{
	uint16_t CNT_L;
	uint16_t CNT_H;
	uint16_t initialClock;
	uint16_t clock;
	uint64_t timeActivated;
	uint64_t overflowTime;
	uint64_t lastUpdateClock;

	uint16_t newControlVal;
	bool newControlWritten = false;

	uint16_t newReloadVal;
	bool newReloadWritten = false;
};

class Timer
{
public:
	Timer(bool NDS9, std::shared_ptr<InterruptManager> interruptManager, std::shared_ptr<Scheduler> scheduler);
	~Timer();

	uint8_t readIO(uint32_t address);
	void writeIO(uint32_t address, uint8_t value);

	static void onSchedulerEvent(void* context);
private:
	static constexpr Event timerEventLUT[4] = { Event::TIMER0,Event::TIMER1,Event::TIMER2,Event::TIMER3 };
	static constexpr InterruptType irqLUT[4] = { InterruptType::Timer0,InterruptType::Timer1,InterruptType::Timer2,InterruptType::Timer3 };
	void event();
	void calculateNextOverflow(int timerIdx, uint64_t timeBase);
	void checkCascade(int timerIdx);
	void setCurrentClock(int idx, uint8_t prescalerSetting, uint64_t timestamp);

	void writeControl(int idx);
	void writeReload(int idx);
	TimerRegister m_timers[4];

	void raiseInterrupt(int timer);
	bool NDS9 = false;

	std::shared_ptr<InterruptManager> m_interruptManager;
	std::shared_ptr<Scheduler> m_scheduler;
};