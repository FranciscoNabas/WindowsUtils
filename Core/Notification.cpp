#include "pch.h"

#include "Notification.h"

namespace WindowsUtils::Core
{
	Notification::_MAPPED_PROGRESS_DATA::_MAPPED_PROGRESS_DATA(
		const WWuString& activity,
		INT activityId,
		const WWuString& currOperation,
		INT parentActId,
		DWORD percentComplete,
		PROGRESS_RECORD_TYPE recType,
		INT secRemaining,
		const WWuString& status
	) : Activity(activity), ActivityId(activityId), CurrentOperation(currOperation), ParentActivityId(parentActId),
			PercentComplete(percentComplete), RecordType(recType), SecondsRemaining(secRemaining), StatusDescription(status)
	{ }

	Notification::_MAPPED_PROGRESS_DATA::~_MAPPED_PROGRESS_DATA() { }

	void NativeWriteProgress(Notification::PNATIVE_CONTEXT context, Notification::PMAPPED_PROGRESS_DATA progData)
	{
		size_t dataSize = sizeof(*progData);
		LPVOID view = MapViewOfFile(context->MappedProgressFile, FILE_MAP_WRITE, 0, 0, dataSize);
		
		// TODO: Handle exceptions
		if (view != NULL)
		{
			memcpy(view, progData, dataSize);
			UnmapViewOfFile(view);
			context->WriteProgressHook(dataSize);
		}
	}

	void NativeWriteWarning(Notification::PNATIVE_CONTEXT context, const WWuString& text)
	{
		size_t textLen = text.Length() + 1;
		LPVOID view = MapViewOfFile(context->MappedWarningFile, FILE_MAP_WRITE, 0, 0, textLen * 2);

		if (view != NULL)
		{
			wcscpy_s((LPWSTR)view, textLen, text.GetBuffer());
			UnmapViewOfFile(view);
			context->WriteWarningHook(textLen * 2);
		}
	}
}