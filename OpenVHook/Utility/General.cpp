#include "General.h"

static HMODULE ourModule;

std::string Utility::GetFilename( std::string filename )
{
	size_t lastIndex = filename.find_last_of("\\") + 1;
	return filename.substr(lastIndex, filename.length() - lastIndex);
}

std::string Utility::GetFilenameWithoutExtension( std::string filename )
{
	size_t lastIndex = filename.find_last_of(".");
	if (lastIndex == -1) {
		return filename;
	}

	return filename.substr(0, lastIndex);
}

std::string Utility::GetRunningExecutableFolder() {

	char fileName[MAX_PATH];
	GetModuleFileNameA( NULL, fileName, MAX_PATH );

	std::string currentPath = fileName;
	return currentPath.substr( 0, currentPath.find_last_of( "\\" ) );
}

std::string Utility::GetOurModuleFolder() {

	char fileName[MAX_PATH];
	GetModuleFileNameA( ourModule, fileName, MAX_PATH );

	std::string currentPath = fileName;
	return currentPath.substr( 0, currentPath.find_last_of( "\\" ) );
}

std::string Utility::GetModuleFullName(const HMODULE module) {

	char fileName[MAX_PATH];
	GetModuleFileNameA(module, fileName, MAX_PATH);
	return fileName;
}

std::string Utility::GetModuleName(const HMODULE module) {

	return GetFilename(GetModuleFullName(module));
}

std::string Utility::GetModuleNameWithoutExtension( const HMODULE module ) {

	const std::string fileNameWithExtension = GetModuleName(module);
	return GetFilenameWithoutExtension(fileNameWithExtension);
}

void Utility::SetOurModuleHandle( const HMODULE module ) {

	ourModule = module;
}

HMODULE Utility::GetOurModuleHandle() {

	return ourModule;
}
