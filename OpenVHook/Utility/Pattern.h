#ifndef __PATTERN_H__
#define __PATTERN_H__

#include "..\OpenVHook.h"
#include "Log.h"

// from boost someplace
template <uint64_t FnvPrime, uint64_t OffsetBasis>
struct basic_fnv_1 {

	uint64_t operator()( std::string const& text ) const {

		uint64_t hash = OffsetBasis;
		for ( std::string::const_iterator it = text.begin(), end = text.end(); it != end; ++it ) {

			hash *= FnvPrime;
			hash ^= *it;
		}

		return hash;
	}
};

const uint64_t fnv_prime = 1099511628211u;
const uint64_t fnv_offset_basis = 14695981039346656037u;

typedef basic_fnv_1<fnv_prime, fnv_offset_basis> fnv_1;

namespace Utility {

	void TransformPattern( const std::string & pattern, std::string & data, std::string & mask );

	class executable_meta {
	private:

		uintptr_t	m_begin;
		uintptr_t	m_end;

	public:

		executable_meta()
			: m_begin( 0 ), m_end( 0 ) {
		}

		void EnsureInit();

		inline uintptr_t begin() { return m_begin; }
		inline uintptr_t end() { return m_end; }
	};

	class pattern_match {
	private:

		void* m_pointer;

	public:

		inline pattern_match( void* pointer ) {

			m_pointer = pointer;
		}

		template<typename T>
		T * get( int offset ) {

			if ( m_pointer == nullptr ) {
				return nullptr;
			}

			char* ptr = reinterpret_cast<char*>( m_pointer );
			return reinterpret_cast<T*>( ptr + offset );
		}

		template<typename T>
		T * get() {

			return get<T>( 0 );
		}
	};

	typedef std::vector<pattern_match> matchVec;

	class pattern {
	private:

		std::string			m_bytes;
		std::string			m_mask;

		uint64_t			m_hash;

		size_t				m_size;

		matchVec			m_matches;

		bool				m_matched;

	private:

		void Initialize( const char* pattern, size_t length );

		bool ConsiderMatch( uintptr_t offset );

		void EnsureMatches( int maxCount );

	public:

		template<size_t Len>
		pattern( const char( &pattern )[Len] ) {

			Initialize( pattern, Len );
		}

		inline pattern & count( int expected ) {

			if ( !m_matched ) {
				EnsureMatches( expected );
			}

			return *this;
		}

		inline size_t size() {

			if ( !m_matched ) {
				EnsureMatches( INT_MAX );
			}

			return m_matches.size();
		}

		inline pattern_match & get( int index ) {

			if ( !m_matched ) {
				EnsureMatches( INT_MAX );
			}

			if ( m_matches.size() == 0 ) {

				m_matches.push_back( pattern_match( nullptr ) );
				return m_matches[0];
			}

			return m_matches[index];
		}

	public:
		// define a hint
		static void hint( uint64_t hash, uintptr_t address );
	};
}

#endif // __PATTERN_H__