#include "PEImage.h"
#include <fstream>

namespace Utility {

	PEImage::PEImage() {

	}

	PEImage::~PEImage() {

		if ( fileBase != nullptr ) {
			VirtualFree( fileBase, 0, MEM_RELEASE );
		}
	}

	bool PEImage::Load( const std::string & path ) {

		filePath = path;

		HANDLE fileHandle = CreateFileA( path.c_str(), FILE_GENERIC_READ, 0x7, NULL, OPEN_EXISTING, 0, NULL );
		if ( fileHandle == INVALID_HANDLE_VALUE ) {
			return false;
		}

		DWORD bytes = 0;
		fileSize = GetFileSize( fileHandle, NULL );
		fileBase = VirtualAlloc( NULL, fileSize, MEM_COMMIT, PAGE_READWRITE );
		ReadFile( fileHandle, fileBase, fileSize, &bytes, NULL );
		CloseHandle( fileHandle );

		bool parseSuccess = ParsePE();
		if ( !parseSuccess ) {
			return false;
		}

		return true;
	}

	bool PEImage::ParsePE() {

		// Get DOS header
		const IMAGE_DOS_HEADER * dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>( fileBase );

		// Not a valid PE
		if ( dosHeader->e_magic != IMAGE_DOS_SIGNATURE ) {
			return false;
		}

		// Get NT header
		ntHeader = reinterpret_cast<const IMAGE_NT_HEADERS64*>( reinterpret_cast<const uint8_t*>(dosHeader)+dosHeader->e_lfanew );

		return true;
	}

	uint64_t PEImage::GetDirectoryAddress( int index ) {

		const IMAGE_DATA_DIRECTORY * dataDirectory = ntHeader->OptionalHeader.DataDirectory;

		return RVAToVA( dataDirectory[index].VirtualAddress );
	}

	uint64_t PEImage::RVAToVA( uint32_t rva ) const {

		const IMAGE_SECTION_HEADER * sectionHeader = reinterpret_cast<const IMAGE_SECTION_HEADER*>( ntHeader + 1 );
		for ( int i = 0; i < ntHeader->FileHeader.NumberOfSections; ++i, ++sectionHeader ) {

			if ( rva >= sectionHeader->VirtualAddress && rva <= sectionHeader->VirtualAddress + sectionHeader->Misc.VirtualSize ) {

				return reinterpret_cast<uint64_t>(fileBase)+( rva - sectionHeader->VirtualAddress + sectionHeader->PointerToRawData );
			}
		}

		return 0;
	}

	bool PEImage::IsOpenVHookCompatible() {

		auto * importTable = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>( GetDirectoryAddress( IMAGE_DIRECTORY_ENTRY_IMPORT ) );
		for ( ; importTable->Name; ++importTable ) {

			char * dllName = reinterpret_cast<char*>( RVAToVA( importTable->Name ) );

			if ( strcmp( dllName, "ScriptHookV.dll" ) == 0 ) {
				return false;
			}
		}

		return true;
	}

	bool PEImage::PatchCompatibility() {

		// Find ScriptHooKV import descriptor
		auto * importTable = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>( GetDirectoryAddress( IMAGE_DIRECTORY_ENTRY_IMPORT ) );
		for ( ; importTable->Name; ++importTable ) {

			char * dllName = reinterpret_cast<char*>( RVAToVA( importTable->Name ) );

			if ( strcmp( dllName, "ScriptHookV.dll" ) == 0 ) {

				// Found it, patch that shit
				ZeroMemory( dllName, strlen( dllName ) );
				strcpy( dllName, "OpenVHook.dll" );

				// Overwrite original file with changes
				std::ofstream file( filePath, std::ios::binary | std::ios::out );
				file.write( reinterpret_cast<char*>( fileBase ), fileSize );
				file.close();

				return true;
			}
		}

		return false;
	}

}
