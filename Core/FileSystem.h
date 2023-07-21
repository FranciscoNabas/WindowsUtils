#pragma once
#pragma unmanaged

#include "MemoryManagement.h"
#include "Utilities.h"
#include "String.h"

namespace WindowsUtils::Core
{
	extern "C" public class __declspec(dllexport) FileSystem
	{
	public:
		DWORD CreateFolderTree(const WuString& path);
		BOOL CheckDirectoryExists(const WuString& path);
		void TrimEndingDirectorySeparator(WuString& path);
	};
}