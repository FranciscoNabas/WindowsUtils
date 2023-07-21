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
		const WuString& imagepath,								// The process image file path.
		ProcessAndThread::VERSION_INFO_PROPERTY propname,	// The property name, or 'key' value.
		WuString& value										// The return value.
	) {
		DWORD result = ERROR_SUCCESS;
		WORD* langncodepage;
		UINT len;
		WCHAR* desc;
		WCHAR text[256];

		LPWSTR lppropname = { 0 };
		switch (propname)
		{
		case ProcessAndThread::FileDescription:
			lppropname = new WCHAR[16];
			wcscpy_s(lppropname, 16, L"FileDescription");
			break;
		case ProcessAndThread::ProductName:
			lppropname = new WCHAR[12];
			wcscpy_s(lppropname, 12, L"ProductName");
			break;
		case ProcessAndThread::FileVersion:
			lppropname = new WCHAR[12];
			wcscpy_s(lppropname, 12, L"FileVersion");
			break;
		case ProcessAndThread::CompanyName:
			lppropname = new WCHAR[12];
			wcscpy_s(lppropname, 12, L"CompanyName");
			break;
		default:
			break;
		}

		if (0 == lppropname)
			return ::GetLastError();

		size_t complen = 256 + wcslen(lppropname) + 2;

		DWORD verinfosize = ::GetFileVersionInfoSizeW(imagepath, NULL);
		LPVOID buffer = (LPVOID)::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, verinfosize);
		if (NULL == buffer)
			return ERROR_NOT_ENOUGH_MEMORY;

		if (!::GetFileVersionInfoW(imagepath, NULL, verinfosize, buffer))
			return GetLastError();

		if (!::VerQueryValueW(buffer, L"\\VarFileInfo\\Translation", (LPVOID*)&langncodepage, &len))
			return GetLastError();

		::StringCchPrintfW(text, complen, TEXT("\\StringFileInfo\\%04x%04x\\"), langncodepage[0], langncodepage[1]);

		std::wstring subbuffer(text);
		std::wstring propertyname(lppropname);
		std::wstring resulttest = subbuffer + propertyname;

		if (::VerQueryValueW(buffer, resulttest.c_str(), (LPVOID*)&desc, &len))
		{

			size_t descsz = wcslen(desc) + 1;
			if (descsz > 1 && !IsNullOrWhiteSpace(desc))
			{
				value = new WCHAR[descsz];
				wcscpy_s(value, descsz, desc);
			}
			else
				value = new WCHAR[1]{ 0 };
		}
		else
			value = new WCHAR[1]{ 0 };

		if (NULL != buffer)
			::HeapFree(::GetProcessHeap(), NULL, buffer);

		delete[] lppropname;

		return result;
	}

	/*
	* Used by the parameter -CloseHandle, from Get-ObjectHandle.
	*/
	DWORD CloseExtProcessHandle(
		HANDLE hExtProcess,				// A valid handle to the external process.
		const WuString& objectName		// The object name, used on GetProcessObjectHandle.
	)
	{
		DWORD result = ERROR_SUCCESS;
		NTSTATUS ntcall = STATUS_SUCCESS;
		HANDLE htarget = NULL;

		LPWSTR lpcpathnoroot = ::PathSkipRootW(rlpcobjectname);
		if (NULL == lpcpathnoroot)
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
			ntcall = NtQueryInformationProcess(rhextprocess, ProcessHandleInformation, buffer.get(), szbuffer, &szbuffneed);

			if (STATUS_SUCCESS != ntcall && STATUS_INFO_LENGTH_MISMATCH != ntcall)
				return ntcall;

			if (STATUS_SUCCESS == ntcall)
				break;

			szbuffer = szbuffneed;

		} while (ntcall == STATUS_INFO_LENGTH_MISMATCH);

		PPROCESS_HANDLE_SNAPSHOT_INFORMATION phsnapinfo = reinterpret_cast<PPROCESS_HANDLE_SNAPSHOT_INFORMATION>(buffer.get());

		for (ULONG i = 0; i < phsnapinfo->NumberOfHandles; i++)
		{
			POBJECT_NAME_INFORMATION pobjnameinfo = (POBJECT_NAME_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(OBJECT_NAME_INFORMATION));
			if (NULL == pobjnameinfo)
				return ERROR_NOT_ENOUGH_MEMORY;

			if (!::DuplicateHandle(rhextprocess, phsnapinfo->Handles[i].HandleValue, ::GetCurrentProcess(), &htarget, 0, FALSE, DUPLICATE_SAME_ACCESS))
				continue;

			ntcall = NtQueryObjectWithTimeout(htarget, ObjectNameInformation, pobjnameinfo, 200);
			if (STATUS_SUCCESS != ntcall)
				return ntcall;

			::CloseHandle(htarget);

			if (pobjnameinfo->Name.Buffer && EndsWith(pobjnameinfo->Name.Buffer, lpcpathnoroot))
			{
				if (!::DuplicateHandle(rhextprocess, phsnapinfo->Handles[i].HandleValue, ::GetCurrentProcess(), &htarget, 0, FALSE, DUPLICATE_CLOSE_SOURCE))
					result = GetLastError();

				else
				{
					::CloseHandle(htarget);
					break;
				}
			}
		}

		return result;
	}
}