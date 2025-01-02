#pragma once
#pragma unmanaged

#include <unordered_map>

#include "../WuString.h"
#include "../Expressions.h"
#include "../SafeHandle.h"
#include "../Notification.h"
#include "../WuException.h"
#include "../ScopedBuffer.h"
#include "../WuList.h"

#include "NtStructures.h"
#include "NtFunctions.h"

// Default registry key constants.
constexpr BYTE ObjectTypeRegistryKeyLength = 88;
constexpr BYTE ObjectTypeRegistryKeyMaxLength = 90;

namespace WindowsUtils::Core
{
	// This enum reflects to the supported PS providers for 'Get-ObjectHandle'.
	enum class SupportedHandleType
	{
		FileSystem,
		Registry,
	};
	
	// This structure represents the information for a single handle from a process.
	// Used by 'Get-ObjectHandle'.
	typedef struct _WU_OBJECT_HANDLE_INFO
	{
		HANDLE     HandleValue;
		WWuString  Type;
		WWuString  Name;

		_WU_OBJECT_HANDLE_INFO(const _WU_OBJECT_HANDLE_INFO& other);
		_WU_OBJECT_HANDLE_INFO(const HANDLE handle, const WWuString& type, const WWuString& name);
		~_WU_OBJECT_HANDLE_INFO();

	} WU_OBJECT_HANDLE_INFO, *PWU_OBJECT_HANDLE_INFO;

	// This structure is used when we 'NtQueryObject' with timeout.
	// It's passed as argument for the new thread.
	typedef struct _WU_QUERY_OBJECT_DATA
	{
		NTSTATUS Status;
		HANDLE DuplicateHandle;
		LPVOID ObjectData;
		OBJECT_INFORMATION_CLASS DataType;

		_WU_QUERY_OBJECT_DATA(HANDLE hDup, OBJECT_INFORMATION_CLASS dataType);
		~_WU_QUERY_OBJECT_DATA();

	} WU_QUERY_OBJECT_DATA, *PWU_QUERY_OBJECT_DATA;

	// This structure is used to get thread process information.
	typedef struct _WU_THREAD_PROCESS_INFORMATION
	{
		DWORD      ThreadId;
		DWORD      ProcessId;
		WWuString  ModuleName;

	} WU_THREAD_PROCESS_INFORMATION, *PWU_THREAD_PROCESS_INFORMATION;
	
	class NtUtilities
	{
	public:
		static WuList<DWORD> GetProcessUsingFile(const WWuString& fileName);
		static WuList<DWORD> GetProcessUsingKey(const WWuString& keyName, bool closeHandle);
		
		static void CloseExternalHandlesToFile(DWORD processId, const WWuString& fileName);
		static void GetProcessImageName(DWORD processId, WWuString& imageName);
		
		static WuList<DWORD> ListRunningProcesses();
		static std::unordered_map<DWORD, WWuString> ListRunningProcessIdAndNames();
		static void ListRunningProcesses(std::unordered_map<DWORD, WWuString>& processMap);
		
		static void GetProcessCommandLine(const ProcessHandle& hProcess, WWuString& commandLine);
		static void ListProcessHandleInformation(const ProcessHandle& hProcess, const bool all, WuList<WU_OBJECT_HANDLE_INFO>& output, const WuNativeContext* context);
		static void QueryObjectWithTimeout(WU_QUERY_OBJECT_DATA& objectData, DWORD timeout);
		static void GetThreadProcessInformation(const HANDLE hThread, WU_THREAD_PROCESS_INFORMATION& threadProcInfo);

	private:
		static HANDLE s_currentProcess;
		static LPWSTR s_objTypeRegistryKey;

		static UCHAR GetRegistryKeyObjectTypeIndex();
	};

	NTSTATUS WINAPI QueryObjectNameThread(PVOID lpThreadParameter);
}