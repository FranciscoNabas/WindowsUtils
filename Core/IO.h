#pragma once
#pragma unmanaged

#include "MemoryManagement.h"
#include "Utilities.h"
#include "String.h"
#include "Common.h"

namespace WindowsUtils::Core
{
	extern "C" public class __declspec(dllexport) IO
	{
	public:
		DWORD CreateFolderTree(const WWuString& path);
		BOOL CheckDirectoryExists(const WWuString& path);
		void TrimEndingDirectorySeparator(WWuString& path);
	};

	class MemoryMappedFile {
	public:
		const PVOID data() const;
		const __uint64 size() const;

		MemoryMappedFile(const WWuString& filePath);
		~MemoryMappedFile();

	private:
		PVOID _view;
		__uint64 _length;
		HANDLE _hFile;
		HANDLE _mappedFile;
	};
}