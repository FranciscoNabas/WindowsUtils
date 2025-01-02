#pragma once
#pragma unmanaged

#include "AccessControl.h"
#include "Utilities.h"

#include "../Support/WuString.h"
#include "../Support/Expressions.h"
#include "../Support/Notification.h"
#include "../Support/Nt/NtUtilities.h"
#include "../Support/IO.h"
#include "../Support/WuException.h"
#include "../Support/Notification.h"
#include "../Support/SafeHandle.h"

#include <unordered_map>
#include <vector>
#include <memory>
#include <Shlwapi.h>
#include <Psapi.h>
#include <stdexcept>

namespace WindowsUtils::Core
{
	/*
	*	~ Get-ObjectHandle
	*/

	enum class VersionInfoProperty
	{
		FileDescription  = 1,
		ProductName      = 2,
		FileVersion      = 3,
		CompanyName      = 4
	};

	typedef struct _OBJECT_INPUT
	{
		WWuString            ObjectName;
		SupportedHandleType  Type;

	} OBJECT_INPUT, * POBJECT_INPUT;

	typedef struct _WU_OBJECT_HANDLE
	{
		SupportedHandleType		Type;											// The path type. 'FileSystem' or 'Registry'.
		WWuString				InputObject;									// Input object path. Helps tracking which handle belongs to which object, when querying multiple objects.
		DWORD					ProcessId;										// ID from the process owning the handle.
		WWuString				Name;											// Process image name. File base name.
		WWuString				ImagePath;										// Process image path.
		std::unordered_map<VersionInfoProperty, const WWuString> VersionInfo;	// Image version information.

		_WU_OBJECT_HANDLE();
		_WU_OBJECT_HANDLE(SupportedHandleType type, const WWuString& inputObj, DWORD pid, const WWuString& name,
			const WWuString& imagePath, const std::unordered_map<VersionInfoProperty, const WWuString>& versionInfo);

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

	} WU_MODULE_INFO, * PWU_MODULE_INFO;

	typedef struct _PROCESS_MODULE_INFO
	{
		DWORD                        ProcessId;
		WWuString                    ImagePath;
		WWuString                    ImageFileName;
		WWuString                    CommandLine;
		std::vector<WU_MODULE_INFO>  ModuleInfo;

		_PROCESS_MODULE_INFO();

	} PROCESS_MODULE_INFO, * PPROCESS_MODULE_INFO;


	/*
	*	~ Main class
	*/

	class ProcessAndThread
	{
	public:
		// Get-ObjectHandle
		static void GetProcessObjectHandle(const WuList<OBJECT_INPUT>& inputList, const bool closeHandle, WuList<WU_OBJECT_HANDLE>& output, const WuNativeContext* context);

		// Get-ProcessModule
		static void GetProcessLoadedModuleInformation(const WuList<DWORD>& processIdList, const bool includeVersionInfo, const bool suppressError, const WuNativeContext* context);

		// Suspend-Process
		static void SuspendProcess(const DWORD processId, const WuNativeContext* context);

		// Resume-Process
		static void ResumeProcess(const DWORD processId, const WuNativeContext* context);

		// Start-ProcessAsUser
		static void RunAs(const WWuString& userName, const WWuString& domain, WWuString& password, WWuString& commandLine, WWuString& titleBar);
	};


	/*
	*	~ Utility functions
	*/

	void GetProccessVersionInfo(const WWuString& imagePath, VersionInfoProperty propertyName, WWuString& value);
}