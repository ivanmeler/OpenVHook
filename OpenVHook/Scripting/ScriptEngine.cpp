#include "ScriptEngine.h"
#include "..\Utility\Pattern.h"
#include "..\Utility\Log.h"
#include "..\DirectXHook\DirectXHook.h"

#include <iostream>
#include <string>


using namespace Utility;

static pgPtrCollection<ScriptThread> * scrThreadCollection;
static uint32_t activeThreadTlsOffset;

static uint32_t * scrThreadId;
static uint32_t * scrThreadCount;

static scriptHandlerMgr * g_scriptHandlerMgr;

int gameVersion = ScriptEngine::GetGameVersion();
const static int searchDepth = ScriptEngine::GameVersionToSearchDepth(gameVersion);
GlobalTable globalTable;


CPools pools;

#pragma pack(push)
#pragma pack(4)		// _unknown 4 bytes
// https://www.unknowncheats.me/forum/grand-theft-auto-v/144028-reversal-thread-81.html#post1931323
struct NativeRegistration {
	uint64_t nextRegBase;
	uint64_t nextRegKey;
	ScriptEngine::NativeHandler handlers[7];
	uint32_t numEntries1;
	uint32_t numEntries2;
	uint32_t _unknown;
	uint64_t hashes[7];

	/*
		// decryption
		key = this ^ nextRegKey  // only lower 32 bits
		nextReg = nextRegBase ^ key<<32 ^ key

		// encryption
		key = this ^ nextRegKey  // only lower 32 bits
		nextRegBase = nextReg ^ key<<32 ^ key
		
		only lower 32 bits of this^nextRegKey are used, higher 32 bits are ignored.
		thus, higher 32 bit of nexRegBase must contain the info of (masked) higher address of next registration.
		the first two members of struct are named as Base/Key respectively in that sense.
	*/
	inline NativeRegistration* getNextRegistration() {
		uint32_t key = static_cast<uint32_t>(reinterpret_cast<uint64_t>(this) ^ nextRegKey);
		return reinterpret_cast<NativeRegistration*>(nextRegBase ^ (static_cast<uint64_t>(key) << 32) ^ key);
	}

	inline void setNextRegistration(NativeRegistration* nextReg, uint64_t nextKey) {
		nextRegKey = nextKey;
		uint32_t key = static_cast<uint32_t>(reinterpret_cast<uint64_t>(this) ^ nextRegKey);
		nextRegBase = reinterpret_cast<uint64_t>(nextReg) ^ (static_cast<uint64_t>(key) << 32) ^ key;
	}

	inline uint32_t getNumEntries() {
		return static_cast<uint32_t>(reinterpret_cast<uint64_t>(&numEntries1)) ^ numEntries1 ^ numEntries2;
	}

	inline uint64_t getHash(uint32_t index) {
		uint32_t key = static_cast<uint32_t>(reinterpret_cast<uint64_t>(&hashes[2 * index]) ^ hashes[2 * index + 1]);
		return hashes[2 * index] ^ (static_cast<uint64_t>(key) << 32) ^ key;
	}
};
#pragma pack(pop)

static NativeRegistration ** registrationTable;

static std::vector<std::pair<ScriptThread*, ScriptThread*>> g_ownedThreads;

static std::unordered_map<uint64_t, ScriptEngine::NativeHandler> foundHashCache;

static eGameState * gameState;

static std::vector<std::string> GameVersionString = {
	"VER_1_0_335_2_STEAM",     
	"VER_1_0_335_2_NOSTEAM",    

	"VER_1_0_350_1_STEAM",     
	"VER_1_0_350_2_NOSTEAM",    

	"VER_1_0_372_2_STEAM",      
	"VER_1_0_372_2_NOSTEAM",    

	"VER_1_0_393_2_STEAM",      
	"VER_1_0_393_2_NOSTEAM",    
	"VER_1_0_393_4_STEAM",     
	"VER_1_0_393_4_NOSTEAM",    

	"VER_1_0_463_1_STEAM",      
	"VER_1_0_463_1_NOSTEAM",    

	"VER_1_0_505_2_STEAM",     
	"VER_1_0_505_2_NOSTEAM",    

	"VER_1_0_573_1_STEAM",      
	"VER_1_0_573_1_NOSTEAM",    

	"VER_1_0_617_1_STEAM",      
	"VER_1_0_617_1_NOSTEAM",    

	"VER_1_0_678_1_STEAM",     
	"VER_1_0_678_1_NOSTEAM",   

	"VER_1_0_757_2_STEAM",     
	"VER_1_0_757_2_NOSTEAM",   
	"VER_1_0_757_4_STEAM",     
	"VER_1_0_757_4_NOSTEAM",    

	"VER_1_0_791_2_STEAM",      
	"VER_1_0_791_2_NOSTEAM",   

	"VER_1_0_877_1_STEAM",     
	"VER_1_0_877_1_NOSTEAM",    

	"VER_1_0_944_2_STEAM",    
	"VER_1_0_944_2_NOSTEAM",  

	"VER_1_0_1011_1_STEAM",    
	"VER_1_0_1011_1_NOSTEAM",   

	"VER_1_0_1032_1_STEAM",     
	"VER_1_0_1032_1_NOSTEAM",  

	"VER_1_0_1103_2_STEAM",     
	"VER_1_0_1103_2_NOSTEAM",  

	"VER_1_0_1180_2_STEAM",    
	"VER_1_0_1180_2_NOSTEAM",   

	"VER_1_0_1290_1_STEAM",     
	"VER_1_0_1290_1_NOSTEAM",   

	"VER_1_0_1365_1_STEAM",     
	"VER_1_0_1365_1_NOSTEAM",   

	"VER_1_0_1493_0_STEAM",     
	"VER_1_0_1493_0_NOSTEAM",   
	"VER_1_0_1493_1_STEAM",     
	"VER_1_0_1493_1_NOSTEAM",   
	
	"VER_1_0_1604_0_STEAM",     
	"VER_1_0_1604_0_NOSTEAM",  
	"VER_1_0_1604_1_STEAM",     
	"VER_1_0_1604_1_NOSTEAM",   

	"VER_1_0_1737_0_STEAM",    
	"VER_1_0_1737_0_NOSTEAM",  
	"VER_1_0_1737_6_STEAM",   
	"VER_1_0_1737_6_NOSTEAM",   

	"VER_1_0_1868_0_STEAM",
	"VER_1_0_1868_0_NOSTEAM",
	"VER_1_0_1868_1_STEAM",
	"VER_1_0_1868_1_NOSTEAM",
	"VER_1_0_1868_4_EGS",

	"VER_1_0_2060_0_STEAM",
	"VER_1_0_2060_0_NOSTEAM",
	"VER_1_0_2060_1_STEAM",
	"VER_1_0_2060_1_NOSTEAM",

	"VER_1_0_2189_0_STEAM",
	"VER_1_0_2189_0_NOSTEAM",

	"VER_1_0_2215_0_STEAM",
	"VER_1_0_2215_0_NOSTEAM",

	"VER_1_0_2245_0_STEAM",
	"VER_1_0_2245_0_NOSTEAM"
};

static std::string GameVersionToString(int version) {
	if (version > GameVersionString.size() - 1 || version < 0) {
		return std::to_string(version);
	}
	return GameVersionString[version];
}

bool ScriptEngine::Initialize() {
	LOG_PRINT("Initializing ScriptEngine...");

	// init Direct3d hook
	if (!g_D3DHook.InitializeHooks())
	{
		LOG_ERROR("Failed to Initialize Direct3d Hooks");
		return false;
	}

	executable_meta executable;
	executable.EnsureInit();

	auto p_launcherCheck = pattern("E8 ? ? ? ? 84 C0 75 0C B2 01 B9 2F").count(1).get(0).get<char>(0);
	memset(p_launcherCheck, 0x90, 21);

	auto p_legalNotice = pattern("72 1F E8 ? ? ? ? 8B 0D").count(1).get(0).get<char>(0);
	memset(p_legalNotice, 0x90, 2);

	auto scrThreadCollectionPattern = pattern("48 8B C8 EB 03 48 8B CB 48 8B 05");

	char * location = scrThreadCollectionPattern.count(1).get(0).get<char>(11);
	if (location == nullptr) {

		LOG_ERROR("Unable to find scrThreadCollection");
		return false;
	}
	scrThreadCollection = reinterpret_cast<decltype(scrThreadCollection)>(location + *(int32_t*)location + 4);
	LOG_DEBUG("scrThreadCollection\t 0x%p (0x%.8X)", scrThreadCollection, reinterpret_cast<uintptr_t>(scrThreadCollection) - executable.begin());

	activeThreadTlsOffset = *pattern("48 8B 04 D0 4A 8B 14 00 48 8B 01 F3 44 0F 2C 42 20").count(1).get(0).get<uint32_t>(-4);
	LOG_DEBUG("activeThreadTlsOffset 0x%.8X", activeThreadTlsOffset);

	auto scrThreadIdPattern = pattern("89 15 ? ? ? ? 48 8B 0C D8");

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

	auto gameStatePattern =	pattern("83 3D ? ? ? ? ? 8A D9 74 0A");

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
	LOG_PRINT("Game version is %s (%d)", GameVersionToString(gameVersion).c_str(), gameVersion);

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

	ScriptThread* orig_thread = collection->at(slot);
	collection->set( slot, thread );

	g_ownedThreads.push_back({ thread, orig_thread });

	LOG_DEBUG( "Created thread, id %d", thread->GetId() );
}

void ScriptEngine::RemoveAllThreads()
{
	std::reverse(g_ownedThreads.begin(), g_ownedThreads.end());
	auto collection = GetThreadCollection();
	int index = 0;
	for (auto slot : * collection) {
		for (auto& [th, orig_th]: g_ownedThreads) {
			if (slot == th) {
				collection->set(index, orig_th);
			}
		}
		index++;
	}
	g_ownedThreads.clear();
}

#if _DEBUG
static int DumpAllNativeHashAndHandlers() {
	LOG_DEBUG("GameBase: %p. Dumping native hashes => handlers. patience babe.", GetModuleHandle(NULL));
	int count = 0;
	FILE* dumpfile = fopen("dumped_natives.log", "w");
	for (int x = 0; x < 0xFF; x++) {
		NativeRegistration* table = registrationTable[x];
		for (; table; table = table->getNextRegistration()) {
			for (uint32_t i = 0; i < table->getNumEntries(); i++) {
				auto newHash = table->getHash(i);
				auto handler = table->handlers[i];
				fprintf(dumpfile, "%llX => %p\n",newHash, handler);
				count++;
			}
		}
	}
	LOG_DEBUG("Dumped %d native hashes and handlers", count);
	fclose(dumpfile);
	return count;
}
#endif

ScriptEngine::NativeHandler ScriptEngine::GetNativeHandler( uint64_t oldHash ) {

#if _DEBUG && 0
	static int dumped_native_count = DumpAllNativeHashAndHandlers();
#endif

	auto cachePair = foundHashCache.find(oldHash);
	if (cachePair != foundHashCache.end()) {
		return cachePair->second;
	}

	NativeHandler handler = nullptr;
	uint64_t newHash = GetNewHashFromOldHash( oldHash );

	if ( newHash == 0 ) {
		LOG_DEBUG("Failed to GetNewHashFromOldHash(%llX)", oldHash);
		handler = nullptr;
	} else {
		NativeRegistration* table = registrationTable[newHash & 0xFF];
		for (; table; table = table->getNextRegistration()) {
			bool found = false;
			for (uint32_t i = 0; i < table->getNumEntries(); i++) {
				if (newHash == table->getHash(i)) {
					handler = table->handlers[i];
					found = true;
					break;
				}
			}
			if (found) break;
		}
	}
	foundHashCache[oldHash] = handler;
	return handler;
}

uint64_t ScriptEngine::GetNewHashFromOldHash( uint64_t oldHash ) {

	if (searchDepth == 0) {
		// no need for conversion
		return oldHash;
	}

	// Algorithm Explained

	// natives.h uses constant oldHashes to represent functions. One oldHash is expected to be mapped to the same function in all game versions.
	// In order to use the same oldHash in all version of games, where hash of the same function changed from version to version,
	// Alexander Blade maintains a hashmap that stores a complete 2-D hash list version by version.

	// The oldHash is expected to be the oldest hash of a function, but in reality it may not exist at hashVer=0 or until latest, or may be even not the actual oldest hash.
	// That is why we need to search from the hashVer=0 to the latest.
	// Once we found the first occurrence of oldHash (of function Fn_i), we should locate the Fn_i line that stores hashes of different hashVers,
	// then search all the way down to the exact hashVersion of the running game.

	// optimized implementation
	// scan row by row at column 0. If nothing found, try column 1, etc
	// if firstly found old hash at (i,j), get the non-zero hash at (i, x) where x->searchDepth(as close as possible) && j<x<=searchDepth
	for (int i = 0; i < fullHashMapCount; i++) {
		for (int j = 0; j <= searchDepth; j++) {
			if (fullHashMap[i][j] == oldHash) {
				// found
				for(int k = searchDepth; k > j; k--) {		// search from latest hash to oldest hash. faster for the most cases
					uint64_t newHash = fullHashMap[i][k];
					if (newHash == 0)
						continue;
					return newHash;
				}
				// all 0 except the first one. No need for conversion
				return oldHash;
			}
		}
	}
	return 0;
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
	case 0x158B48FF:
		return 22;
	case 0x137978C:
		return 23;
	case 0xB86AE800:
		return 24;
	case 0x158B4800:
		return 25;
	case 0x3B830000:
		return 26;
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
	case 0x3DCF2715:
		return 32;
	case 0x5C0FF300:
		return 33;
	case 0x8B4801B0:
		return 34;
	case 0x89587500:
		return 35;
	case 0xC4834801:
		return 36;
	case 0xF36C5010:
		return 37;
	case 0x83483024:
		return 38;
	case 0x3B8005:
		return 39;
	case 0x248489CF:
		return 40;
	case 0x2C0EB25:
		return 41;
	case 0x410102A4:
		return 42;
	case 0xD0590FC5:
		return 43;
	case 0xA7E2B9:
		return 44;
	case 0x8B4C0000:
		return 45;
	case 0x280F3465:
		return 46;
	case 0xFFFA3468:
		return 47;
	case 0x48C48B48:
		return 48;
	case 0xE8304789:
		return 49;
	case 0x8B480477:
		return 50;
	case 0xEBE06529:
		return 51;
	case 0xFF30440:
		return 52;
	case 0x700F4166:
		return 53;
	case 0x8B484874:
		return 54;
	case 0x88693E8:
		return 55;
	case 0xCB8B48D7:
		return 56;
	case 0x89480446:
		return 57;
	case 0xA0C18148:
		return 58;
	case 0x7738432F:
		return 59;
	case 0x3944F98B:
		return 61;
	case 0x126AE900:
		return 63;
	case 0xC1000000:
		return 64;
	case 0x1428D41:
		return 65;
	case 0x33450158:
		return 66;
	case 0xDE80000:
		return 67;
	case 0x448D48CA:
		return 68;
	default:
		if (codeSig == 0) {
			if (*(DWORD*)((DWORD64)pModule + 0xB00000) == 0x7F58E3E8)
				return 60;
			else
				return 62;
		}
		if (codeSig == 0x89605189) {
			if (*(DWORD*)((DWORD64)pModule + 0x1433B08) == 0x245C8948)
				return 6;
			else
				return 8;
		}
		return -1;
	}
}

int ScriptEngine::GameVersionToSearchDepth(int version)
{
	switch (version) {
	case 0:
	case 1:
		return 0;
	case 2:
	case 3:
		return 1;
	case 4:
	case 5:
		return 2;
	case 6:
	case 7:
	case 8:
	case 9:
		return 3;
	case 10:
	case 11:
		return 4;
	case 12:
	case 13:
		return 5;
	case 14:
	case 15:
		return 6;
	case 16:
	case 17:
		return 7;
	case 18:
	case 19:
		return 8;
	case 20:
	case 21:
	case 22:
	case 23:
		return 9;
	case 24:
	case 25:
		return 10;
	case 26:
	case 27:
		return 11;
	case 28:
	case 29:
		return 12;
	case 30:
	case 31:
	case 32:
	case 33:
		return 13;
	case 34:
	case 35:
		return 14;
	case 36:
	case 37:
		return 15;
	case 38:
	case 39:
		return 16;
	case 40:
	case 41:
		return 17;
	case 42:
	case 43:
	case 44:
	case 45:
		return 18;
	case 46:
	case 47:
	case 48:
	case 49:
		return 19;
	case 50:
	case 51:
	case 52:
	case 53:
		return 20;
	case 54:
	case 55:
	case 56:
	case 57:
	case 58:
		return 21;
	case 59:
	case 60:
	case 61:
	case 62:
	case 63:
		return 22;
	case 64:
	case 65:
	case 66:
	case 67:
	case 68:
		return 23;
	default:
		return fullHashMapDepth - 1;
	}
}
