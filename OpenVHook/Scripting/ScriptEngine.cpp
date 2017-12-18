#include "ScriptEngine.h"
#include "NativeHashMap.h"
#include "..\Utility\Pattern.h"
#include "..\Utility\Log.h"

using namespace Utility;

static pgPtrCollection<ScriptThread> * scrThreadCollection;
static uint32_t activeThreadTlsOffset;

static uint32_t * scrThreadId;
static uint32_t * scrThreadCount;

static scriptHandlerMgr * g_scriptHandlerMgr;

int gameVersion = ScriptEngine::GetGameVersion();

GlobalTable globalTable;

CPools pools;

// https://www.unknowncheats.me/forum/grand-theft-auto-v/144028-reversal-thread-81.html#post1931323
struct NativeRegistration {
    uint64_t nextRegistration1;
    uint64_t nextRegistration2;
    ScriptEngine::NativeHandler handlers[7];
    uint32_t numEntries1;
    uint32_t numEntries2;
    uint64_t hashes;

    inline NativeRegistration* getNextRegistration() {
        uintptr_t result;
        auto v5 = reinterpret_cast<uintptr_t>(&nextRegistration1);
        auto v12 = 2i64;
        auto v13 = v5 ^ nextRegistration2;
        auto v14 = (char *)&result - v5;
        do
        {
            *(DWORD*)&v14[v5] = v13 ^ *(DWORD*)v5;
            v5 += 4i64;
            --v12;
        } while (v12);

        return reinterpret_cast<NativeRegistration*>(result);
    }

    inline uint32_t getNumEntries() {
        return ((uintptr_t)&numEntries1) ^ numEntries1 ^ numEntries2;
    }

    inline uint64_t getHash(uint32_t index) {

        auto naddr = 16 * index + reinterpret_cast<uintptr_t>(&nextRegistration1) + 0x54;
        auto v8 = 2i64;
        uint64_t nResult;
        auto v11 = (char *)&nResult - naddr;
        auto v10 = naddr ^  *(DWORD*)(naddr + 8);
        do
        {
            *(DWORD *)&v11[naddr] = v10 ^ *(DWORD*)(naddr);
            naddr += 4i64;
            --v8;
        } while (v8);

        return nResult;
    }
};

static NativeRegistration ** registrationTable;

static std::unordered_set<ScriptThread*> g_ownedThreads;

static std::unordered_map<uint64_t, uint64_t> foundHashCache;

static eGameState * gameState;

bool ScriptEngine::Initialize() {
	LOG_PRINT("Initializing ScriptEngine...");

	executable_meta executable;
	executable.EnsureInit();

	auto scrThreadCollectionPattern = pattern("48 8B C8 EB 03 48 8B CB 48 8B 05");

	char * location = scrThreadCollectionPattern.count(1).get(0).get<char>(11);
	if (location == nullptr) {

		LOG_ERROR("Unable to find scrThreadCollection");
		return false;
	}
	scrThreadCollection = reinterpret_cast<decltype(scrThreadCollection)>(location + *(int32_t*)location + 4);
	LOG_DEBUG("scrThreadCollection\t 0x%p (0x%.8X)", scrThreadCollection, reinterpret_cast<uintptr_t>(scrThreadCollection) - executable.begin());

	activeThreadTlsOffset = 0x830;
	LOG_DEBUG("activeThreadTlsOffset 0x%.8X", activeThreadTlsOffset);

	auto scrThreadIdPattern = pattern("8B 15 ? ? ? ? 48 8B 05 ? ? ? ? FF C2");

	location = scrThreadIdPattern.count(1).get(0).get<char>(0);
	if (location == nullptr) {

		LOG_ERROR("Unable to find scrThreadId");
		return false;
	}
	scrThreadId = reinterpret_cast<decltype(scrThreadId)>(location + *(int32_t*)(location + 2) + 6);
	LOG_DEBUG("scrThreadId\t\t 0x%p (0x%.8X)", scrThreadId, reinterpret_cast<uintptr_t>(scrThreadId) - executable.begin());

	auto scrThreadCountPattern = pattern("FF 0D ? ? ? ? 48 8B F9");

	location = scrThreadCountPattern.get(0).get<char>(2);
	if (location == nullptr) {

		LOG_ERROR("Unable to find scrThreadCount");
		return false;
	}
	scrThreadCount = reinterpret_cast<decltype(scrThreadCount)>(location + *(int32_t*)location + 4);
	LOG_DEBUG("scrThreadCount\t 0x%p (0x%.8X)", scrThreadCount, reinterpret_cast<uintptr_t>(scrThreadCount) - executable.begin());

	auto registrationTablePattern = pattern("76 32 48 8B 53 40");

	location = registrationTablePattern.count(1).get(0).get<char>(9);
	if (location == nullptr) {

		LOG_ERROR("Unable to find registrationTable");
		return false;
	}
	registrationTable = reinterpret_cast<decltype(registrationTable)>(location + *(int32_t*)location + 4);
	LOG_DEBUG("registrationTable\t 0x%p (0x%.8X)", registrationTable, reinterpret_cast<uintptr_t>(registrationTable) - executable.begin());

	auto g_scriptHandlerMgrPattern = pattern("74 17 48 8B C8 E8 ? ? ? ? 48 8D 0D");

	location = g_scriptHandlerMgrPattern.count(1).get(0).get<char>(13);
	if (location == nullptr) {

		LOG_ERROR("Unable to find g_scriptHandlerMgr");
		return false;
	}
	g_scriptHandlerMgr = reinterpret_cast<decltype(g_scriptHandlerMgr)>(location + *(int32_t*)location + 4);
	LOG_DEBUG("g_scriptHandlerMgr\t 0x%p (0x%.8X)", g_scriptHandlerMgr, reinterpret_cast<uintptr_t>(g_scriptHandlerMgr) - executable.begin());

    // vector3 pointer fix
    if (auto void_location = pattern("83 79 18 ? 48 8B D1 74 4A FF 4A 18").count(1).get(0).get<void>())
    {
        scrNativeCallContext::SetVectorResults = (void(*)(scrNativeCallContext*))(void_location);
    }

	//script_location
	auto getScriptIdBlock = pattern("80 78 32 00 75 34 B1 01 E8");
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

	auto gameStatePattern =				pattern("83 3D ? ? ? ? ? 8A D9 74 0A");

	location = gameStatePattern.count(1).get(0).get<char>(2);
	if (location == nullptr) {

		LOG_ERROR("Unable to find gameState");
		return false;
	}
	gameState = reinterpret_cast<decltype(gameState)>(location + *(int32_t*)location + 5);
	LOG_DEBUG("gameState\t\t 0x%p (0x%.8X)", gameState, reinterpret_cast<uintptr_t>(gameState) - executable.begin());

	auto g_globalPtrPattern = pattern("4C 8D 05 ? ? ? ? 4D 8B 08 4D 85 C9 74 11");

	location = g_globalPtrPattern.count(1).get(0).get<char>(0);
	if (location == nullptr) {

		LOG_ERROR("Unable to find g_globalPtr");
		return false;
	}
	//g_globalPtr = reinterpret_cast<decltype(g_globalPtr)>(location + *(int32_t*)(location + 3) + 7);
	globalTable.GlobalBasePtr = (__int64**)(location + *(int*)(location + 3) + 7);
	LOG_DEBUG("g_globalPtr\t\t 0x%p (0x%.8X)", globalTable.GlobalBasePtr, reinterpret_cast<uintptr_t>(globalTable.GlobalBasePtr) - executable.begin());

	//gameVersion = GetGameVersion();
	LOG_PRINT("Game version #%i", gameVersion);

	// Initialize internal pools
	pools.Initialize();

	// Check if game is ready
	LOG_PRINT("Checking if game is ready...");
	while (!scrThreadCollection->begin()) {
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

	for ( auto & itThread : *collection ) {

		auto context = itThread->GetContext();

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

	for ( ; table; table = table->getNextRegistration() ) {

		for ( uint32_t i = 0; i < table->getNumEntries(); i++ ) {

			if ( newHash == table->getHash(i) ) {
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

	auto pair = nativeHashMap.find( oldHash );
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

int ScriptEngine::GetGameVersion()
{
	LPVOID pModule = GetModuleHandleA(NULL);

	DWORD codeSig = *(DWORD*)((DWORD64)pModule + 0x870000);

	switch (codeSig)
	{
	case 0xE8012024:
		return 0;
	case 0xA29410:
		return 1;
	case 0x7D2205FF:
		return 2;
	case 0x1:
		return 3;
	case 0x1ECB9:
		return 4;
	case 0x100FF360:
		return 5;
	case 0x8B48FF79:
		return 7;
	case 0xC4834800:
		return 9;
	case 0xF000001:
		return 10;
	case 0xC86E0F66:
		return 11;
	case 0x57085889:
		return 12;
	case 0x28C48348:
		return 13;
	case 0x4DE2E800:
		return 14;
	case 0x8948C88B:
		return 15;
	case 0xF4397715:
		return 16;
	case 0x48FFF41E:
		return 17;
	case 0x36CB0305:
		return 18;
	case 0xB95A0589:
		return 19;
	case 0x8B48C88B:
		return 20;
	case 0xE80C75D2:
		return 21;
	case 0x137978C:
		return 23;
	case 0xB86AE800:
		return 24;
	case 0x75C68441:
		return 27;
	case 0x828B1C74:
		return 28;
	case 0xD8B4800:
		return 29;
	case 0x3C244C10:
		return 30;
	case 0xB2F4E30D:
		return 31;
	case 0x89587500:
		return 35;
	case 0xC4834801:
		return 36;
	case 0xF36C5010:
		return 37;
    case 0x83483024:
        return 38;
    // todo: 1290 steam
	default:
		return -1;
	}
}
