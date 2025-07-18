/******************************************************************************
/ Pi 4 Sunrise Clock App Task Heap Class Implementation - lopezk38 2025
/
/*****************************************************************************/


/******************************************************************************
/ Dependencies
/*****************************************************************************/

#include <algorithm>
#include <stdexcept>

#include "taskHeap.h"

#ifdef DEBUG
#include <iostream>
#endif


/******************************************************************************
/ Swapping comparison logic for heap
/*****************************************************************************/

class DoSwap
{
    public: bool operator()(tHeap::Task* lhs, tHeap::Task* rhs)
    {
        //Do not swap nodes if lhs scheduled time is later than rhs
        return lhs->scheduledTime > rhs->scheduledTime;
    }
};


/******************************************************************************
/ Task Enum Helper Function Implementations
/*****************************************************************************/

bool tHeap::TASK::isValidTaskCode(TASK::CODE task)
{
	return (task >= TASK::CODE::NONE || task <= TASK::CODE::SET_BRIGHTNESS);
}

std::string tHeap::TASK::toString(TASK::CODE task)
{
	switch (task)
	{
		case TASK::CODE::NONE: return "TASK::CODE::NONE";
		break;
		
		case TASK::CODE::SET_INPUT: return "TASK::CODE::SET_INPUT";
		break;
		
		case TASK::CODE::CHECK_SHOULD_TOGGLE_DISPLAY_PWR: return "TASK::CODE::CHECK_SHOULD_TOGGLE_DISPLAY_PWR";
		break;
		
		case TASK::CODE::DISPLAY_OFF_AND_RESCHEDULE: return "TASK::CODE::DISPLAY_OFF_AND_RESCHEDULE";
		case TASK::CODE::DISPLAY_OFF: return "TASK::CODE::DISPLAY_OFF";
		break;
		
		case TASK::CODE::DISPLAY_ON_STEP1_AND_RESCHEDULE: return "TASK::CODE::DISPLAY_ON_STEP1_AND_RESCHEDULE";
		case TASK::CODE::DISPLAY_ON_STEP1: return "TASK::CODE::DISPLAY_ON_STEP1";
		break;
		
		case TASK::CODE::DISPLAY_ON_STEP2_AND_RESCHEDULE: return "TASK::CODE::DISPLAY_ON_STEP2_AND_RESCHEDULE";
		case TASK::CODE::DISPLAY_ON_STEP2: return "TASK::CODE::DISPLAY_ON_STEP2";
		break;
		
		case TASK::CODE::DISPLAY_TOGGLE_STEP1: return "TASK::CODE::DISPLAY_TOGGLE_STEP1";
		break;
		
		case TASK::CODE::DISPLAY_TOGGLE_STEP2: return "TASK::CODE::DISPLAY_TOGGLE_STEP2";
		break;
		
		case TASK::CODE::SET_BRIGHTNESS_AND_RESCHEDULE: return "TASK::CODE::SET_BRIGHTNESS_AND_RESCHEDULE";
		case TASK::CODE::SET_BRIGHTNESS: return "TASK::CODE::SET_BRIGHTNESS";
		break;
		
		default: return "INVALID CODE";
		break;
	}
}


/******************************************************************************
/ Task Heap Function Implementations
/*****************************************************************************/

tHeap::TaskHeap::~TaskHeap()
{
	for (Task* task : taskHeap)
	{
		if (task) delete task;
	}
	
	return;
}

void tHeap::TaskHeap::pushTask(long scheduledTime, TASK::CODE taskCode)
{
	if (!TASK::isValidTaskCode(taskCode)) throw std::invalid_argument("Attempted to create task with invalid task type");
	
	Task* task = new Task(scheduledTime, taskCode);
	
	//Push and percolate through heap vector
	this->taskHeap.push_back(task);
	std::push_heap(this->taskHeap.begin(), this->taskHeap.end(), DoSwap());
	
	#ifdef DEBUG
	std::cout << "Pushed task {" << scheduledTime << ", " << TASK::toString(taskCode) << '}' << std::endl;
	#endif
	
	return;
}

tHeap::Task* tHeap::TaskHeap::popTask()
{
	//Empty check
	if (this->taskHeap.empty()) throw std::underflow_error("ERROR: Heap underflow");
	
	//Pop and percolate next task to the head
	std::pop_heap(this->taskHeap.begin(), this->taskHeap.end(), DoSwap());
	Task* toReturn = this->taskHeap.back();
	this->taskHeap.pop_back();
	
	#ifdef DEBUG
	std::cout << "Popped task {" << toReturn->scheduledTime << ", " << TASK::toString(toReturn->task) << '}' << std::endl;
	#endif
	
	return toReturn; //This task object is now the responsibility of the client. It must delete it.
}

tHeap::Task* tHeap::TaskHeap::peekTask()
{
	//Empty check
	if (this->taskHeap.empty()) throw std::underflow_error("ERROR: Heap underflow");
	
	#ifdef DEBUG
	std::cout << "Peeked task {" << this->taskHeap.front()->scheduledTime << ", " << TASK::toString(this->taskHeap.front()->task) << '}' << std::endl;
	#endif
	
	return this->taskHeap.front();
}

bool tHeap::TaskHeap::isEmpty()
{
	return this->taskHeap.empty();
}



































