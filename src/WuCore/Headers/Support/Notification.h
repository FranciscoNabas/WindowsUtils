#pragma once
#pragma unmanaged

#include "String.h"
#include "Expressions.h"

namespace WindowsUtils::Core
{
	/// <summary>
	/// Maps to System.Management.Automation.ProgressRecordType.
	/// </summary>
	enum class ProgressRecordType
	{
		Processing,
		Completed
	};

	/// <summary>
	/// The Write* proxies use this to wrap native information to managed one.
	/// </summary>
	enum class WriteOutputType
	{
		TcpingOutput,
		TcpingStatistics,
		TestportOutput,
		WWuString,
		ProcessModuleInfo
	};

	/// <summary>
	/// Defines which type of data is being written. (Do we need this?).
	/// </summary>
	enum class WriteDataType
	{
		Information,
		Progress,
		Object,
		Warning,
		Error
	};

	/// <summary>
	/// Maps to System.Management.Automation.ErrorCategory.
	/// </summary>
	enum class WriteErrorCategory
	{
		NotSpecified = 0,
		OpenError = 1,
		CloseError = 2,
		DeviceError = 3,
		DeadlockDetected = 4,
		InvalidArgument = 5,
		InvalidData = 6,
		InvalidOperation = 7,
		InvalidResult = 8,
		InvalidType = 9,
		MetadataError = 10,
		NotImplemented = 11,
		NotInstalled = 12,
		ObjectNotFound = 13,
		OperationStopped = 14,
		OperationTimeout = 15,
		SyntaxError = 16,
		ParserError = 17,
		PermissionDenied = 18,
		ResourceBusy = 19,
		ResourceExists = 20,
		ResourceUnavailable = 21,
		ReadError = 22,
		WriteError = 23,
		FromStdErr = 24,
		SecurityError = 25,
		ProtocolError = 26,
		ConnectionError = 27,
		AuthenticationError = 28,
		LimitsExceeded = 29,
		QuotaExceeded = 30,
		NotEnabled = 31
	};

	/// <summary>
	/// Used to write progress from native code.
	/// Maps to System.Management.Automation.ProgressRecord.
	/// </summary>
	typedef struct _MAPPED_PROGRESS_DATA
	{
		WWuString Activity;
		int ActivityId;
		WWuString CurrentOperation;
		int ParentActivityId;
		int PercentComplete;
		ProgressRecordType RecordType;
		int SecondsRemaining;
		WWuString StatusDescription;

		_MAPPED_PROGRESS_DATA();
		_MAPPED_PROGRESS_DATA(
			const WWuString& activity,
			int activityId,
			const WWuString& currOperation,
			int parentActId,
			int percentComplete,
			ProgressRecordType recType,
			int secRemaining,
			const WWuString& status
		);

		~_MAPPED_PROGRESS_DATA();

	} MAPPED_PROGRESS_DATA, * PMAPPED_PROGRESS_DATA;

	/// <summary>
	/// Used to write information from native code.
	/// Maps to System.Management.Automation.InformationRecord.
	/// </summary>
	typedef struct _MAPPED_INFORMATION_DATA
	{
		WWuString Computer;
		DWORD NativeThreadId;
		WWuString Text;
		WWuString Source;
		wuvector<WWuString> Tags;
		time_t TimeGenerated;
		WWuString User;

		_MAPPED_INFORMATION_DATA();
		_MAPPED_INFORMATION_DATA(
			const WWuString& computer,
			DWORD natThreadId,
			const WWuString& text,
			const WWuString& source,
			time_t timestamp,
			const WWuString& user
		);

		~_MAPPED_INFORMATION_DATA();

	} MAPPED_INFORMATION_DATA, * PMAPPED_INFORMATION_DATA;

	/// <summary>
	/// Used to write error from native code.
	/// Maps to System.Management.Automation.ErrorRecord.
	/// </summary>
	typedef struct _MAPPED_ERROR_DATA
	{
		DWORD ErrorCode;
		WWuString ErrorMessage;
		WWuString CompactTrace;
		WWuString ErrorId;
		WriteErrorCategory Category;
		WWuString TargetObject;

		_MAPPED_ERROR_DATA(
			const int errorCode,
			const WWuString& message,
			const WWuString& compactTrace,
			const WWuString& errorId,
			WriteErrorCategory category,
			const WWuString& targetObject
		);

		~_MAPPED_ERROR_DATA();

	} MAPPED_ERROR_DATA, * PMAPPED_ERROR_DATA;

	// These function pointers are going to be mapped to the proxy delegates.
	// The delegates then call the C# delegates to write information to PowerShell.
	typedef void(__stdcall* UnmanagedWriteProgress)(const PMAPPED_PROGRESS_DATA data);
	typedef void(__stdcall* UnmanagedWriteWarning)(const WWuString& data);
	typedef void(__stdcall* UnmanagedWriteInformation)(const PMAPPED_INFORMATION_DATA data);
	typedef void(__stdcall* UnmanagedWriteError)(const PMAPPED_ERROR_DATA data);
	typedef void(__stdcall* UnmanagedWriteObject)(const PVOID data, const WriteOutputType objType);

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//																								 //
	// ~ WindowsUtils Native Context																 //
	//																								 //
	// This class is a native representation of the Cmdlet context.									 //
	// 																								 //
	// These function pointers maps to the delegate m_wrappers.										 //
	// These m_wrappers wrap the managed methods.														 //
	//																								 //
	// This is the current workflow:																 //
	//																								 //
	//                               	 Method -> Delegate -> Fn. Ptr.								 //
	//									   +------------------------+								 //
	//									   |						|								 //
	// +CMDLET-------------+      +CONTEXT-.CTOR----+      +CTXT-BASE-.CTOR--+		+m_wrapper-------+ //
	// |   Calls m_wrapper   | +--> | Creates context | ---> |   Creates the   | ---> | Calls native | //
	// | (Context created) |      |		 base		|	   | WuNativeContext |		|   function   | //
	// +-------------------+      +-----------------+      +-----------------+      +--------------+ //
	//																					    |		 //
	//													Managed/Unmanaged Line			    |		 //
	//                                                  ============================================ //
	//						  							|						            |		 //
	//													|									˅        //
	//	+LIB------------+	  +CONTEXT------+	  +CONTEXT-----+    +N-CONTEXT----+	   +N-FUNC----+  //
	//	| Cmdlet.Write* | <-- | Calls Write | <-- | Wraps nat. | <- |    Calls	  | <- | Calls    |  //
	//	+---------------+     |   delegate  |     | data.      |    |  Proxy del. |    | NativeWr.|  //
	//						  +-------------+	  +------------+	+-------------+	   +----------+  //
	///////////////////////////////////////////////////////////////////////////////////////////////////

	extern "C" public class __declspec(dllexport) WuNativeContext
	{
	private:
		UnmanagedWriteProgress m_writeProgressHook;
		UnmanagedWriteWarning m_writeWarningHook;
		UnmanagedWriteInformation m_writeInformationHook;
		UnmanagedWriteObject m_writeObjectHook;
		UnmanagedWriteError m_writeErrorHook;

	public:
		WuNativeContext(
			UnmanagedWriteProgress progPtr,
			UnmanagedWriteWarning warnPtr,
			UnmanagedWriteInformation infoPtr,
			UnmanagedWriteObject objPtr,
			UnmanagedWriteError errorPtr
		);

		~WuNativeContext();

		void NativeWriteWarning(const WWuString& text) const;
		void NativeWriteProgress(const PMAPPED_PROGRESS_DATA progData) const;
		void NativeWriteInformation(const PMAPPED_INFORMATION_DATA infoData) const;
		void NativeWriteError(const PMAPPED_ERROR_DATA errorData) const;
		void NativeWriteObject(const PVOID obj, const WriteOutputType type) const;
	};
}