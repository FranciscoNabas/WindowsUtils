#include "../../pch.h"

#include "../../Headers/Engine/ProcessAndThread.h"

namespace WindowsUtils::Core
{
	/*
	*	~ WU_OBJECT_HANDLE ~
	*/

	_WU_OBJECT_HANDLE::_WU_OBJECT_HANDLE()
		: InputObject(), ProcessId(0), Name(), ImagePath(), VersionInfo() { }

	_WU_OBJECT_HANDLE::_WU_OBJECT_HANDLE(SupportedHandleType type, const WWuString& inputObj, DWORD pid, const WWuString& name,
		const WWuString& imagePath, const std::unordered_map<VersionInfoProperty, const WWuString>& versionInfo)
		: Type(type), InputObject(inputObj), ProcessId(pid), Name(name), ImagePath(imagePath), VersionInfo(versionInfo) { }


	/*
	*	~ PROCESS_MODULE_INFO ~
	*/

	_PROCESS_MODULE_INFO::_PROCESS_MODULE_INFO()
		: ProcessId(0) { }


	/*
	*	~ Get-ObjectHandle ~
	*/

	void ProcessAndThread::GetProcessObjectHandle(const WuList<OBJECT_INPUT>& inputList, const bool closeHandle, WuList<WU_OBJECT_HANDLE>& output, const WuNativeContext* context)
	{
		std::unordered_map<DWORD, WWuString> runningProcessMap = NtUtilities::ListRunningProcessIdAndNames();
		for (const OBJECT_INPUT& input : inputList) {
			WuList<DWORD> processIdList;
			WWuString inputObject;

			// Getting a list of processes using the file system object or the registry key.
			try {
				switch (input.Type) {
				case SupportedHandleType::FileSystem:
				{
					inputObject = input.ObjectName;
					PathStripPath(inputObject.Raw());
					processIdList = NtUtilities::GetProcessUsingFile(input.ObjectName);

					if (closeHandle) {
						WWuString devicePath{ input.ObjectName };
						IO::GetFileDevicePathFromDosPath(devicePath);
						for (const DWORD pid : processIdList)
							NtUtilities::CloseExternalHandlesToFile(pid, devicePath);
					}
				} break;

				case SupportedHandleType::Registry:
				{
					inputObject = input.ObjectName.Split('\\').Back();
					processIdList = NtUtilities::GetProcessUsingKey(input.ObjectName, closeHandle);
				} break;
				}
			}
			catch (const WuNativeException& ex) {
				context->NativeWriteError(ex);
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
							WWuString imageDosPath{ iterator->second };
							IO::GetFileDosPathFromDevicePath(imageDosPath);

							objHandle.ImagePath  = imageDosPath;
							objHandle.Name       = IO::StripPath(imageDosPath);

							getVersionInfo = true;
						}
						else {
							objHandle.Name = iterator->second;

							// These processes are hosted in 'ntoskrnl.exe'.
							if (iterator->second == L"System" ||
								iterator->second == L"Secure System" ||
								iterator->second == L"Registry" ||
								iterator->second == L"Memory Compression") {

								Utilities::GetEnvVariable(L"windir", objHandle.ImagePath);
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

							objHandle.ImagePath  = imagePath;
							objHandle.Name       = IO::StripPath(imagePath);

							getVersionInfo = true;
						}
						else {
							objHandle.Name = imagePath;

							if (imagePath == L"System" ||
								imagePath == L"Secure System" ||
								imagePath == L"Registry" ||
								imagePath == L"Memory Compression") {

								Utilities::GetEnvVariable(L"windir", objHandle.ImagePath);
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
				catch (const WuNativeException& ex) {
					context->NativeWriteError(ex);
					continue;
				}

				output.Add(objHandle);
			}
		}
	}


	/*
	*	~ Get-ProcessModule
	*	https://learn.microsoft.com/windows/win32/psapi/enumerating-all-modules-for-a-process
	*/

	void ProcessAndThread::GetProcessLoadedModuleInformation(
		const WuList<DWORD>& processIdList,				// The process ID list.
		const bool includeVersionInfo,					// True to include module version information.
		const bool suppressError,						// Suppress error output. For when enumerating all processes.
		const WuNativeContext* context					// The Cmdlet native representation.
	)
	{
		// SE_DEBUG_NAME is required to get the process command line.
		PrivilegeCookie privilegeCookie = PrivilegeCookie::Ensure(SE_DEBUG_NAME);

		// When listing the modules for multiple processes, we are going to have
		// repeated modules, and querying information about them every time implies
		// a cost. This map will be a global information cache.
		std::map<WWuString, WU_MODULE_INFO> moduleInfoCache;

		for (const DWORD& processId : processIdList) {
			
			ProcessHandle process{ processId, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false };
			if (!process.Get()) {
				if (!suppressError)
					_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"OpenProcess", WriteErrorCategory::OpenError);

				continue;
			}

			// Getting process image file name.
			DWORD imgNameSize = MAX_PATH;
			WCHAR imgName[MAX_PATH];
			WWuString procImgName;
			WWuString procFullPath;
			if (QueryFullProcessImageName(process.Get(), 0, imgName, &imgNameSize)) {
				procFullPath = WWuString(imgName);
				PathStripPath(imgName);
				procImgName = WWuString(imgName);
			}

			// Getting process command line.
			// Help from: https://stackoverflow.com/questions/6530565/getting-another-process-command-line-in-windows
			WWuString commandLine;
			try {
				NtUtilities::GetProcessCommandLine(process, commandLine);
			}
			catch (const WuNativeException& ex) {
				if (!suppressError)
					throw ex;
			}

			// MS recommends using a big list, getting the right buffer size is
			// not as straight forward as with other functions.
			HMODULE moduleList[1024];
			DWORD returnBytes;
			if (!EnumProcessModules(process.Get(), moduleList, sizeof(moduleList), &returnBytes)) {
				if (!suppressError)
					_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"EnumProcessModules", WriteErrorCategory::InvalidResult);

				continue;
			}

			PROCESS_MODULE_INFO currentProcessInfo;
			currentProcessInfo.ProcessId = processId;
			currentProcessInfo.ImageFileName = procImgName;
			currentProcessInfo.ImagePath = procFullPath;
			currentProcessInfo.CommandLine = commandLine;

			for (size_t i = 0; i < (returnBytes / sizeof(HMODULE)); i++) {
				WCHAR modNameBuffer[MAX_PATH];
				if (GetModuleFileNameEx(process.Get(), moduleList[i], modNameBuffer, MAX_PATH)) {

					// Checking if this module is already in the cache.
					WWuString moduleFileName(modNameBuffer);
					WU_MODULE_INFO moduleInfo;
					try {
						moduleInfo = moduleInfoCache.at(moduleFileName);
						currentProcessInfo.ModuleInfo.push_back(moduleInfo);
						continue;
					}
					catch (const std::out_of_range&) { }

					PathStripPath(moduleFileName.Raw());

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
		}
	}

	void ProcessAndThread::SuspendProcess(const DWORD processId, const WuNativeContext* context)
	{
		NTSTATUS status;
		PrivilegeCookie privilegeCookie = PrivilegeCookie::Ensure(SE_DEBUG_NAME);

		ProcessHandle hProcess(PROCESS_SUSPEND_RESUME, FALSE, processId);
		if (!hProcess.IsValid())
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"OpenProcess", WriteErrorCategory::OpenError);

		if (!NT_SUCCESS(status = NtSuspendProcess(hProcess.Get())))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"NtSuspendProcess", WriteErrorCategory::InvalidResult);
	}

	void ProcessAndThread::ResumeProcess(const DWORD processId, const WuNativeContext* context)
	{
		NTSTATUS status;
		PrivilegeCookie privilegeCookie = PrivilegeCookie::Ensure(SE_DEBUG_NAME);

		ProcessHandle hProcess(PROCESS_SUSPEND_RESUME, FALSE, processId);
		if (!hProcess.IsValid())
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"OpenProcess", WriteErrorCategory::OpenError);

		if (!NT_SUCCESS(status = NtResumeProcess(hProcess.Get())))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"NtResumeProcess", WriteErrorCategory::InvalidResult);
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

		DWORD infoSize = GetFileVersionInfoSizeW(imagePath.Raw(), NULL);
		if (infoSize > 0) {
			std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(infoSize);
			if (!GetFileVersionInfoW(imagePath.Raw(), NULL, infoSize, buffer.get()))
				_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"GetFileVersionInfoW", WriteErrorCategory::InvalidResult);

			if (!VerQueryValue(buffer.get(), L"\\VarFileInfo\\Translation", (LPVOID*)&codePage, &len))
				_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"VerQueryValue", WriteErrorCategory::InvalidResult);

			WWuString text = WWuString::Format(L"\\StringFileInfo\\%04x%04x\\", codePage[0], codePage[1]);
			text += propertyName;

			if (VerQueryValue(buffer.get(), text.Raw(), (LPVOID*)&desc, &len)) {
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
				_WU_RAISE_NATIVE_EXCEPTION(lastError, L"InitializeProcThreadAttributeList", WriteErrorCategory::InvalidResult);
			}
		}

		// This is an opaque structure, working with smart pointers would be a hassle.
		attrList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytesNeeded);
		if (!attrList) {
			password.SecureErase();
			_WU_RAISE_NATIVE_EXCEPTION(ERROR_NOT_ENOUGH_MEMORY, L"HeapAlloc", WriteErrorCategory::QuotaExceeded);
		}

		if (!InitializeProcThreadAttributeList(attrList, 1, 0, &bytesNeeded)) {
			password.SecureErase();
			DeleteProcThreadAttributeList(attrList);
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"InitializeProcThreadAttributeList", WriteErrorCategory::InvalidResult);
		}


		if (!UpdateProcThreadAttribute(attrList, 0, 0x00020019, &processorArch, 2, NULL, NULL)) {
			password.SecureErase();
			DeleteProcThreadAttributeList(attrList);
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"UpdateProcThreadAttribute", WriteErrorCategory::InvalidResult);
		}

		WCHAR currentPath[MAX_PATH] = { 0 };
		GetCurrentDirectoryW(MAX_PATH, currentPath);

		STARTUPINFOEX startupInfo { };
		PROCESS_INFORMATION procInfo { };

		startupInfo.lpAttributeList = attrList;
		startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);
		startupInfo.StartupInfo.lpTitle = titleBar.Raw();

		if (!CreateProcessWithLogonW(
			userName.Raw(),
			domain.Raw(),
			password.Raw(),
			LOGON_WITH_PROFILE,
			NULL,
			commandLine.Raw(),
			CREATE_UNICODE_ENVIRONMENT,
			NULL,
			currentPath,
			&startupInfo.StartupInfo,
			&procInfo
		)) {
			password.SecureErase();
			DeleteProcThreadAttributeList(attrList);
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"CreateProcessWithLogon", WriteErrorCategory::InvalidResult);
		}

		DeleteProcThreadAttributeList(attrList);
		password.SecureErase();
	}
}