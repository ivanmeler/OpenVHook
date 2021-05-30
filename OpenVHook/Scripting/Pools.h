#pragma once

#include "..\OpenVHook.h"
#include "Types.h"

class CPools
{
	fwPool<EntityRef> **	m_pEntityPool = 0;
	VehiclePool ***			m_pVehiclePool = 0;
	GenericPool **			m_pPedPool = 0;
	GenericPool **			m_pObjectPool = 0;
	GenericPool **			m_pPickupsPool = 0;

	int(*m_AddressToEntity)(int64_t) = nullptr;
	
public:
	bool Initialize();

	inline int AddressToEntity(int64_t i) const {
		return m_AddressToEntity(i);
	}

	fwPool<EntityRef> * GetEntityPool() const {
		return *m_pEntityPool;
	}

	VehiclePool * GetVehiclePool() const {
		return **m_pVehiclePool;
	}

	GenericPool * GetPedPool() const {
		return *m_pPedPool;
	}

	GenericPool * GetObjectsPool() const {
		return *m_pObjectPool;
	}

	GenericPool * GetPickupsPool() const {
		return *m_pPickupsPool;
	}
};
