#pragma once

#include "Unmanaged.h"
#include <shlobj.h>
#include <strsafe.h>
#include <shlwapi.h>

namespace WindowsUtils::Core
{
	typedef struct _IO_STATUS_BLOCK {
		union {
			NTSTATUS Status;
			PVOID Pointer;
		} DUMMYUNIONNAME;

		ULONG_PTR Information;
	} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;

	typedef  struct _FILE_PROCESS_IDS_USING_FILE_INFORMATION {
		ULONG NumberOfProcessIdsInList;
		ULONG_PTR ProcessIdList[1];
	} FILE_PROCESS_IDS_USING_FILE_INFORMATION, * PFILE_PROCESS_IDS_USING_FILE_INFORMATION;

	typedef struct _FileProperty {
		PROPERTYKEY	Property;
		LPWSTR		Value;

		_FileProperty(PROPERTYKEY key, LPWSTR value) : Property(key), Value(value) { };

	}FileProperty, * PFileProperty;

	typedef NTSTATUS(__stdcall* _NtQueryInformationFile)(
		HANDLE				fileHandle,
		PIO_STATUS_BLOCK	ioStatusBlock,
		PVOID				fileInformation,
		ULONG				length,
		UINT				fileInformationClass
		);

	NTSTATUS GetNtProcessUsingFileList(LPCWSTR filename, PFILE_PROCESS_IDS_USING_FILE_INFORMATION& pfpidfileinfo);
}