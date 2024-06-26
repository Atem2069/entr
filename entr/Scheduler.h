#pragma once

#include<iostream>
#include<list>

#include"Logger.h"

typedef void(*callbackFn)(void*);

enum class Event
{
	Frame = 0,
	PPU = 1,
	NDS7_TIMER0=2,
	NDS7_TIMER1=3,
	NDS7_TIMER2=4,
	NDS7_TIMER3=5,
	NDS9_TIMER0=6,
	NDS9_TIMER1=7,
	NDS9_TIMER2=8,
	NDS9_TIMER3=9,
	Gamecard=10,
	GXFIFO=11,
	Sample=12
};

struct SchedulerEntry
{
	Event eventType;
	callbackFn callback;
	void* context;
	uint64_t timestamp;
	bool enabled;
};

class Scheduler
{
public:
	Scheduler();
	~Scheduler();

	void addCycles(uint64_t cycles);
	void tick();

	void jumpToNextEvent();

	void setTimestamp(uint64_t time);
	uint64_t getCyclesToNextEvent();
	uint64_t getCurrentTimestamp();
	uint64_t getEventTime();

	void addEvent(Event type, callbackFn callback, void* context, uint64_t time);
	void removeEvent(Event type);

	void invalidateAll();

	Event getLastFiredEvent();
private:
	bool getEntryAtTimestamp(SchedulerEntry& entry);
	uint64_t timestamp;
	uint64_t eventTime;
	std::list<SchedulerEntry> m_entries;

	Event m_lastFiredEvent = Event::Frame;
};