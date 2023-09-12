#pragma once
#pragma unmanaged

#include "MemoryManagement.h"
#include "Utilities.h"
#include "String.h"
#include "Common.h"
#include "SafeHandle.h"

namespace WindowsUtils::Core
{
	typedef enum _FS_OBJECT_TYPE
	{
		Directory,
		File
	} FS_OBJECT_TYPE;

	typedef struct _FS_INFO
	{
		FS_OBJECT_TYPE Type;
		WWuString Name;
		WWuString FullName;
		__uint64 Length;
	} FS_INFO, *PFS_INFO;

	extern "C" public class __declspec(dllexport) IO
	{
	public:
		static WuResult CreateFolderTree(const WWuString& path);
		static BOOL CheckDirectoryExists(const WWuString& path);
		static void TrimEndingDirectorySeparator(WWuString& path);
		static void SplitPath(const WWuString& path, WWuString& directory, WWuString& fileName);
		static void CreatePath(wuvector<WWuString> strVector, WWuString& path);
		static void SetFileAttributesAndDate(const WWuString& filePath, USHORT date, USHORT time, USHORT attributes);
		static WuResult AppendTextToFile(const HANDLE hFile, const WWuString& textS);
		static __uint64 GetFileSize(const WWuString& filePath);
		static wuvector<FS_INFO> EnumerateFileSystemInfo(WWuString& path);
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

	class AbstractPathTree
	{
	public:
		WWuString RootPath;
		DWORD FileCount;
		DWORD DirectoryCount;
		__uint64 TotalLength;
		typedef struct _AptEntry
		{
			FS_OBJECT_TYPE Type;
			WWuString Name;
			WWuString RelativePath;
			WWuString FullPath;
			__uint64 Length;

			_AptEntry(const WWuString& name, const WWuString& filePath, const WWuString& rootPath, __uint64 length, FS_OBJECT_TYPE type);
			~_AptEntry();

		} AptEntry;

		AbstractPathTree();
		AbstractPathTree(const AbstractPathTree& other);
		~AbstractPathTree();

		void PushEntry(const AptEntry& entry);
		wuvector<AptEntry>& GetApt();

		AbstractPathTree& operator=(const AbstractPathTree& other);

	private:
		wuvector<AptEntry> m_apt;
	};
}