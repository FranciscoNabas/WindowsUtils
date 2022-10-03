#include "pch.h"
#include "Unmanaged.h"
#include "Utilities.h"

namespace WindowsUtils
{
	/*
		TODO: improve error handling with the IO_STATUS_BLOCK structure
	*/
	NTSTATUS GetNtProcessUsingFileList(LPCWSTR filename, PFILE_PROCESS_IDS_USING_FILE_INFORMATION& pfpidfileinfo)
	{
		NTSTATUS result = 0;
		ULONG infosize = sizeof(FILE_PROCESS_IDS_USING_FILE_INFORMATION);
		
		PIO_STATUS_BLOCK piostatblock = (PIO_STATUS_BLOCK)LocalAlloc(LMEM_ZEROINIT, sizeof(IO_STATUS_BLOCK));
		if (nullptr == piostatblock)
			return ERROR_NOT_ENOUGH_MEMORY;

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
		
		LocalFree(piostatblock);
		CloseHandle(hfile);

		return result;
	}
}