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
		static WuResult CreateFolderTree(const WWuString& path);
		static BOOL CheckDirectoryExists(const WWuString& path);
		static void TrimEndingDirectorySeparator(WWuString& path);
		static void SplitPath(const WWuString& path, WWuString& directory, WWuString& fileName);
		static void CreatePath(wuvector<WWuString> strVector, WWuString& path);
		static void SetFileAttributesAndDate(const WWuString& filePath, USHORT date, USHORT time, USHORT attributes);
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