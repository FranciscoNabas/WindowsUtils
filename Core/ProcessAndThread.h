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
			FileDescription = 1U,
			ProductName = 2U,
			FileVersion = 3U,
			CompanyName = 4U

		} VERSION_INFO_PROPERTY;

		typedef struct _WU_OBJECT_HANDLE
		{
			WuString		InputObject;									// Input object path. Helps tracking which handle belongs to which object, when querying multiple objects.
			ULONG_PTR		ProcessId;										// ID from the process owning the handle.
			WuString		Name;											// Process image name. File base name.
			WuString		ImagePath;										// Process image path.
			std::map<VERSION_INFO_PROPERTY, const WuString>* VersionInfo;	// Image version information.

			_WU_OBJECT_HANDLE()
			{
				InputObject = *new WuString();
				Name = *new WuString();
				ImagePath = *new WuString();
				VersionInfo = new std::map<VERSION_INFO_PROPERTY, const WuString>();
			}
			_WU_OBJECT_HANDLE(const WuString& inputObj, const WuString& name, DWORD pid, const WuString& imagePath, const std::map<VERSION_INFO_PROPERTY, const WuString>& versionInfo)
				: ProcessId(pid)
			{
				InputObject = *new WuString(inputObj);
				Name = *new WuString(name);
				ImagePath = *new WuString(imagePath);
				VersionInfo = new std::map<VERSION_INFO_PROPERTY, const WuString>();
			}

			~_WU_OBJECT_HANDLE()
			{
				delete &InputObject;
				delete &Name;
				delete &ImagePath;
				delete VersionInfo;
			}
		} WU_OBJECT_HANDLE, *PWU_OBJECT_HANDLE;

		/*=========================================
		==		 Function identification		 ==
		===========================================*/

		// Get-ObjectHandle
		DWORD GetProcessObjectHandle(std::vector<WU_OBJECT_HANDLE>& objectHandleList, std::vector<WuString>& reslist, BOOL closeHandle);
	};

	/*=========================================
	==			 Utility functions			 ==
	===========================================*/

	DWORD GetProccessVersionInfo(const WuString& imagePath, ProcessAndThread::VERSION_INFO_PROPERTY propertyName, WuString& value);
	DWORD CloseExtProcessHandle(HANDLE sourceProcess, const WuString& objectName);
}