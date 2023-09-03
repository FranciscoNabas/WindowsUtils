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

		typedef enum _SUPPORTED_OBJECT_TYPE
		{
			FileSystem = 0,
			Registry = 1
		} SUPPORTED_OBJECT_TYPE;

		typedef struct _OBJECT_INPUT
		{
			WWuString ObjectName;
			SUPPORTED_OBJECT_TYPE Type;

		} OBJECT_INPUT, *POBJECT_INPUT;

		typedef struct _WU_OBJECT_HANDLE
		{
			SUPPORTED_OBJECT_TYPE	Type;								// The path type. 'FileSystem' or 'Registry'.
			WWuString				InputObject;						// Input object path. Helps tracking which handle belongs to which object, when querying multiple objects.
			DWORD					ProcessId;							// ID from the process owning the handle.
			WWuString				Name;								// Process image name. File base name.
			WWuString				ImagePath;							// Process image path.
			wumap<VERSION_INFO_PROPERTY, const WWuString> VersionInfo;	// Image version information.

			_WU_OBJECT_HANDLE()
				: InputObject(), ProcessId(0), Name(), ImagePath(), VersionInfo()
			{ }
			
			_WU_OBJECT_HANDLE(SUPPORTED_OBJECT_TYPE type, const WWuString& inputObj, DWORD pid, const WWuString& name, const WWuString& imagePath, const wumap<VERSION_INFO_PROPERTY, const WWuString>& versionInfo)
				: Type(type), InputObject(inputObj), ProcessId(pid), Name(name), ImagePath(imagePath), VersionInfo(versionInfo)
			{ }

			~_WU_OBJECT_HANDLE() { }

		} WU_OBJECT_HANDLE, *PWU_OBJECT_HANDLE;

		/*=========================================
		==		 Function identification		 ==
		===========================================*/

		// Get-ObjectHandle
		void GetProcessObjectHandle(wuvector<WU_OBJECT_HANDLE>* objectHandleList, wuvector<OBJECT_INPUT>* inputList, bool closeHandle);
	};

	/*=========================================
	==			 Utility functions			 ==
	===========================================*/

	WuResult GetProccessVersionInfo(const WWuString& imagePath, ProcessAndThread::VERSION_INFO_PROPERTY propertyName, WWuString& value);
	void CloseExtProcessHandle(HANDLE sourceProcess, const WWuString& objectName);
}