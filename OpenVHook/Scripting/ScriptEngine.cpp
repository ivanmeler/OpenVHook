#include "ScriptEngine.h"
#include "NativeHashMap.h"
#include "NativeInvoker.h"
#include "..\Utility\Pattern.h"
#include "..\Utility\Log.h"

using namespace Utility;

GlobalTable globalTable;

static pgPtrCollection<ScriptThread> * scrThreadCollection;
static uint32_t activeThreadTlsOffset;

static uint32_t * scrThreadId;
static uint32_t * scrThreadCount;

static scriptHandlerMgr * g_scriptHandlerMgr;

//uint64_t * g_globalPtr;

struct NativeRegistration {

	NativeRegistration * nextRegistration;
	ScriptEngine::NativeHandler handlers[7];
	uint32_t numEntries;
	uint64_t hashes[7];
};

static NativeRegistration ** registrationTable;

static std::unordered_set<ScriptThread*> g_ownedThreads;

static std::unordered_map<uint64_t, uint64_t> foundHashCache;

static eGameState * gameState;

bool ScriptEngine::Initialize() {

	LOG_PRINT( "Initializing ScriptEngine..." );

	auto scrThreadCollectionPattern =	pattern("48 8B C8 EB 03 48 8B CB 48 8B 05");
	auto activeThreadTlsOffsetPattern = pattern("48 8B 04 D0 4A 8B 14 00 48 8B 01 F3 44 0F 2C 42 20");
	auto scrThreadIdPattern =			pattern("89 15 ? ? ? ? 48 8B 0C D8");
	auto scrThreadCountPattern =		pattern("FF 0D ? ? ? ? 48 8B F9");
	auto registrationTablePattern =		pattern("76 61 49 8B 7A 40 48 8D 0D");
	auto g_scriptHandlerMgrPattern =	pattern("74 17 48 8B C8 E8 ? ? ? ? 48 8D 0D");
	auto gameStatePattern =				pattern("83 3D ? ? ? ? ? 8A D9 74 0A");
	auto getScriptIdBlock =				pattern("80 78 32 00 75 34 B1 01 E8");
	auto g_globalPtrPattern =			pattern("4C 8D 05 ? ? ? ? 4D 8B 08 4D 85 C9 74 11");

	executable_meta executable;
	executable.EnsureInit();

	// Get scrThreadCollection
	char * location = scrThreadCollectionPattern.count(1).get(0).get<char>(11);
	if (location == nullptr) {

		LOG_ERROR("Unable to find scrThreadCollection");
		return false;
	}
	scrThreadCollection = reinterpret_cast<decltype(scrThreadCollection)>(location + *(int32_t*)location + 4);
	LOG_DEBUG("scrThreadCollection\t 0x%p (0x%.8X)", scrThreadCollection, reinterpret_cast<uintptr_t>(scrThreadCollection) - executable.begin());


	// Get activet tls offset
	uint32_t * tlsLoc = activeThreadTlsOffsetPattern.count(1).get(0).get<uint32_t>(-4);
	if (tlsLoc == nullptr) {

		LOG_ERROR("Unable to find activeThreadTlsOffset");
		return false;
	}
	activeThreadTlsOffset = *tlsLoc;
	LOG_DEBUG("activeThreadTlsOffset 0x%.8X", activeThreadTlsOffset);

	// Get thread id
	location = scrThreadIdPattern.count(1).get(0).get<char>(2);
	if (location == nullptr) {

		LOG_ERROR("Unable to find scrThreadId");
		return false;
	}
	scrThreadId = reinterpret_cast<decltype(scrThreadId)>(location + *(int32_t*)location + 4);
	LOG_DEBUG("scrThreadId\t\t 0x%p (0x%.8X)", scrThreadId, reinterpret_cast<uintptr_t>(scrThreadId) - executable.begin());

	// Get thread count
	location = scrThreadCountPattern.get(0).get<char>(2);
	if (location == nullptr) {

		LOG_ERROR("Unable to find scrThreadCount");
		return false;
	}
	scrThreadCount = reinterpret_cast<decltype(scrThreadCount)>(location + *(int32_t*)location + 4);
	LOG_DEBUG("scrThreadCount\t 0x%p (0x%.8X)", scrThreadCount, reinterpret_cast<uintptr_t>(scrThreadCount) - executable.begin());

	// Get registration table
	location = registrationTablePattern.count(1).get(0).get<char>(9);
	if (location == nullptr ) {

		LOG_ERROR("Unable to find registrationTable");
		return false;
	}
	registrationTable = reinterpret_cast<decltype(registrationTable)>(location + *(int32_t*)location + 4);
	LOG_DEBUG("registrationTable\t 0x%p (0x%.8X)", registrationTable, reinterpret_cast<uintptr_t>(registrationTable) - executable.begin());

	
	// Get scriptHandlerMgr
	location = g_scriptHandlerMgrPattern.count(1).get(0).get<char>(13);
	if (location == nullptr) {

		LOG_ERROR("Unable to find g_scriptHandlerMgr");
		return false;
	}
	g_scriptHandlerMgr = reinterpret_cast<decltype(g_scriptHandlerMgr)>(location + *(int32_t*)location + 4);
	LOG_DEBUG("g_scriptHandlerMgr\t 0x%p (0x%.8X)", g_scriptHandlerMgr, reinterpret_cast<uintptr_t>(g_scriptHandlerMgr) - executable.begin());

	//script_location
	void * script_location = getScriptIdBlock.count(1).get(0).get<void>(4);
	if (script_location == nullptr) {

		LOG_ERROR("Unable to find getScriptIdBlock");
		return false;
	}

	// ERR_SYS_PURE
	static uint8_t block[2] = { 0xEB };
	unsigned long OldProtection;
	VirtualProtect(script_location, 2, PAGE_EXECUTE_READWRITE, &OldProtection);
	memcpy(&block, script_location, 2);
	VirtualProtect(script_location, 2, OldProtection, NULL);

	// Get game state
	location = gameStatePattern.count(1).get(0).get<char>(2);
	if (location == nullptr) {

		LOG_ERROR("Unable to find gameState");
		return false;
	}
	gameState = reinterpret_cast<decltype(gameState)>(location + *(int32_t*)location + 5);
	LOG_DEBUG("gameState\t\t 0x%p (0x%.8X)", gameState, reinterpret_cast<uintptr_t>(gameState) - executable.begin());


	// Get global ptr
	location = g_globalPtrPattern.count(1).get(0).get<char>(0);
	if (location == nullptr) {

		LOG_ERROR("Unable to find g_globalPtr");
		return false;
	}
	//g_globalPtr = reinterpret_cast<decltype(g_globalPtr)>(location + *(int32_t*)(location + 3) + 7);
	globalTable.GlobalBasePtr = (__int64**)(location + *(int*)(location + 3) + 7);
	LOG_DEBUG("g_globalPtr\t 0x%p (0x%.8X)", globalTable.GlobalBasePtr, reinterpret_cast<uintptr_t>(globalTable.GlobalBasePtr) - executable.begin());

	// Check if game is ready
	LOG_PRINT("Checking if game is ready...");
	while (!GetGameState() == GameStatePlaying ) {		
		Sleep(100);
	}
	LOG_PRINT("Game ready");

	LOG_DEBUG("GtaThread collection size %d", scrThreadCollection->count());

	return true;
}

scriptHandlerMgr * ScriptEngine::GetScriptHandleMgr() {

	return g_scriptHandlerMgr;
}

pgPtrCollection<ScriptThread>* ScriptEngine::GetThreadCollection() {

	return scrThreadCollection;
}

scrThread * ScriptEngine::GetActiveThread() {

	char * moduleTls = *(char**)__readgsqword( 88 );
	return *reinterpret_cast<scrThread**>( moduleTls + activeThreadTlsOffset );
}

void ScriptEngine::SetActiveThread( scrThread * thread ) {

	char * moduleTls = *(char**)__readgsqword( 88 );
	*reinterpret_cast<scrThread**>( moduleTls + activeThreadTlsOffset ) = thread;
}

void ScriptEngine::CreateThread( ScriptThread * thread ) {

	// get a free thread slot
	auto collection = GetThreadCollection();
	int slot = 0;

	for ( auto & thread : *collection ) {

		auto context = thread->GetContext();

		if ( context->m_iThreadId == 0 ) {
			break;
		}

		slot++;
	}

	// did we get a slot?
	if ( slot == collection->count() ) {
		return;
	}

	auto context = thread->GetContext();
	thread->Reset( ( *scrThreadCount ) + 1, nullptr, 0 );

	if ( *scrThreadId == 0 ) {
		( *scrThreadId )++;
	}

	context->m_iThreadId = *scrThreadId;

	( *scrThreadCount )++;
	( *scrThreadId )++;

	collection->set( slot, thread );

	g_ownedThreads.insert( thread );

	LOG_DEBUG( "Created thread, id %d", thread->GetId() );
}

ScriptEngine::NativeHandler ScriptEngine::GetNativeHandler( uint64_t oldHash ) {

	uint64_t newHash = GetNewHashFromOldHash( oldHash );
	if ( newHash == 0 ) {
		return nullptr;
	}

	NativeRegistration * table = registrationTable[newHash & 0xFF];

	for ( ; table; table = table->nextRegistration ) {

		for ( uint32_t i = 0; i < table->numEntries; i++ ) {

			if ( newHash == table->hashes[i] ) {
				return table->handlers[i];
			}
		}
	}

	return nullptr;
}

uint64_t ScriptEngine::GetNewHashFromOldHash( uint64_t oldHash ) {

	auto cachePair = foundHashCache.find( oldHash );
	if ( cachePair != foundHashCache.end() ) {
		return cachePair->second;
	}

	auto pair = findHashPairIt(oldHash);//nativeHashMap.find( oldHash );
	if ( pair == nativeHashMap.end() ) {

		LOG_ERROR( "Failed to find new hash for 0x%p", oldHash );
		return 0;
	}

	foundHashCache[oldHash] = pair->second;
	return pair->second;
}

eGameState ScriptEngine::GetGameState() {

	return *gameState;
}

