#include "../../pch.h"

#include "../../Headers/Support/Notification.h"

namespace WindowsUtils::Core
{
	/*
	*	~ Mapped progress data
	*/

	_MAPPED_PROGRESS_DATA::_MAPPED_PROGRESS_DATA()
		: ActivityId(0), ParentActivityId(-1), PercentComplete(0), RecordType(ProgressRecordType::Processing), SecondsRemaining(-1)
	{ }

	_MAPPED_PROGRESS_DATA::_MAPPED_PROGRESS_DATA(
		const WWuString& activity,
		int activityId,
		const WWuString& currOperation,
		int parentActId,
		int percentComplete,
		ProgressRecordType recType,
		int secRemaining,
		const WWuString& status
	) : Activity(activity), ActivityId(activityId), CurrentOperation(currOperation), ParentActivityId(parentActId),
		PercentComplete(percentComplete), RecordType(recType), SecondsRemaining(secRemaining), StatusDescription(status)
	{ }

	_MAPPED_PROGRESS_DATA::~_MAPPED_PROGRESS_DATA() { }

	/*
	*	~ Mapped information data
	*/

	_MAPPED_INFORMATION_DATA::_MAPPED_INFORMATION_DATA()
		: Computer(), NativeThreadId(0), Source(), Tags(NULL), TimeGenerated(0), User()
	{ }

	_MAPPED_INFORMATION_DATA::_MAPPED_INFORMATION_DATA(
		const WWuString& computer,
		DWORD natThreadId,
		const WWuString& text,
		const WWuString& source,
		time_t timestamp,
		const WWuString& user
	) : Computer(computer), NativeThreadId(natThreadId), Text(text), Source(source), TimeGenerated(timestamp), User(user)
	{ }

	_MAPPED_INFORMATION_DATA::~_MAPPED_INFORMATION_DATA() { }

	/*
	*	~ Mapped error data
	*/

	_MAPPED_ERROR_DATA::_MAPPED_ERROR_DATA(
		const int errorCode,
		const WWuString& message,
		const WWuString& compactTrace,
		const WWuString& errorId,
		WriteErrorCategory category,
		const WWuString& targetObject
	)
		: ErrorCode(errorCode), ErrorMessage(message), CompactTrace(compactTrace), ErrorId(errorId), Category(category), TargetObject(targetObject)
	{ }

	_MAPPED_ERROR_DATA::~_MAPPED_ERROR_DATA() { }

	/*
	*	~ WindowsUtils native context ~
	*/

	WuNativeContext::WuNativeContext(
		UnmanagedWriteProgress progPtr,
		UnmanagedWriteWarning warnPtr,
		UnmanagedWriteInformation infoPtr,
		UnmanagedWriteObject objPtr,
		UnmanagedWriteException exPtr,
		UnmanagedExceptionMarshaler exMarshalerPtr
	) : m_writeProgressHook(progPtr), m_writeWarningHook(warnPtr), m_writeInformationHook(infoPtr), m_writeObjectHook(objPtr),
		m_writeExHook(exPtr), m_exMarshalerHook(exMarshalerPtr)
	{ }

	WuNativeContext::~WuNativeContext() { }

	const UnmanagedExceptionMarshaler& WuNativeContext::GetExceptionMarshaler() const { return m_exMarshalerHook; }

	void WuNativeContext::NativeWriteProgress(PMAPPED_PROGRESS_DATA progData) const { m_writeProgressHook(progData); }
	void WuNativeContext::NativeWriteWarning(const WWuString& text) const { m_writeWarningHook(text); }
	void WuNativeContext::NativeWriteInformation(const PMAPPED_INFORMATION_DATA infoData) const { m_writeInformationHook(infoData); }
	void WuNativeContext::NativeWriteError(const WuException& exception) const { m_writeExHook(exception); }
	void WuNativeContext::NativeWriteObject(const PVOID obj, const WriteOutputType type) const { m_writeObjectHook(obj, type); }
}