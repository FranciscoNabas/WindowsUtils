#include "pch.h"

#include "Notification.h"

namespace WindowsUtils::Core
{
	Notification::_MAPPED_PROGRESS_DATA::_MAPPED_PROGRESS_DATA(
		const LPWSTR& activity,
		INT activityId,
		const LPWSTR& currOperation,
		INT parentActId,
		DWORD percentComplete,
		PROGRESS_RECORD_TYPE recType,
		INT secRemaining,
		const LPWSTR& status
	) : ActivityId(activityId), ParentActivityId(parentActId), PercentComplete(percentComplete), RecordType(recType), SecondsRemaining(secRemaining)
	{
		size_t szActivity = wcslen(activity) + 1;
		size_t szCurrOps = wcslen(currOperation) + 1;
		size_t szStatus = wcslen(status) + 1;

		Activity = (LPWSTR)_memoryManager.Allocate(szActivity);
		CurrentOperation = (LPWSTR)_memoryManager.Allocate(szCurrOps);
		StatusDescription = (LPWSTR)_memoryManager.Allocate(szStatus);

		wcscpy_s(Activity, szActivity, activity);
		wcscpy_s(CurrentOperation, szCurrOps, currOperation);
		wcscpy_s(StatusDescription, szStatus, status);
	}

	Notification::_MAPPED_PROGRESS_DATA::~_MAPPED_PROGRESS_DATA()
	{
		_memoryManager.Free(Activity);
		_memoryManager.Free(CurrentOperation);
		_memoryManager.Free(StatusDescription);
	}

	void NativeWriteProgress(Notification::PNATIVE_CONTEXT context, Notification::PMAPPED_PROGRESS_DATA progData)
	{
		size_t dataSize = sizeof(*progData);
		LPVOID view = MapViewOfFile(context->MappedProgressFile, FILE_MAP_WRITE, 0, 0, dataSize);
		
		// TODO: Handle exceptions
		if (view != NULL)
		{
			memcpy(view, progData, dataSize);
			UnmapViewOfFile(view);
		}

		context->WriteProgressHook(dataSize);
	}

	void NativeWriteWarning(Notification::PNATIVE_CONTEXT context, const LPWSTR& text)
	{
		size_t textLen = wcslen(text) + 1;
		LPVOID view = MapViewOfFile(context->MappedWarningFile, FILE_MAP_WRITE, 0, 0, textLen * 2);

		if (view != NULL)
		{
			wcscpy_s((LPWSTR)view, textLen, text);
			UnmapViewOfFile(view);
		}

		context->WriteWarningHook(textLen * 2);
	}
}