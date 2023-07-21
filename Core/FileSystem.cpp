#include "pch.h"

#include "FileSystem.h"

namespace WindowsUtils::Core
{
	DWORD FileSystem::CreateFolderTree(const WuString& path)
	{
		DWORD result = ERROR_SUCCESS;

		if (CheckDirectoryExists(path))
			return result;

		WuString parent = path;
		if (!parent.Contains('\\'))
			return ERROR_INVALID_DRIVE;

		result = CreateFolderTree(parent);
		if (result != ERROR_SUCCESS)
			return result;

		if (!CreateDirectoryW(path.GetWideBuffer(), NULL))
		{
			result = GetLastError();
			if (result == ERROR_ALREADY_EXISTS)
				result = ERROR_SUCCESS;
		}

		return  result;
	}
	
	BOOL FileSystem::CheckDirectoryExists(const WuString& path)
	{
		DWORD result = GetFileAttributesW(path.GetWideBuffer());
		if (result == INVALID_FILE_ATTRIBUTES)
			return FALSE;

		return (result & FILE_ATTRIBUTE_DIRECTORY) > 0;
	}

	void FileSystem::TrimEndingDirectorySeparator(WuString& path)
	{
		if (path.EndsWith('\\'))
			path.Remove(path.Length() - 1, 1);
	}
}