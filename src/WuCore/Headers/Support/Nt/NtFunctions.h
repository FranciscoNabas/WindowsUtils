#pragma once

#include "NtStructures.h"

extern "C" {

#pragma region Common

	__kernel_entry NTSTATUS NTAPI NtClose(
		_In_ HANDLE Handle
	);

	ULONG NTAPI RtlNtStatusToDosError(
		_In_ NTSTATUS Status
	);

#pragma endregion

#pragma region Objects

	NTSTATUS NTAPI NtDuplicateObject(
		_In_       HANDLE       SourceProcessHandle,
		_In_       HANDLE       SourceHandle,
		_In_opt_   HANDLE       TargetProcessHandle,
		_Out_opt_  PHANDLE      TargetHandle,
		_In_       ACCESS_MASK  DesiredAccess,
		_In_       ULONG        HandleAttributes,
		_In_       ULONG        Options
	);

	__kernel_entry NTSYSCALLAPI NTSTATUS NTAPI NtQueryObject(
		_In_opt_   HANDLE Handle,
		_In_       OBJECT_INFORMATION_CLASS ObjectInformationClass,
		_Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
		_In_       ULONG ObjectInformationLength,
		_Out_opt_  PULONG ReturnLength
	);

#pragma endregion

#pragma region System

	__kernel_entry NTSTATUS NTAPI NtQuerySystemTime(
		_Out_ PLARGE_INTEGER SystemTime
	);

	__kernel_entry NTSTATUS NTAPI NtQuerySystemInformation(
		_In_	   SYSTEM_INFORMATION_CLASS SystemInformationClass,
		_Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
		_In_       ULONG SystemInformationLength,
		_Out_opt_  PULONG ReturnLength
	);

#pragma endregion

#pragma region Process and Thread

	__kernel_entry NTSYSCALLAPI NTSTATUS NTAPI NtOpenProcess(
		_Out_          PHANDLE            ProcessHandle,
		_In_           ACCESS_MASK        DesiredAccess,
		_In_           POBJECT_ATTRIBUTES ObjectAttributes,
		_In_opt_	   PCLIENT_ID         ClientId
	);

	__kernel_entry NTSTATUS NTAPI NtQueryInformationProcess(
		_In_       HANDLE ProcessHandle,
		_In_       PROCESSINFOCLASS ProcessInformationClass,
		_Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
		_In_       ULONG ProcessInformationLength,
		_Out_opt_  PULONG ReturnLength
	);

	__kernel_entry NTSTATUS NTAPI NtQueryInformationThread(
		_In_       HANDLE ThreadHandle,
		_In_       THREADINFOCLASS ThreadInformationClass,
		_Out_writes_bytes_(ThreadInformationLength) PVOID ThreadInformation,
		_In_       ULONG ThreadInformationLength,
		_Out_opt_  PULONG ReturnLength
	);

	NTSTATUS NTAPI RtlCreateUserThread(
		_In_       HANDLE                      Process,
		_In_opt_   PSECURITY_DESCRIPTOR        ThreadSecurityDescriptor,
		_In_       BOOLEAN                     CreateSuspended,
		_In_opt_   ULONG                       ZeroBits,
		_In_opt_   SIZE_T                      MaximumStackSize,
		_In_opt_   SIZE_T                      CommittedStackSize,
		_In_       PUSER_THREAD_START_ROUTINE  StartAddress,
		_In_opt_   PVOID                       Parameter,
		_Out_opt_  PHANDLE                     Thread,
		_Out_opt_  PCLIENT_ID                  ClientId
	);

	NTSTATUS NTAPI NtSuspendProcess(
		_In_ HANDLE hProcess
	);

	NTSTATUS NTAPI NtResumeProcess(
		_In_ HANDLE hProcess
	);

#pragma endregion

#pragma region IO

	__kernel_entry NTSYSCALLAPI NTSTATUS NTAPI NtQueryInformationFile(
		_In_                        HANDLE                  FileHandle,
		_Out_                       PIO_STATUS_BLOCK        IoStatusBlock,
		_Out_writes_bytes_(Length)  PVOID                   FileInformation,
		_In_                        ULONG                   Length,
		_In_                        FILE_INFORMATION_CLASS  FileInformationClass
	);

#pragma endregion

#pragma region Registry

	NTSYSAPI NTSTATUS NTAPI NtOpenKey(
		_Out_ PHANDLE            KeyHandle,
		_In_  ACCESS_MASK        DesiredAccess,
		_In_  POBJECT_ATTRIBUTES ObjectAttributes
	);

	NTSYSAPI NTSTATUS NTAPI NtQueryKey(
		_In_       HANDLE                 KeyHandle,
		_In_       KEY_INFORMATION_CLASS  KeyInformationClass,
		_Out_opt_  PVOID                  KeyInformation,
		_In_       ULONG                  Length,
		_Out_      PULONG                 ResultLength
	);

#pragma endregion

}