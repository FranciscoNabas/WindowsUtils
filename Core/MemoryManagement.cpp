#include "pch.h"

#include "MemoryManagement.h"

namespace WindowsUtils::Core
{
	// Simple allocator definition.
	template <class T>
	WuAllocator<T>::WuAllocator(SIZE_T size)
	{
		_processHeap = GetProcessHeap();
		_buffer = HeapAlloc(_processHeap, HEAP_ZERO_MEMORY, size)
	}

	template <class T>
	WuAllocator<T>::~WuAllocator()
	{
		HeapFree(_processHeap, 0, _buffer);
	}

	template <class T>
	T* WuAllocator<T>::Get()
	{
		return static_cast<T*>(buffer);
	}

	// Memory management.
	WuMemoryManagement& WuMemoryManagement::GetManager()
	{
		static WuMemoryManagement instance;

		return instance;
	}

	PVOID WuMemoryManagement::Allocate(size_t size)
	{
		PVOID block = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
		if (NULL == block)
			throw std::system_error(std::error_code(static_cast<int>(ERROR_NOT_ENOUGH_MEMORY), std::generic_category()));

		MemoryList.push_back(block);

		return block;
	}

	VOID WuMemoryManagement::Free(PVOID block)
	{
		if (IsRegistered(block))
		{
			std::vector<PVOID>::iterator it;
			for (it = MemoryList.begin(); it != MemoryList.end(); it++)
			{
				if (*it == block)
				{
					HeapFree(GetProcessHeap(), NULL, block);
					MemoryList.erase(it);
					break;
				}
			}
		}
	}

	BOOL WuMemoryManagement::IsRegistered(PVOID block)
	{
		if (NULL == block)
			return FALSE;

		for (PVOID regblock : MemoryList)
			if (regblock == block)
				return TRUE;

		return FALSE;
	}

	WuMemoryManagement::~WuMemoryManagement()
	{
		for (PVOID block : MemoryList)
			HeapFree(GetProcessHeap(), NULL, block);
	}
}