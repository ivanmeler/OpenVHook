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

const std::string Utility::GetModuleName( const HMODULE module ) {

	char fileName[MAX_PATH];
	GetModuleFileNameA( module, fileName, MAX_PATH );

	std::string fullPath = fileName;

	size_t lastIndex = fullPath.find_last_of( "\\" ) + 1;
	return fullPath.substr( lastIndex, fullPath.length() - lastIndex );
}

const std::string Utility::GetModuleNameWithoutExtension( const HMODULE module ) {

	const std::string fileNameWithExtension = GetModuleName( module );

	size_t lastIndex = fileNameWithExtension.find_last_of( "." );
	if ( lastIndex == -1 ) {
		return fileNameWithExtension;
	}

	return fileNameWithExtension.substr( 0, lastIndex );
}

void Utility::SetOurModuleHanlde( const HMODULE module ) {

	ourModule = module;
}

const HMODULE Utility::GetOurModuleHandle() {

	return ourModule;
}
