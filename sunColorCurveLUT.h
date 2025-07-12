/******************************************************************************
/ Pi 4 Sunrise Clock App Sun Color Curve LUT - lopezk38 2025
/
/*****************************************************************************/

#ifndef SUNCLOCK_APP_LUT
#define SUNCLOCK_APP_LUT

#ifdef DEBUG
#include <iostream>
#endif

#include "raylib.h"


class sunColor
{
public:

	static constexpr Color sunColorLUT[24] =
	{
		{0, 0, 0, 255},			//0 hrs
		{0, 0, 0, 255},			//1 hr
		{0, 0, 0, 255},			//2 hrs
		{0, 0, 0, 255},			//3 hrs
		{0, 0, 0, 255},			//4
		{0, 25, 50, 255},		//5
		{25, 75, 150, 255},		//6
		{50, 125, 200, 255},	//7
		{200, 255, 255, 255},	//8
		{210, 255, 255, 255},	//9
		{215, 255, 255, 255},	//10
		{220, 255, 255, 255},	//11
		{225, 255, 255, 255},	//12
		{225, 225, 225, 255},	//13
		{200, 225, 175, 255},	//14
		{200, 200, 150, 255},	//15
		{200, 182, 133, 255},	//16
		{200, 175, 125, 255},	//17
		{175, 75, 75, 255},		//18
		{150, 50, 50, 255},		//19
		{100, 33, 33, 255},		//20
		{50, 10, 10, 255},		//21
		{15, 0, 0, 255},		//22
		{0, 0, 0, 255}			//23 hrs
	};
	
	static Color interp(int hour, int minute)
	{
		//Bound inputs
		if (hour < 0 || hour > 23) { hour = 0; }
		if (minute < 0 || minute > 60) { minute = 0; }
		
		//Get base color vals to blend together
		const Color& thisHrColor = sunColorLUT[hour];
		const Color& nextHrColor = sunColorLUT[(hour + 1) % 24];
		
		//Blend colors together
		Color blendedColor = thisHrColor;
		float blendRatio = minute / 60.0;
		blendedColor.r -= (thisHrColor.r - nextHrColor.r) * blendRatio;
		blendedColor.g -= (thisHrColor.g - nextHrColor.g) * blendRatio;
		blendedColor.b -= (thisHrColor.b - nextHrColor.b) * blendRatio;
		
		#ifdef DEBUG
		std::cout << "Time is " << hour << ':' << minute << ". Calculated color is {" 
				  << static_cast<int>(blendedColor.r) << ", " << static_cast<int>(blendedColor.g) << ", " << static_cast<int>(blendedColor.b) << "}" << std::endl;
		#endif
		
		return blendedColor;
	}
};

class sunBrightness
{
public:

	static constexpr unsigned char sunBrightnessLUT[24] =
	{
		0,			//0 hrs
		0,			//1 hr
		0,			//2 hrs
		0,			//3 hrs
		0,			//4
		15,			//5
		25,			//6
		75,			//7
		100,		//8
		100,		//9
		100,		//10
		100,		//11
		100,		//12
		100,		//13
		100,		//14
		100,		//15
		100,		//16
		75,			//17
		50,			//18
		25,			//19
		15,			//20
		0,			//21
		0,			//22
		0			//23 hrs
	};
	
	static unsigned char interp(int hour, int minute)
	{
		//Bound inputs
		if (hour < 0 || hour > 23) { hour = 0; }
		if (minute < 0 || minute > 60) { minute = 0; }
		
		//Get base color vals to blend together
		unsigned char blendedBrightness = sunBrightnessLUT[hour];
		unsigned char nextHrBrightness = sunBrightnessLUT[(hour + 1) % 24];
		
		//Blend brightness together
		float blendRatio = minute / 60.0;
		blendedBrightness -= (blendedBrightness - nextHrBrightness) * blendRatio;
		
		#ifdef DEBUG
		std::cout << "Time is " << hour << ':' << minute << ". Calculated brightness is " 
				  << static_cast<unsigned short>(blendedBrightness) << '%' << std::endl;
		#endif
		
		return blendedBrightness;
	}
};
	
#endif