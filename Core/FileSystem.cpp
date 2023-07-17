#include "pch.h"

#include "FileSystem.h"

namespace WindowsUtils::Core
{
	DWORD FileSystem::CreateFolderTree(const LPWSTR& lpszPath)
	{
		DWORD result = ERROR_SUCCESS;

		if (CheckDirectoryExists(lpszPath))
			return result;

		WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

		size_t pathLen = wcslen(lpszPath) + 1;
		LPWSTR lpszParent = (LPWSTR)MemoryManager.Allocate(pathLen * 2);
		wcscpy_s(lpszParent, pathLen, lpszPath);

		LPWSTR lpszPos = wcsrchr(lpszParent, '\\');
		if (!lpszPos)
			return ERROR_INVALID_DRIVE;

		result = CreateFolderTree(lpszParent);
		if (result != ERROR_SUCCESS)
		{
			MemoryManager.Free(lpszParent);
			return result;
		}

		if (!CreateDirectory(lpszPath, NULL))
		{
			result = GetLastError();
			if (result == ERROR_ALREADY_EXISTS)
				result = ERROR_SUCCESS;
		}

		MemoryManager.Free(lpszParent);

		return  result;
	}
	
	BOOL FileSystem::CheckDirectoryExists(const LPWSTR& lpszDirName)
	{
		DWORD result = GetFileAttributes(lpszDirName);
		if (result == INVALID_FILE_ATTRIBUTES)
			return FALSE;

		return (result & FILE_ATTRIBUTE_DIRECTORY) > 0;
	}

	void FileSystem::TrimEndingDirectorySeparator(const LPWSTR& lpszPath)
	{
		if (EndsWith(lpszPath, L"\\"))
		{
			size_t strLen = wcslen(lpszPath);
			lpszPath[strLen - 1] = 0;
		}
	}
}