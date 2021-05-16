#ifndef __NATIVE_INVOKER_H__
#define __NATIVE_INVOKER_H__

#include "..\OpenVHook.h"

class scrNativeCallContext {
protected:

	void *		m_pReturn;
	uint32_t	m_nArgCount;
	void *		m_pArgs;

	uint32_t	m_nDataCount;
    alignas(uintptr_t)uint8_t m_vectorSpace[192];

public:

	template<typename T>
	inline T GetArgument( int idx ) {

		intptr_t * arguments = (intptr_t*)m_pArgs;
		return *(T*)&arguments[idx];
	}

	template<typename T>
	inline void SetResult( int idx, T value ) {

		intptr_t * returnValues = (intptr_t*)m_pReturn;
		*(T*)&returnValues[idx] = value;
	}

	inline int GetArgumentCount() {

		return m_nArgCount;
	}

	template<typename T>
	inline T GetResult( int idx ) {

		intptr_t * returnValues = (intptr_t*)m_pReturn;
		return *(T*)&returnValues[idx];
	}

    static void(*SetVectorResults)(scrNativeCallContext*);
};

class NativeContext : public scrNativeCallContext {
private:

	// Configuration
	enum {
		MaxNativeParams = 32,
		ArgSize = 8,
	};

	// Anything temporary that we need
	uint8_t m_TempStack[MaxNativeParams * ArgSize];

public:

	inline NativeContext() {

		m_pArgs = &m_TempStack;
		m_pReturn = &m_TempStack;		// It's okay to point both args and return at
		// the same pointer. The game should handle this.
		m_nArgCount = 0;
		m_nDataCount = 0;
		memset(m_TempStack, 0, sizeof(m_TempStack));
	}

    template <typename T>
    inline void Push(T value) {
        static_assert(sizeof(T) <= ArgSize, "Argument has an invalid size");

        *reinterpret_cast<uintptr_t*>(m_TempStack + ArgSize * m_nArgCount) = 0;

        *reinterpret_cast<T*>(m_TempStack + ArgSize * m_nArgCount) = value;

        m_nArgCount++;
    }

    template <typename T>
    inline T GetResult() {

        return *reinterpret_cast<T*>(m_TempStack);
    }
};

#endif // __NATIVE_INVOKER_H__