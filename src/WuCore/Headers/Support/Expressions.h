#pragma once
#pragma unmanaged

#include "../../pch.h"

#include <memory>
#include <queue>
#include <map>

using __uint64 = unsigned __int64;

template <typename T>
using wuqueue = std::queue<T>;

template <typename T>
using wusunique_queue = std::unique_ptr<std::queue<T>>;

template <typename T>
using wusshared_queue = std::shared_ptr<std::queue<T>>;

template <class T>
using wuvector = std::vector<T>;

template <class T>
using wusunique_vector = std::unique_ptr<std::vector<T>>;

template <class T>
using wusshared_vector = std::shared_ptr<std::vector<T>>;

template <class T, class U>
using wumap = std::map<T, U>;

template <class T, class U>
using wusunique_map = std::unique_ptr<std::map<T, U>>;

template <class T, class U>
using wusshared_map = std::shared_ptr<std::map<T, U>>;

template <class T>
[[nodiscard]] wusunique_vector<T> make_wusunique_vector() noexcept
{
	return std::make_unique<std::vector<T>>();
}

template <class T>
[[nodiscard]] wusshared_vector<T> make_wusshared_vector() noexcept
{
	return std::make_shared<std::vector<T>>();
}

template <class T, class U>
[[nodiscard]] wusunique_map<T, U> make_wusunique_map() noexcept
{
	return std::make_unique<std::map<T, U>>();
}

template <class T, class U>
[[nodiscard]] wusshared_map<T, U> make_wusshared_map() noexcept
{
	return std::make_shared<std::map<T, U>>();
}

/*
*	These are helpers to allow using smart pointers with custom allocators.
*	This is mainly so we can create smart pointers with custom sizes.
*/
struct HeapAllocFreer
{
	void operator()(void* p) const noexcept
	{
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
[[nodiscard]] wuunique_ptr<T> make_wuunique()
{
	return std::make_unique<T>();
}

template <class T, class... Args>
[[nodiscard]] wuunique_ptr<T> make_wuunique(Args&&... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template <class T>
[[nodiscard]] wuunique_ha_ptr<T> make_wuunique_ha(size_t size) noexcept
{
	return wuunique_ha_ptr<T>{
		static_cast<T*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size))
	};
}

template <class T, class... Args>
[[nodiscard]] wushared_ptr<T> make_wushared(Args&&... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

template <class T>
[[nodiscard]] wushared_ptr<T> make_wushared()
{
	return std::make_shared<T>();
}

template <class T>
[[nodiscard]] wushared_ptr<T> make_wushared_ha(size_t size) noexcept
{
	return wushared_ptr<T> {
		std::shared_ptr<T>(
			static_cast<T*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size)),
			HeapAllocFreer()
		)
	};
}