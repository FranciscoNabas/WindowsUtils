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

		typedef void(__stdcall* UnmanagedWriteProgress)();
		typedef void(__stdcall* UnmanagedWriteWarning)();
		typedef void(__stdcall* UnmanagedWriteInformation)();

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

	extern "C" public class __declspec(dllexport) WuNativeContext
	{
	private:
		Notification::UnmanagedWriteProgress m_WriteProgressHook;
		Notification::UnmanagedWriteWarning m_WriteWarningHook;
		Notification::UnmanagedWriteInformation m_WriteInformationHook;
		BYTE* m_ProgressBuffer;
		BYTE* m_WarningBuffer;
		BYTE* m_InformationBuffer;

	public:
		WuNativeContext(
			Notification::UnmanagedWriteProgress progPtr,
			Notification::UnmanagedWriteWarning warnPtr,
			Notification::UnmanagedWriteInformation infoPtr,
			BYTE* progBuffer,
			BYTE* warningBuffer,
			BYTE* informationBuffer
		);
		
		~WuNativeContext();

		void NativeWriteWarning(const WWuString& text);
		void NativeWriteProgress(const Notification::PMAPPED_PROGRESS_DATA progData);
		void NativeWriteInformation(const Notification::PMAPPED_INFORMATION_DATA infoData);
	};
}