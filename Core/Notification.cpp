#include "pch.h"

#include "Notification.h"

namespace WindowsUtils::Core
{
	Notification::_MAPPED_PROGRESS_DATA::_MAPPED_PROGRESS_DATA(
		const LPWSTR activity,
		int activityId,
		const LPWSTR currOperation,
		int parentActId,
		WORD percentComplete,
		PROGRESS_RECORD_TYPE recType,
		int secRemaining,
		const LPWSTR status
	) : ActivityId(activityId), ParentActivityId(parentActId),
			PercentComplete(percentComplete), RecordType(recType), SecondsRemaining(secRemaining) {
		
		if (activity == NULL || status == NULL)
			throw "Activity and StatusDescription cannot be null.";

		size_t activitySize = wcslen(activity) + 1;
		size_t statusSize = wcslen(status) + 1;

		Activity = new WCHAR[activitySize];
		StatusDescription = new WCHAR[statusSize];

		wcscpy_s(Activity, activitySize, activity);
		wcscpy_s(StatusDescription, statusSize, status);
		
		if (currOperation != NULL) {
			size_t oprSize = wcslen(currOperation) + 1;
			CurrentOperation = new WCHAR[activitySize];
			wcscpy_s(CurrentOperation, oprSize, currOperation);
		}
		else {
			CurrentOperation = NULL;
		}
	}

	Notification::_MAPPED_PROGRESS_DATA::~_MAPPED_PROGRESS_DATA() {
		delete[] Activity;
		delete[] StatusDescription;

		if (CurrentOperation != NULL)
			delete[] CurrentOperation;
	}

	Notification::_MAPPED_INFORMATION_DATA::_MAPPED_INFORMATION_DATA()
		: Computer(NULL), NativeThreadId(0), Source(NULL), Tags(NULL), TimeGenerated(0), User(NULL)
	{ }

	Notification::_MAPPED_INFORMATION_DATA::_MAPPED_INFORMATION_DATA(
		const LPWSTR computer,
		DWORD natThreadId,
		LPWSTR text,
		const LPWSTR source,
		LPWSTR* tags,
		DWORD tagCount,
		time_t timestamp,
		LPWSTR user
	) : NativeThreadId(natThreadId), TagCount(tagCount), TimeGenerated(timestamp) {
		if (text == NULL)
			throw "Text cannot be null";

		size_t textLen = wcslen(text) + 1;
		Text = new WCHAR[textLen];
		wcscpy_s(Text, textLen, text);

		if (computer != NULL) {
			size_t pcLen = wcslen(computer) + 1;
			Computer = new WCHAR[pcLen];
			wcscpy_s(Computer, pcLen, computer);
		}
		else {
			Computer = NULL;
		}

		if (source != NULL) {
			size_t sourceLen = wcslen(source) + 1;
			Source = new WCHAR[sourceLen];
			wcscpy_s(Source, sourceLen, source);
		}
		else {
			Source = NULL;
		}

		if (user != NULL) {
			size_t userLen = wcslen(user) + 1;
			User = new WCHAR[userLen];
			wcscpy_s(User, userLen, user);
		}
		else {
			User = NULL;
		}

		if (tags != NULL && tagCount > 0) {
			Tags = new LPWSTR[tagCount];
			for (size_t i = 0; i < tagCount; i++) {
				size_t currLen = wcslen(tags[i]) + 1;
				Tags[i] = new WCHAR[currLen];
				wcscpy_s(Tags[i], currLen, tags[i]);
			}
		}
		else {
			Tags = NULL;
		}
	}

	Notification::_MAPPED_INFORMATION_DATA::~_MAPPED_INFORMATION_DATA() {
		delete[] Text;
		if (Computer != NULL)
			delete[] Computer;
		if (Source != NULL)
			delete[] Source;
		if (User != NULL)
			delete[] User;

		if (Tags != NULL) {
			for (size_t i = 0; i < TagCount; i++)
				delete[] Tags[i];

			delete[] Tags;
		}
	}

	void NativeWriteProgress(Notification::PNATIVE_CONTEXT context, Notification::PMAPPED_PROGRESS_DATA progData)
	{
		size_t dataSize = sizeof(Notification::MAPPED_PROGRESS_DATA);

		// 'FILE_MAP_WRITE' is not enough.
		LPVOID view = MapViewOfFile(context->MappedProgressFile, FILE_MAP_ALL_ACCESS, 0, 0, dataSize);
		
		// TODO: Handle exceptions
		if (view != NULL)
		{
			RtlCopyMemory(view, progData, dataSize);
			UnmapViewOfFile(view);
			context->WriteProgressHook(dataSize);
		}
	}

	void NativeWriteWarning(Notification::PNATIVE_CONTEXT context, const WWuString& text)
	{
		size_t textLen = text.Length() + 1;
		LPVOID view = MapViewOfFile(context->MappedWarningFile, FILE_MAP_ALL_ACCESS, 0, 0, textLen * 2);

		if (view != NULL) {
			wcscpy_s((LPWSTR)view, textLen, text.GetBuffer());
			UnmapViewOfFile(view);
			context->WriteWarningHook(textLen * 2);
		}
	}

	void NativeWriteInformation(Notification::PNATIVE_CONTEXT context, Notification::PMAPPED_INFORMATION_DATA infoData) {
		size_t dataSize = sizeof(Notification::MAPPED_INFORMATION_DATA);
		LPVOID view = MapViewOfFile(context->MappedInformationFile, FILE_MAP_ALL_ACCESS, 0, 0, dataSize);

		if (view != NULL) {
			RtlCopyMemory(view, infoData, dataSize);
			UnmapViewOfFile(view);
			context->WriteInformationHook(dataSize);
		}
	}
}