#include "pch.h"
#include "NtUtilities.h"

namespace WindowsUtils::Core
{
	NTSTATUS WINAPI GetNtProcessUsingFile(
		LPCWSTR& rlpcfilename												// File full name.
		, PFILE_PROCESS_IDS_USING_FILE_INFORMATION& rpprocusingfileinfo		// Output with a list of process IDs with handles to the file.
	)
	{
		NTSTATUS result = STATUS_SUCCESS;
		IO_STATUS_BLOCK iostatblock = { 0 };

		HMODULE hmodule = GetModuleHandleW(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hmodule || 0 == hmodule)
			return GetLastError();

		_NtQueryInformationFile NtQueryInformationFile = (_NtQueryInformationFile)GetProcAddress(hmodule, "NtQueryInformationFile");
		if (NULL == NtQueryInformationFile)
			return GetLastError();

		HANDLE hfile = CreateFileW(
			rlpcfilename
			, FILE_READ_ATTRIBUTES
			, FILE_SHARE_READ
			, NULL
			, OPEN_EXISTING
			, FILE_FLAG_BACKUP_SEMANTICS
			, NULL
		);
		if (INVALID_HANDLE_VALUE == hfile)
			return GetLastError();

		ULONG szbuffer = 1 << 10;
		std::unique_ptr<BYTE[]> buffer;
		do
		{
			buffer = std::make_unique<BYTE[]>(szbuffer);
			result = NtQueryInformationFile(hfile, &iostatblock, buffer.get(), szbuffer, FileProcessIdsUsingFileInformation);
			if (STATUS_SUCCESS != result && STATUS_INFO_LENGTH_MISMATCH != result)
				return GetLastError();

			if (STATUS_SUCCESS == result)
				break;

			szbuffer = (ULONG)iostatblock.Information;

		} while (result == STATUS_INFO_LENGTH_MISMATCH);

		rpprocusingfileinfo = (PFILE_PROCESS_IDS_USING_FILE_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (ULONG)iostatblock.Information);
		CopyMemory(rpprocusingfileinfo, reinterpret_cast<PFILE_PROCESS_IDS_USING_FILE_INFORMATION>(buffer.get()), (ULONG)iostatblock.Information);

		CloseHandle(hfile);

		return result;
	}

	DWORD WINAPI NtQueryObjectRaw(LPVOID lpparam)
	{
		NTSTATUS result = STATUS_SUCCESS;
		PTHREAD_FUNC_ARGUMENTS tfuncargs = reinterpret_cast<PTHREAD_FUNC_ARGUMENTS>(lpparam);

		HMODULE hmodule = GetModuleHandleW(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hmodule || 0 == hmodule)
			return result;

		_NtQueryObject NtQueryObject = (_NtQueryObject)GetProcAddress(hmodule, "NtQueryObject");
		if (NULL == NtQueryObject)
			return GetLastError();

		if (tfuncargs->ObjectInformationClass == ObjectNameInformation)
		{
			ULONG szbuffneeded = 0;
			ULONG szbuffer = (ULONG)sizeof(OBJECT_NAME_INFORMATION);

			tfuncargs->ObjectInfo = (POBJECT_NAME_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(OBJECT_NAME_INFORMATION));
			if (NULL == tfuncargs->ObjectInfo)
				return ERROR_NOT_ENOUGH_MEMORY;

			do
			{
				result = NtQueryObject(tfuncargs->ObjectHandle, ObjectNameInformation, tfuncargs->ObjectInfo, szbuffer, &szbuffneeded);
				if (STATUS_SUCCESS != result
					&& STATUS_INFO_LENGTH_MISMATCH != result
					&& result != STATUS_BUFFER_OVERFLOW
					&& result != STATUS_BUFFER_TOO_SMALL)
					return result;

				if (STATUS_SUCCESS == result)
					break;

				szbuffer = szbuffneeded;
				HeapFree(GetProcessHeap(), NULL, tfuncargs->ObjectInfo);

				tfuncargs->ObjectInfo = (POBJECT_NAME_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, szbuffer);
				if (NULL == tfuncargs->ObjectInfo)
					return ERROR_NOT_ENOUGH_MEMORY;

			} while (result == STATUS_BUFFER_TOO_SMALL || result == STATUS_BUFFER_OVERFLOW || result == STATUS_INFO_LENGTH_MISMATCH);
		}
		else
		{
			ULONG szbuffneeded = 0;
			ULONG szbuffer = (ULONG)sizeof(POBJECT_TYPE_INFORMATION);

			tfuncargs->ObjectInfo = (POBJECT_TYPE_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(POBJECT_TYPE_INFORMATION));
			if (NULL == tfuncargs->ObjectInfo)
				return ERROR_NOT_ENOUGH_MEMORY;

			do
			{
				result = NtQueryObject(tfuncargs->ObjectHandle, ObjectNameInformation, tfuncargs->ObjectInfo, szbuffer, &szbuffneeded);
				if (STATUS_SUCCESS != result
					&& STATUS_INFO_LENGTH_MISMATCH != result
					&& result != STATUS_BUFFER_OVERFLOW
					&& result != STATUS_BUFFER_TOO_SMALL)
					return result;

				if (STATUS_SUCCESS == result)
					break;

				szbuffer = szbuffneeded;
				HeapFree(GetProcessHeap(), NULL, tfuncargs->ObjectInfo);

				tfuncargs->ObjectInfo = (POBJECT_TYPE_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, szbuffer);
				if (NULL == tfuncargs->ObjectInfo)
					return ERROR_NOT_ENOUGH_MEMORY;

			} while (result == STATUS_BUFFER_TOO_SMALL || result == STATUS_BUFFER_OVERFLOW || result == STATUS_INFO_LENGTH_MISMATCH);
		}

		return result;
	}

	/*
	* When calling NtQueryObject, the procedure hangs for certain granted accesses and object types.
	* There isn't a pattern, nor documentation on this behavior.
	* We use a separate thread that gets terminated after a timeout.
	*/
	NTSTATUS WINAPI NtQueryObjectWithTimeout(
		HANDLE hobject								// A valid handle to the object.
		, OBJECT_INFORMATION_CLASS objinfoclass		// One of the OBJECT_INFORMATION_CLASS enumerations.
		, PVOID pobjinfo							// Object containing the queried information. The type of object depends on the object information class.
		, ULONG mstimeout							// Maximum timeout in milliseconds.
	)
	{
		NTSTATUS result = STATUS_SUCCESS;
		PTHREAD_FUNC_ARGUMENTS threadcallargs = new THREAD_FUNC_ARGUMENTS;
		DWORD dwthreadid = 0;
		DWORD dwthexitcode = STATUS_SUCCESS;
		HANDLE hthread = NULL;

		threadcallargs->ObjectHandle = hobject;
		threadcallargs->ObjectInformationClass = objinfoclass;

		hthread = CreateThread(NULL, 0, NtQueryObjectRaw, threadcallargs, 0, &dwthreadid);
		if (NULL == hthread)
			return GetLastError();

		DWORD wait = WaitForSingleObject(hthread, mstimeout);
		if (wait != WAIT_OBJECT_0)
			TerminateThread(hthread, 0);
		else
		{
			GetExitCodeThread(hthread, &dwthexitcode);
			CloseHandle(hthread);
		}

		size_t szbuffer = 0;
		size_t szpobjinfo = 0;
		if (objinfoclass == ObjectNameInformation)
		{
			szbuffer = sizeof(*reinterpret_cast<POBJECT_NAME_INFORMATION>(threadcallargs->ObjectInfo));
			szpobjinfo = sizeof(*reinterpret_cast<POBJECT_NAME_INFORMATION>(threadcallargs->ObjectInfo));

			if (szbuffer > szpobjinfo)
			{
				HeapFree(GetProcessHeap(), NULL, pobjinfo);
				pobjinfo = (POBJECT_NAME_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, szbuffer);
				if (NULL == pobjinfo)
					return ERROR_NOT_ENOUGH_MEMORY;
			}

			CopyMemory(pobjinfo, threadcallargs->ObjectInfo, szbuffer);
		}
		else
		{
			szbuffer = sizeof(*reinterpret_cast<POBJECT_TYPE_INFORMATION>(threadcallargs->ObjectInfo));
			szpobjinfo = sizeof(*reinterpret_cast<POBJECT_TYPE_INFORMATION>(threadcallargs->ObjectInfo));

			if (szbuffer > szpobjinfo)
			{
				HeapFree(GetProcessHeap(), NULL, pobjinfo);
				pobjinfo = (POBJECT_TYPE_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, szbuffer);
				if (NULL == pobjinfo)
					return ERROR_NOT_ENOUGH_MEMORY;
			}

			CopyMemory(pobjinfo, threadcallargs->ObjectInfo, szbuffer);
		}

		return result;
	}

	/*
	* Alternative to QueryFullProcessImageNameW.
	* This function returns names from processes like, System, Registry or Secure System.
	*/
	NTSTATUS WINAPI GetProcessImageName(DWORD dwprocessid, LPWSTR& rlpimagename)
	{
		NTSTATUS result = STATUS_SUCCESS;
		ULONG szbuffer = 1 << 12;
		ULONG szneededb = 0;

		HMODULE hmodule = GetModuleHandleW(L"ntdll.dll");
		if (INVALID_HANDLE_VALUE == hmodule || NULL == hmodule)
			return result;

		_NtQuerySystemInformation NtQuerySystemInformation = (_NtQuerySystemInformation)GetProcAddress(hmodule, "NtQuerySystemInformation");

		std::unique_ptr<BYTE[]> buffer;
		do
		{
			buffer = std::make_unique<BYTE[]>(szbuffer);
			result = NtQuerySystemInformation(SystemFullProcessInformation, buffer.get(), szbuffer, &szneededb);
			if (STATUS_SUCCESS != result
				&& STATUS_INFO_LENGTH_MISMATCH != result
				&& result != STATUS_BUFFER_OVERFLOW
				&& result != STATUS_BUFFER_TOO_SMALL)
				return result;

			if (STATUS_SUCCESS == result)
				break;

			szbuffer = szneededb;

		} while (result == STATUS_BUFFER_TOO_SMALL || result == STATUS_BUFFER_OVERFLOW || result == STATUS_INFO_LENGTH_MISMATCH);

		PSYSTEM_PROCESS_INFORMATION psysprocinfo = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(buffer.get());
		do
		{
			if (psysprocinfo->UniqueProcessId == (HANDLE)dwprocessid)
			{
				size_t szimgname = wcslen(psysprocinfo->ImageName.Buffer) + 1;
				if (szimgname > 1)
				{
					rlpimagename = new WCHAR[szimgname];
					wcscpy_s(rlpimagename, szimgname, psysprocinfo->ImageName.Buffer);
				}
				break;
			}
			psysprocinfo = (PSYSTEM_PROCESS_INFORMATION)((PBYTE)psysprocinfo + psysprocinfo->NextEntryOffset);

		} while (psysprocinfo->NextEntryOffset != 0);

		return result;
	}
}