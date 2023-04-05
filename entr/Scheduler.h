#pragma once

#include<iostream>
#include<list>

#include"Logger.h"

typedef void(*callbackFn)(void*);

enum class Event
{
	Frame = 0,
	PPU = 1,
	TIMER0=2,
	TIMER1=3,
	TIMER2=4,
	TIMER3=5
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
	void forceSync(uint64_t delta);
	void tick();

	void jumpToNextEvent();

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
	uint64_t syncDelta = 0;
	bool shouldSync = false;
	std::list<SchedulerEntry> m_entries;

	Event m_lastFiredEvent = Event::Frame;
};