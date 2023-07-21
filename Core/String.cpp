#include "pch.h"

#include "String.h"

namespace WindowsUtils::Core
{
	////////////////////////////////////////////
	//
	// ~ WuString implementation
	//
	////////////////////////////////////////////

	WuString::WuString()
		: _strBuff(NULL), _wideBuff(NULL), _charCount(0), _isWide(false), _isInitialized(false)
	{ }

	WuString::WuString(const size_t charCount, BOOL isWide = TRUE)
	{
		this->Initialize(charCount, isWide);
	}

	// 8-bit character.
	WuString::WuString(const LPSTR buffer)
	{
		if (buffer == NULL)
			this->WuString::WuString();
		else
		{
			// With the null terminating character.
			_charCount = strlen(buffer) + 1;
			PVOID currentProcHeap = GetProcessHeap();

			_strBuff = static_cast<LPSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, _charCount));
			if (_strBuff != NULL)
			{
				_isInitialized = true;
				_isWide = false;

				strcpy_s(_strBuff, _charCount, buffer);
			}
			else
				throw "Insufficient memory.";
		}
	}

	// 16-bit wide character.
	WuString::WuString(const LPWSTR buffer)
	{
		if (buffer == NULL)
			this->WuString::WuString();
		else
		{
			// With the null terminating character.
			_charCount = wcslen(buffer) + 1;
			size_t byteSize = _charCount * 2;
			PVOID currentProcHeap = GetProcessHeap();

			_wideBuff = static_cast<LPWSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, byteSize));
			if (_wideBuff != NULL)
			{
				_isInitialized = true;
				_isWide = true;

				wcscpy_s(_wideBuff, _charCount, buffer);
			}
			else
				throw "Insufficient memory.";
		}
	}

	// Another WuString.
	WuString::WuString(const WuString& other)
	{
		if (other._isInitialized)
		{
			PVOID currentProcHeap = GetProcessHeap();
			_charCount = other._charCount;

			if (other._isWide)
			{
				size_t byteSize = other._charCount * 2;
				_wideBuff = static_cast<LPWSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, byteSize));
				if (_wideBuff != NULL)
				{
					_isInitialized = true;
					_isWide = true;

					wcscpy_s(_wideBuff, _charCount, other._wideBuff);
				}
				else
					throw "Insufficient memory.";
			}
			else
			{
				_strBuff = static_cast<LPSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, _charCount));
				if (_strBuff != NULL)
				{
					_isInitialized = true;
					_isWide = false;

					strcpy_s(_strBuff, _charCount, other._strBuff);
				}
				else
					throw "Insufficient memory.";
			}
		}
		else
			this->WuString::WuString();
	}

	// Destructor.
	WuString::~WuString()
	{
		PVOID currentProcHeap = GetProcessHeap();

		if (_strBuff != NULL)
			HeapFree(currentProcHeap, 0, _strBuff);

		if (_wideBuff != NULL)
			HeapFree(currentProcHeap, 0, _wideBuff);

		_charCount = 0;
		_isInitialized = false;
		_isWide = false;
	}

	// Methods.
	void WuString::Initialize(const size_t charCount, BOOL isWide = TRUE)
	{
		PVOID currentProcHeap = GetProcessHeap();
		if (isWide)
		{
			_charCount = charCount + 1;
			_wideBuff = static_cast<LPWSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, _charCount * 2));
			_isInitialized = true;
			_isWide = true;
		}
		else
		{
			_charCount = charCount + 1;
			_strBuff = static_cast<LPSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, _charCount));
			_isInitialized = true;
			_isWide = false;
		}
	}

	const size_t WuString::Length()
	{
		if (!_isInitialized)
			throw "String is not initialized!";

		// Also update the char count.
		if (_isWide)
			_charCount = wcslen(_wideBuff);
		else
			_charCount = strlen(_strBuff);

		return _charCount;
	}

	const size_t WuString::Length() const
	{
		if (!_isInitialized)
			throw "String is not initialized!";

		// If it's constant, the char count was not modified by the application.
		return _charCount;
	}

	LPSTR WuString::GetBuffer()
	{
		if (!_isInitialized || _isWide)
			throw "String is not initialized!";

		return _strBuff;
	}

	LPSTR WuString::GetBuffer() const
	{
		if (!_isInitialized || _isWide)
			throw "String is not initialized!";

		return _strBuff;
	}

	LPWSTR WuString::GetWideBuffer()
	{
		if (!_isInitialized || !_isWide)
			throw "String is not initialized!";

		return _wideBuff;
	}

	LPWSTR WuString::GetWideBuffer() const
	{
		if (!_isInitialized || !_isWide)
			throw "String is not initialized!";

		return _wideBuff;
	}

	void WuString::Format(const LPSTR format, ...)
	{
		if (format == NULL)
			throw "Format string cannot be null.";

		PVOID currentProcHeap = GetProcessHeap();
		va_list args;

		va_start(args, format);
		_charCount = _vscprintf(format, args);
		_charCount++;

		if (_strBuff != NULL && _isInitialized)
			HeapFree(currentProcHeap, 0, _strBuff);

		_strBuff = static_cast<LPSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, _charCount));
		if (_strBuff != NULL)
		{
			vsprintf_s(_strBuff, _charCount, format, args);
			_isInitialized = true;
		}
	}

	void WuString::Format(const LPWSTR format, ...)
	{
		if (format == NULL)
			throw "Format string cannot be null.";

		PVOID currentProcHeap = GetProcessHeap();
		va_list args;

		va_start(args, format);
		_charCount = _vscwprintf(format, args);
		_charCount++;

		UINT byteSize = _charCount * 2;
		if (_wideBuff != NULL && _isInitialized)
			HeapFree(currentProcHeap, 0, _wideBuff);

		_wideBuff = static_cast<LPWSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, byteSize));
		if (_wideBuff != NULL)
		{
			vswprintf_s(_wideBuff, _charCount, format, args);
			_isInitialized = true;
		}
	}

	void WuString::Remove(size_t index, size_t count)
	{
		if (!_isInitialized)
			throw "String is not initialized!";

		if (count >= _charCount)
			throw "Count can't be greater than the string length.";

		if (index < 0 || index + count > _charCount - 2)
			throw "Index outside of string boundaries.";

		PVOID currentProcHeap = GetProcessHeap();
		size_t newCharCount = _charCount - count;
		if (_isWide)
		{
			LPWSTR newBuff = static_cast<LPWSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, newCharCount * 2));
			if (newBuff != NULL)
			{
				size_t newLastIndex = 0;
				for (size_t i = 0; i < _charCount - 1; i++)
				{
					if (i < index || i > index + count)
					{
						newBuff[newLastIndex] = _wideBuff[i];
						newLastIndex++;
					}
				}
			}
		}
		else
		{
			LPSTR newBuff = static_cast<LPSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, newCharCount * 2));
			if (newBuff != NULL)
			{
				size_t newLastIndex = 0;
				for (size_t i = 0; i < _charCount - 1; i++)
				{
					if (i < index || i > index + count)
					{
						newBuff[newLastIndex] = _strBuff[i];
						newLastIndex++;
					}
				}
			}
		}

		_charCount = newCharCount;
	}

	BOOL WuString::Contains(const CHAR character) const
	{
		if (!_isInitialized)
			throw "String is not initialized!";

		if (strrchr(_strBuff, character))
			return TRUE;

		return FALSE;
	}

	BOOL WuString::Contains(const WCHAR character) const
	{
		if (!_isInitialized)
			throw "String is not initialized!";

		if (wcsrchr(_wideBuff, character))
			return TRUE;

		return FALSE;
	}

	BOOL WuString::EndsWith(const CHAR character) const
	{
		if (!_isInitialized)
			throw "String is not initialized!";

		// 0-based array plus /0.
		if (_strBuff[_charCount - 2] == character)
			return TRUE;

		return FALSE;
	}

	BOOL WuString::EndsWith(const WCHAR character) const
	{
		if (!_isInitialized)
			throw "String is not initialized!";

		// 0-based array plus /0.
		if (_wideBuff[_charCount - 2] == character)
			return TRUE;

		return FALSE;
	}

	// Assignment operator.
	void WuString::operator= (const LPSTR other)
	{
		if (other == NULL)
			this->WuString::WuString();
		else
		{
			PVOID currentProcHeap = GetProcessHeap();
			_charCount = strlen(other) + 1;

			if (_strBuff != NULL && _isInitialized)
				HeapFree(currentProcHeap, 0, _strBuff);

			_strBuff = static_cast<LPSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, _charCount));
			if (_strBuff != NULL)
			{
				strcpy_s(_strBuff, _charCount, other);
				_isInitialized = true;
				_isWide = false;
			}
		}
	}

	void WuString::operator= (const LPWSTR other)
	{
		if (other == NULL)
			this->WuString::WuString();
		else
		{
			PVOID currentProcHeap = GetProcessHeap();
			_charCount = wcslen(other) + 1;

			size_t byteSize = _charCount * 2;
			if (_wideBuff != NULL && _isInitialized)
				HeapFree(currentProcHeap, 0, _wideBuff);

			_wideBuff = static_cast<LPWSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, byteSize));
			if (_wideBuff != NULL)
			{
				wcscpy_s(_wideBuff, _charCount, other);
				_isInitialized = true;
				_isWide = true;
			}
		}
	}

	void WuString::operator= (const WuString& other)
	{
		if (other._isInitialized)
		{
			PVOID currentProcHeap = GetProcessHeap();

			if (other._isWide)
			{
				_charCount = wcslen(other._wideBuff) + 1;

				size_t byteSize = _charCount * 2;
				if (_wideBuff != NULL && _isInitialized)
					HeapFree(currentProcHeap, 0, _wideBuff);

				_wideBuff = static_cast<LPWSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, byteSize));
				if (_wideBuff != NULL)
				{
					wcscpy_s(_wideBuff, _charCount, other._wideBuff);
					_isInitialized = true;
					_isWide = true;
				}
			}
			else
			{
				_charCount = strlen(other._strBuff) + 1;

				if (_strBuff != NULL && _isInitialized)
					HeapFree(currentProcHeap, 0, _strBuff);

				_strBuff = static_cast<LPSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, _charCount));
				if (_strBuff != NULL)
				{
					strcpy_s(_strBuff, _charCount, other._strBuff);
					_isInitialized = true;
					_isWide = false;
				}
			}
		}
		else
			this->WuString::WuString();
	}

	// Compound assignment operator.
	void WuString::operator+=(const LPSTR other)
	{
		if (other != NULL)
		{
			PVOID currentProcHeap = GetProcessHeap();

			size_t newCharCount = strlen(other) + 1;
			_charCount += newCharCount;

			LPSTR newBuff = static_cast<LPSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, _charCount));
			if (newBuff != NULL)
			{
				if (_strBuff != NULL && _isInitialized)
				{
					strcpy_s(newBuff, _charCount, _strBuff);
					HeapFree(currentProcHeap, 0, _strBuff);
				}

				_strBuff = newBuff;
				strcat_s(_strBuff, _charCount, other);
			}
		}
	}

	void WuString::operator+=(const LPWSTR other)
	{
		if (other != NULL)
		{
			PVOID currentProcHeap = GetProcessHeap();

			size_t newCharCount = wcslen(other) + 1;
			_charCount += newCharCount;
			size_t byteSize = _charCount * 2;

			LPWSTR newBuff = static_cast<LPWSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, byteSize));
			if (newBuff != NULL)
			{
				if (_wideBuff != NULL && _isInitialized)
				{
					wcscpy_s(newBuff, _charCount, _wideBuff);
					HeapFree(currentProcHeap, 0, _wideBuff);
				}

				_wideBuff = newBuff;
				wcscat_s(_wideBuff, _charCount, other);
			}
		}
	}

	void WuString::operator+=(const WuString& other)
	{
		if (other._isInitialized)
		{
			PVOID currentProcHeap = GetProcessHeap();

			if (other._isWide)
			{
				size_t newCharCount = wcslen(other._wideBuff) + 1;
				_charCount += newCharCount;
				size_t byteSize = _charCount * 2;

				LPWSTR newBuff = static_cast<LPWSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, byteSize));
				if (newBuff != NULL)
				{
					if (_wideBuff != NULL && _isInitialized)
					{
						wcscpy_s(newBuff, _charCount, _wideBuff);
						HeapFree(currentProcHeap, 0, _wideBuff);
					}

					_wideBuff = newBuff;
					wcscat_s(_wideBuff, _charCount, other._wideBuff);
				}
			}
			else
			{
				size_t newCharCount = strlen(other._strBuff) + 1;
				_charCount += newCharCount;

				LPSTR newBuff = static_cast<LPSTR>(HeapAlloc(currentProcHeap, HEAP_ZERO_MEMORY, _charCount));
				if (newBuff != NULL)
				{
					if (_strBuff != NULL && _isInitialized)
					{
						strcpy_s(newBuff, _charCount, _strBuff);
						HeapFree(currentProcHeap, 0, _strBuff);
					}

					_strBuff = newBuff;
					strcat_s(_strBuff, _charCount, other._strBuff);
				}
			}
		}
	}

	// Equality operator.
	BOOL WuString::operator==(const LPSTR other) const
	{
		BOOL result;

		if (_isInitialized)
		{
			if (_isWide)
				result = FALSE;
			else
				if (other == NULL)
					result = FALSE;
				else
					result = _stricmp(_strBuff, other);
		}
		else
		{
			if (other == NULL)
				result = TRUE;
			else
				result = FALSE;
		}

		return result;
	}

	BOOL WuString::operator==(const LPWSTR other) const
	{
		BOOL result;

		if (_isInitialized)
		{
			if (!_isWide)
				result = FALSE;
			else
				if (other == NULL)
					result = FALSE;
				else
					result = wcscmp(_wideBuff, other);
		}
		else
		{
			if (other == NULL)
				result = TRUE;
			else
				result = FALSE;
		}

		return result;
	}

	BOOL WuString::operator==(const WuString& other) const
	{
		BOOL result;

		if (_isInitialized)
		{
			if (other._isInitialized)
			{
				if (_isWide)
					if (other._isWide)
						result = wcscmp(_wideBuff, other._wideBuff);
					else
						result = FALSE;
				else
					if (other._isWide)
						result = FALSE;
					else
						result = _stricmp(_strBuff, other._strBuff);
			}
			else
				result = FALSE;
		}
		else
		{
			if (other._isInitialized)
				result = FALSE;
			else
				result = TRUE;
		}

		return result;
	}

	// Inequality operator.
	BOOL WuString::operator!=(const LPSTR other) const
	{
		return !(*this == other);
	}

	BOOL WuString::operator!=(const LPWSTR other) const
	{
		return !(*this == other);
	}

	BOOL WuString::operator!=(const WuString& other) const
	{
		return !(*this == other);
	}
}