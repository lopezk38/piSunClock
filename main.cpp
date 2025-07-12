/******************************************************************************
/ Pi 4 Sunrise Clock App - lopezk38 2025
/ Version 0.1
/
/*****************************************************************************/

#ifndef SUNCLOCK_APP_MAIN
#define SUNCLOCK_APP_MAIN

//#define DEBUG


/******************************************************************************
/ Dependencies, namespacing
/*****************************************************************************/

#include <unistd.h>
#include <chrono>

#include "raylib.h"

#include "errorcodes.h"
#include "framebuffercontainer.h"
#include "sunColorCurveLUT.h"

namespace CLOCKAPP {};
using namespace CLOCKAPP;


/******************************************************************************
/ Constants, enums, structs
/*****************************************************************************/

constexpr unsigned int FRAMEBUFFER_DEV = 0;

constexpr int TIMEZONE_OFFSET = -7; //Pacific time
static_assert(TIMEZONE_OFFSET > -24 || TIMEZONE_OFFSET < 24, "TIMEZONE_OFFSET must be a valid timezone");

struct timeStruct
{
	long hour;
	long min;
	long sec;
};


/******************************************************************************
/ Function prototypes
/*****************************************************************************/

timeStruct getTime();


/******************************************************************************
/ Function implementations
/*****************************************************************************/

int main(int argc, char* argv[])
{
	//Init framebuffer
	FrameBufferContainer fBuf(FRAMEBUFFER_DEV);
	
	//Init window
	InitWindow(fBuf.getXRes(), fBuf.getYRes(), "Clock Window");
	
	//Draw color
	#ifndef DEBUG
	SetTargetFPS(1);
	std::cout << "Sun Clock is now running. Press ESC to quit." << std::endl;
	while (!WindowShouldClose())
	{
		BeginDrawing();
		
		timeStruct curTime = getTime();
		Color color = sunColor::interp(curTime.hour, curTime.min);
		ClearBackground(color);
		
		EndDrawing();
	}
	#endif
	#ifdef DEBUG
	//Debug mode, does a quick color sweep through the day in a few seconds
	SetTargetFPS(60);
	std::cout << "Sun Clock is running in debug mode. Press ESC to quit." << std::endl;
	for (int i = 0; i < 24; ++i)
	{
		for (int j = 0; j < 60; ++j)
		{
			BeginDrawing();
			
			DrawText("DEBUG MODE", 20, 20, 40, YELLOW);
			
			timeStruct curTime = getTime();
			Color color = sunColor::interp(i, j);
			ClearBackground(color);
		
			EndDrawing();
		}
	}
	#endif
	
	//Deinit
	CloseWindow(); 
	
	return 0;
}

timeStruct getTime()
{
	
	//Get time snapshot in seconds
	std::chrono::system_clock::time_point timeSnap = std::chrono::system_clock::now();

	//Convert to days (lossy)
	auto timeSnapInDays = std::chrono::time_point_cast<std::chrono::days>(timeSnap);

	//Find amount of seconds from the start of the day by taking advantage of the lossy day conversion
	auto secondsSinceMidnight = timeSnap - timeSnapInDays;

	//Repeat lossy conversion to extract hour, then minute, then second
	auto hour = std::chrono::duration_cast<std::chrono::hours>(secondsSinceMidnight);
	secondsSinceMidnight -= hour;
	auto minute = std::chrono::duration_cast<std::chrono::minutes>(secondsSinceMidnight);
	secondsSinceMidnight -= minute;
	auto second = std::chrono::duration_cast<std::chrono::seconds>(secondsSinceMidnight);
	
	timeStruct curTime = { hour.count(), minute.count(), second.count() };
	
	//Apply timezone offset
	curTime.hour += TIMEZONE_OFFSET;
	if (curTime.hour < 0) curTime.hour += 24;
	if (curTime.hour > 23 ) curTime.hour -= 24;
	
	#ifdef DEBUG
	std::cout << "Time: " << curTime.hour << ":" << curTime.min << ":" << curTime.sec << std::endl;
	#endif
	
	return curTime;
}

#endif


























































