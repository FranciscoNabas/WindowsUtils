#pragma once
#pragma unmanaged

#include "..\pch.h"

class WuAllocator
{
public:
	WuAllocator() noexcept
	{
		_process_heap = GetProcessHeap();
	}

	~WuAllocator() {}

	_NODISCARD_RAW_PTR_ALLOC inline void* allocate(_CRT_GUARDOVERFLOW const size_t size)
	{
		void* block = HeapAlloc(_process_heap, HEAP_ZERO_MEMORY, size);
		if (block == NULL)
			throw "Error not enough memory.";

		return block;
	}

	inline void deallocate(void* const _ptr)
	{
		HeapFree(_process_heap, 0, _ptr);
	}

private:
	void* _process_heap;
};

namespace WindowsUtils::Core
{
	/*
	*	These are helpers to allow using smart pointers with custom allocators.
	*	This is mainly so we can create smart pointers with custom sizes.
	*/
	struct HeapAllocFreer
	{
		void operator()(void* p) const noexcept {
			HeapFree(GetProcessHeap(), 0, p);
		}
	};

	template <class T>
	using wuunique_ptr = std::unique_ptr<T>;

	template <class T>
	using wuunique_ha_ptr = std::unique_ptr<T, HeapAllocFreer>;

	template <class T>
	using wushared_ptr = std::shared_ptr<T>;

	template <class T>
	_NODISCARD wuunique_ptr<T> make_wuunique() {
		return std::make_unique<T>();
	}

	template <class T, class... Args>
	_NODISCARD wuunique_ptr<T> make_wuunique(Args&&... args) {
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template <class T>
	_NODISCARD wuunique_ha_ptr<T> make_wuunique_ha(size_t size) noexcept {
		return wuunique_ha_ptr<T>{
			static_cast<T*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size))
		};
	}

	template <class T, class... Args>
	_NODISCARD wushared_ptr<T> make_wushared(Args&&... args) {
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template <class T>
	_NODISCARD wushared_ptr<T> make_wushared() {
		return std::make_shared<T>();
	}

	template <class T>
	_NODISCARD wushared_ptr<T> make_wushared_ha(size_t size) noexcept {
		return wushared_ptr<T> {
			std::shared_ptr<T>(
				static_cast<T*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size)),
				HeapAllocFreer()
			)
		};
	}
}