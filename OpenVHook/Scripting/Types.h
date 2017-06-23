#pragma once

template <typename T>
class fwPool
{
public:
	T *m_pData;
	uint8_t *m_bitMap;
	int32_t m_count;
	int32_t m_itemSize;
	int32_t m_unkItemIndex;
	int32_t m_freeSlotIndex;
	uint32_t m_flags;
private:
	char pad1[0x4];
public:

	bool full() const
	{
		return m_count - (m_flags & 0x3FFFFFFF) <= 256;
	}

	bool isValid(int32_t index) const
	{
		return ~(m_bitMap[index] >> 7) & 1;
	}

	T * get(int32_t index)
	{
		return m_pData + index * m_itemSize;
	}

	T ** begin() {
		return m_pData;
	}

	T ** end() {
		return (m_pData + m_count);
	}
};

class fwGenericPool : public fwPool<void*>
{
};

class VehiclePool
{
public:
	uint64_t **m_pData;		// off=0x00-0x08
	uint32_t m_size;				// off=0x08-0x0C
	char pad0[0x24];
	uint32_t* m_bitMap;			// off=0x30-0x38
	char pad1[0x28];
	int32_t m_count;			// off=0x60-0x64

	bool isValid(int32_t i) const
	{
		return m_bitMap[i >> 5] >> (i & 0x1F) & 1;
	}

	uint64_t* getAddress(int32_t i) const
	{
		return m_pData[i];
	}
};

class fwScriptGuid
{
public:
	virtual ~fwScriptGuid() = 0;
	void * m_pEntity;
};
