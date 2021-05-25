#ifndef __GENERAL_H__
#define __GENERAL_H__

#include "..\OpenVHook.h"

namespace Utility {

	std::string	GetFilename(std::string filename);
	std::string	GetFilenameWithoutExtension(std::string filename);

	std::string	GetRunningExecutableFolder();
	std::string	GetOurModuleFolder();
	std::string	GetModuleName( const HMODULE module );
	std::string	GetModuleFullName(const HMODULE module);
	std::string	GetModuleNameWithoutExtension( const HMODULE module );

	void		SetOurModuleHandle( const HMODULE module );
	HMODULE		GetOurModuleHandle();

	std::wstring str_to_wstr(const std::string& string);
	std::string  wstr_to_str(const std::wstring& wstring);

	/* General Misc */
	template <typename T>
	inline void SafeRelease(T *&p)
	{
		if (nullptr != p)
		{
			p->Release();
			p = nullptr;
		}
	}
}

#endif // __GENERAL_H__