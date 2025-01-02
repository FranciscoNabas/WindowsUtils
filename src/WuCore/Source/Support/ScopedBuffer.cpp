#include "../../pch.h"

#include "../../Headers/Support/ScopedBuffer.h"

namespace WindowsUtils::Core
{
	HANDLE ScopedBuffer::s_currentProcessHeap = GetProcessHeap();

	ScopedBuffer::ScopedBuffer()
		: m_size{ }, m_memory{ nullptr }, m_isFreed{ false } { }

	ScopedBuffer::ScopedBuffer(const ScopedBuffer& other)
	{
		Init(other.m_size);
		memcpy_s(m_memory, m_size, other.m_memory, other.m_size);
	}

	ScopedBuffer::ScopedBuffer(ScopedBuffer&& other) noexcept
		: m_size{ other.m_size }, m_memory{ other.m_memory }, m_isFreed{ other.m_isFreed }
	{
		other.m_size = 0;
		other.m_memory = nullptr;
		other.m_isFreed = true;
	}

	ScopedBuffer::ScopedBuffer(int size)
	{
		_WU_ASSERT(size >= 0, L"Size can't be smaller than zero!");
		Init(static_cast<__uint64>(size));
	}

	ScopedBuffer::ScopedBuffer(ULONG size)
	{
		Init(static_cast<__uint64>(size));
	}

	ScopedBuffer::ScopedBuffer(__int64 size)
	{
		_WU_ASSERT(size >= 0, L"Size can't be smaller than zero!");
		Init(static_cast<__uint64>(size));
	}

	ScopedBuffer::ScopedBuffer(__uint64 size)
	{
		Init(size);
	}

	ScopedBuffer::~ScopedBuffer()
	{
		if (m_memory && !m_isFreed) {
			HeapFree(s_currentProcessHeap, 0, m_memory);
			m_memory = nullptr;
			m_isFreed = true;
		}

		m_size = 0;
	}

	ScopedBuffer& ScopedBuffer::operator =(ScopedBuffer&& other) noexcept
	{
		m_size     = other.m_size;
		m_memory   = other.m_memory;
		m_isFreed  = other.m_isFreed;

		other.m_size     = 0;
		other.m_memory   = nullptr;
		other.m_isFreed  = true;

		return *this;
	}

	void* ScopedBuffer::Get() { return m_memory; }
	const void* ScopedBuffer::Get() const { return m_memory; }
	const __uint64 ScopedBuffer::Size() const { return m_size; }

	void ScopedBuffer::Resize(int newSize)
	{
		_WU_ASSERT(newSize >= 0, L"New size can't be smaller than zero!");
		Resize(static_cast<__uint64>(newSize));
	}

	void ScopedBuffer::Resize(ULONG newSize)
	{
		Resize(static_cast<__uint64>(newSize));
	}

	void ScopedBuffer::Resize(__int64 newSize)
	{
		_WU_ASSERT(newSize >= 0, L"New size can't be smaller than zero!");
		Resize(static_cast<__uint64>(newSize));
	}

	void ScopedBuffer::Resize(__uint64 newSize)
	{
		if (!m_memory) {
			Init(newSize);
			return;
		}

		if (m_isFreed)
			_WU_RAISE_COR_EXCEPTION_WMESS(COR_E_INVALIDOPERATION, L"Resize", WriteErrorCategory::InvalidOperation, L"The Scoped Buffer was freed.");

		m_memory = HeapReAlloc(s_currentProcessHeap, HEAP_ZERO_MEMORY, m_memory, newSize);
		if (!m_memory)
			_WU_RAISE_COR_EXCEPTION(COR_E_OUTOFMEMORY, L"HeapAlloc", WriteErrorCategory::LimitsExceeded);

		m_size = newSize;
	}

	void ScopedBuffer::Init(__uint64 size)
	{
		m_memory = nullptr;
		m_memory = HeapAlloc(s_currentProcessHeap, HEAP_ZERO_MEMORY, size);
		if (!m_memory)
			_WU_RAISE_COR_EXCEPTION(COR_E_OUTOFMEMORY, L"HeapAlloc", WriteErrorCategory::LimitsExceeded);

		m_size = size;
		m_isFreed = false;
	}
}