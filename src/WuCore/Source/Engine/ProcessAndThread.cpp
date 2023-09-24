#include "../../pch.h"

#include "../../Headers/Engine/ProcessAndThread.h"
#include "../../Headers/Engine/AccessControl.h"
#include "../../Headers/Engine/Utilities.h"
#include "../../Headers/Support/NtUtilities.h"
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
		wuvector<WU_OBJECT_HANDLE>* objectHandleList,	// The output object handle list.
		wuvector<OBJECT_INPUT>* inputList,				// The input list containing the object name and type.
		bool closeHandle								// TRUE to ATTEMPT to close the handle(s).
	)
	{
		DWORD dwResult;

		for (OBJECT_INPUT& input : *inputList) {
			wuvector<DWORD> processIdList;
			WWuString inputObject;
			switch (input.Type) {
				case SupportedObjectType::FileSystem:
				{
					inputObject = input.ObjectName;
					PathStripPath(inputObject.GetBuffer());
					GetProcessUsingFile(input.ObjectName, processIdList);
				} break;

				case SupportedObjectType::Registry:
				{
					inputObject = input.ObjectName.Split('\\').back();
					GetProcessUsingKey(input.ObjectName, processIdList, closeHandle);
				} break;
			}

			for (DWORD pid : processIdList) {
				wuunique_ptr<WU_OBJECT_HANDLE> objHandle = make_wuunique<WU_OBJECT_HANDLE>();

				objHandle->InputObject = inputObject;
				objHandle->ProcessId = pid;
				objHandle->Type = input.Type;

				HANDLE hProcess = OpenProcess(
					PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE,
					FALSE,
					(DWORD)pid
				);
				if (hProcess != NULL) {
					DWORD maxPath = MAX_PATH;
					wuunique_ha_ptr<WCHAR> imgNameBuffer = make_wuunique_ha<WCHAR>(MAX_PATH * 2);

					if (QueryFullProcessImageName(hProcess, 0, imgNameBuffer.get(), &maxPath)) {
						objHandle->ImagePath = imgNameBuffer.get();
						objHandle->Name = objHandle->ImagePath;
						PathStripPathW(objHandle->Name.GetBuffer());
					}

					if (wcslen(imgNameBuffer.get()) == 0) {
						GetProcessImageName(objHandle->ProcessId, objHandle->Name);
						if (objHandle->Name.Length() > 0) {
							objHandle->ImagePath = objHandle->Name;
							PathStripPath(objHandle->Name.GetBuffer());
						}
					}
					else {
						objHandle->ImagePath = imgNameBuffer.get();
						objHandle->Name = objHandle->ImagePath;
						PathStripPathW(objHandle->Name.GetBuffer());
					}

					for (VersionInfoProperty versionInfo : { 
						VersionInfoProperty::FileDescription,
						VersionInfoProperty::ProductName,
						VersionInfoProperty::FileVersion,
						VersionInfoProperty::CompanyName
					}) {
						WWuString value;
						GetProccessVersionInfo(objHandle->ImagePath, versionInfo, value);
						objHandle->VersionInfo.emplace(std::make_pair(versionInfo, value));
					}

					if (closeHandle && input.Type == SupportedObjectType::FileSystem)
						CloseExternalHandlesToFile(hProcess, inputObject);

					CloseHandle(hProcess);
				}
				else {
					if (GetLastError() == ERROR_ACCESS_DENIED) {
						hProcess = OpenProcess(
							PROCESS_QUERY_LIMITED_INFORMATION,
							FALSE,
							objHandle->ProcessId
						);
						if (hProcess == NULL) {
							dwResult = GetLastError();
							if (dwResult != ERROR_ACCESS_DENIED)
								throw WuStdException(dwResult, __FILEW__, __LINE__);
						}

						DWORD maxPath = MAX_PATH;
						wuunique_ha_ptr<WCHAR> imgNameBuffer = make_wuunique_ha<WCHAR>(MAX_PATH * 2);

						if (QueryFullProcessImageName(hProcess, 0, imgNameBuffer.get(), &maxPath)) {
							objHandle->ImagePath = imgNameBuffer.get();
							objHandle->Name = objHandle->ImagePath;
							PathStripPathW(objHandle->Name.GetBuffer());

							for (VersionInfoProperty versionInfo : {
								VersionInfoProperty::FileDescription,
									VersionInfoProperty::ProductName,
									VersionInfoProperty::FileVersion,
									VersionInfoProperty::CompanyName
							}) {
								WWuString value;
								GetProccessVersionInfo(objHandle->ImagePath, versionInfo, value);
								objHandle->VersionInfo.emplace(std::make_pair(versionInfo, value));
							}
						}
						else {
							try {
								GetProcessImageName(objHandle->ProcessId, objHandle->Name);
								objHandle->ImagePath = objHandle->Name;
								PathStripPath(objHandle->Name.GetBuffer());
							}
							catch (const WuStdException&) { /* If it fails we manage the name later. */ }

							if (
								objHandle->Name == L"System" ||
								objHandle->Name == L"Secure System" ||
								objHandle->Name == L"Registry") {
								GetEnvVariable(L"windir", objHandle->ImagePath);

								objHandle->ImagePath += L"\\System32\\ntoskrnl.exe";

								for (VersionInfoProperty versionInfo : {
									VersionInfoProperty::FileDescription,
										VersionInfoProperty::ProductName,
										VersionInfoProperty::FileVersion,
										VersionInfoProperty::CompanyName
								}) {
									WWuString value;
									GetProccessVersionInfo(objHandle->ImagePath, versionInfo, value);
									objHandle->VersionInfo.emplace(std::make_pair(versionInfo, value));
								}
							}
						}
					}
				}

				objectHandleList->push_back(*objHandle.get());
			}
		}
	}

	/*
	*	~ Get-ProcessModule
	*	https://learn.microsoft.com/windows/win32/psapi/enumerating-all-modules-for-a-process
	*/

	void ProcessAndThread::GetProcessLoadedModuleInformation(
		wuvector<DWORD> processIdList,					// The process ID list.
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
				GetProcessCommandLine(hProcess, commandLine);
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
		wuvector<WWuString> privList;
		privList.push_back(L"SeDebugPrivilege");
		AccessControl::AdjustCurrentTokenPrivilege(privList, SE_PRIVILEGE_ENABLED);

		ModuleHandle hNtdll(L"ntdll.dll");
		if (!hNtdll.IsValid()) {
			WuStdException ex { hNtdll.Error(), __FILEW__, __LINE__ };
			ex.Cry(L"ErrorLoadingLibrary", WriteErrorCategory::ResourceUnavailable, L"ntdll.dll", context);

			return;
		}

		ProcessHandle hProcess(PROCESS_SUSPEND_RESUME, FALSE, processId);
		if (!hProcess.IsValid()) {
			WuStdException ex { hProcess.Error(), __FILEW__, __LINE__ };
			ex.Cry(L"ErrorOpeningProcess", WriteErrorCategory::ResourceUnavailable, WWuString::Format(L"%d", processId).GetBuffer(), context);

			return;
		}

		_NtSuspendProcess NtSuspendProcess = (_NtSuspendProcess)GetProcAddress(hNtdll.get(), "NtSuspendProcess");

		NTSTATUS status = NtSuspendProcess(hProcess.get());
		if (status != STATUS_SUCCESS) {
			WuStdException ex { static_cast<int>(GetLastError()), __FILEW__, __LINE__ };
			ex.Cry(L"ErrorSuspendingProcess", WriteErrorCategory::ResourceUnavailable, WWuString::Format(L"%d", processId).GetBuffer(), context);

			return;
		}

		// Never leave your process privileges escalated.
		AccessControl::AdjustCurrentTokenPrivilege(privList, SE_PRIVILEGE_DISABLED);
	}

	void ProcessAndThread::ResumeProcess(DWORD processId, WuNativeContext* context)
	{
		wuvector<WWuString> privList;
		privList.push_back(L"SeDebugPrivilege");
		AccessControl::AdjustCurrentTokenPrivilege(privList, SE_PRIVILEGE_ENABLED);

		ModuleHandle hNtdll(L"ntdll.dll");
		if (!hNtdll.IsValid()) {
			WuStdException ex { hNtdll.Error(), __FILEW__, __LINE__ };
			ex.Cry(L"ErrorLoadingLibrary", WriteErrorCategory::ResourceUnavailable, L"ntdll.dll", context);

			return;
		}

		ProcessHandle hProcess(PROCESS_SUSPEND_RESUME, FALSE, processId);
		if (!hProcess.IsValid()) {
			WuStdException ex { hProcess.Error(), __FILEW__, __LINE__ };
			ex.Cry(L"ErrorOpeningProcess", WriteErrorCategory::OpenError, WWuString::Format(L"%d", processId), context);

			return;
		}

		_NtResumeProcess NtResumeProcess = (_NtResumeProcess)GetProcAddress(hNtdll.get(), "NtResumeProcess");

		NTSTATUS status = NtResumeProcess(hProcess.get());
		if (status != STATUS_SUCCESS) {
			WuStdException ex { static_cast<int>(GetLastError()), __FILEW__, __LINE__ };
			ex.Cry(L"ErrorResumingProcess", WriteErrorCategory::InvalidResult, WWuString::Format(L"%d", processId), context);

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
	* Used by the parameter -CloseHandle, from Get-ObjectHandle.
	*/
	void CloseExtProcessHandle(
		HANDLE hExtProcess,				// A valid handle to the external process.
		const WWuString& objectName		// The object name, used on GetProcessObjectHandle.
	)
	{
		NTSTATUS ntCall = STATUS_SUCCESS;
		HANDLE hTarget = NULL;

		HMODULE hmodule = GetModuleHandle(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hmodule || NULL == hmodule)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		_NtQueryInformationProcess NtQueryInformationProcess = (_NtQueryInformationProcess)::GetProcAddress(hmodule, "NtQueryInformationProcess");

		std::unique_ptr<BYTE[]> buffer;
		ULONG szbuffer = (ULONG)sizeof(PROCESS_HANDLE_SNAPSHOT_INFORMATION);
		ULONG szbuffneed = 0;
		do {
			buffer = std::make_unique<BYTE[]>(szbuffer);
			ntCall = NtQueryInformationProcess(hExtProcess, ProcessHandleInformation, buffer.get(), szbuffer, &szbuffneed);

			if (STATUS_SUCCESS != ntCall && STATUS_INFO_LENGTH_MISMATCH != ntCall)
				throw WuStdException(ntCall, __FILEW__, __LINE__, CoreErrorType::NtError);

			if (STATUS_SUCCESS == ntCall)
				break;

			szbuffer = szbuffneed;

		} while (ntCall == STATUS_INFO_LENGTH_MISMATCH);

		PPROCESS_HANDLE_SNAPSHOT_INFORMATION phsnapinfo = reinterpret_cast<PPROCESS_HANDLE_SNAPSHOT_INFORMATION>(buffer.get());

		for (ULONG i = 0; i < phsnapinfo->NumberOfHandles; i++) {
			wuunique_ptr<BYTE[]> objectNameBuffer;

			if (!DuplicateHandle(hExtProcess, phsnapinfo->Handles[i].HandleValue, ::GetCurrentProcess(), &hTarget, 0, FALSE, DUPLICATE_SAME_ACCESS))
				continue;

			NtQueryObjectWithTimeout(hTarget, OBJECT_INFORMATION_CLASS::ObjectNameInformation, objectNameBuffer, 50);

			CloseHandle(hTarget);

			auto objectNameInfo = reinterpret_cast<POBJECT_NAME_INFORMATION>(objectNameBuffer.get());
			if (objectNameInfo->Name.Buffer) {
				WWuString buffString = objectNameInfo->Name.Buffer;
				if (buffString.EndsWith(objectName)) {
					if (!DuplicateHandle(hExtProcess, phsnapinfo->Handles[i].HandleValue, ::GetCurrentProcess(), &hTarget, 0, FALSE, DUPLICATE_CLOSE_SOURCE)) {
						CloseHandle(hTarget);

						throw WuStdException(GetLastError(), __FILEW__, __LINE__);
					}
				}
			}
		}
	}
}