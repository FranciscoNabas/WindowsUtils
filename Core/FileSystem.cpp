#include "pch.h"

#include "FileSystem.h"

namespace WindowsUtils::Core
{
	DWORD FileSystem::CreateFolderTree(const WWuString& path)
	{
		DWORD result = ERROR_SUCCESS;

		if (CheckDirectoryExists(path))
			return result;

		WWuString parent = path;
		if (!parent.Contains('\\'))
			return ERROR_INVALID_DRIVE;

		result = CreateFolderTree(parent);
		if (result != ERROR_SUCCESS)
			return result;

		if (!CreateDirectoryW(path.GetBuffer(), NULL))
		{
			result = GetLastError();
			if (result == ERROR_ALREADY_EXISTS)
				result = ERROR_SUCCESS;
		}

		return  result;
	}
	
	BOOL FileSystem::CheckDirectoryExists(const WWuString& path)
	{
		DWORD result = GetFileAttributes(path.GetBuffer());
		if (result == INVALID_FILE_ATTRIBUTES)
			return FALSE;

		return (result & FILE_ATTRIBUTE_DIRECTORY) > 0;
	}

	void FileSystem::TrimEndingDirectorySeparator(WWuString& path)
	{
		if (path.EndsWith('\\'))
			path.Remove(path.Length() - 1, 1);
	}
}