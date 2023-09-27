#pragma once
#pragma unmanaged

class WuAllocator
{
public:
	WuAllocator() noexcept
	{
		m_processHeap = GetProcessHeap();
	}

	~WuAllocator() { }

	[[nodiscard]]
	inline void* allocate(_CRT_GUARDOVERFLOW const size_t size)
	{
		void* block = HeapAlloc(m_processHeap, HEAP_ZERO_MEMORY, size);
		if (block == NULL)
			return nullptr;

		return block;
	}

	inline void deallocate(void* const ptr) { HeapFree(m_processHeap, 0, ptr); }

private:
	void* m_processHeap;
};