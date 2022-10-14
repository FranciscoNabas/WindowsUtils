#pragma once

#include "Unmanaged.h"
#include <shlobj.h>
#include <strsafe.h>
#include <shlwapi.h>

#define STATUS_BUFFER_TOO_SMALL 0xC0000023L
#define STATUS_BUFFER_OVERFLOW 0x80000005
#define SystemHandleInformation 16
#define REG_KEY_PATH_LENGTH 1024

namespace WindowsUtils::Core
{
	typedef enum _OBJECT_INFORMATION_CLASS {
		ObjectNameInformation = 1,
		ObjectTypeInformation = 2
	}OBJECT_INFORMATION_CLASS;

	typedef enum _PROCESSINFOCLASS {
		ProcessBasicInformation = 0,
		ProcessDebugPort = 7,
		ProcessWow64Information = 26,
		ProcessImageFileName = 27,
		ProcessBreakOnTermination = 29,
		ProcessHandleInformation = 51
	} PROCESSINFOCLASS;

	typedef struct _PROCESS_HANDLE_TABLE_ENTRY_INFO
	{
		HANDLE HandleValue;
		ULONG_PTR HandleCount;
		ULONG_PTR PointerCount;
		ULONG GrantedAccess;
		ULONG ObjectTypeIndex;
		ULONG HandleAttributes;
		ULONG Reserved;
	} PROCESS_HANDLE_TABLE_ENTRY_INFO, * PPROCESS_HANDLE_TABLE_ENTRY_INFO;


	typedef struct _PROCESS_HANDLE_SNAPSHOT_INFORMATION {
		ULONG_PTR						NumberOfHandles;
		ULONG_PTR						Reserved;
		PROCESS_HANDLE_TABLE_ENTRY_INFO	Handles[1];
	}PROCESS_HANDLE_SNAPSHOT_INFORMATION, *PPROCESS_HANDLE_SNAPSHOT_INFORMATION;

	typedef enum _FILE_INFORMATION_CLASS {
		FileProcessIdsUsingFileInformation = 47
	}FILE_INFORMATION_CLASS;

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
		UNICODE_STRING TypeName;
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
		ULONG ValidAccessMask;
		BOOLEAN SecurityRequired;
		BOOLEAN MaintainHandleCount;
		UCHAR TypeIndex;
		CHAR ReservedByte;
		ULONG PoolType;
		ULONG DefaultPagedPoolCharge;
		ULONG DefaultNonPagedPoolCharge;
	} OBJECT_TYPE_INFORMATION, * POBJECT_TYPE_INFORMATION;

	typedef struct _OBJECT_NAME_INFORMATION {
		UNICODE_STRING Name;
	}OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

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

	typedef struct _THREAD_FUNC_ARGUMENTS {
		HANDLE						ObjectHandle;
		OBJECT_INFORMATION_CLASS	ObjectInformationClass;
		PVOID						ObjectInfo;
		ULONG						BufferSize;
		LPWSTR						ObjectName;
	}THREAD_FUNC_ARGUMENTS, *PTHREAD_FUNC_ARGUMENTS;

	typedef NTSTATUS(NTAPI* _NtQueryInformationFile)(
		HANDLE					FileHandle,
		PIO_STATUS_BLOCK		IoStatusBlock,
		PVOID					FileInformation,
		ULONG					Length,
		FILE_INFORMATION_CLASS	FileInformationClass
		);

	typedef NTSTATUS(NTAPI* _NtQuerySystemInformation)(
		ULONG	SystemInformationClass,
		PVOID	SystemInformation,
		ULONG	SystemInformationLength,
		PULONG	ReturnLength);

	typedef NTSTATUS(NTAPI* _NtQueryObject)(
		HANDLE						Handle,
		OBJECT_INFORMATION_CLASS	ObjectInformationClass,
		PVOID						ObjectInformation,
		ULONG						ObjectInformationLength,
		PULONG						ReturnLength
		);

	typedef NTSTATUS(NTAPI* _NtDuplicateObject)(
		HANDLE		SourceProcessHandle,
		HANDLE		SourceHandle,
		HANDLE		TargetProcessHandle,
		PHANDLE		TargetHandle,
		ACCESS_MASK	DesiredAccess,
		ULONG		Attributes,
		ULONG		Options
		);

	typedef NTSTATUS(NTAPI* _NtQueryKey)(
		HANDLE		KeyHandle,
		int			KeyinformationClass,
		PVOID		KeyInformation,
		ULONG		Length,
		PULONG		ResultLength
		);

	typedef NTSTATUS(NTAPI* _NtQueryInformationProcess)(
		HANDLE				ProcessHandle,
		PROCESSINFOCLASS	ProcessInformationClass,
		PVOID				ProcessInformation,
		ULONG				ProcessInformationLength,
		PULONG				ReturnLength
		);

	NTSTATUS GetNtProcessUsingFileList(LPCWSTR filename, PFILE_PROCESS_IDS_USING_FILE_INFORMATION& pfpidfileinfo);
	NTSTATUS GetNtObjectInformation(Unmanaged::SYSTEM_HANDLE handleObject, LPWSTR& objTypeName, LPWSTR& keyinfo);
	NTSTATUS GetNtKeyInformation(HANDLE hkey, HMODULE hntdll, LPWSTR& keypath);
	NTSTATUS WINAPI NtQueryObjectWithTimeout(HANDLE hobject, OBJECT_INFORMATION_CLASS objinfoclass, PVOID objinfo, ULONG mstimeout);
	DWORD WINAPI NtQueryObjectRaw(LPVOID lpparam);
}