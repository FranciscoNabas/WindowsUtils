#include "pch.h"

#include "ProcessAndThread.h"

namespace WindowsUtils::Core
{
	// Get-ObjectHandle
	DWORD ProcessAndThread::GetProcessObjectHandle(
		std::vector<ProcessAndThread::WU_OBJECT_HANDLE>& rvecobjhandle	// A vector of object handle output objects.
		, std::vector<LPCWSTR>& rvecinputpath					// A vector of input objects.
		, BOOL closehandle = FALSE								// TRUE for closing the handles found.
	)
	{
		DWORD result = ERROR_SUCCESS;

		for (size_t i = 0; i < rvecinputpath.size(); i++)
		{
			/*
			* This strips the path out of the input object so we can add it to the 'InputObject' property.
			* This will need to be changed in the future to support multiple object types.
			*/
			WCHAR lpinputobj[MAX_PATH] = { 0 };
			wcscpy_s(lpinputobj, MAX_PATH, rvecinputpath.at(i));
			PathStripPathW(lpinputobj);

			PFILE_PROCESS_IDS_USING_FILE_INFORMATION pprocidufile = NULL;
			result = GetNtProcessUsingFile(rvecinputpath.at(i), pprocidufile);
			if (result != ERROR_SUCCESS)
				return result;

			for (ULONG j = 0; j < pprocidufile->NumberOfProcessIdsInList; j++)
			{
				PWU_OBJECT_HANDLE uphobj = new WU_OBJECT_HANDLE;
				HANDLE hprocess = NULL;

				size_t szinputobj = wcslen(lpinputobj) + 1;
				uphobj->InputObject = new WCHAR[szinputobj];
				wcscpy_s(uphobj->InputObject, szinputobj, lpinputobj);

				uphobj->ProcessId = (DWORD)pprocidufile->ProcessIdList[j];

				if (uphobj->ProcessId == 10220)
					result = 0;

				hprocess = OpenProcess(
					PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE
					, FALSE
					, (DWORD)pprocidufile->ProcessIdList[j]
				);
				if (NULL != hprocess)
				{
					DWORD szmaxpath = MAX_PATH;
					uphobj->ImagePath = new WCHAR[MAX_PATH]{ 0 };
					uphobj->Name = new WCHAR[MAX_PATH]{ 0 };
					::QueryFullProcessImageNameW(hprocess, 0, uphobj->ImagePath, &szmaxpath);

					if (wcslen(uphobj->ImagePath) == 0)
					{
						result = GetProcessImageName(uphobj->ProcessId, uphobj->Name);
						if (wcslen(uphobj->Name) > 0)
						{
							wcscpy_s(uphobj->ImagePath, MAX_PATH, uphobj->Name);
							::PathStripPathW(uphobj->Name);
						}

					}
					else
					{
						wcscpy_s(uphobj->Name, MAX_PATH, uphobj->ImagePath);
						::PathStripPathW(uphobj->Name);
					}

					for (VERSION_INFO_PROPERTY verinfoprop : { FileDescription, ProductName, FileVersion, CompanyName })
					{
						LPWSTR lpinter = { 0 };
						GetProccessVersionInfo(uphobj->ImagePath, verinfoprop, lpinter);
						uphobj->VersionInfo.emplace(std::make_pair(verinfoprop, LPCWSTR(lpinter)));
					}

					if (closehandle)
					{
						result = CloseExtProcessHandle(hprocess, rvecinputpath.at(i));
						if (ERROR_SUCCESS != result)
							return result;
					}
					::CloseHandle(hprocess);
				}
				else
				{
					if (ERROR_ACCESS_DENIED == ::GetLastError())
					{
						hprocess = OpenProcess(
							PROCESS_QUERY_LIMITED_INFORMATION
							, FALSE
							, (DWORD)pprocidufile->ProcessIdList[j]
						);

						DWORD szmaxpath = MAX_PATH;
						uphobj->ImagePath = new WCHAR[MAX_PATH]{ 0 };
						uphobj->Name = new WCHAR[MAX_PATH]{ 0 };
						if (::QueryFullProcessImageNameW(hprocess, 0, uphobj->ImagePath, &szmaxpath))
						{
							wcscpy_s(uphobj->Name, MAX_PATH, uphobj->ImagePath);
							::PathStripPathW(uphobj->Name);

							for (VERSION_INFO_PROPERTY verinfoprop : { FileDescription, ProductName, FileVersion, CompanyName })
							{
								LPWSTR lpinter;
								GetProccessVersionInfo(uphobj->ImagePath, verinfoprop, lpinter);
								uphobj->VersionInfo.emplace(std::make_pair(verinfoprop, LPCWSTR(lpinter)));
							}
						}
						else
						{
							if (ERROR_SUCCESS == GetProcessImageName(uphobj->ProcessId, uphobj->Name))
							{
								wcscpy_s(uphobj->ImagePath, MAX_PATH, uphobj->Name);
								::PathStripPathW(uphobj->Name);
							}

							if (
								wcscmp(uphobj->Name, L"System") == 0
								|| wcscmp(uphobj->Name, L"Secure System") == 0
								|| wcscmp(uphobj->Name, L"Registry") == 0)
							{
								LPCWSTR windir = L"windir";
								LPWSTR lpenvvalue;
								result = GetEnvVariable(windir, lpenvvalue);
								DWERRORCHECKV(result);

								std::wstring wstrwindir(lpenvvalue);
								std::wstring imagepath = L"\\System32\\ntoskrnl.exe";
								std::wstring fullimagepath = wstrwindir + imagepath;

								wcscpy_s(uphobj->ImagePath, MAX_PATH, fullimagepath.c_str());

								for (VERSION_INFO_PROPERTY verinfoprop : { FileDescription, ProductName, FileVersion, CompanyName })
								{
									LPWSTR lpinter;
									GetProccessVersionInfo(uphobj->ImagePath, verinfoprop, lpinter);
									uphobj->VersionInfo.emplace(std::make_pair(verinfoprop, LPCWSTR(lpinter)));
								}
							}
						}
					}
				}

				rvecobjhandle.push_back(*uphobj);

				delete uphobj;
			}
		}

		return result;
	}

	/*========================================
	==		Utility function definition		==
	==========================================*/

	// Gets process image version information.
	DWORD GetProccessVersionInfo(
		LPWSTR& imagepath,									// The process image file path.
		ProcessAndThread::VERSION_INFO_PROPERTY& propname,	// The property name, or 'key' value.
		LPWSTR& value										// The return value.
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
		HANDLE& rhextprocess		// A valid handle to the external process.
		, LPCWSTR& rlpcobjectname	// The object name, used on GetProcessObjectHandle.
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