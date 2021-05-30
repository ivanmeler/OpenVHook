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
		return m_count - (m_flags & 0x3FFFFFFF) < 256;
	}

	bool isValid(int32_t index) const
	{
		return ~(m_bitMap[index] >> 7) & 1;
	}

	int32_t getHandle(int32_t index) const
	{
		return (index << 8) + m_bitMap[index];
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

class GenericPool
{
public:
	uint64_t m_poolStartAddress;
	uint8_t* m_byteArray;
	int32_t  m_count;
	int32_t  m_itemSize;
	int32_t  _unk1;
	double   _unk2;


	inline bool isValid(int i)
	{
		assert(i >= 0);
		return mask(i) != 0;
	}

	inline uint64_t getAddress(int i)
	{
		assert(i >= 0);
		return mask(i) & (m_poolStartAddress + i * m_itemSize);
	}

private:
	inline long long mask(int i)
	{
		assert(i >= 0);
		long long num1 = m_byteArray[i] & 0x80; // check for high bit.
		return ~((num1 | -num1) >> 63);
	}
};

class VehiclePool
{
public:
	uint64_t *m_pData;		// off=0x00-0x08
	uint32_t m_size;				// off=0x08-0x0C
	char pad0[0x24];
	uint32_t* m_bitMap;			// off=0x30-0x38
	char pad1[0x28];
	int32_t m_count;			// off=0x60-0x64

	bool isValid(int32_t i) const
	{
		assert(i >= 0);
		return m_bitMap[i >> 5] >> (i & 0x1F) & 1;
	}

	uint64_t getAddress(int32_t i) const
	{
		return m_pData[i];
	}
};

struct EntityRef
{
	char pad[0x8];
	void * m_pEntity;
};
