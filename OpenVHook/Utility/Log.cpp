#include "Log.h"
#include "Console.h"
#include "General.h"

#include <time.h>

namespace Utility {

	static Log g_Log;

	void Log::Print( const char * fmt, ... ) {

		char buf[2048] = { 0 };
		va_list va_alist;

		va_start( va_alist, fmt );
		vsprintf_s( buf, fmt, va_alist );
		va_end( va_alist );

		char buff2[2048] = { 0 };
		sprintf_s( buff2, "%s %s\n", GetTimeFormatted().c_str(), buf );

		if ( Utility::GetConsole()->IsAllocated() ) {

			Utility::GetConsole()->SetTextColor( ConsoleForeground::WHITE );
			printf( buff2 );
		}

		LogToFile( buff2 );
	}

	void Log::Debug( const char * fmt, ... ) {
#ifdef _DEBUG 
		char buf[2048] = { 0 };
		va_list va_alist;

		va_start( va_alist, fmt );
		vsprintf_s( buf, fmt, va_alist );
		va_end( va_alist );

		char buff2[2048] = { 0 };

		if ( Utility::GetConsole()->IsAllocated() ) {

			sprintf_s( buff2, "%s %s\n", GetTimeFormatted().c_str(), buf );
			Utility::GetConsole()->SetTextColor( ConsoleForeground::GRAY );
			printf( buff2 );
		}

		sprintf_s( buff2, "%s Debug: %s\n", GetTimeFormatted().c_str(), buf );
		LogToFile( buff2 );
#endif
	}

	void Log::Warning( const char * fmt, ... ) {

		char buf[2048] = { 0 };
		va_list va_alist;

		va_start( va_alist, fmt );
		vsprintf_s( buf, fmt, va_alist );
		va_end( va_alist );

		char buff2[2048] = { 0 };

		if ( Utility::GetConsole()->IsAllocated() ) {

			sprintf_s( buff2, "%s %s\n", GetTimeFormatted().c_str(), buf );
			Utility::GetConsole()->SetTextColor( ConsoleForeground::YELLOW );
			printf( buff2 );
		}

		sprintf_s( buff2, "%s Warning: %s\n", GetTimeFormatted().c_str(), buf );
		LogToFile( buff2 );
	}

	void Log::Error( const char * fmt, ... ) {

		char buf[2048] = { 0 };
		va_list va_alist;

		va_start( va_alist, fmt );
		vsprintf_s( buf, fmt, va_alist );
		va_end( va_alist );

		char buff2[2048] = { 0 };

		if ( Utility::GetConsole()->IsAllocated() ) {

			sprintf_s( buff2, "%s %s\n", GetTimeFormatted().c_str(), buf );
			Utility::GetConsole()->SetTextColor( ConsoleForeground::RED );
			printf( buff2 );
		}

		sprintf_s( buff2, "%s Error: %s\n", GetTimeFormatted().c_str(), buf );
		LogToFile( buff2 );
	}

	const std::string Log::GetTimeFormatted() const {

		struct tm timeStruct;
		time_t currTime = time( NULL );
		localtime_s( &timeStruct, &currTime );

		char buff[48];
		sprintf_s( buff, "[%02d:%02d:%02d]", timeStruct.tm_hour, timeStruct.tm_min, timeStruct.tm_sec );
		return buff;
	}

	void Log::LogToFile( const char * buff ) {

		const std::string fileName = GetOurModuleFolder() + "\\OpenVHook.log";

		FILE * logFile = nullptr;

		fopen_s( &logFile, fileName.c_str(), "a+" );
		if ( logFile != nullptr ) {

			if ( firstEntry ) {

				fwrite( "\n", 1, 1, logFile );
				firstEntry = false;
			}

			fwrite( buff, strlen( buff ), 1, logFile );
			fclose( logFile );
		}
	}

	Log * GetLog() {

		return &g_Log;
	}
}