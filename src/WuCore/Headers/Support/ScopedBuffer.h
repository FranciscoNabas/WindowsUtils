#pragma once

#include "Assertion.h"
#include "Expressions.h"
#include "WuException.h"

namespace WindowsUtils::Core
{
	/// <summary>
	/// Represents a generic buffer whose memory gets freed once it goes out of scope.
	/// </summary>
	/// <remarks>
	/// In contrast to smart pointers this buffer doesn't have a defined type to wrap.
	/// It's useful when calling native functions that the buffer return type depends on the parameters.
	/// </remarks>
	class ScopedBuffer
	{
	public:
		ScopedBuffer();
		ScopedBuffer(const ScopedBuffer& other);
		ScopedBuffer(ScopedBuffer&& other) noexcept;

		ScopedBuffer(int size);
		ScopedBuffer(ULONG size);
		ScopedBuffer(__int64 size);
		ScopedBuffer(__uint64 size);
		~ScopedBuffer();

		void* Get();
		const void* Get() const;
		const __uint64 Size() const;

		void Resize(int newSize);
		void Resize(ULONG newSize);
		void Resize(__int64 newSize);
		void Resize(__uint64 newSize);

		ScopedBuffer& operator =(ScopedBuffer&& other) noexcept;

	private:
		static HANDLE s_currentProcessHeap;

		bool m_isFreed;
		void* m_memory;
		__uint64 m_size;

		void Init(__uint64 size);
	};
}