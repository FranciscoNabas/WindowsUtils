#pragma once
#pragma unmanaged

#include <array>
#include <memory>

#include <PathCch.h>
#include <Shlwapi.h>

#include "WuString.h"
#include "Expressions.h"
#include "SafeHandle.h"
#include "WuException.h"


namespace WindowsUtils::Core
{
	enum class FsObjectType
	{
		Directory,
		File,
	};

	typedef struct _FS_INFO
	{
		FsObjectType  Type;
		WWuString     Name;
		WWuString     FullName;
		__uint64      Length;

	} FS_INFO, * PFS_INFO;

	extern "C" public class __declspec(dllexport) IO
	{
	public:
		_NODISCARD static WWuString StripPath(const WWuString& path);
		_NODISCARD static WWuString RemoveFileSpec(const WWuString& path, const bool throwOnFail);
		static void CreateFolderTree(const WWuString& path);
		static BOOL CheckDirectoryExists(const WWuString& path);
		static void TrimEndingDirectorySeparator(WWuString& path);
		static void SplitPath(const WWuString& path, WWuString& directory, WWuString& fileName);
		static void CreatePath(WuList<WWuString> strList, WWuString& path);
		static void SetFileAttributesAndDate(const WWuString& filePath, USHORT date, USHORT time, USHORT attributes);
		static void AppendTextToFile(const HANDLE hFile, const WWuString& textS);
		static __uint64 GetFileSize(const WWuString& filePath);
		static WuList<FS_INFO> EnumerateFileSystemInfo(WWuString& path);
		static bool FileExists(const WWuString& filePath);
		static void GetFileDosPathFromDevicePath(WWuString& devicePath);
		static void GetFileDevicePathFromDosPath(WWuString& dosPath);
		static void WriteByteArrayToTempFile(BYTE* data, DWORD length, _Out_ WWuString& filePath);

		static constexpr bool ContainsInvalidFileNameChars(const WWuString& name)
		{
			return name.IndexOfAny(s_invalidFileNameChars, 41) != static_cast<size_t>(-1);
		}

		static constexpr bool ContainsInvalidPathNameChars(const WWuString& path)
		{
			return path.IndexOfAny(s_invalidPathNameChars, 33) != static_cast<size_t>(-1);
		}


	private:
		static constexpr wchar_t s_invalidFileNameChars[] = {
			L'\u0022', L'\u003C', L'\u003E', L'\u007C', L'\u0000', L'\u0001',
			L'\u0002', L'\u0003', L'\u0004', L'\u0005', L'\u0006', L'\u0007',
			L'\u0008', L'\u0009', L'\u000A', L'\u000B', L'\u000C', L'\u000D',
			L'\u000E', L'\u000F', L'\u0010', L'\u0011', L'\u0012', L'\u0013',
			L'\u0014', L'\u0015', L'\u0016', L'\u0017', L'\u0018', L'\u0019',
			L'\u001A', L'\u001B', L'\u001C', L'\u001D', L'\u001E', L'\u001F',
			L'\u003A', L'\u002A', L'\u003F', L'\u005C', L'\u002F'
		};

		static constexpr wchar_t s_invalidPathNameChars[] = {
			L'\u007C', L'\u0000', L'\u0001', L'\u0002', L'\u0003', L'\u0004',
			L'\u0005', L'\u0006', L'\u0007', L'\u0008', L'\u0009', L'\u000A',
			L'\u000B', L'\u000C', L'\u000D', L'\u000E', L'\u000F', L'\u0010',
			L'\u0011', L'\u0012', L'\u0013', L'\u0014', L'\u0015', L'\u0016',
			L'\u0017', L'\u0018', L'\u0019', L'\u001A', L'\u001B', L'\u001C',
			L'\u001D', L'\u001E', L'\u001F'
		};

		static WuList<WWuString> SplitDriveStrings(const LPWSTR drives, const DWORD charCount);
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
		std::vector<AptEntry>& GetApt();

		AbstractPathTree& operator=(const AbstractPathTree& other);

	private:
		std::vector<AptEntry> m_apt;
	};
}