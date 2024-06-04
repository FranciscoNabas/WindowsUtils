#include "../../pch.h"

#include "../../Headers/Engine/ProcessAndThread.h"
#include "../../Headers/Engine/AccessControl.h"
#include "../../Headers/Engine/Utilities.h"
#include "../../Headers/Support/WuStdException.h"
#include "../../Headers/Support/Notification.h"
#include "../../Headers/Support/SafeHandle.h"

#include <Shlwapi.h>
#include <Psapi.h>
#include <stdexcept>

namespace WindowsUtils::Core
{
	/*
	*	~ Get-ObjectHandle
	*/

	void ProcessAndThread::GetProcessObjectHandle(
		wuvector<WU_OBJECT_HANDLE>& objectHandleList,	// The output object handle list.
		wuvector<OBJECT_INPUT>& inputList,				// The input list containing the object name and type.
		bool closeHandle,								// TRUE to ATTEMPT to close the handle(s).
		WuNativeContext* context
	)
	{
		// Listing all running processes, with process ID and image name.
		std::unordered_map<DWORD, WWuString> runningProcessMap;
		try {
			NtUtilities::ListRunningProcessesWithImageName(runningProcessMap);
		}
		catch (const WuStdException& ex) {
			ex.Cry(L"ErrorListingRunningProcesses", WriteErrorCategory::InvalidResult, L"", context);
			throw ex;
		}

		// Going for each input object.
		for (const OBJECT_INPUT& input : inputList) {
			wuvector<DWORD> processIdList;
			WWuString inputObject;
			
			// Getting a list of processes using the file system object or the registry key.
			try {
				switch (input.Type) {
					case SupportedHandleType::FileSystem:
					{
						inputObject = input.ObjectName;
						PathStripPath(inputObject.GetBuffer());
						NtUtilities::GetProcessUsingFile(input.ObjectName, processIdList);

						if (closeHandle) {
							WWuString devicePath { input.ObjectName };
							IO::GetFileDevicePathFromDosPath(devicePath);
							for (const DWORD pid : processIdList)
								NtUtilities::CloseExternalHandlesToFile(pid, devicePath);
						}
					} break;

					case SupportedHandleType::Registry:
					{
						inputObject = input.ObjectName.Split('\\').back();
						NtUtilities::GetProcessUsingKey(input.ObjectName, processIdList, runningProcessMap, closeHandle);
					} break;
				}
			}
			catch (const WuStdException& ex) {
				ex.Cry(L"ErrorListingProcessUsingObject", WriteErrorCategory::InvalidResult, input.ObjectName, context);
				continue;
			}

			// If 'closeHandle' we don't want to get process information.
			if (closeHandle)
				continue;

			// For each process ID found, get process information.
			for (DWORD pid : processIdList) {
				WU_OBJECT_HANDLE objHandle;

				objHandle.InputObject = inputObject;
				objHandle.ProcessId = pid;
				objHandle.Type = input.Type;

				try {
					// Attempting to get the image name from our map.
					bool getVersionInfo = false;
					if (auto iterator = runningProcessMap.find(pid); iterator != runningProcessMap.end()) {
						if (iterator->second.StartsWith(L"\\Device\\HarddiskVolume")) {

							// Converting NT path format to DOS path format.
							WWuString imageDosPath { iterator->second };
							IO::GetFileDosPathFromDevicePath(imageDosPath);

							objHandle.ImagePath = imageDosPath;
							objHandle.Name = imageDosPath;
							PathStripPath(objHandle.Name.GetBuffer());

							getVersionInfo = true;
						}
						else {
							objHandle.Name = iterator->second;

							// These processes are hosted in 'ntoskrnl.exe'.
							if (iterator->second == L"System" ||
								iterator->second == L"Secure System" ||
								iterator->second == L"Registry" ||
								iterator->second == L"Memory Compression") {

								GetEnvVariable(L"windir", objHandle.ImagePath);
								objHandle.ImagePath += L"\\System32\\ntoskrnl.exe";

								getVersionInfo = true;
							}
						}
					}
					// Process was not on the map. We try to get it once again.
					else {
						WWuString imagePath;
						NtUtilities::GetProcessImageName(objHandle.ProcessId, imagePath);

						if (imagePath.StartsWith(L"\\Device\\HarddiskVolume")) {
							IO::GetFileDosPathFromDevicePath(imagePath);

							objHandle.ImagePath = imagePath;
							objHandle.Name = imagePath;
							PathStripPath(objHandle.Name.GetBuffer());

							getVersionInfo = true;
						}
						else {
							objHandle.Name = imagePath;

							if (imagePath == L"System" ||
								imagePath == L"Secure System" ||
								imagePath == L"Registry" ||
								imagePath == L"Memory Compression") {

								GetEnvVariable(L"windir", objHandle.ImagePath);
								objHandle.ImagePath += L"\\System32\\ntoskrnl.exe";

								getVersionInfo = true;
							}
						}
					}

					if (getVersionInfo) {
						for (const VersionInfoProperty& versionInfo : {
							VersionInfoProperty::FileDescription,
							VersionInfoProperty::ProductName,
							VersionInfoProperty::FileVersion,
							VersionInfoProperty::CompanyName
							}) {
							WWuString value;
							GetProccessVersionInfo(objHandle.ImagePath, versionInfo, value);
							objHandle.VersionInfo.emplace(versionInfo, value);
						}
					}
				}
				catch (const WuStdException& ex) {
					ex.Cry(L"ErrorGettingProcessHandleInformation", WriteErrorCategory::InvalidResult, WWuString::Format(L"Process ID: %d", pid), context);
					continue;
				}
				
				objectHandleList.push_back(objHandle);
			}
		}
	}

	/*
	*	~ Get-ProcessModule
	*	https://learn.microsoft.com/windows/win32/psapi/enumerating-all-modules-for-a-process
	*/

	void ProcessAndThread::GetProcessLoadedModuleInformation(
		wuvector<DWORD>& processIdList,					// The process ID list.
		bool includeVersionInfo,						// True to include module version information.
		bool suppressError,								// Suppress error output. For when enumerating all processes.
		WuNativeContext* context						// The Cmdlet native representation.
	)
	{
		// SE_DEBUG_NAME is required to get the process command line.
		wuvector<WWuString> privList;
		privList.push_back(L"SeDebugPrivilege");
		AccessControl::AdjustCurrentTokenPrivilege(privList, SE_PRIVILEGE_ENABLED);

		// When listing the modules for multiple processes, we are going to have
		// repeated modules, and querying information about them every time implies
		// a cost. This map will be a global information cache.
		wumap<WWuString, WU_MODULE_INFO> moduleInfoCache;

		for (DWORD& processId : processIdList) {
			HANDLE hProcess = OpenProcess(
				PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
				FALSE,
				processId
			);

			if (hProcess == NULL) {
				if (!suppressError) {
					WuStdException stdException(GetLastError(), __FILEW__, __LINE__);
					stdException.Cry(
						L"ErrorOpeningHandleToProcess",
						WWuString::Format(L"Error opening handle to process ID '%d': %ws", processId, stdException.Message().GetBuffer()),
						WriteErrorCategory::OpenError,
						WWuString::Format(L"%d", processId),
						context
					);
				}

				continue;
			}

			// Getting process image file name.
			DWORD imgNameSize = MAX_PATH;
			WCHAR imgName[MAX_PATH];
			WWuString procImgName;
			WWuString procFullPath;
			if (QueryFullProcessImageName(hProcess, 0, imgName, &imgNameSize)) {
				procFullPath = WWuString(imgName);
				PathStripPath(imgName);
				procImgName = WWuString(imgName);
			}

			// Getting process command line.
			// Help from: https://stackoverflow.com/questions/6530565/getting-another-process-command-line-in-windows
			WWuString commandLine;
			try {
				NtUtilities::GetProcessCommandLine(hProcess, commandLine);
			}
			catch (const WuStdException& ex) {
				if (!suppressError) {
					WuStdException stdException(GetLastError(), __FILEW__, __LINE__);
					stdException.Cry(
						L"ErrorGettingProcessCommandLine",
						WWuString::Format(L"Failed getting command line for process ID '%d': %ws", processId, ex.Message().GetBuffer()),
						WriteErrorCategory::ReadError,
						WWuString::Format(L"%d", processId),
						context
					);
				}
			}

			// MS recommends using a big list, getting the right buffer size is
			// not as straight forward as with other functions.
			HMODULE moduleList[1024];
			DWORD returnBytes;
			if (!EnumProcessModules(hProcess, moduleList, sizeof(moduleList), &returnBytes)) {
				if (!suppressError) {
					WuStdException stdException(GetLastError(), __FILEW__, __LINE__);
					stdException.Cry(
						L"ErrorEnumeratingProcessModules",
						WWuString::Format(L"Error enumerating modules for process ID '%d': %ws", processId, stdException.Message().GetBuffer()),
						WriteErrorCategory::InvalidResult,
						WWuString::Format(L"%d", processId),
						context
					);
				}

				CloseHandle(hProcess);
				continue;
			}

			PROCESS_MODULE_INFO currentProcessInfo;
			currentProcessInfo.ProcessId = processId;
			currentProcessInfo.ImageFileName = procImgName;
			currentProcessInfo.ImagePath = procFullPath;
			currentProcessInfo.CommandLine = commandLine;

			for (size_t i = 0; i < (returnBytes / sizeof(HMODULE)); i++) {
				WCHAR modNameBuffer[MAX_PATH];
				if (GetModuleFileNameEx(hProcess, moduleList[i], modNameBuffer, MAX_PATH)) {

					// Checking if this module is already in the cache.
					WWuString moduleFileName(modNameBuffer);
					WU_MODULE_INFO moduleInfo;
					try {
						moduleInfo = moduleInfoCache.at(moduleFileName);
						currentProcessInfo.ModuleInfo.push_back(moduleInfo);
						continue;
					}
					catch (const std::out_of_range&) { }

					PathStripPath(moduleFileName.GetBuffer());

					moduleInfo.ModuleName = moduleFileName;
					moduleInfo.ModulePath = modNameBuffer;

					if (includeVersionInfo) {
						for (VersionInfoProperty versionInfo : {
							VersionInfoProperty::FileDescription,
								VersionInfoProperty::ProductName,
								VersionInfoProperty::FileVersion,
								VersionInfoProperty::CompanyName
						}) {
							WWuString value;
							GetProccessVersionInfo(modNameBuffer, versionInfo, value);

							switch (versionInfo) {
								case VersionInfoProperty::FileDescription:
									moduleInfo.VersionInfo.FileDescription = value;
									break;

								case VersionInfoProperty::ProductName:
									moduleInfo.VersionInfo.ProductName = value;
									break;

								case VersionInfoProperty::FileVersion:
									moduleInfo.VersionInfo.FileVersion = value;
									break;

								case VersionInfoProperty::CompanyName:
									moduleInfo.VersionInfo.CompanyName = value;
									break;
							}
						}
					}

					currentProcessInfo.ModuleInfo.push_back(moduleInfo);
					moduleInfoCache.emplace(moduleInfo.ModulePath, moduleInfo);
				}
			}

			// Writing to output.
			context->NativeWriteObject(&currentProcessInfo, WriteOutputType::ProcessModuleInfo);
			CloseHandle(hProcess);
		}

		// Never leave your process privileges escalated.
		AccessControl::AdjustCurrentTokenPrivilege(privList, SE_PRIVILEGE_DISABLED);
	}

	void ProcessAndThread::SuspendProcess(DWORD processId, WuNativeContext* context)
	{
		NTSTATUS status;
		wuvector<WWuString> privList;
		privList.push_back(L"SeDebugPrivilege");
		AccessControl::AdjustCurrentTokenPrivilege(privList, SE_PRIVILEGE_ENABLED);

		try {
			ProcessHandle hProcess(PROCESS_SUSPEND_RESUME, FALSE, processId);
			if (!NT_SUCCESS(status = NtFunctions::GetInstance()->NtSuspendProcess(hProcess))) {
				WuStdException ex { status, __FILEW__, __LINE__, CoreErrorType::NtError };
				ex.Cry(L"ErrorSuspendingProcess", WriteErrorCategory::ResourceUnavailable, WWuString::Format(L"%d", processId).GetBuffer(), context);

				return;
			}
		}
		catch (const WuStdException& ex) {
			ex.Cry(L"ErrorOpeningProcess", WriteErrorCategory::ResourceUnavailable, WWuString::Format(L"%d", processId).GetBuffer(), context);
			return;
		}

		// Never leave your process privileges escalated.
		AccessControl::AdjustCurrentTokenPrivilege(privList, SE_PRIVILEGE_DISABLED);
	}

	void ProcessAndThread::ResumeProcess(DWORD processId, WuNativeContext* context)
	{
		NTSTATUS status;
		wuvector<WWuString> privList;
		privList.push_back(L"SeDebugPrivilege");
		AccessControl::AdjustCurrentTokenPrivilege(privList, SE_PRIVILEGE_ENABLED);

		try {
			ProcessHandle hProcess(PROCESS_SUSPEND_RESUME, FALSE, processId);
			if (!NT_SUCCESS(status = NtFunctions::GetInstance()->NtResumeProcess(hProcess))) {
				WuStdException ex { status, __FILEW__, __LINE__, CoreErrorType::NtError };
				ex.Cry(L"ErrorResumingProcess", WriteErrorCategory::InvalidResult, WWuString::Format(L"%d", processId), context);

				return;
			}
		}
		catch (const WuStdException& ex) {
			ex.Cry(L"ErrorOpeningProcess", WriteErrorCategory::OpenError, WWuString::Format(L"%d", processId), context);
			return;
		}

		// Never leave your process privileges escalated.
		AccessControl::AdjustCurrentTokenPrivilege(privList, SE_PRIVILEGE_DISABLED);
	}

	/*========================================
	==		Utility function definition		==
	==========================================*/

	// Gets process image version information.
	void GetProccessVersionInfo(
		const WWuString& imagePath,				// The process image file path.
		VersionInfoProperty propName,			// The property name, or 'key' value.
		WWuString& value						// The return value.
	)
	{
		LPWORD codePage = NULL;
		LPWSTR desc = NULL;
		UINT len;

		WWuString propertyName;
		switch (propName) {
			case VersionInfoProperty::FileDescription:
				propertyName = L"FileDescription";
				break;
			case VersionInfoProperty::ProductName:
				propertyName = L"ProductName";
				break;
			case VersionInfoProperty::FileVersion:
				propertyName = L"FileVersion";
				break;
			case VersionInfoProperty::CompanyName:
				propertyName = L"CompanyName";
				break;
			default:
				break;
		}

		DWORD infoSize = GetFileVersionInfoSizeW(imagePath.GetBuffer(), NULL);
		wuunique_ha_ptr<void> buffer = make_wuunique_ha<void>(infoSize);

		if (infoSize > 0) {
			if (!GetFileVersionInfo(imagePath.GetBuffer(), NULL, infoSize, buffer.get()))
				throw WuStdException(GetLastError(), __FILEW__, __LINE__);

			if (!VerQueryValue(buffer.get(), L"\\VarFileInfo\\Translation", (LPVOID*)&codePage, &len))
				throw WuStdException(GetLastError(), __FILEW__, __LINE__);

			WWuString text = WWuString::Format(L"\\StringFileInfo\\%04x%04x\\", codePage[0], codePage[1]);
			text += propertyName;

			if (VerQueryValue(buffer.get(), text.GetBuffer(), (LPVOID*)&desc, &len)) {
				if (!WWuString::IsNullOrWhiteSpace(desc))
					value = desc;
				else
					value = L"";
			}
			else
				value = L"";
		}
		else
			value = L"";
	}

	/*
	*	~ Start-ProcessAsUser
	*
	*	For this function I tried reverse engineering
	*	'runas.exe' to the best of my ability (which is
	*	not much).
	*/
	void ProcessAndThread::RunAs(const WWuString& userName, const WWuString& domain, WWuString& password, WWuString& commandLine, WWuString& titleBar)
	{
		WORD processorArch;
		size_t bytesNeeded;
		SYSTEM_INFO sysInfo;

		LPPROC_THREAD_ATTRIBUTE_LIST attrList;

		wuunique_ptr<STARTUPINFOEX>startupInfo = make_wuunique<STARTUPINFOEX>();
		wuunique_ptr<PROCESS_INFORMATION>procInfo = make_wuunique<PROCESS_INFORMATION>();

		GetNativeSystemInfo(&sysInfo);
		switch (sysInfo.wProcessorArchitecture) {
			case 9:
				processorArch = -31132;
				break;
			case 5:
				processorArch = 452;
				break;
			case 12:
				processorArch = -21916;
				break;
			case 0:
				processorArch = 332;
				break;
			default:
				processorArch = 0xFFFF;
				break;
		}

		if (!InitializeProcThreadAttributeList(NULL, 1, 0, &bytesNeeded)) {
			DWORD lastError = GetLastError();
			if (lastError != ERROR_INSUFFICIENT_BUFFER) {
				password.SecureErase();
				throw WuStdException(lastError, __FILEW__, __LINE__);
			}
		}

		// This is an opaque structure, working with smart pointers would be a hassle.
		attrList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytesNeeded);
		if (attrList == NULL) {
			password.SecureErase();
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);
		}

		if (!InitializeProcThreadAttributeList(attrList, 1, 0, &bytesNeeded)) {
			password.SecureErase();
			DeleteProcThreadAttributeList(attrList);
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);
		}


		if (!UpdateProcThreadAttribute(attrList, 0, 0x00020019, &processorArch, 2, NULL, NULL)) {
			password.SecureErase();
			DeleteProcThreadAttributeList(attrList);
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);
		}

		WCHAR currentPath[MAX_PATH] = { 0 };
		GetCurrentDirectoryW(MAX_PATH, currentPath);

		startupInfo->lpAttributeList = attrList;
		startupInfo->StartupInfo.cb = 112;
		startupInfo->StartupInfo.lpTitle = titleBar.GetBuffer();

		if (!CreateProcessWithLogonW(
			userName.GetBuffer(),
			domain.GetBuffer(),
			password.GetBuffer(),
			LOGON_WITH_PROFILE,
			NULL,
			commandLine.GetBuffer(),
			CREATE_UNICODE_ENVIRONMENT,
			NULL,
			currentPath,
			&startupInfo->StartupInfo,
			procInfo.get()
		)) {
			password.SecureErase();
			DeleteProcThreadAttributeList(attrList);
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);
		}

		DeleteProcThreadAttributeList(attrList);
		password.SecureErase();
	}
}