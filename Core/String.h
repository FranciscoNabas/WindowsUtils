#pragma once
#pragma unmanaged

#include "MemoryManagement.h"

namespace WindowsUtils::Core
{
	////////////////////////////////////////////////////////////////
	// WindowsUtils string.
	// 
	// A multi-purpose string class that encapsulates both 8-bit,
	// and 16-bit C-style strings.
	// 
	// It abstracts allocation with the constructors, and
	// overloaded operators.
	//
	// -------------------------------------------------------------
	// ATTENTION!
	// 
	// Currently this implementation does not support both 8-bit, and
	// 16-bit strings simultaneously.
	// 
	// It also does not offer conversion, so any comparison between a
	// 8-bit and a 16-bit string will return FALSE.
	/////////////////////////////////////////////////////////////////

	class WuString
	{
	private:
		LPSTR _strBuff;
		LPWSTR _wideBuff;
		size_t _charCount;
		bool _isWide;
		bool _isInitialized;

	public:
		WuString();
		WuString(const LPSTR buffer);
		WuString(const LPWSTR buffer);
		WuString(const WuString& other);

		~WuString();

		const size_t Length();
		const size_t Length() const;
		LPSTR GetBuffer();
		LPSTR GetBuffer() const;
		LPWSTR GetWideBuffer();
		LPWSTR GetWideBuffer() const;

		void Format(const LPSTR format, ...);
		void Format(const LPWSTR format, ...);

		void Remove(size_t index, size_t count);

		BOOL Contains (const CHAR character) const;
		BOOL Contains(const WCHAR character) const;

		BOOL EndsWith(const CHAR character) const;
		BOOL EndsWith(const WCHAR character) const;

		void operator= (const LPSTR other);
		void operator= (const LPWSTR other);
		void operator= (const WuString& other);

		void operator+= (const LPSTR other);
		void operator+= (const LPWSTR other);
		void operator+= (const WuString& other);

		BOOL operator== (const LPSTR other) const;
		BOOL operator== (const LPWSTR other) const;
		BOOL operator== (const WuString& other) const;

		BOOL operator!= (const LPSTR other) const;
		BOOL operator!= (const LPWSTR other) const;
		BOOL operator!= (const WuString& other) const;
	};
}