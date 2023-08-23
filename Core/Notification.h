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
		typedef enum _WRITE_TEXT_TYPE {
			Information,
			Warning
		} WRITE_TEXT_TYPE;

		typedef enum _PROGRESS_RECORD_TYPE
		{
			Processing,
			Completed
		} PROGRESS_RECORD_TYPE;

		typedef struct _MAPPED_PROGRESS_DATA
		{
			LPWSTR Activity;
			int ActivityId;
			LPWSTR CurrentOperation;
			int ParentActivityId;
			WORD PercentComplete;
			PROGRESS_RECORD_TYPE RecordType;
			int SecondsRemaining;
			LPWSTR StatusDescription;

			_MAPPED_PROGRESS_DATA(
				const LPWSTR activity,
				int activityId,
				const LPWSTR currOperation,
				int parentActId,
				WORD percentComplete,
				PROGRESS_RECORD_TYPE recType,
				int secRemaining,
				const LPWSTR status
			);

			_MAPPED_PROGRESS_DATA()
				: Activity(NULL), ActivityId(0), CurrentOperation(NULL), ParentActivityId(-1), PercentComplete(0), RecordType(PROGRESS_RECORD_TYPE::Processing), SecondsRemaining(-1), StatusDescription(NULL)
			{ }

			~_MAPPED_PROGRESS_DATA();

		} MAPPED_PROGRESS_DATA, *PMAPPED_PROGRESS_DATA;

		typedef struct _MAPPED_INFORMATION_DATA {
			LPWSTR Computer;
			DWORD NativeThreadId;
			LPWSTR Text;
			LPWSTR Source;
			LPWSTR* Tags;
			DWORD TagCount;
			time_t TimeGenerated;
			LPWSTR User;

			_MAPPED_INFORMATION_DATA();
			_MAPPED_INFORMATION_DATA(const LPWSTR computer, DWORD natThreadId, LPWSTR text, const LPWSTR source, LPWSTR* tags, DWORD tagCount, time_t timestamp, LPWSTR user);
			~_MAPPED_INFORMATION_DATA();

		} MAPPED_INFORMATION_DATA, *PMAPPED_INFORMATION_DATA;

		typedef void(__stdcall* UnmanagedWriteProgress)(size_t dataSize);
		typedef void(__stdcall* UnmanagedWriteWarning)(size_t dataSize);
		typedef void(__stdcall* UnmanagedWriteInformation)(size_t dataSize);

		typedef struct _NATIVE_CONTEXT
		{
			UnmanagedWriteProgress WriteProgressHook;
			UnmanagedWriteWarning WriteWarningHook;
			UnmanagedWriteInformation WriteInformationHook;
			HANDLE MappedProgressFile;
			HANDLE MappedWarningFile;
			HANDLE MappedInformationFile;

			_NATIVE_CONTEXT(UnmanagedWriteProgress progPtr, UnmanagedWriteWarning warnPtr, UnmanagedWriteInformation infoPtr, HANDLE hMappedProg, HANDLE hMappedWarn, HANDLE hMappedInfo)
				: WriteProgressHook(progPtr), WriteWarningHook(warnPtr), WriteInformationHook(infoPtr), MappedProgressFile(hMappedProg), MappedWarningFile(hMappedWarn), MappedInformationFile(hMappedInfo) { }

		} NATIVE_CONTEXT, *PNATIVE_CONTEXT;
	};

	void NativeWriteProgress(Notification::PNATIVE_CONTEXT context, Notification::PMAPPED_PROGRESS_DATA progData);
	void NativeWriteWarning(Notification::PNATIVE_CONTEXT context, const WWuString& text);
	void NativeWriteInformation(Notification::PNATIVE_CONTEXT context, Notification::PMAPPED_INFORMATION_DATA infoData);
}