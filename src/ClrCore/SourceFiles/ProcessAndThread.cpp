#include "..\pch.h"

#include "..\Headers\ProcessAndThread.h"

namespace WindowsUtils::Core
{
	// Get-ObjectHandle
	void ProcessAndThread::GetProcessObjectHandle(
		wuvector<WU_OBJECT_HANDLE>* objectHandleList,	// The output object handle list.
		wuvector<OBJECT_INPUT>* inputList,				// The input list containing the object name and type.
		bool closeHandle								// TRUE to ATTEMPT to close the handle(s).
	) {
		DWORD dwResult;

		for (OBJECT_INPUT& input : *inputList)
		{
			wuvector<DWORD> processIdList;
			WWuString inputObject;
			switch (input.Type) {
				case FileSystem:
				{
					inputObject = input.ObjectName;
					PathStripPath(inputObject.GetBuffer());
					GetProcessUsingFile(input.ObjectName, processIdList);
				} break;

				case Registry:
				{
					inputObject = input.ObjectName.Split('\\').back();
					GetProcessUsingObject(input.ObjectName, processIdList, closeHandle);
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

						for (VERSION_INFO_PROPERTY versionInfo : { FileDescription, ProductName, FileVersion, CompanyName }) {
							WWuString value;
							GetProccessVersionInfo(objHandle->ImagePath, versionInfo, value);
							objHandle->VersionInfo.emplace(std::make_pair(versionInfo, value));
						}
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

					for (VERSION_INFO_PROPERTY versionInfo : { FileDescription, ProductName, FileVersion, CompanyName }) {
						WWuString value;
						GetProccessVersionInfo(objHandle->ImagePath, versionInfo, value);
						objHandle->VersionInfo.emplace(std::make_pair(versionInfo, value));
					}

					if (closeHandle && input.Type == FileSystem)
						CloseExtProcessHandle(hProcess, inputObject);
					
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

							for (VERSION_INFO_PROPERTY versionInfo : { FileDescription, ProductName, FileVersion, CompanyName }) {
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
								WuResult result = GetEnvVariable(L"windir", objHandle->ImagePath);
								if (result.Result != ERROR_SUCCESS)
									throw WuStdException(result.Result, __FILEW__, __LINE__);

								objHandle->ImagePath += L"\\System32\\ntoskrnl.exe";

								for (VERSION_INFO_PROPERTY versionInfo : { FileDescription, ProductName, FileVersion, CompanyName }) {
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

	/*========================================
	==		Utility function definition		==
	==========================================*/

	// Gets process image version information.
	WuResult GetProccessVersionInfo(
		const WWuString& imagePath,							// The process image file path.
		ProcessAndThread::VERSION_INFO_PROPERTY propName,	// The property name, or 'key' value.
		WWuString& value									// The return value.
	) {
		LPWORD codePage = NULL;
		LPWSTR desc = NULL;
		UINT len;

		WWuString propertyName;
		switch (propName)
		{
		case ProcessAndThread::FileDescription:
			propertyName = L"FileDescription";
			break;
		case ProcessAndThread::ProductName:
			propertyName = L"ProductName";
			break;
		case ProcessAndThread::FileVersion:
			propertyName = L"FileVersion";
			break;
		case ProcessAndThread::CompanyName:
			propertyName = L"CompanyName";
			break;
		default:
			break;
		}

		DWORD infoSize = GetFileVersionInfoSizeW(imagePath.GetBuffer(), NULL);
		wuunique_ha_ptr<void> buffer = make_wuunique_ha<void>(infoSize);

		if (!GetFileVersionInfo(imagePath.GetBuffer(), NULL, infoSize, buffer.get()))
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		if (!VerQueryValue(buffer.get(), L"\\VarFileInfo\\Translation", (LPVOID*)&codePage, &len))
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		WWuString text = WWuString::Format(L"\\StringFileInfo\\%04x%04x\\", codePage[0], codePage[1]);
		text += propertyName;

		if (VerQueryValue(buffer.get(), text.GetBuffer(), (LPVOID*)&desc, &len))
		{
			if (!WWuString::IsNullOrWhiteSpace(desc))
				value = desc;
			else
				value = L"";
		}
		else
			value = L"";

		return WuResult();
	}

	/*
	* Used by the parameter -CloseHandle, from Get-ObjectHandle.
	*/
	void CloseExtProcessHandle(
		HANDLE hExtProcess,				// A valid handle to the external process.
		const WWuString& objectName		// The object name, used on GetProcessObjectHandle.
	) {
		NTSTATUS ntCall = STATUS_SUCCESS;
		HANDLE hTarget = NULL;

		WWuString pathNoRoot = PathSkipRoot(objectName.GetBuffer());
		if (pathNoRoot.Length() == 0)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		HMODULE hmodule = GetModuleHandle(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hmodule || NULL == hmodule)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		_NtQueryInformationProcess NtQueryInformationProcess = (_NtQueryInformationProcess)::GetProcAddress(hmodule, "NtQueryInformationProcess");

		std::unique_ptr<BYTE[]> buffer;
		ULONG szbuffer = (ULONG)sizeof(PROCESS_HANDLE_SNAPSHOT_INFORMATION);
		ULONG szbuffneed = 0;
		do
		{
			buffer = std::make_unique<BYTE[]>(szbuffer);
			ntCall = NtQueryInformationProcess(hExtProcess, ProcessHandleInformation, buffer.get(), szbuffer, &szbuffneed);

			if (STATUS_SUCCESS != ntCall && STATUS_INFO_LENGTH_MISMATCH != ntCall)
				throw WuStdException(ntCall, __FILEW__, __LINE__, NtError);

			if (STATUS_SUCCESS == ntCall)
				break;

			szbuffer = szbuffneed;

		} while (ntCall == STATUS_INFO_LENGTH_MISMATCH);

		PPROCESS_HANDLE_SNAPSHOT_INFORMATION phsnapinfo = reinterpret_cast<PPROCESS_HANDLE_SNAPSHOT_INFORMATION>(buffer.get());

		for (ULONG i = 0; i < phsnapinfo->NumberOfHandles; i++)
		{
			wuunique_ptr<BYTE[]> objectNameBuffer;

			if (!DuplicateHandle(hExtProcess, phsnapinfo->Handles[i].HandleValue, ::GetCurrentProcess(), &hTarget, 0, FALSE, DUPLICATE_SAME_ACCESS))
				continue;

			NtQueryObjectWithTimeout(hTarget, ObjectNameInformation, objectNameBuffer, 50);

			CloseHandle(hTarget);

			auto objectNameInfo = reinterpret_cast<POBJECT_NAME_INFORMATION>(objectNameBuffer.get());
			if (objectNameInfo->Name.Buffer)
			{
				WWuString buffString = objectNameInfo->Name.Buffer;
				if (buffString.EndsWith(pathNoRoot))
				{
					if (!DuplicateHandle(hExtProcess, phsnapinfo->Handles[i].HandleValue, ::GetCurrentProcess(), &hTarget, 0, FALSE, DUPLICATE_CLOSE_SOURCE)) {
						CloseHandle(hTarget);

						throw WuStdException(GetLastError(), __FILEW__, __LINE__);
					}
				}
			}
		}
	}
}