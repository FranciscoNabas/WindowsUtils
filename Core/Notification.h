#pragma once
#pragma unmanaged

#include "Utilities.h"
#include "MemoryManagement.h"

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
				const LPWSTR& activity,
				INT activityId,
				const LPWSTR& currOperation,
				INT parentActId,
				DWORD percentComplete,
				PROGRESS_RECORD_TYPE recType,
				INT secRemaining,
				const LPWSTR& status
			);

			_MAPPED_PROGRESS_DATA()
				: Activity(NULL), ActivityId(0), CurrentOperation(NULL), ParentActivityId(-1), PercentComplete(0), RecordType(PROGRESS_RECORD_TYPE::Processing), SecondsRemaining(-1), StatusDescription(NULL)
			{ }

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

	void NativeWriteProgress(Notification::PNATIVE_CONTEXT context, Notification::PMAPPED_PROGRESS_DATA progData);
	void NativeWriteWarning(Notification::PNATIVE_CONTEXT context, const LPWSTR& text);
}