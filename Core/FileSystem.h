#pragma once
#pragma unmanaged

#include "Utilities.h"

namespace WindowsUtils::Core
{
	extern "C" public class __declspec(dllexport) FileSystem
	{
	public:
		DWORD CreateFolderTree(const LPWSTR& lpszPath);
		BOOL CheckDirectoryExists(const LPWSTR& lpszDirName);
		void TrimEndingDirectorySeparator(const LPWSTR& lpszPath);
	};
}