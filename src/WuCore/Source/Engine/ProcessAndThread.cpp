#include "../../pch.h"

#include "../../Headers/Engine/ProcessAndThread.h"

namespace WindowsUtils::Core
{
#pragma region Get-ObjectHandle
	
	GETHANDLE_INPUT::GETHANDLE_INPUT(const WWuString& name, const SupportedHandleType type)
		: ObjectName(name), Type(type) { }


	OBJECT_HANDLE::OBJECT_HANDLE(const SupportedHandleType type, const HANDLE value, const WWuString& inputObj, const DWORD pid)
		: Type(type), HandleValue(value), InputObject(inputObj), ProcessId(pid), Name(), ImagePath(), VersionInfo() { }


	void ProcessAndThread::GetProcessObjectHandle(const WuList<GETHANDLE_INPUT>& inputList, const bool closeHandle, const bool isAdmin, const WuNativeContext* context)
	{
		std::unordered_map<DWORD, WWuString> runningProcessMap;
		if (isAdmin)
			runningProcessMap = NtUtilities::ListRunningProcessIdAndNames();

		for (const GETHANDLE_INPUT& input : inputList) {
			WWuString inputObject = input.ObjectName.Split('\\').Back();

			// Getting a list of processes using the object.
			std::unordered_map<HANDLE, DWORD> handleProcessInfo;
			try {
				switch (input.Type) {
				case SupportedHandleType::FileSystem:
				{
					WWuString devicePath = IO::GetFileDevicePathFromDosPath(input.ObjectName);
					handleProcessInfo = NtUtilities::GetProcessUsingObject(devicePath, input.Type, closeHandle);
				} break;

				case SupportedHandleType::Registry:
				{
					handleProcessInfo = NtUtilities::GetProcessUsingObject(input.ObjectName, input.Type, closeHandle);
				} break;

				default:
					_WU_RAISE_COR_EXCEPTION_WMESS(COR_E_ARGUMENT, L"GetProcessObjectHandle", WriteErrorCategory::InvalidArgument, L"Invalid object type.");
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
			for (const auto& handleInfo : handleProcessInfo) {
				DWORD pid = handleInfo.second;
				OBJECT_HANDLE objHandle(input.Type, handleInfo.first, inputObject, pid);

				try {
					bool getVersionInfo = false;
					if (isAdmin) {
						// Attempting to get the image name from our map.
						if (auto iterator = runningProcessMap.find(pid); iterator != runningProcessMap.end()) {
							if (iterator->second.StartsWith(L"\\Device\\HarddiskVolume")) {
								objHandle.ImagePath = IO::GetFileDosPathFromDevicePath(iterator->second);
								objHandle.Name = IO::StripPath(objHandle.ImagePath);

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
						else {
							DWORD buffSize = MAX_PATH;
							WCHAR imageBuffer[MAX_PATH]{ };
							ProcessHandle hProcess(pid, PROCESS_QUERY_LIMITED_INFORMATION, false);
							if (QueryFullProcessImageName(hProcess.Get(), 0, imageBuffer, &buffSize)) {
								objHandle.ImagePath = imageBuffer;
								objHandle.Name = IO::StripPath(imageBuffer);

								getVersionInfo = true;
							}
						}
					}
					else {
						DWORD buffSize = MAX_PATH;
						WCHAR imageBuffer[MAX_PATH]{ };
						ProcessHandle hProcess(pid, PROCESS_QUERY_LIMITED_INFORMATION, false);
						if (QueryFullProcessImageName(hProcess.Get(), 0, imageBuffer, &buffSize)) {
							objHandle.ImagePath = imageBuffer;
							objHandle.Name = IO::StripPath(imageBuffer);

							getVersionInfo = true;
						}
					}
					
					if (getVersionInfo) {
						for (const VersionInfoProperty& versionInfo : {
							VersionInfoProperty::FileDescription,
							VersionInfoProperty::ProductName,
							VersionInfoProperty::FileVersion,
							VersionInfoProperty::CompanyName
						}) {
							objHandle.VersionInfo.emplace(versionInfo, GetProccessVersionInfo(objHandle.ImagePath, versionInfo));
						}
					}
				}
				catch (const WuNativeException& ex) {
					context->NativeWriteError(ex);
					continue;
				}

				context->NativeWriteObject(&objHandle, WriteOutputType::ObjectHandle);
			}
		}
	}

#pragma endregion

#pragma region Get-ProcessModule

	MODULE_INFORMATION::MODULE_INFORMATION(const WWuString& name, const WWuString& path)
		: ModuleName(name), ModulePath(path) { }


	PROCESS_MODULE_INFO::PROCESS_MODULE_INFO(const DWORD processId, const WWuString& imagePath, const WWuString& fileName, const WWuString& commandLine)
		: ProcessId(processId), ImagePath(imagePath), ImageFileName(fileName), CommandLine(commandLine) { }


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
		std::map<WWuString, MODULE_INFORMATION> moduleInfoCache;

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

			PROCESS_MODULE_INFO currentProcessInfo(processId, procFullPath, procImgName, commandLine);
			for (size_t i = 0; i < (returnBytes / sizeof(HMODULE)); i++) {
				WCHAR modNameBuffer[MAX_PATH];
				if (GetModuleFileNameEx(process.Get(), moduleList[i], modNameBuffer, MAX_PATH)) {

					// Checking if this module is already in the cache.
					WWuString moduleFileName(modNameBuffer);
					try {
						MODULE_INFORMATION moduleInfo = moduleInfoCache.at(moduleFileName);
						currentProcessInfo.ModuleInfo.Add(moduleInfo);

						continue;
					}
					catch (const std::out_of_range&) { }

					PathStripPath(moduleFileName.Raw());

					MODULE_INFORMATION moduleInfo(IO::StripPath(modNameBuffer), modNameBuffer);
					if (includeVersionInfo) {
						for (VersionInfoProperty versionInfo : {
							VersionInfoProperty::FileDescription,
								VersionInfoProperty::ProductName,
								VersionInfoProperty::FileVersion,
								VersionInfoProperty::CompanyName
						}) {
							WWuString value = GetProccessVersionInfo(modNameBuffer, versionInfo);

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

					currentProcessInfo.ModuleInfo.Add(moduleInfo);
					moduleInfoCache.emplace(moduleInfo.ModulePath, moduleInfo);
				}
			}

			// Writing to output.
			context->NativeWriteObject(&currentProcessInfo, WriteOutputType::ProcessModuleInfo);
		}
	}

#pragma endregion

#pragma region Suspend/Resume-Process

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

#pragma endregion

#pragma region Start-ProcessAsUser

	void ProcessAndThread::RunAs(const WWuString& userName, const WWuString& domain, const WWuString& password, const WWuString& commandLine, const WWuString& titleBar)
	{
		WORD processorArch{ };
		size_t bytesNeeded{ };
		SYSTEM_INFO sysInfo{ };

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
			if (lastError != ERROR_INSUFFICIENT_BUFFER)
				_WU_RAISE_NATIVE_EXCEPTION(lastError, L"InitializeProcThreadAttributeList", WriteErrorCategory::InvalidResult);
		}

		ScopedBuffer buffer(bytesNeeded);
		LPPROC_THREAD_ATTRIBUTE_LIST attrList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(buffer.Get());

		if (!InitializeProcThreadAttributeList(attrList, 1, 0, &bytesNeeded)) {
			DeleteProcThreadAttributeList(attrList);
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"InitializeProcThreadAttributeList", WriteErrorCategory::InvalidResult);
		}

		if (!UpdateProcThreadAttribute(attrList, 0, 0x00020019, &processorArch, 2, NULL, NULL)) {
			DeleteProcThreadAttributeList(attrList);
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"UpdateProcThreadAttribute", WriteErrorCategory::InvalidResult);
		}

		WCHAR currentPath[MAX_PATH]{ };
		GetCurrentDirectoryW(MAX_PATH, currentPath);

		STARTUPINFOEX startupInfo{ };
		PROCESS_INFORMATION procInfo{ };

		startupInfo.lpAttributeList = attrList;
		startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);
		startupInfo.StartupInfo.lpTitle = const_cast<LPWSTR>(titleBar.Raw());

		if (!CreateProcessWithLogonW(
			userName.Raw(),
			domain.Raw(),
			password.Raw(),
			LOGON_WITH_PROFILE,
			NULL,
			const_cast<LPWSTR>(commandLine.Raw()),
			CREATE_UNICODE_ENVIRONMENT,
			NULL,
			currentPath,
			&startupInfo.StartupInfo,
			&procInfo
		)) {
			DeleteProcThreadAttributeList(attrList);
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"CreateProcessWithLogon", WriteErrorCategory::InvalidResult);
		}

		DeleteProcThreadAttributeList(attrList);
	}

#pragma endregion
	
#pragma region Utilities

	// Gets process image version information.
	WWuString ProcessAndThread::GetProccessVersionInfo(
		const WWuString& imagePath,				// The process image file path.
		const VersionInfoProperty propName		// The property name, or 'key' value.
	)
	{
		UINT len;
		LPWSTR desc{ };
		LPWORD codePage{ };

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
					return WWuString(desc);
			}
		}

		return WWuString();
	}

#pragma endregion
}