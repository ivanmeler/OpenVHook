#ifndef __GENERAL_H__
#define __GENERAL_H__

#include "..\OpenVHook.h"

namespace Utility {

	const std::string	GetRunningExecutableFolder();

	const std::string	GetOurModuleFolder();

	void				SetOurModuleHanlde( const HMODULE module );
	const HMODULE		GetOurModuleHandle();
}

#endif // __GENERAL_H__