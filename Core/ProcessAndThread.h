#pragma once
#pragma unmanaged

#include "Utilities.h"
#include "NtUtilities.h"

namespace WindowsUtils::Core
{
	extern "C" public class __declspec(dllexport) ProcessAndThread
	{
	public:
		/*========================================
		==		Unmanaged object definition		==
		==========================================*/
		
		// Get-ObjectHandle
		typedef enum _VERSION_INFO_PROPERTY
		{
			FileDescription = 1U
			, ProductName = 2U
			, FileVersion = 3U
			, CompanyName = 4U

		} VERSION_INFO_PROPERTY;

		typedef struct _WU_OBJECT_HANDLE
		{
			LPWSTR		InputObject;								// Input object path. Helps tracking which handle belongs to which object, when querying multiple objects.
			DWORD		ProcessId;									// ID from the process owning the handle.
			LPWSTR		Name;										// Process image name. File base name.
			LPWSTR		ImagePath;									// Process image path.
			std::map<VERSION_INFO_PROPERTY, LPCWSTR> VersionInfo;	// Image version information.

			_WU_OBJECT_HANDLE() { }
			_WU_OBJECT_HANDLE(LPWSTR inpobj, LPWSTR name, DWORD pid, LPWSTR imgpath, std::map<VERSION_INFO_PROPERTY, LPCWSTR> verinfo)
				: InputObject(inpobj), Name(name), ProcessId(pid), ImagePath(imgpath), VersionInfo(verinfo) { }
		} WU_OBJECT_HANDLE, * PWU_OBJECT_HANDLE;

		/*=========================================
		==		 Function identification		 ==
		===========================================*/

		// Get-ObjectHandle
		DWORD GetProcessObjectHandle(std::vector<WU_OBJECT_HANDLE>& ppvecfho, std::vector<LPCWSTR>& reslist, BOOL closeHandle);
	};

	/*=========================================
	==			 Utility functions			 ==
	===========================================*/

	DWORD GetProccessVersionInfo(LPWSTR& imagepath, ProcessAndThread::VERSION_INFO_PROPERTY& propname, LPWSTR& value);
	DWORD CloseExtProcessHandle(HANDLE& hsourceproc, LPCWSTR& objname);
}