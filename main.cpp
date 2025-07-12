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
#include "ddcutil_c_api.h"
#include "ddcutil_status_codes.h"

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

//Time
timeStruct getTime();

//Brightness and DDC
DDCA_Display_Handle ddcInit();
DDCA_Status setDDCBrightness(DDCA_Display_Handle displayHandle, unsigned char brightness);
void ddcDeinit(DDCA_Display_Handle displayHandle);


/******************************************************************************
/ Function implementations
/*****************************************************************************/

int main(int argc, char* argv[])
{
	//Init DDC
	DDCA_Display_Handle displayHandle = ddcInit();
	
	//Init framebuffer
	FrameBufferContainer fBuf(FRAMEBUFFER_DEV);
	
	//Init window
	InitWindow(fBuf.getXRes(), fBuf.getYRes(), "Clock Window");
	
	//Draw color
	#ifndef DEBUG
	SetTargetFPS(1);
	std::cout << "Sun Clock is now running. Press ESC to quit." << std::endl;
	
	//Setup time
	timeStruct curTime = getTime();
	
	//Setup initial brightness
	setDDCBrightness(displayHandle, sunBrightness::interp(curTime.hour, curTime.min));
	bool shouldUpdateBrightness = false; //For debouncing
	
	//Main loop
	while (!WindowShouldClose())
	{
		BeginDrawing();
		
		//Set color
		curTime = getTime();
		Color color = sunColor::interp(curTime.hour, curTime.min);
		ClearBackground(color);
		
		//Set brightness every 30 minutes
		if (!(curTime.min % 30))
		{
			if (shouldUpdateBrightness) //Debouncing check
			{
				setDDCBrightness(displayHandle, sunBrightness::interp(curTime.hour, curTime.min));
				shouldUpdateBrightness = false;
			}
		}
		else shouldUpdateBrightness = true;
		
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
			
			//Set color
			timeStruct curTime = getTime();
			Color color = sunColor::interp(i, j);
			ClearBackground(color);
			
			//Set brightness
			if (!(i % 4) && !j) //Only do it once every four seconds (@60fps) to prevent excessive stress on the monitor
			{
				setDDCBrightness(displayHandle, sunBrightness::interp(i, j));
			}
		
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

DDCA_Display_Handle ddcInit()
{
	DDCA_Display_Identifier displayID;
	DDCA_Display_Ref displayRef;
	DDCA_Display_Handle displayHandle = nullptr;
	
	//Identify and enumerate display
	ddca_create_dispno_display_identifier(FRAMEBUFFER_DEV + 1, &displayID); //DDC starts at 1, not 0 like device number. Add 1 to compensate
	DDCA_Status result = ddca_get_display_ref(displayID, &displayRef);
	
	if (result) 
	{
		//Non 0 result is an error
		std::cerr << "ERROR: Unable to find DDC display. DDCA Status: " << ddca_rc_name(result) << ": " << ddca_rc_desc(result) << std::endl;
		throw result;
	}
	
	ddca_free_display_identifier(displayID); //Cleanup
	
	//Connect to display
	result = ddca_open_display2(displayRef, false, &displayHandle);
	
	if (result) 
	{
		//Non 0 result is an error
		std::cerr << "ERROR: Unable to connect to DDC display. DDCA Status: " << ddca_rc_name(result) << ": " << ddca_rc_desc(result) << std::endl;
		throw result;
	}
	
	return displayHandle;
}

DDCA_Status setDDCBrightness(DDCA_Display_Handle displayHandle, unsigned char brightness)
{
	//Check if we are connected to a display
	if (!displayHandle) return DDCRC_INVALID_DISPLAY;
	
	//Enforce bounds on brightness
	if (brightness > 100) brightness = 100;
	
	//Use DDC to command brightness level. 0x10 code is brighness.	
	DDCA_Status result = ddca_set_non_table_vcp_value(displayHandle, 0x10, 0x0, brightness);
	
	#ifdef DEBUG
	std::cout << "Set brightness to " << brightness << " with status code " << result << ": " << ddca_rc_name(result) << ": " << ddca_rc_desc(result) << std::endl;
	#endif
	
	return result;
}

void ddcDeinit(DDCA_Display_Handle displayHandle)
{
	ddca_close_display(displayHandle);
	
	return;
}

#endif


























































