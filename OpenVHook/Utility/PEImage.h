#ifndef __PE_IMAGE__
#define __PE_IMAGE__

#include "..\OpenVHook.h"

namespace Utility {

	class PEImage {
	public:

		PEImage();
		~PEImage();

		bool			Load( const std::string & path );

		bool			IsOpenVHookCompatible();

		bool			PatchCompatibility();

	private:

		uint64_t		GetDirectoryAddress( int index );
		uint64_t		RVAToVA( uint32_t rva ) const;

		bool			ParsePE();

	private:

		void *			fileBase = nullptr;
		uint32_t		fileSize = 0;
		std::string		filePath;
		const IMAGE_NT_HEADERS64 * ntHeader = nullptr;
	};

}

#endif // __PE_IMAGE__