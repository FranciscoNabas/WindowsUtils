#pragma once
#pragma unmanaged

#include "String.h"
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

		typedef enum _WRITE_OUTPUT_TYPE : DWORD
		{
			TCPING_OUTPUT,
			TCPING_STATISTICS,
			TESTPORT_OUTPUT,
			WWUSTRING,
			LAB_STRUCT,
			PROCESS_MODULE_INFO
		} WRITE_OUTPUT_TYPE;

		typedef enum _WRITE_DATA_TYPE
		{
			InformationData,
			ProgressData,
			ObjectData,
			WarningData,
			ErrorData
		} WRITE_DATA_TYPE;

		typedef enum _WRITE_ERROR_CATEGORY
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
		} WRITE_ERROR_CATEGORY;

		typedef struct _MAPPED_PROGRESS_DATA
		{
			LPWSTR Activity;
			int ActivityId;
			LPWSTR CurrentOperation;
			int ParentActivityId;
			int PercentComplete;
			PROGRESS_RECORD_TYPE RecordType;
			int SecondsRemaining;
			LPWSTR StatusDescription;

			_MAPPED_PROGRESS_DATA(
				const LPWSTR activity,
				int activityId,
				const LPWSTR currOperation,
				int parentActId,
				int percentComplete,
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

		typedef struct _MAPPED_ERROR_DATA
		{
			DWORD ErrorCode;
			LPWSTR ErrorMessage;
			LPWSTR CompactTrace;
			LPWSTR ErrorId;
			WRITE_ERROR_CATEGORY Category;
			LPWSTR TargetObject;

			_MAPPED_ERROR_DATA(const WuStdException& exception, const LPWSTR errorId, WRITE_ERROR_CATEGORY category, const LPWSTR targetObject);
			_MAPPED_ERROR_DATA(const WuStdException& exception, const LPWSTR message, const LPWSTR errorId, WRITE_ERROR_CATEGORY category, const LPWSTR targetObject);
			~_MAPPED_ERROR_DATA();

		} MAPPED_ERROR_DATA, *PMAPPED_ERROR_DATA;

		typedef void(__stdcall* UnmanagedWriteProgress)();
		typedef void(__stdcall* UnmanagedWriteWarning)();
		typedef void(__stdcall* UnmanagedWriteInformation)();
		typedef void(__stdcall* UnmanagedWriteError)();
		typedef void(__stdcall* UnmanagedWriteObject)(WRITE_OUTPUT_TYPE objType);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////
	//																								 //
	// ~ WindowsUtils Native Context																 //
	//																								 //
	// This class is the evolution of the 'NATIVE_CONTEXT' struct.									 //
	// It's a native representation of the Cmdlet context.											 //
	// 																								 //
	// These function pointers maps to the delegate wrappers.										 //
	// These wrappers wrap the managed methods.														 //
	//																								 //
	// This is the current workflow:																 //
	//																								 //
	//                               	 Method -> Delegate -> Fn. Ptr.								 //
	//									   +------------------------+								 //
	//									   |						|								 //
	// +CMDLET-------------+      +CONTEXT-.CTOR----+      +CTXT-BASE-.CTOR--+		+WRAPPER-------+ //
	// |   Calls Wrapper   | +--> | Creates context | ---> |   Creates the   | ---> | Calls native | //
	// | (Context created) |      |		 base		|	   | WuNativeContext |		|   function   | //
	// +-------------------+      +-----------------+      +-----------------+      +--------------+ //
	//																					    |		 //
	//											Managed/Unmanaged Line					    |		 //
	//                                          #=================================================== //
	//						  					|								            |		 //
	//											|											˅        //
	//	+LIB------------+	  +CONTEXT------+	|  +N-CONTEXT---+    +N-CONTEXT----+	+N-FUNC----+ //
	//	| Cmdlet.Write* | <-- | Marshal raw | <-|- | Calls Hook | <- | Zeroes /    | <- | Calls    | //
	//	+---------------+     |     data    |   |  +------------+	 | Copies mem. |    | NativeWr.| //
	//						  +-------------+	|					 +-------------+	+----------+ //
	///////////////////////////////////////////////////////////////////////////////////////////////////

	extern "C++" public class __declspec(dllexport) WuNativeContext
	{
	private:
		Notification::UnmanagedWriteProgress m_WriteProgressHook;
		Notification::UnmanagedWriteWarning m_WriteWarningHook;
		Notification::UnmanagedWriteInformation m_WriteInformationHook;
		Notification::UnmanagedWriteObject m_WriteObjectHook;
		Notification::UnmanagedWriteError m_WriteErrorHook;
		BYTE* m_ProgressBuffer;
		BYTE* m_WarningBuffer;
		BYTE* m_InformationBuffer;
		BYTE* m_objectBuffer;
		BYTE* m_ErrorBuffer;

	public:
		WuNativeContext(
			Notification::UnmanagedWriteProgress progPtr,
			Notification::UnmanagedWriteWarning warnPtr,
			Notification::UnmanagedWriteInformation infoPtr,
			Notification::UnmanagedWriteObject objPtr,
			Notification::UnmanagedWriteError errorPtr,
			BYTE* progBuffer,
			BYTE* warningBuffer,
			BYTE* informationBuffer,
			BYTE* objectBuffer,
			BYTE* errorBuffer
		);
		
		~WuNativeContext();

		void NativeWriteWarning(const WWuString& text);
		void NativeWriteProgress(const Notification::PMAPPED_PROGRESS_DATA progData);
		void NativeWriteInformation(const Notification::PMAPPED_INFORMATION_DATA infoData);
		void NativeWriteError(const Notification::PMAPPED_ERROR_DATA errorData);
		
		// If I declare this here and define in the .cpp file an unresolved external error
		// pops up. Fuck if I know why.
		template <class T>
		void NativeWriteObject(T* objData, Notification::WRITE_OUTPUT_TYPE type)
		{
			size_t dataSize = sizeof(T);

			RtlZeroMemory(m_objectBuffer, 128);
			RtlCopyMemory(m_objectBuffer, objData, dataSize);
			m_WriteObjectHook(type);
		}
	};
}