#include "pch.h"

#include "ProcessAndThread.h"

namespace WindowsUtils::Core
{
	// Get-ObjectHandle
	DWORD ProcessAndThread::GetProcessObjectHandle(
		std::vector<ProcessAndThread::WU_OBJECT_HANDLE>& objectHandleInfo,	// A vector of object handle output objects.
		std::vector<WuString>& inputPath,									// A vector of input objects.
		BOOL closeHandle = FALSE											// TRUE for closing the handles found.
	) {
		DWORD result = ERROR_SUCCESS;

		for (WuString path : inputPath)
		{
			/*
			* This strips the path out of the input object so we can add it to the 'InputObject' property.
			* This will need to be changed in the future to support multiple object types.
			*/
			WuString inputObject = path;
			PathStripPathW(inputObject.GetWideBuffer());

			std::shared_ptr<FILE_PROCESS_IDS_USING_FILE_INFORMATION> procUsingFile;
			result = GetNtProcessUsingFile(path, procUsingFile);
			if (result != ERROR_SUCCESS)
				return result;

			for (ULONG j = 0; j < procUsingFile.get()->NumberOfProcessIdsInList; j++)
			{
				WuAllocator<WU_OBJECT_HANDLE> objHandle(sizeof(WU_OBJECT_HANDLE));

				objHandle.Get()->InputObject = inputObject;
				objHandle.Get()->ProcessId = procUsingFile.get()->ProcessIdList[j];

				if (objHandle.Get()->ProcessId == 10220)
					result = 0;

				HANDLE hProcess = OpenProcess(
					PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE,
					FALSE,
					(DWORD)procUsingFile.get()->ProcessIdList[j]
				);
				if (hProcess != NULL)
				{
					DWORD maxPath = MAX_PATH;
					objHandle.Get()->ImagePath.Initialize(MAX_PATH);
					objHandle.Get()->Name.Initialize(MAX_PATH);
					::QueryFullProcessImageNameW(hProcess, 0, objHandle.Get()->ImagePath.GetWideBuffer(), &maxPath);

					if (objHandle.Get()->ImagePath.Length() == 0)
					{
						result = GetProcessImageName(objHandle.Get()->ProcessId, objHandle.Get()->Name);
						if (objHandle.Get()->Name.Length() > 0)
						{
							objHandle.Get()->ImagePath = objHandle.Get()->Name;
							::PathStripPathW(objHandle.Get()->Name.GetWideBuffer());
						}

					}
					else
					{
						objHandle.Get()->Name = objHandle.Get()->ImagePath;
						::PathStripPathW(objHandle.Get()->Name.GetWideBuffer());
					}

					for (VERSION_INFO_PROPERTY versionInfo : { FileDescription, ProductName, FileVersion, CompanyName })
					{
						WuString value;
						GetProccessVersionInfo(objHandle.Get()->ImagePath, versionInfo, value);
						objHandle.Get()->VersionInfo->emplace(std::make_pair(versionInfo, value));
					}

					if (CloseHandle)
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
							procUsingFile.get()->ProcessIdList[j]
						);

						DWORD maxPath = MAX_PATH;
						objHandle.Get()->ImagePath.Initialize(MAX_PATH);
						objHandle.Get()->Name.Initialize(MAX_PATH);
						if (QueryFullProcessImageNameW(hProcess, 0, objHandle.Get()->ImagePath.GetWideBuffer(), &maxPath))
						{
							objHandle.Get()->Name = objHandle.Get()->ImagePath;
							::PathStripPathW(objHandle.Get()->Name.GetWideBuffer());

							for (VERSION_INFO_PROPERTY versionInfo : { FileDescription, ProductName, FileVersion, CompanyName })
							{
								WuString value;
								GetProccessVersionInfo(objHandle.Get()->ImagePath, versionInfo, value);
								objHandle.Get()->VersionInfo->emplace(std::make_pair(versionInfo, value));
							}
						}
						else
						{
							if (GetProcessImageName(objHandle.Get()->ProcessId, objHandle.Get()->Name) == ERROR_SUCCESS)
							{
								objHandle.Get()->ImagePath = objHandle.Get()->Name;
								::PathStripPathW(objHandle.Get()->Name.GetWideBuffer());
							}

							if (
								objHandle.Get()->Name == L"System" ||
								objHandle.Get()->Name == L"Secure System" ||
								objHandle.Get()->Name == L"Registry")
							{
								LPCWSTR windir = L"windir";
								LPWSTR lpenvvalue;
								result = GetEnvVariable(windir, lpenvvalue);
								DWERRORCHECKV(result);

								objHandle.Get()->ImagePath = lpenvvalue;
								objHandle.Get()->ImagePath += L"\\System32\\ntoskrnl.exe";

								for (VERSION_INFO_PROPERTY versionInfo : { FileDescription, ProductName, FileVersion, CompanyName })
								{
									WuString value;
									GetProccessVersionInfo(objHandle.Get()->ImagePath, versionInfo, value);
									objHandle.Get()->VersionInfo->emplace(std::make_pair(versionInfo, value));
								}
							}
						}
					}
				}

				objectHandleInfo.push_back(*objHandle.Get());
			}
		}

		return result;
	}

	/*========================================
	==		Utility function definition		==
	==========================================*/

	// Gets process image version information.
	DWORD GetProccessVersionInfo(
		const WuString& imagePath,							// The process image file path.
		ProcessAndThread::VERSION_INFO_PROPERTY propName,	// The property name, or 'key' value.
		WuString& value										// The return value.
	) {
		DWORD result = ERROR_SUCCESS;
		LPWORD codePage = NULL;
		LPWSTR desc = NULL;
		UINT len;

		WuString propertyName;
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

		DWORD infoSize = ::GetFileVersionInfoSizeW(imagePath.GetWideBuffer(), NULL);
		WuAllocator<BYTE[]> buffer(infoSize);

		if (!::GetFileVersionInfoW(imagePath.GetWideBuffer(), NULL, infoSize, buffer.Get()))
			return GetLastError();

		if (!::VerQueryValueW(buffer.Get(), L"\\VarFileInfo\\Translation", (LPVOID*)&codePage, &len))
			return GetLastError();

		WuString text(propertyName.Length() + 258);
		text.Format(L"\\StringFileInfo\\%04x%04x\\", codePage[0], codePage[1]);
		text += propertyName;

		if (::VerQueryValueW(buffer.Get(), text.GetWideBuffer(), (LPVOID*)&desc, &len))
		{
			if (!WuString::IsNullOrWhiteSpace(desc))
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
		const WuString& objectName		// The object name, used on GetProcessObjectHandle.
	) {
		DWORD result = ERROR_SUCCESS;
		NTSTATUS ntCall = STATUS_SUCCESS;
		HANDLE hTarget = NULL;

		WuString pathNoRoot = ::PathSkipRootW(objectName.GetWideBuffer());
		if (!pathNoRoot.IsInitialized())
			return ::GetLastError();

		HMODULE hmodule = ::GetModuleHandleW(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hmodule || NULL == hmodule)
			return ::GetLastError();

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
			WuAllocator<OBJECT_NAME_INFORMATION> objectNameInfo;

			if (!::DuplicateHandle(hExtProcess, phsnapinfo->Handles[i].HandleValue, ::GetCurrentProcess(), &hTarget, 0, FALSE, DUPLICATE_SAME_ACCESS))
				continue;

			ntCall = NtQueryObjectWithTimeout(hTarget, ObjectNameInformation, objectNameInfo.Get(), 200);
			if (STATUS_SUCCESS != ntCall)
				return ntCall;

			::CloseHandle(hTarget);

			if (objectNameInfo.Get()->Name.Buffer)
			{
				WuString buffString = objectNameInfo.Get()->Name.Buffer;
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