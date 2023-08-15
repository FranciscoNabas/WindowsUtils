#pragma once
#pragma unmanaged

#include "Utilities.h"
#include "String.h"
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
			WWuString Activity;
			int ActivityId;
			WWuString CurrentOperation;
			int ParentActivityId;
			WORD PercentComplete;
			PROGRESS_RECORD_TYPE RecordType;
			int SecondsRemaining;
			WWuString StatusDescription;

			_MAPPED_PROGRESS_DATA(
				const WWuString& activity,
				int activityId,
				const WWuString& currOperation,
				int parentActId,
				WORD percentComplete,
				PROGRESS_RECORD_TYPE recType,
				int secRemaining,
				const WWuString& status
			);

			_MAPPED_PROGRESS_DATA()
				: Activity(), ActivityId(0), CurrentOperation(), ParentActivityId(-1), PercentComplete(0), RecordType(PROGRESS_RECORD_TYPE::Processing), SecondsRemaining(-1), StatusDescription()
			{ }

			~_MAPPED_PROGRESS_DATA();

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
	void NativeWriteWarning(Notification::PNATIVE_CONTEXT context, const WWuString& text);
}