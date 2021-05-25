#include "ScriptManager.h"
#include "ScriptEngine.h"
#include "..\Utility\Log.h"
#include "..\Utility\General.h"
#include "..\ASI Loader\ASILoader.h"
#include "..\DirectXHook\DirectXHook.h"
#include "Types.h"

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
			__try {
				LOG_PRINT("Launching script %s", reinterpret_cast<Script*>(handler)->name.c_str());
				reinterpret_cast<Script*>( handler )->Run();
			} __except (EXCEPTION_EXECUTE_HANDLER) {

				LOG_ERROR("Error in script->Run");
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

bool ScriptManagerThread::LoadScripts() {

	if (!m_scripts.empty()) return false;

	// load known scripts
	for (auto && scriptName : m_scriptNames)
	{
		LOG_PRINT("Loading \"%s\"", scriptName.c_str());
		HMODULE module = LoadLibraryA(scriptName.c_str());
		if (module) {
			LOG_PRINT("\tLoaded \"%s\" => 0x%p", scriptName.c_str(), module);
		} else {
			LOG_DEBUG("\tSkip \"%s\"", scriptName.c_str());
		}
	}

	// ASILoader::Initialize only load new DLLs if called multiple times.
	ASILoader::Initialize();

	return true;
}

void ScriptManagerThread::FreeScripts() {

	scriptMap tempScripts;

	for (auto && pair : m_scripts) {
		tempScripts[pair.first] = pair.second;
	}

	for (auto && pair : tempScripts) {
		FreeLibrary( pair.first );
	}

	m_scripts.clear();
}

void ScriptManagerThread::AddScript( HMODULE module, void( *fn )( ) ) {

	const std::string moduleName = GetModuleFullName( module );
	const std::string shortName = GetFilename(moduleName);

	if (m_scripts.find( module ) == m_scripts.end())	
		LOG_PRINT("Registering script '%s' (0x%p)", shortName.c_str(), fn);
	else 
		LOG_PRINT("Registering additional script thread '%s' (0x%p)", shortName.c_str(), fn);

	if ( find(m_scriptNames.begin(), m_scriptNames.end(), 
		moduleName ) == m_scriptNames.end() )
	{
		m_scriptNames.push_back( moduleName );
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

void DLL_EXPORT scriptRegisterAdditionalThread(HMODULE module, void(*function)()) {

	g_ScriptManagerThread.AddScript(module, function);
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

			LOG_ERROR( "Error in nativeCall" );
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

