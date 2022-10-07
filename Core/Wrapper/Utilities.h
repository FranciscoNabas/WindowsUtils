#pragma once

#include "Unmanaged.h"
#include <shlobj.h>
#include <strsafe.h>
#include <shlwapi.h>

#define STATUS_BUFFER_TOO_SMALL 0xC0000023L
#define STATUS_BUFFER_OVERFLOW 0x80000005
#define SystemHandleInformation 16
#define ObjectNameInformation 1
#define REG_KEY_PATH_LENGTH 1024

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

	typedef struct _UNICODE_STRING {
		USHORT Length;
		USHORT MaximumLength;
		PWSTR Buffer;
	} UNICODE_STRING, * PUNICODE_STRING;

	typedef enum _POOL_TYPE {
		NonPagedPool,
		PagedPool,
		NonPagedPoolMustSucceed,
		DontUseThisType,
		NonPagedPoolCacheAligned,
		PagedPoolCacheAligned,
		NonPagedPoolCacheAlignedMustS
	} POOL_TYPE, * PPOOL_TYPE;

	typedef struct _OBJECT_TYPE_INFORMATION {
		UNICODE_STRING Name;
		ULONG TotalNumberOfObjects;
		ULONG TotalNumberOfHandles;
		ULONG TotalPagedPoolUsage;
		ULONG TotalNonPagedPoolUsage;
		ULONG TotalNamePoolUsage;
		ULONG TotalHandleTableUsage;
		ULONG HighWaterNumberOfObjects;
		ULONG HighWaterNumberOfHandles;
		ULONG HighWaterPagedPoolUsage;
		ULONG HighWaterNonPagedPoolUsage;
		ULONG HighWaterNamePoolUsage;
		ULONG HighWaterHandleTableUsage;
		ULONG InvalidAttributes;
		GENERIC_MAPPING GenericMapping;
		ULONG ValidAccess;
		BOOLEAN SecurityRequired;
		BOOLEAN MaintainHandleCount;
		USHORT MaintainTypeList;
		POOL_TYPE PoolType;
		ULONG PagedPoolUsage;
		ULONG NonPagedPoolUsage;
	} OBJECT_TYPE_INFORMATION, * POBJECT_TYPE_INFORMATION;

	typedef struct _KEY_NAME_INFORMATION {
		ULONG NameLength;
		WCHAR Name[1];
	} KEY_NAME_INFORMATION, * PKEY_NAME_INFORMATION;

	typedef enum _KEY_INFORMATION_CLASS {
		KeyBasicInformation,
		KeyNodeInformation,
		KeyFullInformation,
		KeyNameInformation,
		KeyCachedInformation,
		KeyFlagsInformation,
		KeyVirtualizationInformation,
		KeyHandleTagsInformation,
		KeyTrustInformation,
		KeyLayerInformation,
		MaxKeyInfoClass
	} KEY_INFORMATION_CLASS;

	typedef NTSTATUS(__stdcall* _NtQueryInformationFile)(
		HANDLE				fileHandle,
		PIO_STATUS_BLOCK	ioStatusBlock,
		PVOID				fileInformation,
		ULONG				length,
		UINT				fileInformationClass
		);

	typedef NTSTATUS(__stdcall* _NtQuerySystemInformation)(
		ULONG	SystemInformationClass,
		PVOID	SystemInformation,
		ULONG	SystemInformationLength,
		PULONG	ReturnLength);

	typedef NTSTATUS(__stdcall* _NtQueryObject)(
		HANDLE						Handle,
		OBJECT_INFORMATION_CLASS	ObjectInformationClass,
		PVOID						ObjectInformation,
		ULONG						ObjectInformationLength,
		PULONG						ReturnLength
		);

	typedef NTSTATUS(__stdcall* _NtDuplicateObject)(
		HANDLE		SourceProcessHandle,
		HANDLE		SourceHandle,
		HANDLE		TargetProcessHandle,
		PHANDLE		TargetHandle,
		ACCESS_MASK	DesiredAccess,
		ULONG		Attributes,
		ULONG		Options
		);

	typedef NTSTATUS(__stdcall* _NtQueryKey)(
		HANDLE		KeyHandle,
		int			KeyinformationClass,
		PVOID		KeyInformation,
		ULONG		Length,
		PULONG		ResultLength
		);

	NTSTATUS GetNtProcessUsingFileList(LPCWSTR filename, PFILE_PROCESS_IDS_USING_FILE_INFORMATION& pfpidfileinfo);
	NTSTATUS GetNtObjectInformation(Unmanaged::SYSTEM_HANDLE handleObject, LPWSTR& objTypeName, LPWSTR& keyinfo);
	NTSTATUS GetNtKeyInformation(HANDLE hkey, HMODULE hntdll, LPWSTR& keypath);
}