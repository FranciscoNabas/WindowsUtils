#include "..\pch.h"

#include "..\Headers\Notification.h"

namespace WindowsUtils::Core
{
	/*
	*	~ Mapped progress data ~
	*/

	Notification::_MAPPED_PROGRESS_DATA::_MAPPED_PROGRESS_DATA(
		const LPWSTR activity,
		int activityId,
		const LPWSTR currOperation,
		int parentActId,
		int percentComplete,
		PROGRESS_RECORD_TYPE recType,
		int secRemaining,
		const LPWSTR status
	) : ActivityId(activityId), ParentActivityId(parentActId),
		PercentComplete(percentComplete), RecordType(recType), SecondsRemaining(secRemaining)
	{

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

	Notification::_MAPPED_PROGRESS_DATA::~_MAPPED_PROGRESS_DATA()
	{
		delete[] Activity;
		delete[] StatusDescription;

		if (CurrentOperation != NULL)
			delete[] CurrentOperation;
	}

	/*
	*	~ Mapped information data ~
	*/

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
	) : NativeThreadId(natThreadId), TagCount(tagCount), TimeGenerated(timestamp)
	{
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

	Notification::_MAPPED_INFORMATION_DATA::~_MAPPED_INFORMATION_DATA()
	{
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

	/*
	*	~ WindowsUtils native context ~
	*/

	WuNativeContext::WuNativeContext(
		Notification::UnmanagedWriteProgress progPtr,
		Notification::UnmanagedWriteWarning warnPtr,
		Notification::UnmanagedWriteInformation infoPtr,
		Notification::UnmanagedWriteObject objPtr,
		BYTE* progBuffer,
		BYTE* warningBuffer,
		BYTE* informationBuffer,
		BYTE* objectBuffer

	) : m_WriteProgressHook(progPtr), m_WriteWarningHook(warnPtr), m_WriteInformationHook(infoPtr), m_WriteObjectHook(objPtr),
			m_ProgressBuffer(progBuffer), m_WarningBuffer(warningBuffer), m_InformationBuffer(informationBuffer), m_objectBuffer(objectBuffer)
	{ }

	WuNativeContext::~WuNativeContext() { }

	void WuNativeContext::NativeWriteProgress(Notification::PMAPPED_PROGRESS_DATA progData)
	{
		size_t dataSize = sizeof(Notification::MAPPED_PROGRESS_DATA);

		RtlZeroMemory(m_ProgressBuffer, 128);
		RtlCopyMemory(m_ProgressBuffer, progData, dataSize);
		m_WriteProgressHook();
	}

	void WuNativeContext::NativeWriteWarning(const WWuString& text)
	{
		size_t textLen = text.Length() + 1;

		RtlZeroMemory(m_WarningBuffer, 128);
		wcscpy_s((LPWSTR)m_WarningBuffer, textLen, text.GetBuffer());
		m_WriteWarningHook();
	}

	void WuNativeContext::NativeWriteInformation(const Notification::PMAPPED_INFORMATION_DATA infoData)
	{
		size_t dataSize = sizeof(Notification::MAPPED_INFORMATION_DATA);
		
		RtlZeroMemory(m_InformationBuffer, 128);
		RtlCopyMemory(m_InformationBuffer, infoData, dataSize);
		m_WriteInformationHook();
	}
}