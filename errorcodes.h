/******************************************************************************
/ Pi 4 Sunrise Clock App Error Code Enums - lopezk38 2025
/ Version 0.1
/
/*****************************************************************************/

#ifndef SUNCLOCK_APP_ERRCODES
#define SUNCLOCK_APP_ERRCODES

namespace RESDATA_ERR
{
	enum CODE
	{
		SUCCESS = 0,
		RD_FAIL = 1
	};
}

namespace FBDESC_ERR
{
	enum CODE
	{
		SUCCESS = 0,
		OPEN_FAIL = 1,
		RD_ERROR = 2
	};
}

#endif