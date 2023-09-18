#pragma once
#pragma unmanaged

#include "Utilities.h"
#include "Expressions.h"
#include "NtUtilities.h"
#include "Notification.h"
#include "AccessControl.h"

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

		typedef struct _WU_MODULE_INFO
		{
			WWuString ModuleName;
			WWuString ModulePath;
			struct
			{
				WWuString FileDescription;
				WWuString ProductName;
				WWuString FileVersion;
				WWuString CompanyName;

			} VersionInfo;

			_WU_MODULE_INFO() { }
			~_WU_MODULE_INFO() { }

		} WU_MODULE_INFO, *PWU_MODULE_INFO;

		typedef struct _PROCESS_MODULE_INFO
		{
			DWORD ProcessId;
			WWuString ImagePath;
			WWuString ImageFileName;
			WWuString CommandLine;
			size_t ModuleInfoCount;
			PWU_MODULE_INFO ModuleInfo;

			_PROCESS_MODULE_INFO()
				: ProcessId(0), ModuleInfo(NULL), ModuleInfoCount(0) { }

			~_PROCESS_MODULE_INFO() { }

			void SetModuleInfo(VectorArrayWrapper<WU_MODULE_INFO>& wrappedVec)
			{
				if (wrappedVec.Count() > 0) {
					ModuleInfoCount = wrappedVec.Count();
					ModuleInfo = new ProcessAndThread::WU_MODULE_INFO[ModuleInfoCount];

					const ProcessAndThread::WU_MODULE_INFO* modList = wrappedVec.Array();
					for (size_t i = 0; i < ModuleInfoCount; i++) {
						ModuleInfo[i].ModuleName = modList[i].ModuleName;
						ModuleInfo[i].ModulePath = modList[i].ModulePath;
						ModuleInfo[i].VersionInfo.FileDescription = modList[i].VersionInfo.FileDescription;
						ModuleInfo[i].VersionInfo.ProductName = modList[i].VersionInfo.ProductName;
						ModuleInfo[i].VersionInfo.FileVersion = modList[i].VersionInfo.FileVersion;
						ModuleInfo[i].VersionInfo.CompanyName = modList[i].VersionInfo.CompanyName;
					}
				}
			}

		} PROCESS_MODULE_INFO, *PPROCESS_MODULE_INFO;

		/*=========================================
		==		 Function identification		 ==
		===========================================*/

		// Get-ObjectHandle
		void GetProcessObjectHandle(wuvector<WU_OBJECT_HANDLE>* objectHandleList, wuvector<OBJECT_INPUT>* inputList, bool closeHandle);
		
		// Get-ProcessModule
		void GetProcessLoadedModuleInformation(wuvector<DWORD> processIdList, bool includeVersionInfo, bool suppressError, WuNativeContext* context);
	};

	/*=========================================
	==			 Utility functions			 ==
	===========================================*/

	WuResult GetProccessVersionInfo(const WWuString& imagePath, ProcessAndThread::VERSION_INFO_PROPERTY propertyName, WWuString& value);
	void CloseExtProcessHandle(HANDLE sourceProcess, const WWuString& objectName);
}