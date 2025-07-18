/******************************************************************************
/ Pi 4 Sunrise Clock App - lopezk38 2025
/ Version 1.0
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
#include <sstream>

#include "raylib.h"
#include "ddcutil_c_api.h"
#include "ddcutil_status_codes.h"

#include "errorcodes.h"
#include "framebuffercontainer.h"
#include "sunColorCurveLUT.h"
#include "clockTextColorCurveLUT.h"

#include "taskHeap.h"

using namespace std::chrono_literals;


/******************************************************************************
/ Constants, enums, structs
/*****************************************************************************/

//Raylib Drawing Settings
constexpr unsigned int FRAMEBUFFER_DEV = 0; // /dev/fb0
constexpr unsigned int FRAME_RATE = 1; //FPS
constexpr unsigned int TEXT_SIZE = 250;
constexpr bool HOUR_LEADING_ZERO = true;

//DDC (monitor control) settings
constexpr unsigned char VCP_INPUT_CODE = 0x3; //DVI-D

constexpr std::chrono::minutes BRIGHTNESS_UPDATE_FREQ = 30min;

constexpr bool POWEROFF_ON_ZERO_BRIGHTNESS = true;
constexpr std::chrono::minutes POWERCHECK_UPDATE_FREQ = 15min;
constexpr std::chrono::seconds POWERON_STEP_DELAY = 2s;
constexpr std::chrono::seconds POWERON_BRIGHTNESS_UPD_DELAY = 2s;

//Time settings
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
void drawClockText(const timeStruct& curTime, const int xRes, const int yRes);

//Brightness and DDC
DDCA_Display_Handle ddcInit();
DDCA_Status setDDCBrightness(DDCA_Display_Handle displayHandle, unsigned char brightness);
DDCA_Status setDisplayInput(DDCA_Display_Handle displayHandle, unsigned char vcpInputCode);
DDCA_Status toggleDisplayPower(DDCA_Display_Handle displayHandle);
DDCA_Status displayPowerOff(DDCA_Display_Handle displayHandle);
DDCA_Status displayPowerOn(DDCA_Display_Handle displayHandle);
bool isDisplayOn(DDCA_Display_Handle displayHandle);
void ddcDeinit(DDCA_Display_Handle displayHandle);


/******************************************************************************
/ Function implementations
/*****************************************************************************/

int main(int argc, char* argv[])
{
	unsigned char currentBrightness = 1; //Will be updated later
	bool powerCheckInProgress = false; //Used to lock powerchecks
	tHeap::TaskHeap taskSchedule;
	
	//Init DDC
	DDCA_Display_Handle displayHandle = ddcInit();
	
	//Init framebuffer
	FrameBufferContainer fBuf(FRAMEBUFFER_DEV);
	
	//Cache resolution
	int xRes = fBuf.getXRes();
	int yRes = fBuf.getYRes();
	
	//Init window
	InitWindow(xRes, yRes, "Clock Window");
	
	//Draw color
	#ifndef DEBUG
	SetTargetFPS(FRAME_RATE); //1 FPS by default
	std::cout << "Sun Clock is now running. Press ESC to quit." << std::endl;
	
	//Setup time
	timeStruct curTime = getTime();
	
	//Setup initial brightness
	setDDCBrightness(displayHandle, SunBrightness::interp(curTime.hour, curTime.min));

	long schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + BRIGHTNESS_UPDATE_FREQ).time_since_epoch()).count();
	taskSchedule.pushTask(schTime, tHeap::TASK::CODE::SET_BRIGHTNESS_AND_RESCHEDULE); //Schedule another brightness update
	
	//Setup power update schedule if the feature is enabled
	if (POWEROFF_ON_ZERO_BRIGHTNESS)
	{
		schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + POWERCHECK_UPDATE_FREQ).time_since_epoch()).count();
		taskSchedule.pushTask(schTime, tHeap::TASK::CODE::CHECK_SHOULD_TOGGLE_DISPLAY_PWR); //Schedule a power update
	}
	
	//Main loop
	while (!WindowShouldClose())
	{
		BeginDrawing();

		//Set color
		curTime = getTime();
		Color color = SunColor::interp(curTime.hour, curTime.min);
		ClearBackground(color);
		
		//Draw clock
		drawClockText(curTime, xRes, yRes);
		
		//Get current time for scheduler
		long curTimeSeconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		
		//Check for commands to execute
		while (!taskSchedule.isEmpty() && taskSchedule.peekTask()->scheduledTime <= curTimeSeconds)
		{
			//Time to execute
			tHeap::Task* taskToExecute = taskSchedule.popTask();
			if (taskToExecute)
			{
				if (tHeap::TASK::isValidTaskCode(taskToExecute->task))
				{
					switch (taskToExecute->task)
					{
						default:
						case tHeap::TASK::CODE::NONE:
						{
							//Do nothing
							break;
						}
						
						case tHeap::TASK::CODE::SET_INPUT:
						{
							setDisplayInput(displayHandle, VCP_INPUT_CODE); //Execute
							
							break;
						}
						
						case tHeap::TASK::CODE::CHECK_SHOULD_TOGGLE_DISPLAY_PWR:
						{
							//Lock power check to prevent bouncing
							if (powerCheckInProgress)
							{
								//Reschedule the blocked task
								taskSchedule.pushTask(1, tHeap::TASK::CODE::CHECK_SHOULD_TOGGLE_DISPLAY_PWR);
								
								break;
							}
							
							if (currentBrightness)
							{
								//Brightness is non zero, turn on the display. Function will ignore redundant calls
								taskSchedule.pushTask(0, tHeap::TASK::CODE::DISPLAY_ON_STEP1_AND_RESCHEDULE); //Schedule next step to run immediately
							}
							else
							{
								//Brightness is zero, turn off the display. Function will ignore redundant calls
								taskSchedule.pushTask(0, tHeap::TASK::CODE::DISPLAY_OFF_AND_RESCHEDULE); //Schedule next step to run immediately
							}
							
							break;
						}

						case tHeap::TASK::CODE::DISPLAY_OFF_AND_RESCHEDULE:
						{
							//Schedule next check
							schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + POWERCHECK_UPDATE_FREQ).time_since_epoch()).count();
							taskSchedule.pushTask(schTime, tHeap::TASK::CODE::CHECK_SHOULD_TOGGLE_DISPLAY_PWR);
									
							//Unlock
							powerCheckInProgress = false;
							
							//Bleed through
						}
						case tHeap::TASK::CODE::DISPLAY_OFF:
						{
							displayPowerOff(displayHandle); //Execute
								
							break;
						}
						
						case tHeap::TASK::CODE::DISPLAY_ON_STEP1_AND_RESCHEDULE:
						{
							//Make sure display is not on
							if (isDisplayOn(displayHandle))
							{
								//It's on already. Just reschedule the check
									
								//Schedule next check
								long schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + POWERCHECK_UPDATE_FREQ).time_since_epoch()).count();
								taskSchedule.pushTask(schTime, tHeap::TASK::CODE::CHECK_SHOULD_TOGGLE_DISPLAY_PWR);
									
								//Unlock
								powerCheckInProgress = false;
									
								break;
							}
							
							setDisplayInput(displayHandle, VCP_INPUT_CODE); //Execute. Must set input to soft wake monitor before it will accept powerOn command
							
							//Schedule next step
							long schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + POWERON_STEP_DELAY).time_since_epoch()).count();
							taskSchedule.pushTask(schTime, tHeap::TASK::CODE::DISPLAY_ON_STEP2_AND_RESCHEDULE);
							
							break;
						}
						
						case tHeap::TASK::CODE::DISPLAY_ON_STEP1:
						{
							setDisplayInput(displayHandle, VCP_INPUT_CODE); //Execute. Must set input to soft wake monitor before it will accept powerOn command
							
							//Schedule next step
							long schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + POWERON_STEP_DELAY).time_since_epoch()).count();
							taskSchedule.pushTask(schTime, tHeap::TASK::CODE::DISPLAY_ON_STEP2);
							
							break;
						}
						
						case tHeap::TASK::CODE::DISPLAY_ON_STEP2_AND_RESCHEDULE:
						{
							//Schedule next check
							long schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + POWERCHECK_UPDATE_FREQ).time_since_epoch()).count();
							taskSchedule.pushTask(schTime, tHeap::TASK::CODE::CHECK_SHOULD_TOGGLE_DISPLAY_PWR);
								
							//Unlock
							powerCheckInProgress = false;
								
							//Bleed through
						}
						case tHeap::TASK::CODE::DISPLAY_ON_STEP2:
						{
							displayPowerOn(displayHandle); //Execute
							
							//Schedule brightness update
							schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + POWERON_BRIGHTNESS_UPD_DELAY).time_since_epoch()).count();
							taskSchedule.pushTask(schTime, tHeap::TASK::CODE::SET_BRIGHTNESS);
							
							break;
						}
			
						case tHeap::TASK::CODE::DISPLAY_TOGGLE_STEP1:
						{
							setDisplayInput(displayHandle, VCP_INPUT_CODE); //Execute. Must set input to soft wake monitor before it will accept powerOn command
							
							//Schedule next step
							long schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + POWERON_BRIGHTNESS_UPD_DELAY).time_since_epoch()).count();
							taskSchedule.pushTask(schTime, tHeap::TASK::CODE::DISPLAY_TOGGLE_STEP2);
							break;
						}
						
						case tHeap::TASK::CODE::DISPLAY_TOGGLE_STEP2:
						{
							toggleDisplayPower(displayHandle); //Execute
							
							//Schedule brightness update
							schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + POWERON_BRIGHTNESS_UPD_DELAY).time_since_epoch()).count();
							taskSchedule.pushTask(schTime, tHeap::TASK::CODE::SET_BRIGHTNESS);
							
							break;
						}
						
						case tHeap::TASK::CODE::SET_BRIGHTNESS_AND_RESCHEDULE:
						{
							//Reschedule
							long schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + BRIGHTNESS_UPDATE_FREQ).time_since_epoch()).count();
							taskSchedule.pushTask(schTime, tHeap::TASK::CODE::SET_BRIGHTNESS_AND_RESCHEDULE);
							
							//Bleed through
						}
						case tHeap::TASK::CODE::SET_BRIGHTNESS:
						{
							unsigned char targetBrightness = SunBrightness::interp(curTime.hour, curTime.min); //Calc next brightness
							setDDCBrightness(displayHandle, targetBrightness); //Tell monitor to adjust to the requested brightness
							currentBrightness = targetBrightness; //Keep track of current state
							
							break;
						}
					}
				}
				else std::cerr << "ERROR: Attempted to run invalid task!" << std::endl;
				
				//Executed. Clean up
				delete taskToExecute;
			}
			else std::cerr << "ERROR: Attempted to run task which was nullptr!" << std::endl;
		}
		
		EndDrawing();
	}
	#endif
	#ifdef DEBUG
	//Debug mode, does a quick color sweep through the day in a few seconds
	SetTargetFPS(60);
	std::cout << "Sun Clock is running in debug mode. Press ESC to quit." << std::endl;
	
	//Setup brightness update schedule
	long schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + 2s).time_since_epoch()).count();
	taskSchedule.pushTask(schTime, tHeap::TASK::CODE::SET_BRIGHTNESS_AND_RESCHEDULE); //Schedule a brightness update 2 sec from now
	
	//Setup power update schedule if the feature is enabled
	if (POWEROFF_ON_ZERO_BRIGHTNESS)
	{
		schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + 1s).time_since_epoch()).count();
		taskSchedule.pushTask(schTime, tHeap::TASK::CODE::CHECK_SHOULD_TOGGLE_DISPLAY_PWR); //Schedule a power update 1 sec from now
	}
	
	//Do day cycle sim
	for (int i = 0; i < 24; ++i)
	{
		for (int j = 0; j < 60; ++j)
		{
			BeginDrawing();
			
			DrawText("DEBUG MODE", 20, 20, 40, YELLOW);
			
			//Set color
			timeStruct curTime = getTime();
			Color color = SunColor::interp(i, j);
			ClearBackground(color);
			
			//Draw clock
			drawClockText({i, j, 0}, xRes, yRes);
			
			//Get current time for scheduler
			long curTimeSeconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			
			//Check for commands to execute
			while (!taskSchedule.isEmpty() && taskSchedule.peekTask()->scheduledTime <= curTimeSeconds)
			{
				//Time to execute
				tHeap::Task* taskToExecute = taskSchedule.popTask();
				if (taskToExecute)
				{
					if (tHeap::TASK::isValidTaskCode(taskToExecute->task))
					{
						switch (taskToExecute->task)
						{
							default:
							case tHeap::TASK::CODE::NONE:
							{
								//Do nothing
								break;
							}
							
							case tHeap::TASK::CODE::SET_INPUT:
							{
								setDisplayInput(displayHandle, VCP_INPUT_CODE); //Execute
								break;
							}
							
							case tHeap::TASK::CODE::CHECK_SHOULD_TOGGLE_DISPLAY_PWR:
							{
								//Lock power check to prevent bouncing
								if (powerCheckInProgress)
								{
									//Reschedule the blocked task
									taskSchedule.pushTask(1, tHeap::TASK::CODE::CHECK_SHOULD_TOGGLE_DISPLAY_PWR);
									
									break;
								}
								
								std::cout << "Checking if we should toggle the display power. Current brightness is " << static_cast<short>(currentBrightness) << '%' << std::endl;
								
								if (currentBrightness)
								{
									//Brightness is non zero, turn on the display. Function will ignore redundant calls
									taskSchedule.pushTask(0, tHeap::TASK::CODE::DISPLAY_ON_STEP1_AND_RESCHEDULE); //Schedule next step to run immediately
								}
								else
								{
									//Brightness is zero, turn off the display. Function will ignore redundant calls
									taskSchedule.pushTask(0, tHeap::TASK::CODE::DISPLAY_OFF_AND_RESCHEDULE); //Schedule next step to run immediately
								}
								
								break;
							}
							case tHeap::TASK::CODE::DISPLAY_OFF_AND_RESCHEDULE:
							{
								//Schedule next check for 5sec from now
								schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + 5s).time_since_epoch()).count();
								taskSchedule.pushTask(schTime, tHeap::TASK::CODE::CHECK_SHOULD_TOGGLE_DISPLAY_PWR);
										
								//Unlock
								powerCheckInProgress = false;
								
								//Bleed through
							}
							case tHeap::TASK::CODE::DISPLAY_OFF:
							{
								displayPowerOff(displayHandle); //Execute
								
								break;
							}
							
							case tHeap::TASK::CODE::DISPLAY_ON_STEP1_AND_RESCHEDULE:
							{
								//Make sure display is not on
								if (isDisplayOn(displayHandle))
								{
									//It's on already. Just reschedule the check
									
									//Schedule next check for 5sec from now
									long schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + 5s).time_since_epoch()).count();
									taskSchedule.pushTask(schTime, tHeap::TASK::CODE::CHECK_SHOULD_TOGGLE_DISPLAY_PWR);
									
									//Unlock
									powerCheckInProgress = false;
									
									std::cout << "Requested display to power on but it was already on" << std::endl;
									
									break;
								}
								
								setDisplayInput(displayHandle, VCP_INPUT_CODE); //Execute. Must set input to soft wake monitor before it will accept powerOn command
								
								//Schedule next step for 2 seconds from now
								long schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + 2s).time_since_epoch()).count();
								taskSchedule.pushTask(schTime, tHeap::TASK::CODE::DISPLAY_ON_STEP2_AND_RESCHEDULE);
								
								break;
							}
				
							case tHeap::TASK::CODE::DISPLAY_ON_STEP1:
							{
								setDisplayInput(displayHandle, VCP_INPUT_CODE); //Execute. Must set input to soft wake monitor before it will accept powerOn command
								
								//Schedule next step for 2 second from now
								long schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + 2s).time_since_epoch()).count();
								taskSchedule.pushTask(schTime, tHeap::TASK::CODE::DISPLAY_ON_STEP2);
								
								break;
							}
							
							case tHeap::TASK::CODE::DISPLAY_ON_STEP2_AND_RESCHEDULE:
							{
								//Schedule next check for 5sec from now
								long schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + 5s).time_since_epoch()).count();
								taskSchedule.pushTask(schTime, tHeap::TASK::CODE::CHECK_SHOULD_TOGGLE_DISPLAY_PWR);
									
								//Unlock
								powerCheckInProgress = false;
									
								//Bleed through
							}
							case tHeap::TASK::CODE::DISPLAY_ON_STEP2:
							{
								displayPowerOn(displayHandle); //Execute
								
								//Schedule brightness update
								schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + POWERON_BRIGHTNESS_UPD_DELAY).time_since_epoch()).count();
								taskSchedule.pushTask(schTime, tHeap::TASK::CODE::SET_BRIGHTNESS);
								
								break;
							}
				
							case tHeap::TASK::CODE::DISPLAY_TOGGLE_STEP1:
							{
								setDisplayInput(displayHandle, VCP_INPUT_CODE); //Execute. Must set input to soft wake monitor before it will accept powerOn command
								
								//Schedule next step for 2 seconds from now
								long schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + 2s).time_since_epoch()).count();
								taskSchedule.pushTask(schTime, tHeap::TASK::CODE::DISPLAY_TOGGLE_STEP2);
								break;
							}
							
							case tHeap::TASK::CODE::DISPLAY_TOGGLE_STEP2:
							{
								toggleDisplayPower(displayHandle); //Execute
								
								//Schedule brightness update for 2 seconds from now
								schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + 2s).time_since_epoch()).count();
								taskSchedule.pushTask(schTime, tHeap::TASK::CODE::SET_BRIGHTNESS);
								
								break;
							}
							
							case tHeap::TASK::CODE::SET_BRIGHTNESS_AND_RESCHEDULE:
							{
								//Reschedule for 5 sec from now
								long schTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + 5s).time_since_epoch()).count();
								taskSchedule.pushTask(schTime, tHeap::TASK::CODE::SET_BRIGHTNESS_AND_RESCHEDULE);
								
								//Bleed through
							}
							case tHeap::TASK::CODE::SET_BRIGHTNESS:
							{
								unsigned char targetBrightness = SunBrightness::interp(i, j); //Calc next brightness
								setDDCBrightness(displayHandle, targetBrightness); //Tell monitor to adjust to the requested brightness
								currentBrightness = targetBrightness; //Keep track of current state
								
								break;
							}
						}
					}
					else std::cerr << "ERROR: Attempted to run invalid task!" << std::endl;
					
					//Executed. Clean up
					delete taskToExecute;
				}
				else std::cerr << "ERROR: Attempted to run task which was nullptr!" << std::endl;		
			}
			if (!taskSchedule.isEmpty()) std::cout << "Next task due in " << taskSchedule.peekTask()->scheduledTime - curTimeSeconds << " seconds" << std::endl;
		
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

void drawClockText(const timeStruct& curTime, const int xRes, const int yRes)
{
	static std::stringstream clockTextBuf; //static because there is no need to constantly construct and deconstruct it
	
	//Build time string
	//Convert from military hour to regular AM/PM hours (0-24 to 0-12)
	short stdHr = curTime.hour % 12;
	if (stdHr == 0) stdHr = 12;
	
	if (stdHr < 10 && HOUR_LEADING_ZERO) clockTextBuf << '0'; //Add a leading 0 to hours if HOUR_LEADING_ZERO is on and hour is a single digit number
	clockTextBuf << stdHr << ':';
	if (curTime.min < 10) clockTextBuf << '0'; //Add a leading zero to minute if minute is a single digit number
	clockTextBuf << curTime.min;
	const char* timeStr = clockTextBuf.str().c_str(); //Raylib requires a C string
	
	//Calculate correct offsets to center the clock text
	int xOffset = (xRes - MeasureText(timeStr, TEXT_SIZE)) / 2;
	int yOffset = (yRes - TEXT_SIZE) / 2;
	
	//Print the clock on the center of the screen
	DrawText(timeStr, xOffset, yOffset, TEXT_SIZE, ClockTextColor::interp(curTime.hour, curTime.min));
	
	//Flush stringstream
	clockTextBuf.str("");
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
	
	//Use DDC to command brightness level. 0x10 code is brightness.	
	DDCA_Status result = ddca_set_non_table_vcp_value(displayHandle, 0x10, 0x0, brightness);
	
	#ifdef DEBUG
	std::cout << "Set brightness to " << static_cast<short>(brightness) << " with status code " << result << ": " << ddca_rc_name(result) << ": " << ddca_rc_desc(result) << std::endl;
	#endif
	
	return result;
}

DDCA_Status setDisplayInput(DDCA_Display_Handle displayHandle, unsigned char vcpInputCode)
{
	if (!displayHandle) return DDCRC_INVALID_DISPLAY;
	
	//Use DDC to command display input
	DDCA_Status inputCmdResult = ddca_set_non_table_vcp_value(displayHandle, 0x60, 0x0, vcpInputCode); //Input command
	
	#ifdef DEBUG
	std::cout << "Attempted to set display input to " << static_cast<short>(VCP_INPUT_CODE) 
			  << ". Got status code " << inputCmdResult << ": " << ddca_rc_name(inputCmdResult) << ": " << ddca_rc_desc(inputCmdResult) << std::endl;
	#endif
	
	return inputCmdResult;
}

DDCA_Status toggleDisplayPower(DDCA_Display_Handle displayHandle)
{
	//Check if we are connected to a display
	if (!displayHandle) return DDCRC_INVALID_DISPLAY;
	
	//Use DDC to command power toggle
	DDCA_Status powerCmdResult = ddca_set_non_table_vcp_value(displayHandle, 0xD6, 0x0, 0x5); //Power command
	
	#ifdef DEBUG		  
	std::cout << "Attempted to toggle display power. Got status code " << powerCmdResult << ": " << ddca_rc_name(powerCmdResult) << ": "
			  << ddca_rc_desc(powerCmdResult) << std::endl;
	#endif
	
	return powerCmdResult;
}

DDCA_Status displayPowerOff(DDCA_Display_Handle displayHandle)
{
	//Check if we are connected to a display
	if (!displayHandle) return DDCRC_INVALID_DISPLAY;
	
	//Check if display is already off
	if (!isDisplayOn(displayHandle))
	{
		//It's off. Just return OK
		
		#ifdef DEBUG		  
		std::cout << "Requested display to power off but it was already off" << std::endl;
		#endif
		
		return DDCRC_OK;
	}
	
	//Send DDC command to turn off the display
	DDCA_Status result = ddca_set_non_table_vcp_value(displayHandle, 0xD6, 0x0, 0x5); //Power command
	
	#ifdef DEBUG		  
	std::cout << "Requested display to power off. Got status code: " << ddca_rc_name(result) << ": "
			  << ddca_rc_desc(result) << std::endl;
	#endif
	
	return result;
	
}

DDCA_Status displayPowerOn(DDCA_Display_Handle displayHandle)
{
	//Check if we are connected to a display
	if (!displayHandle) return DDCRC_INVALID_DISPLAY;
	
	//Check if display is already on
	if (isDisplayOn(displayHandle))
	{
		//It's on. Just return OK
		
		#ifdef DEBUG		  
		std::cout << "Requested display to power on but it was already on" << std::endl;
		#endif
		
		return DDCRC_OK;
	}
	
	//Send DDC command to turn on the display
	DDCA_Status result = ddca_set_non_table_vcp_value(displayHandle, 0xD6, 0x0, 0x5); //Power command
	
	#ifdef DEBUG		  
	std::cout << "Requested display to power on. Got status code: " << ddca_rc_name(result) << ": "
			  << ddca_rc_desc(result) << std::endl;
	#endif
	
	return result;
}

bool isDisplayOn(DDCA_Display_Handle displayHandle)
{
	//Check if we are connected to a display
	if (!displayHandle) return true; //Assume display is on if we cannot talk to it
	
	//Use DDC command to request power mode (code 0xD6)
	DDCA_Non_Table_Vcp_Value readPowerValueStruct;
	DDCA_Status powerStatusResult = ddca_get_non_table_vcp_value(displayHandle, 0xD6, &readPowerValueStruct);
	unsigned char readPowerValue = readPowerValueStruct.sl; //Only need the low byte
	
	#ifdef DEBUG		  
	std::cout << "Requested display power status. Got state code " << static_cast<short>(readPowerValue) << " with status code: " << ddca_rc_name(powerStatusResult) << ": "
			  << ddca_rc_desc(powerStatusResult) << std::endl;
	#endif
	
	//Use DDC command to request the current monitor input if the monitor is on. This is because it allows us to determine if the monitor is soft on or fully on
	if (readPowerValue != 0x5)
	{
		DDCA_Non_Table_Vcp_Value readInputValueStruct;
		DDCA_Status inputStatusResult = ddca_get_non_table_vcp_value(displayHandle, 0x60, &readInputValueStruct);
		unsigned char readInputValue = readInputValueStruct.sl; //Only need the low byte
		
		#ifdef DEBUG		  
		std::cout << "Requested display input status. Got input code " << static_cast<short>(readInputValue) << " with status code: " << ddca_rc_name(inputStatusResult) << ": "
				  << ddca_rc_desc(inputStatusResult) << std::endl;
		#endif
		
		if (inputStatusResult) return true; //If we got an error, assume the monitor is on to prevent unstable state
		
		return readInputValue; //Any non-zero number is a valid input. Monitor gives 0 when soft-on, which we are considering off.
	}
	
	return false; //Monitor must be off if we got here
}

void ddcDeinit(DDCA_Display_Handle displayHandle)
{
	ddca_close_display(displayHandle);
	
	return;
}

#endif


























































