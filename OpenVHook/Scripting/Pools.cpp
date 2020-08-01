#include "Pools.h"
#include "..\Utility\Pattern.h"
#include "..\Utility\Log.h"

using namespace Utility;

bool CPools::Initialize()
{
	LOG_PRINT("Initializing Pools...");

	executable_meta executable;
	executable.EnsureInit();

	auto pedPoolPattern = pattern("48 8B 05 ? ? ? ? 41 0F BF C8 0F BF 40 10"); 

	char * location = pedPoolPattern.count(1).get(0).get<char>(3);
	if (location == nullptr) {

		LOG_ERROR("Unable to find pedPoolPattern");
		return false;
	}
	m_pPedPool = reinterpret_cast<decltype(m_pPedPool)>(location + *(int32_t*)location + 7);
	LOG_DEBUG("pedPool\t 0x%p (0x%.8X)", m_pPedPool, reinterpret_cast<uintptr_t>(m_pPedPool) - executable.begin());

	auto objectPoolPattern = pattern("48 8B 05 ? ? ? ? 8B 78 10 85 FF"); 

	location = objectPoolPattern.count(1).get(0).get<char>(3);
	if (location == nullptr) {

		LOG_ERROR("Unable to find objectPoolPattern");
		return false;
	}
	m_pObjectPool = reinterpret_cast<decltype(m_pObjectPool)>(location + *(int32_t*)location + 7);
	LOG_DEBUG("objectPool\t 0x%p (0x%.8X)", m_pObjectPool, reinterpret_cast<uintptr_t>(m_pObjectPool) - executable.begin());

	auto pickupPoolPattern = pattern("4C 8B 05 ? ? ? ? 40 8A F2 8B E9");  

	location = pickupPoolPattern.count(1).get(0).get<char>(3);
	if (location == nullptr) {

		LOG_ERROR("Unable to find pickupsPoolPattern");
		return false;
	}
	m_pPickupsPool = reinterpret_cast<decltype(m_pPickupsPool)>(location + *(int32_t*)location + 7);
	LOG_DEBUG("pickupsPool\t 0x%p (0x%.8X)", m_pPickupsPool, reinterpret_cast<uintptr_t>(m_pPickupsPool) - executable.begin());

	auto vehiclePoolPattern = pattern("48 8B 05 ? ? ? ? F3 0F 59 F6 48 8B 08"); 

	location = vehiclePoolPattern.count(1).get(0).get<char>(3);
	if (location == nullptr) {

		LOG_ERROR("Unable to find vehiclePoolPattern");
		return false;
	}
	m_pVehiclePool = reinterpret_cast<decltype(m_pVehiclePool)>(location + *(int32_t*)location + 7);
	LOG_DEBUG("vehiclePool\t 0x%p (0x%.8X)", m_pVehiclePool, reinterpret_cast<uintptr_t>(m_pVehiclePool) - executable.begin());

	auto entityPoolPattern = pattern("4C 8B 0D ? ? ? ? 44 8B C1 49 8B 41 08");  

	location = entityPoolPattern.count(1).get(0).get<char>(3);
	if (location == nullptr) {

		LOG_ERROR("Unable to find entityPoolPattern");
		return false;
	}
	m_pEntityPool = reinterpret_cast<decltype(m_pEntityPool)>(location + *(int32_t*)location + 7);
	LOG_DEBUG("entityPool\t 0x%p (0x%.8X)", m_pEntityPool, reinterpret_cast<uintptr_t>(m_pEntityPool) - executable.begin());

	LOG_PRINT("Pool Init Finished");

	return true;
}
