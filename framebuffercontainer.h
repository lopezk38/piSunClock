/******************************************************************************
/ Pi 4 Sunrise Clock App Framebuffer Container Class Spec - lopezk38 2025
/
/*****************************************************************************/

#ifndef SUNCLOCK_APP_FBCONT
#define SUNCLOCK_APP_FBCONT

/******************************************************************************
/ Dependencies, namespacing
/*****************************************************************************/

#include <linux/fb.h>

#include <string>
#include <iostream>
#include <exception>

#include "errorcodes.h"


/******************************************************************************
/ Class specification
/*****************************************************************************/

class FrameBufferContainer
{
	
private:
	
	const int bufNum = -1;
	int descriptor = -1;
	struct fb_var_screeninfo resData; //Contains screen resolution data
	
	bool errorState = false;
	
	RESDATA_ERR::CODE loadScreenInfo();
	FBDESC_ERR::CODE openFrameBuffer();
	
public:

	FrameBufferContainer(int bufNum): bufNum(bufNum)
	{
		//Check for valid buffer number
		if (bufNum < 0)
		{
			//Invalid number given
			
			errorState = true;
			std::cerr << "ERROR: An invalid buffer device number was given, could not open" << std::endl;
			return;
		}
		
		openFrameBuffer();
		loadScreenInfo();
		
		if (errorState) throw std::runtime_error("Framebuffer could not be accessed");
		
		return;
	}

	~FrameBufferContainer()
	{
		if (descriptor != -1) close (descriptor);
		return;
	}
	
	int getXRes()
	{
		if (errorState) throw std::runtime_error("Framebuffer could not be accessed");
		return resData.xres;
	}
	
	int getYRes()
	{
		if (errorState) throw std::runtime_error("Framebuffer could not be accessed");
		return resData.yres;
	}
	
	int getScreenBPP()
	{
		if (errorState) throw std::runtime_error("Framebuffer could not be accessed");
		return resData.bits_per_pixel;
	}
	
	fb_var_screeninfo getAllScreenInfo()
	{
		if (errorState) throw std::runtime_error("Framebuffer could not be accessed");
		return resData;
	}
};

#endif