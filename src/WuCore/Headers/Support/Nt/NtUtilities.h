#pragma once
#pragma unmanaged

#include <unordered_map>
#include <memory>

#include "../WuString.h"
#include "../Expressions.h"
#include "../SafeHandle.h"
#include "../Notification.h"
#include "../WuException.h"
#include "../ScopedBuffer.h"
#include "../WuList.h"

#include "NtStructures.h"
#include "NtFunctions.h"


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
		static std::unordered_map<HANDLE, DWORD> GetProcessUsingObject(const WWuString& objectName, const SupportedHandleType type, const bool closeHandle);
		
		static WuList<DWORD> ListRunningProcesses();
		static std::unordered_map<DWORD, WWuString> ListRunningProcessIdAndNames();
		static void ListRunningProcesses(std::unordered_map<DWORD, WWuString>& processMap);
		
		static void GetProcessCommandLine(const ProcessHandle& hProcess, WWuString& commandLine);
		static void ListProcessHandleInformation(const ProcessHandle& hProcess, const bool all, WuList<WU_OBJECT_HANDLE_INFO>& output, const WuNativeContext* context);
		static void QueryObjectWithTimeout(WU_QUERY_OBJECT_DATA& objectData, DWORD timeout);

	private:
		static HANDLE s_currentProcess;
		static WWuString s_objTypeRegistryKey;
		static WWuString s_objTypeFile;

		static constexpr BYTE s_objectTypeRegistryKeyLength = 88;
		static constexpr BYTE s_objectTypeRegistryKeyMaxLength = 90;

		static UCHAR GetObjectTypeIndex(const SupportedHandleType type);
		static WWuString GetEnvVariable(const WWuString& name);

		// I don't know what's the correct way to perform this kind of pointer arithmetic in C++ without having
		// undefined behavior or implementation defined behavior, or which kind of cast is the next plague and you
		// should be ashamed of using it.
		// I spent a couple of hours on the interwebs studying casts and going through forums, but C++ developers
		// have their heads soooo up their asses that they are more preoccupied on telling you what you're doing is
		// wrong and how could you and you should reconsider your pathetic existence instead of think for a second
		// that we are not in the pinnacle of technology and some APIs requires you to do some pointer arithmetic (omg
		// heresy).
		// I hope I'm wrong and that this very function implementation will be the one and only reason to the next
		// greatest vulnerability exploit that will stop the world for 500 decades.
		//
		// Aight, after this little rant I ran some tests on my Windows 11 machine and this produced repeatable results.
		// So fuck it. I don't like C-Style casts and there is NO way I'm getting this right without fucking with
		// someone else's feelings.
		
		static constexpr PSYSTEM_PROCESS_INFORMATION GetNextProcess(const PSYSTEM_PROCESS_INFORMATION process)
		{
			return process->NextEntryOffset
				? reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(reinterpret_cast<PBYTE>(process) + process->NextEntryOffset)
				: nullptr;
		}
	};

	NTSTATUS WINAPI QueryObjectNameThread(PVOID lpThreadParameter);
}