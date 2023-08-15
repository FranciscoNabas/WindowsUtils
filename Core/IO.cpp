#include "pch.h"

#include "IO.h"

namespace WindowsUtils::Core
{
	DWORD IO::CreateFolderTree(const WWuString& path)
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
	
	BOOL IO::CheckDirectoryExists(const WWuString& path)
	{
		DWORD result = GetFileAttributes(path.GetBuffer());
		if (result == INVALID_FILE_ATTRIBUTES)
			return FALSE;

		return (result & FILE_ATTRIBUTE_DIRECTORY) > 0;
	}

	void IO::TrimEndingDirectorySeparator(WWuString& path)
	{
		if (path.EndsWith('\\'))
			path.Remove(path.Length() - 1, 1);
	}

	MemoryMappedFile::MemoryMappedFile(const WWuString& filePath) 
		: _mappedFile(NULL), _view(NULL), _length(0) {

		_hFile = CreateFile(filePath.GetBuffer(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (_hFile == INVALID_HANDLE_VALUE)
			throw WuOsException(GetLastError());

		LARGE_INTEGER length;
		if (!GetFileSizeEx(_hFile, &length))
			throw WuOsException(GetLastError());

		_mappedFile = CreateFileMapping(_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
		if (_mappedFile == NULL)
			throw WuOsException(GetLastError());

		_view = MapViewOfFile(_mappedFile, FILE_MAP_READ, 0, 0, length.QuadPart);
		if (_view == NULL)
			throw WuOsException(GetLastError());

		_length = length.QuadPart;
	}

	MemoryMappedFile::~MemoryMappedFile() {
		UnmapViewOfFile(_view);
		CloseHandle(_mappedFile);
		CloseHandle(_hFile);
	}

	const PVOID MemoryMappedFile::data() const {
		return _view;
	}

	const __uint64 MemoryMappedFile::size() const {
		return _length;
	}
}