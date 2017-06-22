#ifndef __ASI_LOADER_H__
#define __ASI_LOADER_H__

#include "..\OpenVHook.h"

class ASILoader {
public:
	static void Initialize();

	static void LoadPlugins();

	static void UnloadPlugins();

	static bool Loaded();
};

#endif // __ASI_LOADER_H__