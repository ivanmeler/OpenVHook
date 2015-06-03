#include "General.h"

static HMODULE ourModule;

const std::string Utility::GetRunningExecutableFolder() {

	char fileName[MAX_PATH];
	GetModuleFileNameA( NULL, fileName, MAX_PATH );

	std::string currentPath = fileName;
	return currentPath.substr( 0, currentPath.find_last_of( "\\" ) );
}

const std::string Utility::GetOurModuleFolder() {

	char fileName[MAX_PATH];
	GetModuleFileNameA( ourModule, fileName, MAX_PATH );

	std::string currentPath = fileName;
	return currentPath.substr( 0, currentPath.find_last_of( "\\" ) );
}

void Utility::SetOurModuleHanlde( const HMODULE module ) {

	ourModule = module;
}

const HMODULE Utility::GetOurModuleHandle() {

	return ourModule;
}

