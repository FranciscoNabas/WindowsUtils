#pragma once
#pragma unmanaged

#include "String.h"
#include "Expressions.h"

#include <SubAuth.h>

constexpr LONG STATUS_BUFFER_TOO_SMALL = 0xC0000023L;
constexpr LONG STATUS_BUFFER_OVERFLOW = 0x80000005;
constexpr LONG STATUS_INFO_LENGTH_MISMATCH = 0xC0000004;

namespace WindowsUtils::Core
{
	/*=========================================
	==	  Undocumented object definition	 ==
	===========================================*/

	/*
	* These objects and procedures are not documented.
	* Microsoft recommends using the documented APIs because these objects and procedures can change without notice.
	* Most of these objects are defined on the great Process Hacker / System Informer tool.
	* https://github.com/winsiderss/systeminformer
	*/

	enum class KEY_INFORMATION_CLASS
	{
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
	};

	// Objects

	enum class OBJECT_INFORMATION_CLASS
	{
		ObjectNameInformation = 1,
		ObjectTypeInformation = 2
	};

	typedef struct _OBJECT_NAME_INFORMATION
	{
		UNICODE_STRING Name;
	}OBJECT_NAME_INFORMATION, * POBJECT_NAME_INFORMATION;

	typedef struct _OBJECT_TYPE_INFORMATION
	{
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

	// General
	typedef struct _IO_STATUS_BLOCK
	{
		union
		{
			NTSTATUS Status;
			PVOID Pointer;
		} DUMMYUNIONNAME;

		ULONG_PTR Information;
	} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;

	// File
	typedef  struct _FILE_PROCESS_IDS_USING_FILE_INFORMATION
	{
		ULONG NumberOfProcessIdsInList;
		ULONG_PTR ProcessIdList[1];
	} FILE_PROCESS_IDS_USING_FILE_INFORMATION, * PFILE_PROCESS_IDS_USING_FILE_INFORMATION;

	typedef enum _FILE_INFORMATION_CLASS
	{
		FileProcessIdsUsingFileInformation = 47
	}FILE_INFORMATION_CLASS;

	// Process and Thread

	typedef struct _THREAD_FUNC_ARGUMENTS
	{
		HANDLE						ObjectHandle;
		OBJECT_INFORMATION_CLASS	ObjectInformationClass;
		wuunique_ptr<BYTE[]>& ObjectInfo;
	}THREAD_FUNC_ARGUMENTS, * PTHREAD_FUNC_ARGUMENTS;

	typedef struct _THREAD_FUNC_ARGUMENTS2
	{
		HANDLE						ObjectHandle;
		OBJECT_INFORMATION_CLASS	ObjectInformationClass;
		wuunique_ptr<BYTE[]>& ObjectInfo;
		HANDLE						EndEvent;
	}THREAD_FUNC_ARGUMENTS2, * PTHREAD_FUNC_ARGUMENTS2;

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

	typedef struct _PROCESS_HANDLE_SNAPSHOT_INFORMATION
	{
		ULONG_PTR						NumberOfHandles;
		ULONG_PTR						Reserved;
		PROCESS_HANDLE_TABLE_ENTRY_INFO	Handles[1];
	}PROCESS_HANDLE_SNAPSHOT_INFORMATION, * PPROCESS_HANDLE_SNAPSHOT_INFORMATION;

	typedef enum _PROCESSINFOCLASS
	{
		ProcessBasicInformation = 0,
		ProcessDebugPort = 7,
		ProcessWow64Information = 26,
		ProcessImageFileName = 27,
		ProcessBreakOnTermination = 29,
		ProcessHandleInformation = 51
	}PROCESSINFOCLASS;

	// System
	typedef enum _SYSTEM_INFORMATION_CLASS
	{
		SystemProcessInformation = 6,
		SystemFullProcessInformation = 148
	}SYSTEM_INFORMATION_CLASS;

	typedef LONG KPRIORITY;

	typedef struct _CLIENT_ID
	{
		HANDLE UniqueProcess;
		HANDLE UniqueThread;
	} CLIENT_ID;

	typedef struct _SYSTEM_THREAD_INFORMATION
	{
		LARGE_INTEGER KernelTime;
		LARGE_INTEGER UserTime;
		LARGE_INTEGER CreateTime;
		ULONG WaitTime;
		PVOID StartAddress;
		CLIENT_ID ClientId;
		LONG Priority;
		LONG BasePriority;
		ULONG ContextSwitches;
		ULONG ThreadState;
		ULONG WaitReason;
	} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;

	typedef struct _SYSTEM_PROCESS_INFORMATION
	{
		ULONG NextEntryOffset;
		ULONG NumberOfThreads;
		LARGE_INTEGER WorkingSetPrivateSize; // since VISTA
		ULONG HardFaultCount; // since WIN7
		ULONG NumberOfThreadsHighWatermark; // since WIN7
		ULONGLONG CycleTime; // since WIN7
		LARGE_INTEGER CreateTime;
		LARGE_INTEGER UserTime;
		LARGE_INTEGER KernelTime;
		UNICODE_STRING ImageName;
		KPRIORITY BasePriority;
		HANDLE UniqueProcessId;
		HANDLE InheritedFromUniqueProcessId;
		ULONG HandleCount;
		ULONG SessionId;
		ULONG_PTR UniqueProcessKey; // since VISTA (requires SystemExtendedProcessInformation)
		SIZE_T PeakVirtualSize;
		SIZE_T VirtualSize;
		ULONG PageFaultCount;
		SIZE_T PeakWorkingSetSize;
		SIZE_T WorkingSetSize;
		SIZE_T QuotaPeakPagedPoolUsage;
		SIZE_T QuotaPagedPoolUsage;
		SIZE_T QuotaPeakNonPagedPoolUsage;
		SIZE_T QuotaNonPagedPoolUsage;
		SIZE_T PagefileUsage;
		SIZE_T PeakPagefileUsage;
		SIZE_T PrivatePageCount;
		LARGE_INTEGER ReadOperationCount;
		LARGE_INTEGER WriteOperationCount;
		LARGE_INTEGER OtherOperationCount;
		LARGE_INTEGER ReadTransferCount;
		LARGE_INTEGER WriteTransferCount;
		LARGE_INTEGER OtherTransferCount;
		SYSTEM_THREAD_INFORMATION Threads[1]; // SystemProcessInformation
	} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;

	typedef struct _KEY_NAME_INFORMATION
	{
		ULONG NameLength;
		WCHAR Name[1];
	} KEY_NAME_INFORMATION, * PKEY_NAME_INFORMATION;

	typedef struct _PEB_LDR_DATA
	{
		BYTE       Reserved1[8];
		PVOID      Reserved2[3];
		LIST_ENTRY InMemoryOrderModuleList;
	} PEB_LDR_DATA, * PPEB_LDR_DATA;

	typedef VOID(WINAPI* PPS_POST_PROCESS_INIT_ROUTINE)(VOID);

	typedef struct _RTL_USER_PROCESS_PARAMETERS
	{
		BYTE           Reserved1[16];
		PVOID          Reserved2[10];
		UNICODE_STRING ImagePathName;
		UNICODE_STRING CommandLine;
	} RTL_USER_PROCESS_PARAMETERS, * PRTL_USER_PROCESS_PARAMETERS;

	typedef struct _PEB
	{
		BYTE                          Reserved1[2];
		BYTE                          BeingDebugged;
		BYTE                          Reserved2[1];
		PVOID                         Reserved3[2];
		PPEB_LDR_DATA                 Ldr;
		PRTL_USER_PROCESS_PARAMETERS  ProcessParameters;
		BYTE                          Reserved4[104];
		PVOID                         Reserved5[52];
		PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
		BYTE                          Reserved6[128];
		PVOID                         Reserved7[1];
		ULONG                         SessionId;
	} PEB, * PPEB;

	typedef struct _PROCESS_BASIC_INFORMATION
	{
		PVOID Reserved1;
		PPEB PebBaseAddress;
		PVOID Reserved2[2];
		ULONG_PTR UniqueProcessId;
		PVOID Reserved3;
	} PROCESS_BASIC_INFORMATION;

	// Procedures

	typedef NTSTATUS(NTAPI* _NtQueryInformationFile)(
		HANDLE					FileHandle,
		PIO_STATUS_BLOCK		IoStatusBlock,
		PVOID					FileInformation,
		ULONG					Length,
		FILE_INFORMATION_CLASS	FileInformationClass
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

	typedef NTSTATUS(NTAPI* _NtQueryInformationProcess)(
		HANDLE				ProcessHandle,
		PROCESSINFOCLASS	ProcessInformationClass,
		PVOID				ProcessInformation,
		ULONG				ProcessInformationLength,
		PULONG				ReturnLength
		);

	typedef NTSTATUS(NTAPI* _NtQueryObject)(
		HANDLE						Handle,
		OBJECT_INFORMATION_CLASS	ObjectInformationClass,
		PVOID						ObjectInformation,
		ULONG						ObjectInformationLength,
		PULONG						ReturnLength
		);

	typedef NTSTATUS(NTAPI* _NtQuerySystemInformation)(
		SYSTEM_INFORMATION_CLASS	SystemInformationClass,
		PVOID						SystemInformation,
		ULONG						SystemInformationLength,
		PULONG						ReturnLength
		);

	typedef NTSTATUS(NTAPI* _NtQueryKey)(
		__in HANDLE					KeyHandle,
		__in KEY_INFORMATION_CLASS	KeyInformationClass,
		__out_opt PVOID				KeyInformation,
		__in ULONG					Length,
		__out PULONG				ResultLength
		);

	typedef NTSTATUS(NTAPI* _NtSuspendProcess)(__in HANDLE hProcess);

	typedef NTSTATUS(NTAPI* _NtResumeProcess)(__in HANDLE hProcess);

	typedef __kernel_entry NTSTATUS(NTAPI* _NtQuerySystemTime)(__out PLARGE_INTEGER SystemTime);

	/*========================================
	==		 Function identification		==
	==========================================*/

	void GetProcessUsingFile(const WWuString& fileName, wuvector<DWORD>& processIdList);
	void GetProcessUsingKey(const WWuString& ntKeyName, wuvector<DWORD>& resultPidList, bool closeHandle);
	void CloseExternalHandlesToFile(HANDLE hProcess, const WWuString& fileName);
	void GetRunnningProcessIdList(wuvector<DWORD>& procIdList);
	DWORD WINAPI NtQueryObjectRaw(LPVOID param);
	void NtQueryObjectWithTimeout(HANDLE hObject, OBJECT_INFORMATION_CLASS objInfoClass, wuunique_ptr<BYTE[]>& objectInfo, ULONG timeout);
	void GetProcessImageName(DWORD processId, WWuString& imageName);
	void GetProcessCommandLine(HANDLE hProcess, WWuString& commandLine);
}