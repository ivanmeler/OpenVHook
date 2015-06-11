#ifndef __GENERAL_H__
#define __GENERAL_H__

#include "..\OpenVHook.h"

namespace Utility {

	const std::string	GetRunningExecutableFolder();
	const std::string	GetOurModuleFolder();
	const std::string	GetModuleName( const HMODULE module );
	const std::string	GetModuleNameWithoutExtension( const HMODULE module );

	void				SetOurModuleHanlde( const HMODULE module );
	const HMODULE		GetOurModuleHandle();
}

#endif // __GENERAL_H__