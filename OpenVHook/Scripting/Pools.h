#pragma once

#include "..\OpenVHook.h"
#include "Types.h"

class CPools
{
	fwPool<EntityRef> **		m_pEntityPool = 0;
	VehiclePool ***				m_pVehiclePool = 0;
	fwGenericPool **			m_pPedPool = 0;
	fwGenericPool **			m_pObjectPool = 0;
	fwGenericPool **			m_pPickupsPool = 0;


public:
	bool Initialize();

	fwPool<EntityRef> * GetEntityPool() const {
		return *m_pEntityPool;
	}

	VehiclePool * GetVehiclePool() const {
		return **m_pVehiclePool;
	}

	fwGenericPool * GetPedPool() const {
		return *m_pPedPool;
	}

	fwGenericPool * GetObjectsPool() const {
		return *m_pObjectPool;
	}

	fwGenericPool * GetPickupsPool() const {
		return *m_pPickupsPool;
	}
};
