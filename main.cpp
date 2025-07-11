/******************************************************************************
/ Pi 4 Sunrise Clock App - lopezk38 2025
/ Version 0.1
/
/*****************************************************************************/

#ifndef SUNCLOCK_APP_MAIN
#define SUNCLOCK_APP_MAIN

#define DEBUG 1


/******************************************************************************
/ Dependencies, namespacing
/*****************************************************************************/

#include <unistd.h>

#include "raylib.h"

#include "errorcodes.h"
#include "framebuffercontainer.h"

namespace CLOCKAPP {};
using namespace CLOCKAPP;


/******************************************************************************
/ Constants, enums
/*****************************************************************************/

constexpr int FRAMEBUFFER_DEV = 0;


/******************************************************************************
/ Function prototypes
/*****************************************************************************/


/******************************************************************************
/ Function implementations
/*****************************************************************************/

int main(int argc, char* argv[])
{
	//Init framebuffer
	FrameBufferContainer fBuf(FRAMEBUFFER_DEV);
	
	//Init window
	InitWindow(fBuf.getXRes(), fBuf.getYRes(), "Clock Window");
	SetTargetFPS(1);
	
	//Draw color
	BeginDrawing();
	ClearBackground(RAYWHITE);
	EndDrawing();
	
	sleep(5);
	
	//Deinit
	CloseWindow(); 
	
	return 0;
}

#endif


























































