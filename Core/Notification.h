#pragma once
#pragma unmanaged

#include "Utilities.h"

namespace WindowsUtils::Core
{
	extern "C" public class __declspec(dllexport) Notification
	{
	public:
		typedef enum _PROGRESS_RECORD_TYPE
		{
			Processing,
			Completed
		} PROGRESS_RECORD_TYPE;

		typedef struct _MAPPED_PROGRESS_DATA
		{
			LPWSTR Activity;
			INT ActivityId;
			LPWSTR CurrentOperation;
			INT ParentActivityId;
			DWORD PercentComplete;
			PROGRESS_RECORD_TYPE RecordType;
			INT SecondsRemaining;
			LPWSTR StatusDescription;

			_MAPPED_PROGRESS_DATA(
				LPWSTR const& activity,
				INT activityId,
				LPWSTR const& currOperation,
				INT parentActId,
				DWORD percentComplete,
				PROGRESS_RECORD_TYPE recType,
				INT secRemaining,
				LPWSTR const& status
			);
			~_MAPPED_PROGRESS_DATA();

		private:
			WuMemoryManagement& _memoryManager = WuMemoryManagement::GetManager();

		} MAPPED_PROGRESS_DATA, *PMAPPED_PROGRESS_DATA;

		typedef void(__stdcall* UnmanagedWriteProgress)(size_t dataSize);
		typedef void(__stdcall* UnmanagedWriteWarning)(size_t dataSize);

		typedef struct _NATIVE_CONTEXT
		{
			UnmanagedWriteProgress WriteProgressHook;
			UnmanagedWriteWarning WriteWarningHook;
			HANDLE MappedProgressFile;
			HANDLE MappedWarningFile;

			_NATIVE_CONTEXT(UnmanagedWriteProgress progPtr, UnmanagedWriteWarning warnPtr, HANDLE hMappedProg, HANDLE hMappedWarn)
				: WriteProgressHook(progPtr), WriteWarningHook(warnPtr), MappedProgressFile(hMappedProg), MappedWarningFile(hMappedWarn) { }

		} NATIVE_CONTEXT, *PNATIVE_CONTEXT;
	};

	void NativeWriteProgress(Notification::PNATIVE_CONTEXT const& context, Notification::PMAPPED_PROGRESS_DATA progData);
	void NativeWriteWarning(Notification::PNATIVE_CONTEXT const& context, LPWSTR const& text);
}