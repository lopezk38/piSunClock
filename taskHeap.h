/******************************************************************************
/ Pi 4 Sunrise Clock App Task Heap Class Spec - lopezk38 2025
/
/*****************************************************************************/

#ifndef SUNCLOCK_THEAP
#define SUNCLOCK_THEAP

//#define DEBUG

/******************************************************************************
/ Dependencies, namespace
/*****************************************************************************/

#include <vector>
#include <string>

namespace tHeap {


/******************************************************************************
/ TASK enum and helpers
/*****************************************************************************/

namespace TASK
{
	enum CODE
	{
		NONE,
		SET_INPUT,
		CHECK_SHOULD_TOGGLE_DISPLAY_PWR,
		DISPLAY_OFF_AND_RESCHEDULE,
		DISPLAY_OFF,
		DISPLAY_ON_STEP1_AND_RESCHEDULE,
		DISPLAY_ON_STEP1,
		DISPLAY_ON_STEP2_AND_RESCHEDULE,
		DISPLAY_ON_STEP2,
		DISPLAY_TOGGLE_STEP1,
		DISPLAY_TOGGLE_STEP2,
		SET_BRIGHTNESS_AND_RESCHEDULE,
		SET_BRIGHTNESS
	};
	
	bool isValidTaskCode(TASK::CODE task);

	std::string toString(TASK::CODE task);
}


struct Task
{
	long scheduledTime;
	TASK::CODE task;
	
	Task() : scheduledTime(-1), task(TASK::CODE::NONE) {};
	Task(long scheduledTime, TASK::CODE task) : scheduledTime(scheduledTime), task(task) {};
};

class TaskHeap
{
private:

	std::vector<Task*> taskHeap;
	
public:

	~TaskHeap();
	
	void pushTask(long scheduledTime, TASK::CODE task);
	Task* popTask();
	Task* peekTask();
	
	bool isEmpty();
};
}

#endif








































