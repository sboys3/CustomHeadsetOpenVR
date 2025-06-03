//============ Copyright (c) Valve Corporation, All rights reserved. ============
#include "DriverLog.h"

#include <stdarg.h>
#include <stdio.h>

#if !defined( WIN32 )
#define vsnprintf_s vsnprintf
#endif

#ifdef _WIN32
#include "windows.h"
#endif

static void DriverLogVarArgs( const char *pMsgFormat, va_list args )
{
	char buf[ 1024 ];
	vsnprintf_s( buf, sizeof( buf ), pMsgFormat, args );
	
	if(vr::VRDriverContext() != nullptr){
		vr::VRDriverLog()->Log( buf );
	}else{
		#ifdef _WIN32
		// If there is not driver context output for a debugger
		OutputDebugStringA( buf );
		OutputDebugStringA( "\n" );
		#endif
	}
}


void DriverLog( const char *pMsgFormat, ... )
{
	va_list args;
	va_start( args, pMsgFormat );

	DriverLogVarArgs( pMsgFormat, args );

	va_end( args );
}


void DebugDriverLog( const char *pMsgFormat, ... )
{
#ifdef _DEBUG
	va_list args;
	va_start( args, pMsgFormat );

	DriverLogVarArgs( pMsgFormat, args );

	va_end( args );
#endif
}
