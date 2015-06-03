#ifndef __LOG_H__
#define __LOG_H__

#include "..\OpenVHook.h"

namespace Utility {

	class Log {
	public:

		void				Print( const char * fmt, ... );
		void				Debug( const char * fmt, ... );
		void				Warning( const char * fmt, ... );
		void				Error( const char * fmt, ... );
				
	private:

		void				LogToFile( const char * buff );

		const std::string	GetTimeFormatted() const;

		bool				firstEntry = true;
	};

	Log *					GetLog();
}

#endif // __LOG_H__