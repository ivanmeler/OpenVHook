#ifndef __LOG_H__
#define __LOG_H__

#include "..\OpenVHook.h"

#define LOG_PRINT( fmt, ...)	GetLog()->Write( eLogType::LogTypePrint,	fmt, ##__VA_ARGS__ )
#define LOG_DEBUG( fmt, ...)	GetLog()->Write( eLogType::LogTypeDebug,	fmt, ##__VA_ARGS__ )
#define LOG_WARNING( fmt, ...)	GetLog()->Write( eLogType::LogTypeWarning,	fmt, ##__VA_ARGS__ )
#define LOG_ERROR( fmt, ...)	GetLog()->Write( eLogType::LogTypeError,	fmt, ##__VA_ARGS__ )

namespace Utility {

	enum eLogType {
		LogTypePrint,
		LogTypeDebug,
		LogTypeWarning,
		LogTypeError,
	};

	typedef std::map<int32_t, int32_t> intIntMap;
	typedef std::map<int32_t, std::string> intStringMap;

	class Log {
	public:

		Log();
		~Log();

		void				Write( eLogType logType, const char * fmt, ... );
				
	private:

		void				LogToFile( const char * buff );

		const std::string	GetTimeFormatted() const;


		intIntMap			logTypeToColorMap;
		intStringMap		logTypeToFormatMap;
		bool				firstEntry = true;
	};

	Log *					GetLog();
}

#endif // __LOG_H__