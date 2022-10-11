#include "pch.h"
#include "Unmanaged.h"
#include "Utilities.h"


namespace WindowsUtils::Core
{
	/*
		TODO: improve error handling with the IO_STATUS_BLOCK structure
	*/
	NTSTATUS GetNtProcessUsingFileList(LPCWSTR filename, PFILE_PROCESS_IDS_USING_FILE_INFORMATION& pfpidfileinfo)
	{
		NTSTATUS result = 0;
		ULONG infosize = sizeof(FILE_PROCESS_IDS_USING_FILE_INFORMATION);
		
		PIO_STATUS_BLOCK piostatblock = (PIO_STATUS_BLOCK)LocalAlloc(LMEM_ZEROINIT, sizeof(IO_STATUS_BLOCK));
		ALLCHECK(piostatblock);

		HMODULE hmodule = GetModuleHandleW(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hmodule || 0 == hmodule)
			return GetLastError();

		_NtQueryInformationFile NtQueryInformationFile = (_NtQueryInformationFile)GetProcAddress(hmodule, "NtQueryInformationFile");

		HANDLE hfile = CreateFileW(filename, FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (INVALID_HANDLE_VALUE == hfile)
			return GetLastError();

		/*
			Initially, we allocate enough memory on 'pfpidfileinfo' to hold information about one process.
			If the memory is not big enough, 'NtQueryInformationFile' returns 'STATUS_INFO_LENGTH_MISMATCH'
			And the 'Information' property of the IO_STATUS_BLOCK is the size necessary.
			So we reallocate enough space to hold the process information.
		*/
		do
		{
			result = NtQueryInformationFile(hfile, piostatblock, pfpidfileinfo, infosize, 47);
			if (STATUS_SUCCESS != result && STATUS_INFO_LENGTH_MISMATCH != result)
				return result;

			if (STATUS_SUCCESS == result)
				break;

			infosize = (ULONG)piostatblock->Information;
			LocalFree(pfpidfileinfo);
			
			pfpidfileinfo = (PFILE_PROCESS_IDS_USING_FILE_INFORMATION)LocalAlloc(LMEM_ZEROINIT, infosize);
			if (NULL == pfpidfileinfo)
				return (NTSTATUS)GetLastError();


		} while (result == STATUS_INFO_LENGTH_MISMATCH);
		
		LOCFREEWCHECK(piostatblock);
		CloseHandle(hfile);

		return result;
	}

	NTSTATUS Unmanaged::GetNtSystemInformation(std::vector<SystemHandleOutInfo>& pvout)
	{
		NTSTATUS result = 0;
		ULONG infosize = 0x10000;
		ULONG neededsize = 0;

		PSYSTEM_HANDLE_INFORMATION psystemhinfo = (PSYSTEM_HANDLE_INFORMATION)LocalAlloc(LMEM_ZEROINIT, infosize);
		ALLCHECK(psystemhinfo);

		HMODULE hmodule = GetModuleHandleW(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hmodule || 0 == hmodule)
			return GetLastError();

		_NtQuerySystemInformation NtQuerySystemInformation = (_NtQuerySystemInformation)GetProcAddress(hmodule, "NtQuerySystemInformation");
		_NtDuplicateObject NtDuplicateObject = (_NtDuplicateObject)GetProcAddress(hmodule, "NtDuplicateObject");

		do
		{
			result = NtQuerySystemInformation(SystemHandleInformation, psystemhinfo, infosize, &neededsize);

			if (STATUS_SUCCESS != result && STATUS_INFO_LENGTH_MISMATCH != result)
				return result;

			if (STATUS_SUCCESS == result)
				break;

			infosize = neededsize;
			LocalFree(psystemhinfo);
			psystemhinfo = (PSYSTEM_HANDLE_INFORMATION)LocalAlloc(LMEM_ZEROINIT, infosize);

		} while (result == STATUS_INFO_LENGTH_MISMATCH);

		UINT count = 0;
		for (ULONG i = 0; i < psystemhinfo->HandleCount; i++)
		{
			SYSTEM_HANDLE single = psystemhinfo->Handles[i];
			PSystemHandleOutInfo inner = new SystemHandleOutInfo();

			/*
				Trying to get a handle to the system process is no bueno from User mode.
				Some of these processes ID changes between logon sessions.
				This is for testing purpose only.
			*/
			if (single.ProcessId != 4 // System
				&& single.ProcessId != 552 // smss
				&& single.ProcessId != 652 // csrss
				&& single.ProcessId != 948 // csrss
				&& single.ProcessId != 484 //wininit
				&& single.ProcessId != 952 // services
				&& single.ProcessId != 1044 //Lsalso
				&& single.ProcessId != 1052 // lsass
				&& single.ProcessId != 1120) // winlogon
			{
				result = GetNtObjectInformation(single, inner->ObjectTypeName, inner->ExtPropertyInfo);
			}
			inner->Flags = single.Flags;
			inner->GrantedAccess = single.GrantedAccess;
			inner->ObjectTypeNumber = single.ObjectTypeNumber;
			inner->ProcessId = single.ProcessId;

			pvout.push_back(*inner);

			delete inner;
		}

		LOCFREEWCHECK(psystemhinfo);

		return result;
	}

	NTSTATUS GetNtObjectInformation(Unmanaged::SYSTEM_HANDLE handleObject, LPWSTR& objTypeName, LPWSTR& keyinfo)
	{
		NTSTATUS result = STATUS_SUCCESS;
		HANDLE hdup = INVALID_HANDLE_VALUE;
		HANDLE hprocess = INVALID_HANDLE_VALUE;
		ULONG neededsz = 0;
		ULONG infosize = (ULONG)sizeof(OBJECT_TYPE_INFORMATION);

		HMODULE hmodule = GetModuleHandleW(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hmodule || 0 == hmodule)
			return GetLastError();

		_NtDuplicateObject NtDuplicateObject = (_NtDuplicateObject)GetProcAddress(hmodule, "NtDuplicateObject");
		_NtQueryObject NtQueryObject = (_NtQueryObject)GetProcAddress(hmodule, "NtQueryObject");

		POBJECT_TYPE_INFORMATION objinfo = (POBJECT_TYPE_INFORMATION)LocalAlloc(LMEM_ZEROINIT, infosize);
		ALLCHECK(objinfo);

		hprocess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, handleObject.ProcessId);
		if (INVALID_HANDLE_VALUE == hprocess || NULL == hprocess)
			return GetLastError();

		result = NtDuplicateObject(hprocess, (HANDLE)handleObject.Handle, GetCurrentProcess(), &hdup, 0, 0, DUPLICATE_SAME_ACCESS);
		NTSTATUSCHECK(result, FALSE);

		do
		{
			result = NtQueryObject(hdup, ObjectTypeInformation, objinfo, infosize, &neededsz);

			if (STATUS_SUCCESS != result && STATUS_INFO_LENGTH_MISMATCH != result)
				return result;

			if (STATUS_SUCCESS == result)
				break;

			infosize = neededsz;
			LOCFREEWCHECK(objinfo);
			objinfo = (POBJECT_TYPE_INFORMATION)LocalAlloc(LMEM_ZEROINIT, infosize);

		} while (result == STATUS_INFO_LENGTH_MISMATCH);
		
		size_t tnamesize = wcslen(objinfo->Name.Buffer) + 1;
		objTypeName = new WCHAR[tnamesize];

		wcscpy_s(objTypeName, tnamesize, objinfo->Name.Buffer);
		if (wcscmp(objTypeName, L"Key") == 0)
		{
			result = GetNtKeyInformation(hdup, hmodule, keyinfo);
		}
				
		LOCFREEWCHECK(objinfo);
		CloseHandle(hdup);
		CloseHandle(hprocess);

		return result;
	}

	NTSTATUS GetNtKeyInformation(HANDLE hkey, HMODULE hntdll, LPWSTR& keypath)
	{
		NTSTATUS result = STATUS_SUCCESS;
		ULONG strsize = 0;
		ULONG sizeneeded = 0;
		std::wstring _keypath;

		_NtQueryKey NtQueryKey = (_NtQueryKey)GetProcAddress(hntdll, "NtQueryKey");

		ULONG infosize = (ULONG)sizeof(KEY_NAME_INFORMATION);
		PKEY_NAME_INFORMATION pknameinfo;

		std::vector<UCHAR> buffer(FIELD_OFFSET(KEY_NAME_INFORMATION, Name) + sizeof(WCHAR) * REG_KEY_PATH_LENGTH);
		do
		{
			result = NtQueryKey(hkey, KeyNameInformation, buffer.data(), (ULONG)buffer.size(), &sizeneeded);
			
			// Why have only one buffer size error, when we can have two and confuse everyone?
			if (STATUS_SUCCESS != result && STATUS_BUFFER_TOO_SMALL != result && result != STATUS_BUFFER_OVERFLOW)
				return result;

			if (STATUS_SUCCESS == result)
				break;

			buffer.resize(sizeneeded);

		} while (result == STATUS_BUFFER_TOO_SMALL || result == STATUS_BUFFER_OVERFLOW);

		pknameinfo = reinterpret_cast<PKEY_NAME_INFORMATION>(buffer.data());
		_keypath.assign(pknameinfo->Name, pknameinfo->NameLength / sizeof(WCHAR));


		strsize = (ULONG)wcslen(_keypath.c_str()) + 1;
		keypath = new WCHAR[strsize];

		wcscpy_s(keypath, strsize, _keypath.c_str());

		return result;

	}
}