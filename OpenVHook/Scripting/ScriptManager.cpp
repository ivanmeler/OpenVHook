#include "ScriptManager.h"
#include "ScriptEngine.h"
#include "..\Utility\Log.h"
#include "..\Utility\General.h"
#include "..\DirectXHook\DirectXHook.h"
#include "..\Utility\PEImage.h"
#include "Types.h"
#include "..\Input\InputHook.h"

using namespace Utility;

#pragma comment(lib, "winmm.lib")
#define DLL_EXPORT __declspec( dllexport )

enum eGameVersion;

ScriptManagerThread g_ScriptManagerThread;

static HANDLE		mainFiber;
static Script *		currentScript;

std::mutex mutex;

void Script::Tick() {

	if ( mainFiber == nullptr ) {
		mainFiber = ConvertThreadToFiber( nullptr );
	}

	if ( timeGetTime() < wakedAt ) {
		return;
	}

	if ( scriptFiber ) {

		currentScript = this;
		SwitchToFiber( scriptFiber );
		currentScript = nullptr;
	}

	else if (ScriptEngine::GetGameState() == GameStatePlaying) {

		scriptFiber = CreateFiber(NULL, [](LPVOID handler) {
			const char* script_name = "";
			__try {
				script_name = reinterpret_cast<Script*>(handler)->name.c_str();
				LOG_PRINT("Launching script %s", script_name);
				reinterpret_cast<Script*>( handler )->Run();
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				// here we can not easily break & debug.
				// check dump at C:\Users\<UserName>\AppData\Local\CrashDumps\GTA5.exe.<PID>.dmp
				LOG_ERROR("Error in script->Run %s", script_name);
			}
		}, this );
	}
}

void Script::Run() {

	callbackFunction();
}

void Script::Yield( uint32_t time ) {

	wakedAt = timeGetTime() + time;
	SwitchToFiber( mainFiber );
}

void ScriptManagerThread::DoRun() {

	std::unique_lock<std::mutex> lock(mutex);

	scriptMap thisIterScripts( m_scripts );

	for ( auto & pair : thisIterScripts ) {
		for ( auto & script : pair.second ) {
			script->Tick();
		}	
	}
}

eThreadState ScriptManagerThread::Reset( uint32_t scriptHash, void * pArgs, uint32_t argCount ) {

	// Collect all scripts
	scriptMap tempScripts;

	for ( auto && pair : m_scripts ) {
		tempScripts[pair.first] = pair.second;
	}

	// Clear the scripts
	m_scripts.clear();

	// Start all scripts
	for ( auto && pair : tempScripts ) {
		for ( auto & script : pair.second ) {
			AddScript( pair.first, script->GetCallbackFunction() );
		}
	}

	return ScriptThread::Reset( scriptHash, pArgs, argCount );
}

size_t ScriptManagerThread::LoadScripts() {
	
	assert(m_scripts.empty());
	
	LOG_PRINT( "Loading *.asi plugins" );

	const std::string currentFolder = GetRunningExecutableFolder();

	const auto loadPlugins = [&]( const std::string& asiFolder ) {

		const std::string asiSearchQuery = asiFolder + "\\*.asi";

		WIN32_FIND_DATAA fileData;
		HANDLE fileHandle = FindFirstFileA( asiSearchQuery.c_str(), &fileData );
		if ( fileHandle != INVALID_HANDLE_VALUE ) {

			do {

				const std::string pluginPath = asiFolder + "\\" + fileData.cFileName;

				LOG_PRINT( "Loading \"%s\"", pluginPath.c_str() );

				PEImage pluginImage;
				if ( !pluginImage.Load( pluginPath ) ) {

					LOG_ERROR( "\tFailed to load image" );
					continue;
				}

				if (std::find(m_scriptNames.begin(), m_scriptNames.end(), fileData.cFileName) != m_scriptNames.end()) {
					LOG_DEBUG("\tSkip \"%s\"", fileData.cFileName);
					continue;
				}

				HMODULE module = LoadLibraryA( pluginPath.c_str() );
				if ( module ) {
					LOG_PRINT( "\tLoaded \"%s\" => 0x%p", fileData.cFileName, module );
				} else {
					DWORD errorMessageID = ::GetLastError();
					if ( errorMessageID == 0 )
						LOG_ERROR( "\tFailed to load" );

					LPSTR messageBuffer = nullptr;
					size_t size = FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
												  NULL, errorMessageID, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), ( LPSTR )&messageBuffer, 0, NULL );

					std::string message( messageBuffer, size );

					//Free the buffer.
					LocalFree( messageBuffer );
					LOG_ERROR( "\tFailed to load: %s", message.c_str() );
				}

			} while ( FindNextFileA( fileHandle, &fileData ) );

			FindClose( fileHandle );
		}
	};

	loadPlugins( currentFolder );

	loadPlugins( currentFolder + "\\asi" );

	LOG_PRINT( "Finished loading *.asi plugins" );

	assert(m_scripts.size() == m_scriptNames.size());

	return m_scripts.size();
}

void ScriptManagerThread::FreeScripts() {
	
	scriptMap tempScripts {m_scripts};

	for (auto && pair : tempScripts) {
		FreeLibrary( pair.first );
	}

	m_scripts.clear();
	m_scriptNames.clear();
}

size_t ScriptManagerThread::Count()
{
	return m_scriptNames.size();
}

void ScriptManagerThread::AddScript( HMODULE module, void( *fn )( ) ) {
	
	std::unique_lock<std::mutex> lock(mutex); 

	const std::string moduleName = GetModuleFullName( module );
	const std::string shortName = GetFilename(moduleName);

	if (m_scripts.find( module ) == m_scripts.end())	
		LOG_PRINT("Registering script '%s' (0x%p!0x%p)", shortName.c_str(), module, fn);
	else 
		LOG_PRINT("Registering additional script thread '%s' (0x%p!0x%p)", shortName.c_str(), module, fn);

	if ( find(m_scriptNames.begin(), m_scriptNames.end(), 
		shortName ) == m_scriptNames.end() )
	{
		m_scriptNames.push_back( shortName );
	}

	m_scripts[module].push_back(std::make_shared<Script>( fn, shortName ));
}

void ScriptManagerThread::RemoveScript( void( *fn )( ) ) {

	for (auto & pair : m_scripts) {
		for (auto script : pair.second) {
			if (script->GetCallbackFunction() == fn) {

				RemoveScript(pair.first);

				break;
			}
		}
	}
}

void ScriptManagerThread::RemoveScript( HMODULE module ) {

	std::unique_lock<std::mutex> lock(mutex);

	auto pair = m_scripts.find( module );
	if ( pair == m_scripts.end() ) {

		LOG_ERROR( "Could not find script for module 0x%p", module );
		return;
	}

	LOG_PRINT( "Unregistered script '%s'", GetModuleNameWithoutExtension( module ).c_str() );
	m_scripts.erase( pair );
}

void DLL_EXPORT scriptWait( unsigned long waitTime ) {

	currentScript->Yield( waitTime );
}

void DLL_EXPORT scriptRegister( HMODULE module, void( *function )( ) ) {

	g_ScriptManagerThread.AddScript( module, function );
}

void DLL_EXPORT scriptRegisterAdditionalThread(HMODULE module, void(*LP_SCRIPT_MAIN)()) {

	g_ScriptManagerThread.AddScript(module, LP_SCRIPT_MAIN);
}

void DLL_EXPORT scriptUnregister( void( *function )( ) ) {

	g_ScriptManagerThread.RemoveScript( function );
}

void DLL_EXPORT scriptUnregister( HMODULE module ) {
	
	g_ScriptManagerThread.RemoveScript( module );
}

eGameVersion DLL_EXPORT getGameVersion() {

	return (eGameVersion)gameVersion;
}

static ScriptManagerContext g_context;
static uint64_t g_hash;

void DLL_EXPORT nativeInit( uint64_t hash ) {

	g_context.Reset();
	g_hash = hash;
}

void DLL_EXPORT nativePush64( uint64_t value ) {

	g_context.Push( value );
}

DLL_EXPORT uint64_t * nativeCall() {

	auto fn = ScriptEngine::GetNativeHandler( g_hash );

	if ( fn != 0 ) {

		__try {

			fn( &g_context );
			scrNativeCallContext::SetVectorResults(&g_context);
		} __except ( EXCEPTION_EXECUTE_HANDLER ) {
#if 1
			// some entities in pools failed in these functions
			// cant find another native function to stop spamming
			// guess no one need them either
			if (g_hash != 0x9F47B058362C84B5		// Entity::GET_ENTITY_MODEL
				&& g_hash != 0x7239B21A38F536BA)	// ENTITY::DOES_ENTITY_EXIST
#endif
			LOG_ERROR( "Error in nativeCall: oldHash=>handler(a1, ...): %p=>%p(%x)", g_hash, fn, g_context.GetArgument<uint64_t>(0));
			
			// zero result on exception so we can possibly handle it in script
			g_context.ClearResult();
		}
	}

	return reinterpret_cast<uint64_t*>( g_context.GetResultPointer() );
}

typedef void( *TKeyboardFn )( DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow );

static std::set<TKeyboardFn> g_keyboardFunctions;

void DLL_EXPORT keyboardHandlerRegister( TKeyboardFn function ) {

	g_keyboardFunctions.insert( function );
}

void DLL_EXPORT keyboardHandlerUnregister( TKeyboardFn function ) {

	g_keyboardFunctions.erase( function );
}

void ScriptManager::HandleKeyEvent(DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow) {

	auto functions = g_keyboardFunctions;

	for (auto & function : functions) {
		function(key, repeats, scanCode, isExtended, isWithAlt, wasDownBefore, isUpNow);
	}
}

DLL_EXPORT uint64_t* getGlobalPtr(int index)
{
	return (uint64_t*)globalTable.AddressOf(index);
}

BYTE DLL_EXPORT *getScriptHandleBaseAddress(int handle) {

	if (handle != -1)
	{
		int index = handle >> 8;

		auto entityPool = pools.GetEntityPool();

		if (index < entityPool->m_count && entityPool->m_bitMap[index] == (handle & 0xFF))
		{
			auto result = entityPool->m_pData[index];

			return (BYTE*)result.m_pEntity;
		}
	}

	return NULL;
}

int DLL_EXPORT worldGetAllVehicles(int* array, int arraySize) {

	int index = 0;

	auto vehiclePool = pools.GetVehiclePool();

	for (auto i = 0; i < vehiclePool->m_count; i++)
	{
		if (i >= arraySize) break;

		if (vehiclePool->m_bitMap[i] >= 0)
		{
			array[index++] = (i << 8) + vehiclePool->m_bitMap[i];
		}
	}

	return index;
}

int DLL_EXPORT worldGetAllPeds(int* array, int arraySize) {

	int index = 0;

	auto pedPool = pools.GetPedPool();

	for (auto i = 0; i < pedPool->m_count; i++)
	{
		if (i >= arraySize) break;

		if (pedPool->m_bitMap[i] >= 0)
		{
			array[index++] = pedPool->getHandle(i);
		}
	}

	return index;
}

int DLL_EXPORT worldGetAllObjects(int* array, int arraySize) {

	int index = 0;

	auto objectPool = pools.GetObjectsPool();

	for (auto i = 0; i < objectPool->m_count; i++)
	{
		if (i >= arraySize) break;

		if (objectPool->m_bitMap[i] >= 0)
		{
			array[index++] = objectPool->getHandle(i);
		}
	}

	return index;
}

int DLL_EXPORT worldGetAllPickups(int* array, int arraySize) {

	int index = 0;

	auto pickupPool = pools.GetPickupsPool();

	for (auto i = 0; i < pickupPool->m_count; i++)
	{
		if (i >= arraySize) break;

		if (pickupPool->m_bitMap[i] >= 0)
		{
			array[index++] = pickupPool->getHandle(i);
		}
	}

	return index;
}

DLL_EXPORT int createTexture(const char* fileName)
{	
	return g_D3DHook.CreateTexture(fileName);
}

DLL_EXPORT void drawTexture(int id, int index, int level, int time,
	float sizeX, float sizeY, float centerX, float centerY,
	float posX, float posY, float rotation, float screenHeightScaleFactor,
	float r, float g, float b, float a)
{
	g_D3DHook.DrawTexture(id, index, level, time,
		sizeX, sizeY, centerX, centerY,
		posX, posY, rotation, screenHeightScaleFactor,
		r, g, b, a);
}

/*Input*/
DLL_EXPORT void WndProcHandlerRegister(TWndProcFn function) 
{
	g_WndProcCb.insert(function);
}

DLL_EXPORT void WndProcHandlerUnregister(TWndProcFn function) 
{
	g_WndProcCb.erase(function);
}

/* D3d SwapChain */
DLL_EXPORT void presentCallbackRegister(PresentCallback cb) 
{
	g_D3DHook.AddCallback(cb);
}

DLL_EXPORT void presentCallbackUnregister(PresentCallback cb) 
{
	g_D3DHook.RemoveCallback(cb);
}
