#pragma once
#pragma unmanaged

#include "String.h"
#include "Expressions.h"

namespace WindowsUtils::Core
{
	enum class FsObjectType
	{
		Directory,
		File
	};

	typedef struct _FS_INFO
	{
		FsObjectType Type;
		WWuString Name;
		WWuString FullName;
		__uint64 Length;
	} FS_INFO, * PFS_INFO;

	extern "C" public class __declspec(dllexport) IO
	{
	public:
		static void CreateFolderTree(const WWuString& path);
		static BOOL CheckDirectoryExists(const WWuString& path);
		static void TrimEndingDirectorySeparator(WWuString& path);
		static void SplitPath(const WWuString& path, WWuString& directory, WWuString& fileName);
		static void CreatePath(wuvector<WWuString> strVector, WWuString& path);
		static void SetFileAttributesAndDate(const WWuString& filePath, USHORT date, USHORT time, USHORT attributes);
		static void AppendTextToFile(const HANDLE hFile, const WWuString& textS);
		static __uint64 GetFileSize(const WWuString& filePath);
		static wuvector<FS_INFO> EnumerateFileSystemInfo(WWuString& path);
		static bool FileExists(const WWuString& filePath);
		static void GetFileDosPathFromDevicePath(WWuString& devicePath);
		static void GetFileDevicePathFromDosPath(WWuString& dosPath);
		static void WriteByteArrayToTempFile(BYTE* data, DWORD length, _Out_ WWuString& filePath);

	private:
		static wuvector<WWuString> SplitDriveStrings(const LPWSTR drives, const DWORD charCount);
	};

	class MemoryMappedFile
	{
	public:
		const PVOID data() const;
		const __uint64 size() const;

		MemoryMappedFile(const WWuString& filePath);
		~MemoryMappedFile();

	private:
		PVOID m_view;
		__uint64 m_length;
		HANDLE m_hFile;
		HANDLE m_mappedFile;
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
			FsObjectType Type;
			WWuString Name;
			WWuString RelativePath;
			WWuString FullPath;
			__uint64 Length;

			_AptEntry(const WWuString& name, const WWuString& filePath, const WWuString& rootPath, __uint64 length, FsObjectType type);
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