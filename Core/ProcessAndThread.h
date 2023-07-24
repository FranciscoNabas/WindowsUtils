#pragma once
#pragma unmanaged

#include "Utilities.h"
#include "Expressions.h"
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
			FileDescription = 1U,
			ProductName = 2U,
			FileVersion = 3U,
			CompanyName = 4U

		} VERSION_INFO_PROPERTY;

		typedef struct _WU_OBJECT_HANDLE
		{
			WuString		InputObject;									// Input object path. Helps tracking which handle belongs to which object, when querying multiple objects.
			DWORD			ProcessId;										// ID from the process owning the handle.
			WuString		Name;											// Process image name. File base name.
			WuString		ImagePath;										// Process image path.
			wumap<VERSION_INFO_PROPERTY, const WuString> VersionInfo;		// Image version information.

			_WU_OBJECT_HANDLE()
				: InputObject(), ProcessId(0), Name(), ImagePath(), VersionInfo()
			{ }
			
			_WU_OBJECT_HANDLE(const WuString& inputObj, DWORD pid, const WuString& name, const WuString& imagePath, const wumap<VERSION_INFO_PROPERTY, const WuString>& versionInfo)
				: InputObject(inputObj), ProcessId(pid), Name(name), ImagePath(imagePath), VersionInfo(versionInfo)
			{ }

			~_WU_OBJECT_HANDLE() { }

		} WU_OBJECT_HANDLE, *PWU_OBJECT_HANDLE;

		/*=========================================
		==		 Function identification		 ==
		===========================================*/

		// Get-ObjectHandle
		DWORD GetProcessObjectHandle(wuvector<WU_OBJECT_HANDLE>* objectHandleList, wuvector<WuString>* resList, BOOL closeHandle);
	};

	/*=========================================
	==			 Utility functions			 ==
	===========================================*/

	DWORD GetProccessVersionInfo(const WuString& imagePath, ProcessAndThread::VERSION_INFO_PROPERTY propertyName, WuString& value);
	DWORD CloseExtProcessHandle(HANDLE sourceProcess, const WuString& objectName);
}