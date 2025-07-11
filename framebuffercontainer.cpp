/******************************************************************************
/ Pi 4 Sunrise Clock App Framebuffer Container Implementation - lopezk38 2025
/
/*****************************************************************************/

/******************************************************************************
/ Dependencies, namespacing
/*****************************************************************************/

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h> 
#include <cstring>

#include "framebuffercontainer.h"


/******************************************************************************
/ Class implementation
/*****************************************************************************/

RESDATA_ERR::CODE FrameBufferContainer::loadScreenInfo()
{
	if (ioctl(this->descriptor, FBIOGET_VSCREENINFO, &this->resData))
	{
		//Unable to retrieve screen info
		std::cerr << "ERROR: Could not load screen info" << std::endl;
		return RESDATA_ERR::CODE::RD_FAIL;
	}
	
	#if DEBUG
	std::cout << "Loaded screen data. Res: " << this->resData.xres << "x" << this->resData.yres << std::endl;
	#endif
	
	return RESDATA_ERR::CODE::SUCCESS;
}

FBDESC_ERR::CODE FrameBufferContainer::openFrameBuffer()
{
	//Build device directory string
	std::string devDirTemp = "/dev/fb"; //7 chars
	devDirTemp.append(std::to_string(this->bufNum));
	const char* devDir = devDirTemp.c_str();
	
	//Build device directory string
	//std::string devNum = std::to_string(this->bufNum); //Get device number as string
	//const char* devDir = new char[8 + devNum.length()]; //Allocate a buffer of the exact size to fit the directory w/ null term
	//strncpy(devDir, "/dev/fb", 8 + devNum.length()); //Copy the beginning of the directory to the buffer
	//strncpy(devDir + 7, devNum.c_str(), devNum.length()); //Copy the device number string to the end of the buffer
	
	this->descriptor = open(devDir, O_RDWR);
	
	if (this->descriptor == -1)
	{
		std::cerr << "ERROR: Failed to open framebuffer device at " << devDir << std::endl;
		return FBDESC_ERR::CODE::OPEN_FAIL;
	}
	
	#if DEBUG
	std::cout << "Opened framebuffer device at " << devDir << " with descriptor " << this->descriptor << std::endl;
	#endif
	
	return FBDESC_ERR::CODE::SUCCESS;
}