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
		UINT _charCount;
		bool _isWide;
		bool _isInitialized;

	public:
		WuString();
		WuString(const LPSTR buffer);
		WuString(const LPWSTR buffer);
		WuString(const WuString& other);

		~WuString();

		const UINT Length();
		LPSTR GetBuffer();
		LPWSTR GetWideBuffer();

		void Format(const LPSTR format, ...);
		void Format(const LPWSTR format, ...);

		void operator= (const LPSTR other);
		void operator= (const LPWSTR other);
		void operator= (const WuString& other);

		void operator+= (const LPSTR other);
		void operator+= (const LPWSTR other);
		void operator+= (const WuString& other);

		BOOL operator== (const LPSTR other);
		BOOL operator== (const LPWSTR other);
		BOOL operator== (const WuString& other);

		BOOL operator!= (const LPSTR other);
		BOOL operator!= (const LPWSTR other);
		BOOL operator!= (const WuString& other);
	};
}