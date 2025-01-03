#pragma once
#pragma unmanaged

#include <unordered_map>
#include <memory>
#include <stdexcept>

#include <Shlwapi.h>
#include <Psapi.h>

#include "AccessControl.h"
#include "Utilities.h"

#include "../Support/WuString.h"
#include "../Support/Expressions.h"
#include "../Support/Notification.h"
#include "../Support/Nt/NtUtilities.h"
#include "../Support/IO.h"
#include "../Support/WuException.h"
#include "../Support/SafeHandle.h"


namespace WindowsUtils::Core
{
#pragma region Get-ObjectHandle

	/// <summary>
	/// An identifier for a file's version information.
	/// </summary>
	enum class VersionInfoProperty
	{
		FileDescription  = 1,
		ProductName      = 2,
		FileVersion      = 3,
		CompanyName      = 4,
	};

	/// <summary>
	/// Input for querying open handles to objects.
	/// </summary>
	struct GETHANDLE_INPUT
	{
		WWuString            ObjectName;
		SupportedHandleType  Type;

		GETHANDLE_INPUT(const WWuString& objName, const SupportedHandleType type);
	};

	/// <summary>
	/// Object open handle information.
	/// </summary>
	struct OBJECT_HANDLE
	{
		SupportedHandleType		Type;											// The path type. 'FileSystem' or 'Registry'.
		HANDLE					HandleValue;									// The handle value.
		WWuString				InputObject;									// Input object path. Helps tracking which handle belongs to which object, when querying multiple objects.
		DWORD					ProcessId;										// ID from the process owning the handle.
		WWuString				Name;											// Process image name. File base name.
		WWuString				ImagePath;										// Process image path.
		std::unordered_map<VersionInfoProperty, const WWuString> VersionInfo;	// Image version information.

		OBJECT_HANDLE(const SupportedHandleType type, const HANDLE value, const WWuString& inputObj, const DWORD pid);
	};

#pragma endregion

#pragma region Get-ProcessModule

	/// <summary>
	/// Contains information about a module.
	/// </summary>
	struct MODULE_INFORMATION
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

		MODULE_INFORMATION(const WWuString& name, const WWuString& path);
	};

	/// <summary>
	/// Information about modules loaded for a process.
	/// </summary>
	struct PROCESS_MODULE_INFO
	{
		DWORD                        ProcessId;
		WWuString                    ImagePath;
		WWuString                    ImageFileName;
		WWuString                    CommandLine;
		WuList<MODULE_INFORMATION>	 ModuleInfo;

		PROCESS_MODULE_INFO(const DWORD processId, const WWuString& imagePath, const WWuString& fileName, const WWuString& commandLine);
	};

#pragma endregion

#pragma region MainAPI
	
	class ProcessAndThread
	{
	public:
		/// <summary>
		/// Writes to the stream open handle information for an object.
		/// </summary>
		/// <cmdlet>Get-ObjectHandle</cmdlet>
		/// <param name="inputList">The object input list.</param>
		/// <param name="closeHandle">True to close open handles.</param>
		/// <param name="context">The native Cmdlet context.</param>
		static void GetProcessObjectHandle(const WuList<GETHANDLE_INPUT>& inputList, const bool closeHandle, const bool isAdmin, const WuNativeContext* context);

		/// <summary>
		/// Writes to the stream information about modules loaded for one or more processes.
		/// </summary>
		/// <cmdlet>Get-ProcessModule</cmdlet>
		/// <param name="processIdList">The input process id list.</param>
		/// <param name="includeVersionInfo">True to include file version information for the modules.</param>
		/// <param name="suppressError">Silently continues on errors.</param>
		/// <param name="context">The native Cmdlet context.</param>
		static void GetProcessLoadedModuleInformation(const WuList<DWORD>& processIdList, const bool includeVersionInfo, const bool suppressError, const WuNativeContext* context);

		/// <summary>
		/// Suspends a process.
		/// </summary>
		/// <cmdlet>Suspend-Process</cmdlet>
		/// <param name="processId">The process id.</param>
		/// <param name="context">The native Cmdlet context.</param>
		static void SuspendProcess(const DWORD processId, const WuNativeContext* context);

		/// <summary>
		/// Resumes a process.
		/// </summary>
		/// <cmdlet>Resume-Process</cmdlet>
		/// <param name="processId">The process id.</param>
		/// <param name="context">The native Cmdlet context.</param>
		static void ResumeProcess(const DWORD processId, const WuNativeContext* context);

		/// <summary>
		/// Starts a process as another user.
		/// </summary>
		/// <cmdlet>Start-ProcessAsUser</cmdlet>
		/// <param name="userName">The user account name.</param>
		/// <param name="domain">The user account domain name.</param>
		/// <param name="password">The user account password.</param>
		/// <param name="commandLine">The process command line.</param>
		/// <param name="titleBar">The title for the title bar.</param>
		static void RunAs(const WWuString& userName, const WWuString& domain, const WWuString& password, const WWuString& commandLine, const WWuString& titleBar);


		// Utilities.

		/// <summary>
		/// Gets version information property value for an image file.
		/// </summary>
		/// <param name="imagePath">The image path.</param>
		/// <param name="propertyName">The version information property name.</param>
		/// <returns>The version information property value.</returns>
		static WWuString GetProccessVersionInfo(const WWuString& imagePath, const VersionInfoProperty propertyName);
	};

#pragma endregion
}