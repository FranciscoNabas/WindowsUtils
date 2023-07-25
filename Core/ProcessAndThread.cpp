#include "pch.h"

#include "ProcessAndThread.h"

namespace WindowsUtils::Core
{
	// Get-ObjectHandle
	DWORD ProcessAndThread::GetProcessObjectHandle(
		wuvector<WU_OBJECT_HANDLE>* objectHandleList,		// A vector of object handle output objects.
		wuvector<WWuString>* inputPath,						// A vector of input objects.
		BOOL closeHandle = FALSE							// TRUE for closing the handles found.
	) {
		DWORD result = ERROR_SUCCESS;

		for (WWuString path : *inputPath)
		{
			/*
			* This strips the path out of the input object so we can add it to the 'InputObject' property.
			* This will need to be changed in the future to support multiple object types.
			*/
			WWuString inputObject = path;
			PathStripPath(inputObject.GetBuffer());

			wuunique_ha_ptr<FILE_PROCESS_IDS_USING_FILE_INFORMATION> procUsingFile;
			result = GetNtProcessUsingFile(path, procUsingFile);
			if (result != ERROR_SUCCESS)
				return result;

			for (ULONG j = 0; j < procUsingFile->NumberOfProcessIdsInList; j++)
			{
				wuunique_ptr<WU_OBJECT_HANDLE> objHandle = make_wuunique<WU_OBJECT_HANDLE>();

				objHandle->InputObject = inputObject;
				objHandle->ProcessId = static_cast<DWORD>(procUsingFile->ProcessIdList[j]);

				if (objHandle->ProcessId == 10220)
					result = 0;

				HANDLE hProcess = OpenProcess(
					PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE,
					FALSE,
					(DWORD)procUsingFile->ProcessIdList[j]
				);
				if (hProcess != NULL)
				{
					DWORD maxPath = MAX_PATH;
					wuunique_ha_ptr<WCHAR> imgNameBuffer = make_wuunique_ha<WCHAR>(MAX_PATH * 2);

					if (!QueryFullProcessImageName(hProcess, 0, imgNameBuffer.get(), &maxPath))
						return GetLastError();

					if (wcslen(imgNameBuffer.get()) == 0)
					{
						result = GetProcessImageName(objHandle->ProcessId, objHandle->Name);
						if (objHandle->Name.Length() > 0)
						{
							objHandle->ImagePath = objHandle->Name;
							PathStripPath(objHandle->Name.GetBuffer());
						}
					}
					else
					{
						objHandle->ImagePath = imgNameBuffer.get();
						objHandle->Name = objHandle->ImagePath;
						PathStripPathW(objHandle->Name.GetBuffer());
					}

					for (VERSION_INFO_PROPERTY versionInfo : { FileDescription, ProductName, FileVersion, CompanyName })
					{
						WWuString value;
						GetProccessVersionInfo(objHandle->ImagePath, versionInfo, value);
						objHandle->VersionInfo.emplace(std::make_pair(versionInfo, value));
					}

					if (closeHandle)
					{
						result = CloseExtProcessHandle(hProcess, path);
						if (ERROR_SUCCESS != result)
							return result;
					}
					CloseHandle(hProcess);
				}
				else
				{
					if (GetLastError() == ERROR_ACCESS_DENIED)
					{
						hProcess = OpenProcess(
							PROCESS_QUERY_LIMITED_INFORMATION,
							FALSE,
							objHandle->ProcessId
						);
						if (hProcess == NULL)
						{
							result = GetLastError();
							if (result != ERROR_ACCESS_DENIED)
								return result;
							else
								result = ERROR_SUCCESS;
						}

						DWORD maxPath = MAX_PATH;
						wuunique_ha_ptr<WCHAR> imgNameBuffer = make_wuunique_ha<WCHAR>(MAX_PATH * 2);

						if (QueryFullProcessImageName(hProcess, 0, imgNameBuffer.get(), &maxPath))
						{
							objHandle->ImagePath = imgNameBuffer.get();
							objHandle->Name = objHandle->ImagePath;
							PathStripPathW(objHandle->Name.GetBuffer());

							for (VERSION_INFO_PROPERTY versionInfo : { FileDescription, ProductName, FileVersion, CompanyName })
							{
								WWuString value;
								GetProccessVersionInfo(objHandle->ImagePath, versionInfo, value);
								objHandle->VersionInfo.emplace(std::make_pair(versionInfo, value));
							}
						}
						else
						{
							if (GetProcessImageName(objHandle->ProcessId, objHandle->Name) == ERROR_SUCCESS)
							{
								objHandle->ImagePath = objHandle->Name;
								PathStripPath(objHandle->Name.GetBuffer());
							}

							if (
								objHandle->Name == L"System" ||
								objHandle->Name == L"Secure System" ||
								objHandle->Name == L"Registry")
							{
								result = GetEnvVariable(L"windir", objHandle->ImagePath);
								DWERRORCHECKV(result);

								objHandle->ImagePath += L"\\System32\\ntoskrnl.exe";

								for (VERSION_INFO_PROPERTY versionInfo : { FileDescription, ProductName, FileVersion, CompanyName })
								{
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

		return result;
	}

	/*========================================
	==		Utility function definition		==
	==========================================*/

	// Gets process image version information.
	DWORD GetProccessVersionInfo(
		const WWuString& imagePath,							// The process image file path.
		ProcessAndThread::VERSION_INFO_PROPERTY propName,	// The property name, or 'key' value.
		WWuString& value									// The return value.
	) {
		DWORD result = ERROR_SUCCESS;
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
			return GetLastError();

		if (!VerQueryValue(buffer.get(), L"\\VarFileInfo\\Translation", (LPVOID*)&codePage, &len))
			return GetLastError();

		WWuString text;
		text.Format(L"\\StringFileInfo\\%04x%04x\\", codePage[0], codePage[1]);
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

		return result;
	}

	/*
	* Used by the parameter -CloseHandle, from Get-ObjectHandle.
	*/
	DWORD CloseExtProcessHandle(
		HANDLE hExtProcess,				// A valid handle to the external process.
		const WWuString& objectName		// The object name, used on GetProcessObjectHandle.
	) {
		DWORD result = ERROR_SUCCESS;
		NTSTATUS ntCall = STATUS_SUCCESS;
		HANDLE hTarget = NULL;

		WWuString pathNoRoot = PathSkipRoot(objectName.GetBuffer());
		if (pathNoRoot.Length() == 0)
			return GetLastError();

		HMODULE hmodule = GetModuleHandle(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hmodule || NULL == hmodule)
			return GetLastError();

		_NtQueryInformationProcess NtQueryInformationProcess = (_NtQueryInformationProcess)::GetProcAddress(hmodule, "NtQueryInformationProcess");

		std::unique_ptr<BYTE[]> buffer;
		ULONG szbuffer = (ULONG)sizeof(PROCESS_HANDLE_SNAPSHOT_INFORMATION);
		ULONG szbuffneed = 0;
		do
		{
			buffer = std::make_unique<BYTE[]>(szbuffer);
			ntCall = NtQueryInformationProcess(hExtProcess, ProcessHandleInformation, buffer.get(), szbuffer, &szbuffneed);

			if (STATUS_SUCCESS != ntCall && STATUS_INFO_LENGTH_MISMATCH != ntCall)
				return ntCall;

			if (STATUS_SUCCESS == ntCall)
				break;

			szbuffer = szbuffneed;

		} while (ntCall == STATUS_INFO_LENGTH_MISMATCH);

		PPROCESS_HANDLE_SNAPSHOT_INFORMATION phsnapinfo = reinterpret_cast<PPROCESS_HANDLE_SNAPSHOT_INFORMATION>(buffer.get());

		for (ULONG i = 0; i < phsnapinfo->NumberOfHandles; i++)
		{
			
			wuunique_ptr<OBJECT_NAME_INFORMATION> objectNameInfo = make_wuunique<OBJECT_NAME_INFORMATION>();

			if (!::DuplicateHandle(hExtProcess, phsnapinfo->Handles[i].HandleValue, ::GetCurrentProcess(), &hTarget, 0, FALSE, DUPLICATE_SAME_ACCESS))
				continue;

			ntCall = NtQueryObjectWithTimeout(hTarget, ObjectNameInformation, objectNameInfo.get(), 200);
			if (STATUS_SUCCESS != ntCall)
				return ntCall;

			::CloseHandle(hTarget);

			if (objectNameInfo->Name.Buffer)
			{
				WWuString buffString = objectNameInfo->Name.Buffer;
				if (buffString.EndsWith(pathNoRoot))
				{
					if (!::DuplicateHandle(hExtProcess, phsnapinfo->Handles[i].HandleValue, ::GetCurrentProcess(), &hTarget, 0, FALSE, DUPLICATE_CLOSE_SOURCE))
						result = GetLastError();

					else
					{
						::CloseHandle(hTarget);
						break;
					}
				}
			}
		}

		return result;
	}
}