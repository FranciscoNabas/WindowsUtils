#pragma once
#pragma unmanaged

namespace WindowsUtils::Core
{
	template <class T>
	class WuAllocator
	{
	public:
		WuAllocator();
		WuAllocator(SIZE_T size);
		~WuAllocator();

		T* get();

		T* operator-> ();

	private:
		T* _buffer;
		PVOID _processHeap;
	};

	/*
	* Memory management class using the singleton design pattern.
	* Reference: https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
	*/

	class WuMemoryManagement
	{
	public:
		static WuMemoryManagement& GetManager();
		PVOID Allocate(size_t size);
		VOID Free(PVOID block);

	private:
		BOOL IsRegistered(PVOID block);
		std::vector<PVOID> MemoryList;

		WuMemoryManagement() { }
		~WuMemoryManagement();

	public:
		WuMemoryManagement(WuMemoryManagement const&) = delete;
		void operator=(WuMemoryManagement const&) = delete;
	};
}