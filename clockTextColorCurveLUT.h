/******************************************************************************
/ Pi 4 Sunrise Clock App Clock Text Color Curve LUT - lopezk38 2025
/
/*****************************************************************************/

#ifndef SUNCLOCK_APP_CLOCK_LUT
#define SUNCLOCK_APP_CLOCK_LUT

#ifdef DEBUG
#include <iostream>
#endif

#include "raylib.h"


class ClockTextColor
{
public:

	static constexpr Color TextColorLUT[24] =
	{
		{100, 0, 0, 255},	//0 hrs
		{100, 0, 0, 255},	//1 hr
		{100, 0, 0, 255},	//2 hrs
		{100, 0, 0, 255},	//3 hrs
		{100, 0, 0, 255},	//4
		{100, 0, 0, 255},	//5
		{50, 0, 0, 255},	//6
		{25, 0, 0, 255},	//7
		{0, 0, 0, 255},		//8
		{0, 0, 0, 255},		//9
		{0, 0, 0, 255},		//10
		{0, 0, 0, 255},		//11
		{0, 0, 0, 255},		//12
		{0, 0, 0, 255},		//13
		{0, 0, 0, 255},		//14
		{0, 7, 7, 255},		//15
		{25, 30, 30, 255},	//16
		{50, 50, 50, 255},	//17
		{125, 75, 75, 255},	//18
		{150, 100, 100, 255},	//19
		{125, 75, 75, 255},	//20
		{100, 30, 30, 255},	//21
		{100, 7, 7, 255},	//22
		{100, 0, 0, 255}	//23 hrs
	};
	
	static Color interp(int hour, int minute)
	{
		//Bound inputs
		if (hour < 0 || hour > 23) { hour = 0; }
		if (minute < 0 || minute > 60) { minute = 0; }
		
		//Get base color vals to blend together
		const Color& thisHrColor = TextColorLUT[hour];
		const Color& nextHrColor = TextColorLUT[(hour + 1) % 24];
		
		//Blend colors together
		Color blendedColor = thisHrColor;
		float blendRatio = minute / 60.0;
		blendedColor.r -= (thisHrColor.r - nextHrColor.r) * blendRatio;
		blendedColor.g -= (thisHrColor.g - nextHrColor.g) * blendRatio;
		blendedColor.b -= (thisHrColor.b - nextHrColor.b) * blendRatio;
		
		#ifdef DEBUG
		std::cout << "Time is " << hour << ':' << minute << ". Calculated text color is {" 
				  << static_cast<int>(blendedColor.r) << ", " << static_cast<int>(blendedColor.g) << ", " << static_cast<int>(blendedColor.b) << "}" << std::endl;
		#endif
		
		return blendedColor;
	}
};

#endif