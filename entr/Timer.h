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
	Timer();
	void init(bool NDS9, InterruptManager* interruptManager, Scheduler* scheduler);
	~Timer();

	uint8_t readIO(uint32_t address);
	void writeIO(uint32_t address, uint8_t value);

	static void onSchedulerEvent(void* context);
private:
	static constexpr Event timerEventLUT[2][4] = { {Event::NDS7_TIMER0,Event::NDS7_TIMER1,Event::NDS7_TIMER2,Event::NDS7_TIMER3}, {Event::NDS9_TIMER0,Event::NDS9_TIMER1,Event::NDS9_TIMER2,Event::NDS9_TIMER3} };
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

	InterruptManager* m_interruptManager;
	Scheduler* m_scheduler;
};