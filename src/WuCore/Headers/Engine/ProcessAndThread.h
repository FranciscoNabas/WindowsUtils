#pragma once
#pragma unmanaged

#include "../Support/String.h"
#include "../Support/Expressions.h"
#include "../Support/Notification.h"

namespace WindowsUtils::Core
{
	/*
	*	~ Get-ObjectHandle
	*/

	enum class VersionInfoProperty
	{
		FileDescription = 1,
		ProductName = 2,
		FileVersion = 3,
		CompanyName = 4
	};

	enum class SupportedObjectType
	{
		FileSystem,
		Registry
	};

	typedef struct _OBJECT_INPUT
	{
		WWuString ObjectName;
		SupportedObjectType Type;

	} OBJECT_INPUT, * POBJECT_INPUT;

	typedef struct _WU_OBJECT_HANDLE
	{
		SupportedObjectType		Type;								// The path type. 'FileSystem' or 'Registry'.
		WWuString				InputObject;						// Input object path. Helps tracking which handle belongs to which object, when querying multiple objects.
		DWORD					ProcessId;							// ID from the process owning the handle.
		WWuString				Name;								// Process image name. File base name.
		WWuString				ImagePath;							// Process image path.
		wumap<VersionInfoProperty, const WWuString> VersionInfo;	// Image version information.

		_WU_OBJECT_HANDLE()
			: InputObject(), ProcessId(0), Name(), ImagePath(), VersionInfo()
		{ }

		_WU_OBJECT_HANDLE(SupportedObjectType type, const WWuString& inputObj, DWORD pid, const WWuString& name, const WWuString& imagePath, const wumap<VersionInfoProperty, const WWuString>& versionInfo)
			: Type(type), InputObject(inputObj), ProcessId(pid), Name(name), ImagePath(imagePath), VersionInfo(versionInfo)
		{ }

		~_WU_OBJECT_HANDLE() { }

	} WU_OBJECT_HANDLE, * PWU_OBJECT_HANDLE;

	/*
	*	~ Get-ProcessModule
	*/

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

	} WU_MODULE_INFO, * PWU_MODULE_INFO;

	typedef struct _PROCESS_MODULE_INFO
	{
		DWORD ProcessId;
		WWuString ImagePath;
		WWuString ImageFileName;
		WWuString CommandLine;
		wuvector<WU_MODULE_INFO> ModuleInfo;

		_PROCESS_MODULE_INFO()
			: ProcessId(0)
		{ }

		~_PROCESS_MODULE_INFO() { }

	} PROCESS_MODULE_INFO, * PPROCESS_MODULE_INFO;

	/*
	*	~ Main class
	*/

	extern "C" public class __declspec(dllexport) ProcessAndThread
	{
	public:
		// Get-ObjectHandle
		void GetProcessObjectHandle(wuvector<WU_OBJECT_HANDLE>* objectHandleList, wuvector<OBJECT_INPUT>* inputList, bool closeHandle);

		// Get-ProcessModule
		void GetProcessLoadedModuleInformation(wuvector<DWORD> processIdList, bool includeVersionInfo, bool suppressError, WuNativeContext* context);

		// Suspend-Process
		void SuspendProcess(DWORD processId, WuNativeContext* context);

		// Resume-Process
		void ResumeProcess(DWORD processId, WuNativeContext* context);
	};

	/*
	*	~ Utility functions
	*/

	void GetProccessVersionInfo(const WWuString& imagePath, VersionInfoProperty propertyName, WWuString& value);
	void CloseExtProcessHandle(HANDLE sourceProcess, const WWuString& objectName);
}