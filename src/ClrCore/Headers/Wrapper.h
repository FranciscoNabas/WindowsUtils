#pragma once
#pragma unmanaged

#include "String.h"
#include "Common.h"
#include "Network.h"
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
using namespace System::Runtime::Serialization;
using namespace System::Runtime::InteropServices;
using namespace Microsoft::Win32;

namespace WindowsUtils
{
	public enum class PreferredIpProtocol : UInt16
	{
		None,
		IPv4,
		IPv6
	};

	public enum class WriteOutputType : UInt32
	{
		TCPING_OUTPUT,
		TCPING_STATISTICS
	};

	[Serializable()]
	public ref class NativeException : public Exception
	{
	public:
		property Int32 ErrorCode { Int32 get() { return _errorCode; } }

#if defined(_DEBUG)
		property String^ CompactTrace { String^ get() { return _compactTrace; } }

		NativeException(WuResult& result)
			: Exception(gcnew String(result.Message.GetBuffer())),
			_errorCode(result.Result),
			_compactTrace(gcnew String(result.CompactTrace.GetBuffer()))
		{ }

		NativeException(const WuStdException& ex)
			: Exception(gcnew String(ex.Message().GetBuffer())),
			_errorCode(ex.ErrorCode()),
			_compactTrace(gcnew String(ex.CompactTrace().GetBuffer()))
		{ }
#else
		NativeException(WuResult& result)
			: Exception(gcnew String(result.Message.GetBuffer())),
			_errorCode(result.Result)
		{ }

		NativeException(const WuStdException& ex)
			: Exception(gcnew String(ex.Message().GetBuffer())),
			_errorCode(ex.ErrorCode())
		{ }
#endif
		NativeException(Int32 errorCode)
			: Exception((gcnew String(WuResult::GetErrorMessage(errorCode, false).GetBuffer()))->Trim()),
			_errorCode(errorCode)
		{ }

		NativeException(Int32 errorCode, String^ message)
			: Exception(message), _errorCode(errorCode)
		{ }

		NativeException(Int32 errorCode, String^ message, Exception^ inner_exception)
			: Exception(message, inner_exception), _errorCode(errorCode)
		{ }

	protected:
		NativeException()
			: Exception()
		{ }

		NativeException(SerializationInfo^ info, StreamingContext context)
			: Exception(info, context)
		{ }

	private:
		Int32 _errorCode;
#if defined(_DEBUG)
		String^ _compactTrace;
#endif
	};
}

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
		property String^ UserName { String^ get() { return gcnew String(wrapper->UserName.GetBuffer()); } }
		property String^ SessionName { String^ get() { return gcnew String(wrapper->SessionName.GetBuffer()); } }
		property TimeSpan IdleTime {
			TimeSpan get()
			{
				if (wrapper->LastInputTime.QuadPart == 0)
					return TimeSpan::Zero;
				else {
					::FILETIME datePivot;
					datePivot.dwLowDateTime = wrapper->LastInputTime.LowPart;
					datePivot.dwHighDateTime = wrapper->LastInputTime.HighPart;

				}

				return DateTime::Now - DateTime::FromFileTime(wrapper->LastInputTime.QuadPart);
			}
		}
		property DateTime^ LogonTime {
			DateTime^ get()
			{
				if (wrapper->LogonTime.QuadPart == 0)
					return nullptr;

				return DateTime::FromFileTime(wrapper->LogonTime.QuadPart);
			}
		}
		property UInt32 SessionState { UInt32 get() { return wrapper->SessionState; } }
		property String^ ComputerName { String^ get() { return _computerName; } }

		ComputerSessionBase();
		ComputerSessionBase(TerminalServices::WU_COMPUTER_SESSION excompsess);
		ComputerSessionBase(TerminalServices::WU_COMPUTER_SESSION excompsess, String^ inppcname);
		~ComputerSessionBase();

	protected:
		!ComputerSessionBase();

	private:
		String^ _computerName;
		TerminalServices::PWU_COMPUTER_SESSION wrapper;
	};

	// Get-ObjectHandle
	public ref class ObjectHandleBase
	{
	public:
		property String^ InputObject { String^ get() { return gcnew String(wrapper->InputObject.GetBuffer()); } }
		property String^ Name {
			String^ get()
			{
				if (!WWuString::IsNullOrEmpty(wrapper->Name))
					return gcnew String(wrapper->Name.GetBuffer());

				return nullptr;
			}
		}
		property UInt32 ProcessId { UInt32 get() { return wrapper->ProcessId; } }
		property String^ Description
		{
			String^ get()
			{
				auto search = wrapper->VersionInfo.find(ProcessAndThread::FileDescription);
				if (search != wrapper->VersionInfo.end()) {
					if (WWuString::IsNullOrEmpty(search->second)) {
						if (!WWuString::IsNullOrEmpty(wrapper->Name)) {
							WWuString name = wrapper->Name;
							::PathStripPathW(name.GetBuffer());

							return gcnew String(name.GetBuffer());
						}
					}
					else
						return gcnew String(search->second.GetBuffer());
				}
				else {
					if (!WWuString::IsNullOrEmpty(wrapper->Name)) {
						WWuString name = wrapper->Name;
						::PathStripPathW(name.GetBuffer());

						return gcnew String(name.GetBuffer());
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
					if (!WWuString::IsNullOrEmpty(search->second))
						return gcnew String(search->second.GetBuffer());

				return nullptr;
			}
		}
		property String^ FileVersion
		{
			String^ get()
			{
				auto search = wrapper->VersionInfo.find(ProcessAndThread::FileVersion);
				if (search != wrapper->VersionInfo.end())
					if (!WWuString::IsNullOrEmpty(search->second))
						return gcnew String(search->second.GetBuffer());

				return nullptr;
			}
		}
		property String^ CompanyName
		{
			String^ get()
			{
				auto search = wrapper->VersionInfo.find(ProcessAndThread::CompanyName);
				if (search != wrapper->VersionInfo.end())
					if (!WWuString::IsNullOrEmpty(search->second))
						return gcnew String(search->second.GetBuffer());

				return nullptr;
			}
		}
		property String^ ImagePath {
			String^ get()
			{
				if (!WWuString::IsNullOrEmpty(wrapper->ImagePath))
					return gcnew String(wrapper->ImagePath.GetBuffer());

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
		property String^ Message { String^ get() { return (gcnew String(wrapper->Message.GetBuffer()))->Trim(); } }

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
		delegate void WriteProgressWrapper();
		delegate void WriteWarningWrapper();
		delegate void WriteInformationWrapper();
		delegate void WriteObjectWrapper(WriteOutputType type);

		CmdletContextBase(
			WriteProgressWrapper^ progWrapper,
			WriteWarningWrapper^ warnWrapper,
			WriteInformationWrapper^ infoWrapper,
			WriteObjectWrapper^objWrapper,
			Byte* progressBuffer,
			Byte* warningBuffer,
			Byte* infoBuffer,
			Byte* objBuffer
		);

		~CmdletContextBase();

		WuNativeContext* GetUnderlyingContext();

	protected:
		!CmdletContextBase();

	private:
		WuNativeContext* _nativeContext;
		GCHandle _progressGcHandle;
		GCHandle _warningGcHandle;
		GCHandle _informationGcHandle;
		GCHandle _objectGcHandle;
	};

	// TEST ONLY
	public ref class TokenPrivilege
	{
	public:
		property String^ PrivilegeName { String^ get() { return _privName; } }
		property Int32 Attributes { Int32 get() { return _attr; } }

		TokenPrivilege(String^ privilege, Int32 attr)
			: _privName(privilege), _attr(attr)
		{ }
		~TokenPrivilege() { }

	protected:
		!TokenPrivilege() { }

	private:
		String^ _privName;
		Int32 _attr;
	};
	// END TEST

	public ref class WuManagedCabinet
	{
	public:
		property List<String^>^ BundleCabinetPaths {
			List<String^>^ get()
			{
				return _bundleCabinetPaths;
			}
		}

		void ExpandCabinetFile(String^ destination);

		WuManagedCabinet(String^ filePath, CmdletContextBase^ context);
		~WuManagedCabinet();

	protected:
		!WuManagedCabinet();

	private:
		List<String^>^ _bundleCabinetPaths;
		WuCabinet* _nativeCabinet;
	};

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
		array<String^>^ GetStringArrayFromDoubleNullTerminatedCStyleArray(const LPWSTR pvNativeArray, DWORD dwszBytes);
		array<String^>^ GetStringArrayFromDoubleNullTerminatedCStyleArray(IntPtr nativeArray, DWORD dwszBytes);
		void LogonAndImpersonateUser(String^ userName, SecureString^ password);

		// Containers
		void ExpandArchiveFile(Object^ archiveObject, String^ destination, ArchiveFileType fileType);

		// Start-Tcping
		void StartTcpPing(String^ destination, Int32 port, Int32 count, Int32 timeout, Int32 interval, PreferredIpProtocol ipProt, Int32 failThreshold, bool continuous,
			bool jitter, bool fqdn, bool force, bool single, String^ outFile, bool append, CmdletContextBase^ context, [Out] bool% isCancel);

		// Start-ProcessAsUser
		void StartProcessAsUser(String^ userName, String^ domain, SecureString^ password, String^ commandLine, String^ titleBar);

	private:
		Utilities* utlptr;
		TerminalServices* wtsptr;
		Services* svcptr;
		AccessControl* acptr;
		Registry* regptr;
		ProcessAndThread* patptr;
		Containers* ctnptr;
		Network* ntwptr;

		Object^ GetRegistryValue(String^ computerName, String^ userName, WWuString& password, RegistryHive hive, String^ subKey, String^ valueName);
		array<Object^>^ GetRegistryValueList(String^ computerName, String^ userName, WWuString& password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList);
		array<String^>^ GetRegistrySubKeyNames(String^ computerName, String^ userName, WWuString& password, RegistryHive hive, String^ subKey);
		void LogonAndImpersonateUser(String^ userName, WWuString& password);
	};

	WWuString GetWideStringFromSystemString(String^ string);
}