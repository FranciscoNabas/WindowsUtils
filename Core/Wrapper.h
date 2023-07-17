#pragma once
#pragma unmanaged

#include "Registry.h"
#include "Services.h"
#include "Utilities.h"
#include "Containers.h"
#include "Notification.h"
#include "AccessControl.h"
#include "TerminalServices.h"
#include "ProcessAndThread.h"

#pragma managed
#include <vcclr.h>

using namespace System;
using namespace System::IO;
using namespace System::Security;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;
using namespace Microsoft::Win32;

namespace WindowsUtils::Core
{
	/*========================================
	==	  Managed object identification		==
	==========================================*/

	public enum class ArchiveFileType : DWORD
	{
		Cabinet
	};

	// Invoke-RemoteMessage
	public ref class MessageResponseBase
	{
	public:
		property UInt32 SessionId { UInt32 get() { return wrapper->SessionId; } }
		property UInt32 Response { UInt32 get() { return wrapper->Response; } }

		MessageResponseBase();
		MessageResponseBase(TerminalServices::WU_MESSAGE_RESPONSE exmessres);
		~MessageResponseBase();

	protected:
		!MessageResponseBase();

	private:
		TerminalServices::PWU_MESSAGE_RESPONSE wrapper;
	};

	// Get-ComputerSession
	public ref class ComputerSessionBase
	{
	public:
		property Int32 SessionId { Int32 get() { return wrapper->SessionId; } }
		property String^ UserName { String^ get() { return gcnew String(wrapper->UserName); } }
		property String^ SessionName { String^ get() { return gcnew String(wrapper->SessionName); } }
		property TimeSpan IdleTime {
			TimeSpan get()
			{
				if (wrapper->LastInputTime.QuadPart == 0)
					return TimeSpan::Zero;
				else
				{
					::FILETIME datepivot;
					datepivot.dwLowDateTime = wrapper->LastInputTime.LowPart;
					datepivot.dwHighDateTime = wrapper->LastInputTime.HighPart;

				}

				return DateTime::Now - DateTime::FromFileTime(wrapper->LastInputTime.QuadPart);
			}
		}
		property DateTime^ LogonTime {
			DateTime^ get() {
				if (wrapper->LogonTime.QuadPart == 0)
					return nullptr;

				return DateTime::FromFileTime(wrapper->LogonTime.QuadPart);
			}
		}
		property UInt32 SessionState { UInt32 get() { return wrapper->SessionState; } }
		property String^ ComputerName { String^ get() { return _computername; } }

		ComputerSessionBase();
		ComputerSessionBase(TerminalServices::WU_COMPUTER_SESSION excompsess);
		ComputerSessionBase(TerminalServices::WU_COMPUTER_SESSION excompsess, String^ inppcname);
		~ComputerSessionBase();

	protected:
		!ComputerSessionBase();

	private:
		String^ _computername;
		TerminalServices::PWU_COMPUTER_SESSION wrapper;
	};

	// Get-ObjectHandle
	public ref class ObjectHandleBase
	{
	public:
		property String^ InputObject { String^ get() { return gcnew String(wrapper->InputObject); } }
		property String^ Name {
			String^ get() {
				if (NULL != wrapper->Name)
					return gcnew String(wrapper->Name);

				return nullptr;
			}
		}
		property UInt32 ProcessId { UInt32 get() { return wrapper->ProcessId; } }
		property String^ Description
		{
			String^ get()
			{
				auto search = wrapper->VersionInfo.find(ProcessAndThread::FileDescription);
				if (search != wrapper->VersionInfo.end())
				{
					if (NULL != search->second)
					{
						if (wcslen(search->second) == 0)
						{
							if (NULL != wrapper->Name)
								if (wcslen(wrapper->Name) > 0)
								{
									WCHAR pinter[MAX_PATH]{ 0 };
									wcscpy_s(pinter, MAX_PATH, wrapper->Name);
									::PathStripPathW(pinter);

									return gcnew String(pinter);
								}
						}
						else
							return gcnew String(search->second);
					}
					else
					{
						if (NULL != wrapper->Name)
							if (wcslen(wrapper->Name) > 0)
							{
								WCHAR pinter[MAX_PATH]{ 0 };
								wcscpy_s(pinter, MAX_PATH, wrapper->Name);
								::PathStripPathW(pinter);

								return gcnew String(pinter);
							}
					}
				}
				else
				{
					if (NULL != wrapper->Name)
						if (wcslen(wrapper->Name) > 0)
						{
							WCHAR pinter[MAX_PATH]{ 0 };
							wcscpy_s(pinter, MAX_PATH, wrapper->Name);
							::PathStripPathW(pinter);

							return gcnew String(pinter);
						}
				}

				return nullptr;
			}
		}
		property String^ ProductName
		{
			String^ get()
			{
				auto search = wrapper->VersionInfo.find(ProcessAndThread::ProductName);
				if (search != wrapper->VersionInfo.end())
					if (NULL != search->second)
						return gcnew String(search->second);

				return nullptr;
			}
		}
		property String^ FileVersion
		{
			String^ get()
			{
				auto search = wrapper->VersionInfo.find(ProcessAndThread::FileVersion);
				if (search != wrapper->VersionInfo.end())
					if (NULL != search->second)
						return gcnew String(search->second);

				return nullptr;
			}
		}
		property String^ CompanyName
		{
			String^ get()
			{
				auto search = wrapper->VersionInfo.find(ProcessAndThread::CompanyName);
				if (search != wrapper->VersionInfo.end())
					if (NULL != search->second)
						return gcnew String(search->second);

				return nullptr;
			}
		}
		property String^ ImagePath {
			String^ get() {
				if (NULL != wrapper->ImagePath)
					return gcnew String(wrapper->ImagePath);

				return nullptr;
			}
		}

		ObjectHandleBase();
		ObjectHandleBase(ProcessAndThread::WU_OBJECT_HANDLE objhandle);
		~ObjectHandleBase();

	protected:
		!ObjectHandleBase();

	private:
		ProcessAndThread::PWU_OBJECT_HANDLE wrapper;
	};

	// Get-ResourceMessageTable
	public ref class ResourceMessageTableCore
	{
	public:
		property Int64 Id { Int64 get() { return wrapper->Id; } }
		property String^ Message { String^ get() { return (gcnew String(wrapper->Message))->Trim(); } }

		ResourceMessageTableCore();
		ResourceMessageTableCore(Utilities::WU_RESOURCE_MESSAGE_TABLE);
		~ResourceMessageTableCore();

	protected:
		!ResourceMessageTableCore();

	private:
		Utilities::PWU_RESOURCE_MESSAGE_TABLE wrapper;
	};

	// A wrapper for the Cmdlet context
	public ref class CmdletContextBase
	{
	public:
		delegate void WriteProgressWrapper(UInt64 dataSize);
		delegate void WriteWarningWrapper(UInt64 dataSize);

		CmdletContextBase(WriteProgressWrapper^ progWrapper, WriteWarningWrapper^ warnWrapper, IntPtr mappedProgData, IntPtr mappedWarnData);
		~CmdletContextBase();

		Notification::PNATIVE_CONTEXT GetUnderlyingContext();

	protected:
		!CmdletContextBase();

	private:
		Notification::PNATIVE_CONTEXT _nativeContext;
		GCHandle _progressGcHandle;
		GCHandle _warningGcHandle;
	};

	// TEST ONLY
	public ref class TokenPrivilege
	{
	public:
		property String^ PrivilegeName { String^ get() { return _privName; } }
		property Int32 Attributes { Int32 get() { return _attr; } }

		TokenPrivilege(String^ privilege, Int32 attr)
			: _privName(privilege), _attr(attr) { }
		~TokenPrivilege() { }
		
	protected:
		!TokenPrivilege() { }

	private:
		String^ _privName;
		Int32 _attr;
	};
	// END TEST

	/*=========================================
	==	  Wrapper function identification	 ==
	===========================================*/

	public ref class Wrapper
	{
	public:
		// Invoke-RemoteMessage
		array<MessageResponseBase^>^ SendRemoteMessage(IntPtr session, array<Int32>^ sessionid, String^ title, String^ message, UInt32 style, Int32 timeout, Boolean wait);

		// Get-ComputerSession
		array<ComputerSessionBase^>^ GetComputerSession(String^ computername, IntPtr session, Boolean onlyactive, Boolean includesystemsession);

		// Disconnect-Session
		Void DisconnectSession(IntPtr session, UInt32^ sessionid, Boolean wait);

		// Get-FormattedError
		String^ GetFormattedError(Int32 errorcode);

		// Get-LastWin32Error
		String^ GetLastWin32Error();

		// Get-ObjectHandle
		array<ObjectHandleBase^>^ GetProcessObjectHandle(array<String^>^ fileName, Boolean closeHandle);

		// Send-Click
		Void SendClick();

		// Get-ResourceMessageTable
		array<ResourceMessageTableCore^>^ GetResourceMessageTable(String^ libpath);

		// Get-MsiProperties
		Dictionary<String^, String^>^ GetMsiProperties(String^ filepath);
		
		// Remove-Service
		void RemoveService(String^ servicename, String^ computerName, bool stopservice, CmdletContextBase^ context);
		void RemoveService(IntPtr hservice, String^ servicename, String^ computername, bool stopservice, CmdletContextBase^ context);
		void RemoveService(String^ servicename, bool stopservice, CmdletContextBase^ context);

		// Get-ServiceSecurity
		String^ GetServiceSecurityDescriptorString(String^ serviceName, String^ computerName, bool audit);
		String^ GetServiceSecurityDescriptorString(String^ serviceName, bool audit);
		String^ GetServiceSecurityDescriptorString(IntPtr hService, bool audit);

		// Set-ServiceSecurity
		void SetServiceSecurity(String^ serviceName, String^ computerName, String^ sddl, bool audit, bool changeOwner);
		void SetServiceSecurity(String^ serviceName, String^ sddl, bool audit, bool changeOwner);

		// Expand-File
		void ExpandFile(String^ fileFullName, String^ destination, ArchiveFileType fileType);

		// Registry operation
		Object^ GetRegistryValue(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, String^ valueName);
		Object^ GetRegistryValue(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, String^ valueName);
		Object^ GetRegistryValue(String^ computerName, RegistryHive hive, String^ subKey, String^ valueName);
		Object^ GetRegistryValue(RegistryHive hive, String^ subKey, String^ valueName);
		Object^ GetRegistryValue(IntPtr hRegistry, String^ subKey, String^ valueName);

		array<String^>^ GetRegistrySubKeyNames(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey);
		array<String^>^ GetRegistrySubKeyNames(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey);
		array<String^>^ GetRegistrySubKeyNames(String^ computerName, RegistryHive hive, String^ subKey);
		array<String^>^ GetRegistrySubKeyNames(RegistryHive hive, String^ subKey);
		array<String^>^ GetRegistrySubKeyNames(IntPtr hRegistry, String^ subKey);

		array<Object^>^ GetRegistryValueList(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList);
		array<Object^>^ GetRegistryValueList(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList);
		array<Object^>^ GetRegistryValueList(String^ computerName, RegistryHive hive, String^ subKey, array<String^>^ valueNameList);
		array<Object^>^ GetRegistryValueList(RegistryHive hive, String^ subKey, array<String^>^ valueNameList);
		array<Object^>^ GetRegistryValueList(IntPtr hRegistry, String^ subKey, array<String^>^ valueNameList);

		// Utilities
		array<String^>^ GetStringArrayFromDoubleNullTermninatedCStyleArray(const LPWSTR& pvNativeArray, DWORD dwszBytes);
		array<String^>^ GetStringArrayFromDoubleNullTermninatedCStyleArray(IntPtr nativeArray, DWORD dwszBytes);
		void LogonAndImpersonateUser(String^ userName, SecureString^ password);

	private:
		Utilities* utlptr;
		TerminalServices* wtsptr;
		Services* svcptr;
		AccessControl* acptr;
		Registry* regptr;
		ProcessAndThread* patptr;
		Containers* ctnptr;

		Object^ GetRegistryValue(String^ computerName, String^ userName, const LPWSTR& lpszPassword, RegistryHive hive, String^ subKey, String^ valueName);
		array<Object^>^ GetRegistryValueList(String^ computerName, String^ userName, const LPWSTR& lpszPassword, RegistryHive hive, String^ subKey, array<String^>^ valueNameList);
		array<String^>^ GetRegistrySubKeyNames(String^ computerName, String^ userName, const LPWSTR& lpszPassword, RegistryHive hive, String^ subKey);
		void LogonAndImpersonateUser(String^ userName, const LPWSTR& lpszPassword);
	};

	Exception^ FDIErrorToException(FDIERROR err);

	public ref class NativeExceptionBase : public Exception
	{
	public:

		property int NativeErrorCode {
			int get() {
				return _nativeErrorCode;
			}
		}

		NativeExceptionBase() : Exception() { }

		NativeExceptionBase(int nativeErrorCode)
			: Exception(wrapper->GetFormattedError(nativeErrorCode)) {

			_nativeErrorCode = nativeErrorCode;
		}

		NativeExceptionBase(int nativeErrorCode, String^ message)
			: Exception(message) {

			_nativeErrorCode = nativeErrorCode;
		}

		NativeExceptionBase(int nativeErrorCode, String^ message, Exception^ innerException)
			: Exception(message, innerException) {

			_nativeErrorCode = nativeErrorCode;
		}

	private:
		int _nativeErrorCode;
		Wrapper^ wrapper = gcnew Wrapper();
	};
}