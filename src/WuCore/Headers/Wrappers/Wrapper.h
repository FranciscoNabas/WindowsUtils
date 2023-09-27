#pragma once
#pragma unmanaged

#include "../Engine/Services.h"
#include "../Engine/AccessControl.h"
#include "../Engine/Registry.h"

#pragma managed

#include "../../Headers/Wrappers/Objects/MessageResponse.h"
#include "../../Headers/Wrappers/Objects/ComputerSession.h"
#include "../../Headers/Wrappers/Objects/ObjectHandle.h"
#include "../../Headers/Wrappers/Objects/ResourceMessageTable.h"
#include "../../Headers/Wrappers/Objects/WuManagedCabinet.h"
#include "../../Headers/Wrappers/Objects/NetworkFileInfo.h"

namespace WindowsUtils::Core
{
	using namespace System::Security;
	using namespace Microsoft::Win32;

	/*
	*	~ Ze main wrapper
	*/

	public ref class Wrapper
	{
	public:
		// Invoke-RemoteMessage
		array<MessageResponse^>^ SendRemoteMessage(IntPtr session, array<Int32>^ sessionid, String^ title, String^ message, UInt32 style, Int32 timeout, Boolean wait);

		// Get-ComputerSession
		array<ComputerSession^>^ GetComputerSession(String^ computername, Boolean onlyactive, Boolean includesystemsession);

		// Disconnect-Session
		Void DisconnectSession(IntPtr session, UInt32^ sessionid, Boolean wait);

		// Get-FormattedError
		String^ GetFormattedError(Int32 errorcode, ErrorType source);

		// Get-LastWin32Error
		String^ GetLastWin32Error();

		// Get-ObjectHandle
		array<ObjectHandle^>^ GetProcessObjectHandle(array<ObjectHandleInput^>^ inputList, Boolean closeHandle);

		// Send-Click
		Void SendClick();

		// Get-ResourceMessageTable
		array<ResourceMessageTable^>^ GetResourceMessageTable(String^ libpath);

		// Get-MsiProperties
		Dictionary<String^, Object^>^ GetMsiProperties(String^ filepath);

		// Remove-Service
		void RemoveService(String^ servicename, String^ computerName, bool stopservice, CmdletContextBase^ context, bool noWait);
		void RemoveService(String^ servicename, bool stopservice, CmdletContextBase^ context, bool noWait);

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
		void CompressArchiveFile(String^ path, String^ destination, String^ namePrefix, int maxCabSize, CabinetCompressionType compressionType, ArchiveFileType type, CmdletContextBase^ context);

		// Start-Tcping
		void StartTcpPing(String^ destination, Int32 port, Int32 count, Int32 timeout, Int32 interval, Int32 failThreshold, bool continuous,
			bool jitter, bool fqdn, bool force, bool single, String^ outFile, bool append, CmdletContextBase^ context, [Out] bool% isCancel);

		// Start-ProcessAsUser
		void StartProcessAsUser(String^ userName, String^ domain, SecureString^ password, String^ commandLine, String^ titleBar);

		// Get-NetworkFile
		List<NetworkFileInfo^>^ GetNetworkFile(String^ computerName, String^ basePath, String^ userName, bool includeSessionName);

		// Close-NetworkFile
		void CloseNetworkFile(String^ computerName, Int32 fileId);

		// Test-Port
		void TestNetworkPort(String^ destination, UInt32 port, TransportProtocol protocol, UInt32 timeout, CmdletContextBase^ context);

		// Get-ProcessModule
		void ListProcessModule(array<UInt32>^ processIdList, bool includeVersionInfo, CmdletContextBase^ context);
		void ListProcessModule(bool includeVersionInfo, CmdletContextBase^ context);

		// Suspend-Process
		void SuspendProcess(UInt32 processId, CmdletContextBase^ context);

		// Resume-Process
		void ResumeProcess(UInt32 processId, CmdletContextBase^ context);

		// Utilities
		static String^ GetRegistryNtPath(String^ keyPath);

	private:
		Utilities* utlPtr;
		TerminalServices* wtsPtr;
		Services* svcPtr;
		AccessControl* acPtr;
		Registry* regPtr;
		ProcessAndThread* patPtr;
		Network* ntwPtr;

		Object^ GetRegistryValue(String^ computerName, String^ userName, WWuString& password, RegistryHive hive, String^ subKey, String^ valueName);
		array<Object^>^ GetRegistryValueList(String^ computerName, String^ userName, WWuString& password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList);
		array<String^>^ GetRegistrySubKeyNames(String^ computerName, String^ userName, WWuString& password, RegistryHive hive, String^ subKey);
		void LogonAndImpersonateUser(String^ userName, WWuString& password);
	};

	/*
	*	~ Utility functions
	*/

	WWuString GetWideStringFromSystemString(String^ string);
	static void GetAptFromPath(String^ path, AbstractPathTree* apt);
}