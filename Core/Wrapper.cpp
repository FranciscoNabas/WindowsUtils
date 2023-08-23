#include "pch.h"
#include "Wrapper.h"

namespace WindowsUtils::Core
{
	/*========================================
	==	    Managed object definition		==
	==========================================*/

	//Invoke-RemoteMessage
	MessageResponseBase::MessageResponseBase() : wrapper(new TerminalServices::WU_MESSAGE_RESPONSE) { }
	MessageResponseBase::MessageResponseBase(TerminalServices::WU_MESSAGE_RESPONSE unwrapped)
	{
		wrapper = new TerminalServices::WU_MESSAGE_RESPONSE(
			unwrapped.SessionId,
			unwrapped.Response
		);
	}
	MessageResponseBase::~MessageResponseBase() { delete wrapper; }
	MessageResponseBase::!MessageResponseBase() { delete wrapper; }

	//Get-ComputerSession
	ComputerSessionBase::ComputerSessionBase() : wrapper(new TerminalServices::WU_COMPUTER_SESSION) { }
	ComputerSessionBase::ComputerSessionBase(TerminalServices::WU_COMPUTER_SESSION unwrapped)
	{
		wrapper = new TerminalServices::WU_COMPUTER_SESSION(
			unwrapped.SessionId,
			unwrapped.UserName,
			unwrapped.SessionName,
			unwrapped.LastInputTime,
			unwrapped.LogonTime,
			unwrapped.SessionState
		);
	}
	ComputerSessionBase::ComputerSessionBase(TerminalServices::WU_COMPUTER_SESSION unwrapped, String^ computerName)
	{
		_computerName = computerName;
		wrapper = new TerminalServices::WU_COMPUTER_SESSION(
			unwrapped.SessionId,
			unwrapped.UserName,
			unwrapped.SessionName,
			unwrapped.LastInputTime,
			unwrapped.LogonTime,
			unwrapped.SessionState
		);
	}
	ComputerSessionBase::~ComputerSessionBase() { delete wrapper; }
	ComputerSessionBase::!ComputerSessionBase() { delete wrapper; }

	// Get-ObjectHandle
	ObjectHandleBase::ObjectHandleBase() : wrapper(new ProcessAndThread::WU_OBJECT_HANDLE) { }
	ObjectHandleBase::ObjectHandleBase(ProcessAndThread::WU_OBJECT_HANDLE objectHandle)
	{
		wrapper = new ProcessAndThread::WU_OBJECT_HANDLE;
		wrapper->ImagePath = objectHandle.ImagePath;
		wrapper->Name = objectHandle.Name;
		wrapper->InputObject = objectHandle.InputObject;
		wrapper->ProcessId = objectHandle.ProcessId;
		wrapper->VersionInfo = objectHandle.VersionInfo;
	}
	ObjectHandleBase::~ObjectHandleBase() { delete wrapper; }
	ObjectHandleBase::!ObjectHandleBase() { delete wrapper; }

	// Get-ResourceMessageTable
	ResourceMessageTableCore::ResourceMessageTableCore() :wrapper(new Utilities::WU_RESOURCE_MESSAGE_TABLE) { }
	ResourceMessageTableCore::ResourceMessageTableCore(Utilities::WU_RESOURCE_MESSAGE_TABLE messageTable) { wrapper = new Utilities::WU_RESOURCE_MESSAGE_TABLE(messageTable.Id, messageTable.Message.GetBuffer()); }
	ResourceMessageTableCore::~ResourceMessageTableCore() { delete wrapper; }
	ResourceMessageTableCore::!ResourceMessageTableCore() { delete wrapper; }

	// Managed cabinet wrapper
	WuManagedCabinet::WuManagedCabinet(String^ filePath, CmdletContextBase^ context) {
		_bundleCabinetPaths = gcnew List<String^>();
		
		WWuString wrappedFilePath = GetWideStringFromSystemString(filePath);
		_nativeCabinet = new WuCabinet(wrappedFilePath, context->GetUnderlyingContext());

		for (const CABINET_PROCESSING_INFO& cabInfo : _nativeCabinet->GetCabinetInfo()) {
			_bundleCabinetPaths->Add(gcnew String(cabInfo.Path.GetBuffer()));
		}
	}

	void WuManagedCabinet::ExpandCabinetFile(String^ destination) {
		WWuString wrappedDestination = GetWideStringFromSystemString(destination);
		
		WuResult result = _nativeCabinet->ExpandCabinetFile(wrappedDestination);
		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result);
	}

	WuManagedCabinet::~WuManagedCabinet() { }
	WuManagedCabinet::!WuManagedCabinet() { }

	/*========================================
	==		Wrapper function definition		==
	==========================================*/

	// Invoke-RemoteMessage
	array<MessageResponseBase^>^ Wrapper::SendRemoteMessage(IntPtr session, array<Int32>^ sessionid, String^ title, String^ message, UInt32 style, Int32 timeout, Boolean wait)
	{
		WuResult result;
		
		wusunique_vector<TerminalServices::WU_MESSAGE_RESPONSE> responseList = make_wusunique_vector<TerminalServices::WU_MESSAGE_RESPONSE>();
		wusunique_vector<DWORD> sessionIdList;

		WWuString wuTitle = GetWideStringFromSystemString(title);
		WWuString wuMessage = GetWideStringFromSystemString(message);

		if (sessionid == nullptr)
			result = wtsptr->SendMessage(wuTitle, wuMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, responseList.get(), (HANDLE)session);

		else
		{
			sessionIdList = make_wusunique_vector<DWORD>();
			for (int i = 0; i < sessionid->Length; i++)
				sessionIdList->push_back((DWORD)sessionid[i]);

			result = wtsptr->SendMessage(wuTitle, wuMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, sessionIdList.get(), responseList.get(), (HANDLE)session);
		}

		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result);

		List<MessageResponseBase^>^ output = gcnew List<MessageResponseBase^>();
		for(auto& response : *responseList)
			if (response.Response != 0)
				output->Add(gcnew MessageResponseBase(response));

		return output->ToArray();
	}

	// Get-ComputerSession
	array<ComputerSessionBase^>^ Wrapper::GetComputerSession(String^ computerName, IntPtr session, Boolean activeOnly, Boolean includeSystemSessions)
	{
		wusunique_vector<TerminalServices::WU_COMPUTER_SESSION> sessionList = make_wusunique_vector<TerminalServices::WU_COMPUTER_SESSION>();
		WuResult result = wtsptr->GetEnumeratedSession(sessionList.get(), (HANDLE)session, activeOnly, includeSystemSessions);
		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result);

		List<ComputerSessionBase^>^ output = gcnew List<ComputerSessionBase^>((int)sessionList->size());

		if (computerName != nullptr)
			for (auto& sessionInfo : *sessionList)
				output->Add(gcnew ComputerSessionBase(sessionInfo, computerName));
		else
			for (auto& sessionInfo : *sessionList)
				output->Add(gcnew ComputerSessionBase(sessionInfo));

		return output->ToArray();
	}

	// Disconnect-Session
	Void Wrapper::DisconnectSession(IntPtr session, UInt32^ sessionid, Boolean wait)
	{
		WuResult result;
		if (nullptr == sessionid)
			result = wtsptr->DisconnectSession((HANDLE)session, NULL, wait);
		else
		{
			result = wtsptr->DisconnectSession((HANDLE)session, (DWORD)sessionid, wait);
			if (result.Result != ERROR_SUCCESS)
				throw gcnew NativeException(result);
		}
	}

	// Get-FormattedError
	String^ Wrapper::GetFormattedError(Int32 errorCode)
	{
		WWuString errorMessage;
		WuResult result = utlptr->GetFormattedError((DWORD)errorCode, errorMessage);
		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result.Result, String::Format("'GetFormattedError' failed with {0}", result.Result));

		return (gcnew String(errorMessage.GetBuffer()))->Trim();
	}
	// Get-LastWin32Error
	String^ Wrapper::GetLastWin32Error()
	{
		WWuString errorMessage;
		WuResult result = utlptr->GetFormattedWin32Error(errorMessage);
		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result.Result, String::Format("'GetFormattedError' failed with {0}", result.Result));

		return (gcnew String(errorMessage.GetBuffer()))->Trim();
	}

	// Get-ObjectHandle
	array<ObjectHandleBase^>^ Wrapper::GetProcessObjectHandle(array<String^>^ fileName, Boolean closeHandle)
	{
		wusunique_vector<ProcessAndThread::WU_OBJECT_HANDLE> ppOutput = make_wusunique_vector<ProcessAndThread::WU_OBJECT_HANDLE>();
		wusunique_vector<WWuString> reslist = make_wusunique_vector<WWuString>();

		for each (String^ name in fileName)
		{
			WWuString wuName = GetWideStringFromSystemString(name);
			reslist->push_back(wuName);
		}

		WuResult result = patptr->GetProcessObjectHandle(ppOutput.get(), reslist.get(), closeHandle);
		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result);

		if (ppOutput->size() == 0)
			return nullptr;

		List<ObjectHandleBase^>^ output = gcnew List<ObjectHandleBase^>();
		for (auto& handleInfo : *ppOutput)
		{
			if (!WWuString::IsNullOrEmpty(handleInfo.Name))
				output->Add(gcnew ObjectHandleBase(handleInfo));
		}

		return output->ToArray();
	}

	// Send-Click
	Void Wrapper::SendClick()
	{
		WuResult result = utlptr->SendClick();
		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result);
	}

	// Get-ResourceMessageTable
	array<ResourceMessageTableCore^>^ Wrapper::GetResourceMessageTable(String^ libPath)
	{
		wusunique_vector<Utilities::WU_RESOURCE_MESSAGE_TABLE> messageTable = make_wusunique_vector<Utilities::WU_RESOURCE_MESSAGE_TABLE>();
		
		WWuString wuLibPath = GetWideStringFromSystemString(libPath);
		
		WuResult result = utlptr->GetResourceMessageTable(messageTable.get(), wuLibPath);
		if (result.Result != ERROR_SUCCESS)
		{
			if (result.Result == ERROR_BAD_EXE_FORMAT)
			{
				PathStripPathW(wuLibPath.GetBuffer());
				String^ errormsg = gcnew String(wuLibPath.GetBuffer());
				throw gcnew NativeException(result.Result, errormsg + " is not a valid Win32 application.");
			}
			else
				throw gcnew NativeException(result);
		}

		List<ResourceMessageTableCore^>^ output = gcnew List<ResourceMessageTableCore^>();
		for (auto& tableEntry : *messageTable)
			output->Add(gcnew ResourceMessageTableCore(tableEntry));

		return output->ToArray();
	}

	// Get-MsiProperties
	Dictionary<String^, String^>^ Wrapper::GetMsiProperties(String^ filePath)
	{
		Dictionary<String^, String^>^ output = gcnew Dictionary<String^, String^>();
		wusunique_map<WWuString, WWuString> propertyMap = make_wusunique_map<WWuString, WWuString>();
		WWuString wuFileName = GetWideStringFromSystemString(filePath);

		WuResult result = utlptr->GetMsiProperties(propertyMap.get(), wuFileName);
		if (result.Result != ERROR_SUCCESS && result.Result != ERROR_NO_MORE_ITEMS)
		{
			WWuString msiError;
			WuResult inResult = utlptr->GetMsiExtendedError(msiError);
			if (inResult.Result != ERROR_SUCCESS)
				throw gcnew NativeException(result.Result, gcnew String(result.Message.GetBuffer()), gcnew SystemException(gcnew String(msiError.GetBuffer())));
			else
				throw gcnew NativeException(result);
		}

		std::map<WWuString, WWuString>::iterator itr;
		for (itr = propertyMap->begin(); itr != propertyMap->end(); itr++)
			output->Add(gcnew String(itr->first.GetBuffer()), gcnew String(itr->second.GetBuffer()));

		return output;
	}

	// Remove-Service
	void Wrapper::RemoveService(String^ serviceName, String^ computerName, bool stopService, CmdletContextBase^ context)
	{
		WWuString wuComputerName = GetWideStringFromSystemString(computerName);
		WWuString wuServiceName = GetWideStringFromSystemString(serviceName);

		WuResult result = svcptr->RemoveService(wuServiceName, wuComputerName, stopService, context->GetUnderlyingContext());
		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result);
	}
	
	void Wrapper::RemoveService(String^ serviceName, bool stopService, CmdletContextBase^ context)
	{
		WWuString wuServiceName = GetWideStringFromSystemString(serviceName);

		WuResult result = svcptr->RemoveService(wuServiceName, L"", stopService, context->GetUnderlyingContext());
		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result);
	}

	void Wrapper::RemoveService(IntPtr hService, String^ serviceName, String^ computerName, bool stopService, CmdletContextBase^ context)
	{
		WWuString wuComputerName = GetWideStringFromSystemString(computerName);
		WWuString wuServiceName = GetWideStringFromSystemString(serviceName);


		HANDLE wService = static_cast<HANDLE>(hService);
		SC_HANDLE schService = static_cast<SC_HANDLE>(wService);
		WuResult result = svcptr->RemoveService(schService, wuServiceName, wuComputerName, stopService, context->GetUnderlyingContext());

		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result);
	}

	// Get-ServiceSecurity
	String^ Wrapper::GetServiceSecurityDescriptorString(String^ serviceName, String^ computerName, bool audit)
	{
		WuResult result;

		WWuString wuServiceName = GetWideStringFromSystemString(computerName);
		WWuString wuComputerName = GetWideStringFromSystemString(serviceName);

		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		if (audit)
			secInfo |= SACL_SECURITY_INFORMATION;

		WWuString sddl;
		DWORD size = 0;
		if (audit)
			result = svcptr->GetServiceSecurity(wuServiceName, wuComputerName, sddl, &size, TRUE);
		else
			result = svcptr->GetServiceSecurity(wuServiceName, wuComputerName, sddl, &size);

		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result);

		String^ manSddl = gcnew String(sddl.GetBuffer());

		return manSddl;
	}

	String^ Wrapper::GetServiceSecurityDescriptorString(String^ serviceName, bool audit)
	{
		WuResult result;

		WWuString wuServiceName = GetWideStringFromSystemString(serviceName);
		
		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		if (audit)
			secInfo |= SACL_SECURITY_INFORMATION;

		WWuString sddl;
		DWORD size = 0;
		if (audit)
			result = svcptr->GetServiceSecurity(wuServiceName, L"", sddl, &size, TRUE);
		else
			result = svcptr->GetServiceSecurity(wuServiceName, L"", sddl, &size);

		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result);

		String^ manSddl = gcnew String(sddl.GetBuffer());

		return manSddl;
	}

	String^ Wrapper::GetServiceSecurityDescriptorString(IntPtr hService, bool audit)
	{
		WuResult result;
		HANDLE phService = static_cast<HANDLE>(hService);
		SC_HANDLE whService = static_cast<SC_HANDLE>(phService);

		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		if (audit)
			secInfo |= SACL_SECURITY_INFORMATION;

		WWuString sddl;
		DWORD size = 0;
		if (audit)
			result = svcptr->GetServiceSecurity(whService, sddl, &size, TRUE);
		else
			result = svcptr->GetServiceSecurity(whService, sddl, &size);

		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result);

		String^ manSddl = gcnew String(sddl.GetBuffer());

		return manSddl;
	}

	// Set-ServiceSecurity
	void Wrapper::SetServiceSecurity(String^ serviceName, String^ computerName, String^ sddl, bool audit, bool changeOwner)
	{
		WWuString wuServiceName = GetWideStringFromSystemString(serviceName);
		WWuString wuComputerName = GetWideStringFromSystemString(computerName);
		WWuString wuSddl = GetWideStringFromSystemString(sddl);
		
		WuResult result = svcptr->SetServiceSecurity(wuServiceName, wuComputerName, wuSddl, audit, changeOwner);
		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result);
	}

	void Wrapper::SetServiceSecurity(String^ serviceName, String^ sddl, bool audit, bool changeOwner)
	{
		WWuString wuServiceName = GetWideStringFromSystemString(serviceName);
		WWuString wuSddl = GetWideStringFromSystemString(sddl);

		WuResult result = svcptr->SetServiceSecurity(wuServiceName, L"", wuSddl, audit, changeOwner);
		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result);
	}

	// Registry operations
	Object^ Wrapper::GetRegistryValue(RegistryHive hive, String^ subKey, String^ valueName)
	{
		WWuString wuPass = L"";
		return GetRegistryValue(L"", L"", wuPass, hive, subKey, valueName);
	}

	Object^ Wrapper::GetRegistryValue(String^ computerName, RegistryHive hive, String^ subKey, String^ valueName)
	{
		WWuString wuPass = L"";
		return GetRegistryValue(computerName, L"", wuPass, hive, subKey, valueName);
	}

	Object^ Wrapper::GetRegistryValue(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, String^ valueName)
	{
		WWuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistryValue(L"", userName, wuPass, hive, subKey, valueName);
	}

	Object^ Wrapper::GetRegistryValue(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, String^ valueName)
	{
		WWuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistryValue(computerName, userName, wuPass, hive, subKey, valueName);
	}

	Object^ Wrapper::GetRegistryValue(IntPtr hRegistry, String^ subKey, String^ valueName)
	{
		DWORD dwValueType = 0;
		DWORD dwBytesReturned = 0;
		wuunique_ha_ptr<void> data;

		WWuString wuSubKey = GetWideStringFromSystemString(subKey);
		WWuString wuValueName = GetWideStringFromSystemString(valueName);

		HKEY whReg = (HKEY)hRegistry.ToPointer();
		WuResult result = regptr->GetRegistryKeyValue(whReg, wuSubKey, wuValueName, dwValueType, data, dwBytesReturned);
		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result);

		Object^ output;
		switch (dwValueType)
		{
		case REG_BINARY:
		{
			output = gcnew array<byte>(dwBytesReturned);
			Marshal::Copy((IntPtr)data.get(), (array<byte>^)output, 0, dwBytesReturned);
		}
		break;

		case REG_DWORD:
		{
			DWORD* dwData = reinterpret_cast<DWORD*>(data.get());
			output = gcnew Int32(*dwData);
		}
		break;

		case REG_EXPAND_SZ:
		{
			// Getting the necessary buffer.
			LPCWSTR strData = reinterpret_cast<LPCWSTR>(data.get());
			DWORD charCount = ExpandEnvironmentStrings(strData, NULL, 0);
			if (charCount == 0)
				throw gcnew NativeException(GetLastError());

			
			DWORD bytesNeeded = charCount * 2;
			wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);
			charCount = ExpandEnvironmentStrings(strData, buffer.get(), bytesNeeded);
			if (charCount == 0)
				throw gcnew NativeException(GetLastError());

			output = gcnew String(buffer.get());
		}
		break;

		case REG_LINK:
			output = gcnew String(reinterpret_cast<LPWSTR>(data.get()));
			break;

		case REG_MULTI_SZ:
			output = GetStringArrayFromDoubleNullTerminatedCStyleArray(reinterpret_cast<LPWSTR>(data.get()), dwBytesReturned);
			break;

		case REG_QWORD:
		{
			long long* qwData = reinterpret_cast<long long*>(data.get());
			output = gcnew Int64(*qwData);
		}
		break;

		case REG_SZ:
			output = gcnew String(reinterpret_cast<LPWSTR>(data.get()));
			break;

		default:
			throw gcnew ArgumentException(String::Format("Invalid registry type '{0}'.", dwValueType));
			break;
		}

		return output;
	}

	Object^ Wrapper::GetRegistryValue(
		String^ computerName,	  // The computer name. If the computer is remote, it needs Remote Registry enabled.
		String^ userName,		  // User name to impersonate before connecting to the registry.
		WWuString& password,		  // User password.
		RegistryHive hive,		  // The root hive.
		String^ subKey,			  // The subkey path.
		String^ valueName		  // The value property name.
	) {
		DWORD dwValueType = 0;
		DWORD dwBytesReturned = 0;
		wuunique_ha_ptr<void> data;

		// Managing logon.
		if (!String::IsNullOrEmpty(userName))
			LogonAndImpersonateUser(userName, password);
		
		WWuString wuComputerName;
		if (!String::IsNullOrEmpty(computerName))
			wuComputerName = GetWideStringFromSystemString(computerName);

		WWuString wuSubKey = GetWideStringFromSystemString(subKey);
		WWuString wuValueName = GetWideStringFromSystemString(valueName);

		WuResult result = regptr->GetRegistryKeyValue(wuComputerName, (HKEY)hive, wuSubKey, wuValueName, dwValueType, data, dwBytesReturned);
		if (result.Result != ERROR_SUCCESS)
		{
			RevertToSelf();
			throw gcnew NativeException(result);
		}

		Object^ output;
		switch (dwValueType)
		{
		case REG_BINARY:
		{
			output = gcnew array<byte>(dwBytesReturned);
			Marshal::Copy((IntPtr)data.get(), (array<byte>^)output, 0, dwBytesReturned);
		}
		break;

		case REG_DWORD:
		{
			DWORD* dwData = reinterpret_cast<DWORD*>(data.get());
			output = gcnew Int32(*dwData);
		}
		break;

		case REG_EXPAND_SZ:
		{
			// Getting the necessary buffer.
			LPCWSTR strData = reinterpret_cast<LPCWSTR>(data.get());
			DWORD charCount = ExpandEnvironmentStrings(strData, NULL, 0);
			if (charCount == 0)
				throw gcnew NativeException(GetLastError());


			DWORD bytesNeeded = charCount * 2;
			wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);
			charCount = ExpandEnvironmentStrings(strData, buffer.get(), bytesNeeded);
			if (charCount == 0)
				throw gcnew NativeException(GetLastError());

			output = gcnew String(buffer.get());
		}
		break;

		case REG_LINK:
			output = gcnew String(reinterpret_cast<LPWSTR>(data.get()));
			break;

		case REG_MULTI_SZ:
			output = GetStringArrayFromDoubleNullTerminatedCStyleArray(reinterpret_cast<LPWSTR>(data.get()), dwBytesReturned);
			break;

		case REG_QWORD:
		{
			long long* qwData = reinterpret_cast<long long*>(data.get());
			output = gcnew Int64(*qwData);
		}
		break;

		case REG_SZ:
			output = gcnew String(reinterpret_cast<LPWSTR>(data.get()));
			break;

		default:
			throw gcnew ArgumentException(String::Format("Invalid registry type '{0}'.", dwValueType));
			break;
		}

		RevertToSelf();

		return output;
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(RegistryHive hive, String^ subKey)
	{
		WWuString wuPass = L"";
		return GetRegistrySubKeyNames(L"", L"", wuPass, hive, subKey);
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(String^ computerName, RegistryHive hive, String^ subKey)
	{
		WWuString wuPass = L"";
		return GetRegistrySubKeyNames(computerName, L"", wuPass, hive, subKey);
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey)
	{
		WWuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistrySubKeyNames(L"", userName, wuPass, hive, subKey);
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey)
	{
		WWuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistrySubKeyNames(computerName, userName, wuPass, hive, subKey);
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(IntPtr hRegistry, String^ subKey)
	{
		WuResult result;
		wusunique_vector<WWuString> subkeyNameVec = make_wusunique_vector<WWuString>();

		WWuString wuSubKey = GetWideStringFromSystemString(subKey);

		HKEY whReg = (HKEY)hRegistry.ToPointer();
		result = regptr->GetRegistrySubkeyNames(whReg, wuSubKey, 0, subkeyNameVec.get());
		if (result.Result != ERROR_SUCCESS)
			throw gcnew NativeException(result);

		List<String^>^ output = gcnew List<String^>();
		for (WWuString singleName : *subkeyNameVec)
			output->Add(gcnew String(singleName.GetBuffer()));

		RevertToSelf();

		return output->ToArray();
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(String^ computerName, String^ userName, WWuString& password, RegistryHive hive, String^ subKey)
	{
		WuResult result;
		wusunique_vector<WWuString> subkeyNameVec = make_wusunique_vector<WWuString>();

		// Managing logon.
		if (!String::IsNullOrEmpty(userName))
			LogonAndImpersonateUser(userName, password);

		WWuString wuComputerName;
		if (!String::IsNullOrEmpty(computerName))
			wuComputerName = GetWideStringFromSystemString(computerName);

		WWuString wuSubKey = GetWideStringFromSystemString(subKey);

		result = regptr->GetRegistrySubkeyNames(wuComputerName, (HKEY)hive, wuSubKey, 0, subkeyNameVec.get());
		if (result.Result != ERROR_SUCCESS)
		{
			RevertToSelf();
			throw gcnew NativeException(result);
		}

		List<String^>^ output = gcnew List<String^>();
		for (WWuString singleName : *subkeyNameVec)
			output->Add(gcnew String(singleName.GetBuffer()));

		RevertToSelf();

		return output->ToArray();
	}

	array<Object^>^ Wrapper::GetRegistryValueList(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		WWuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return Wrapper::GetRegistryValueList(L"", userName, wuPass, hive, subKey, valueNameList);
	}

	array<Object^>^ Wrapper::GetRegistryValueList(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		WWuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return Wrapper::GetRegistryValueList(computerName, userName, wuPass, hive, subKey, valueNameList);
	}

	array<Object^>^ Wrapper::GetRegistryValueList(String^ computerName, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		WWuString wuPass = L"";
		return Wrapper::GetRegistryValueList(computerName, L"", wuPass, hive, subKey, valueNameList);
	}

	array<Object^>^ Wrapper::GetRegistryValueList(RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		WWuString wuPass = L"";
		return Wrapper::GetRegistryValueList(L"", L"", wuPass, hive, subKey, valueNameList);
	}

	array<Object^>^ Wrapper::GetRegistryValueList(IntPtr hRegistry, String^ subKey, array<String^>^ valueNameList)
	{
		WuResult nativeResult;
		DWORD dwValCount = valueNameList->Length;
		PVALENT pValList = new VALENT[valueNameList->Length];
		WWuString* wuValueNames = new WWuString[valueNameList->Length];
		
		wuunique_ha_ptr<void> buffer;
		WWuString wuSubKey = GetWideStringFromSystemString(subKey);

		for (DWORD i = 0; i < dwValCount; i++)
		{
			wuValueNames[i] = GetWideStringFromSystemString(valueNameList[i]);
			pValList[i].ve_valuename = wuValueNames[i].GetBuffer();
		}
		
		HKEY whReg = (HKEY)hRegistry.ToPointer();
		nativeResult = regptr->GetRegistryKeyValueList(whReg, wuSubKey, pValList, dwValCount, buffer);
		if (nativeResult.Result != ERROR_SUCCESS)
			throw gcnew NativeException(nativeResult);

		array<Object^>^ output = gcnew array<Object^>(dwValCount);
		for (DWORD i = 0; i < dwValCount; i++)
		{
			PVOID pvData = (PVOID)pValList[i].ve_valueptr;
			switch (pValList[i].ve_type)
			{
			case REG_BINARY:
			{
				output[i] = gcnew array<byte>(pValList[i].ve_valuelen);
				Marshal::Copy((IntPtr)pvData, (array<byte>^)output[i], 0, pValList[i].ve_valuelen);
			}
			break;

			case REG_DWORD:
			{
				DWORD* dwData = static_cast<DWORD*>(pvData);
				output[i] = gcnew Int32(*dwData);
			}
			break;

			case REG_EXPAND_SZ:
			{
				LPCWSTR strData = reinterpret_cast<LPCWSTR>((LPCWSTR)pvData);
				DWORD charCount = ExpandEnvironmentStrings(strData, NULL, 0);
				if (charCount == 0)
					throw gcnew NativeException(GetLastError());


				DWORD bytesNeeded = charCount * 2;
				wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);
				charCount = ExpandEnvironmentStrings(strData, buffer.get(), bytesNeeded);
				if (charCount == 0)
					throw gcnew NativeException(GetLastError());

				output[i] = gcnew String(buffer.get());
			}
			break;

			case REG_LINK:
				output[i] = gcnew String((LPWSTR)pvData);
				break;

			case REG_MULTI_SZ:
				output[i] = GetStringArrayFromDoubleNullTerminatedCStyleArray((LPWSTR)pvData, pValList[i].ve_valuelen);
				break;

			case REG_QWORD:
			{
				long long* qwData = static_cast<long long*>(pvData);
				output[i] = gcnew Int64(*qwData);
			}
			break;

			case REG_SZ:
				output[i] = gcnew String((LPWSTR)pvData);
				break;

			default:
				throw gcnew ArgumentException(String::Format("Invalid registry type '{0}'.", pValList[i].ve_type));
			}
		}

		return output;
	}

	array<Object^>^ Wrapper::GetRegistryValueList(String^ computerName, String^ userName, WWuString& password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		WuResult nativeResult;
		DWORD dwValCount = valueNameList->Length;
		PVALENT pValList = new VALENT[valueNameList->Length];
		WWuString* wuValueNames = new WWuString[valueNameList->Length];
		wuunique_ha_ptr<void> buffer;

		// Managing logon.
		if (!String::IsNullOrEmpty(userName))
			LogonAndImpersonateUser(userName, password);

		WWuString wuComputerName;
		if (!String::IsNullOrEmpty(computerName))
			wuComputerName = GetWideStringFromSystemString(computerName);

		WWuString wuSubKey = GetWideStringFromSystemString(subKey);

		for (DWORD i = 0; i < dwValCount; i++)
		{
			wuValueNames[i] = GetWideStringFromSystemString(valueNameList[i]);
			pValList[i].ve_valuename = wuValueNames[i].GetBuffer();
		}

		nativeResult = regptr->GetRegistryKeyValueList(wuComputerName, (HKEY)hive, wuSubKey, pValList, dwValCount, buffer);
		if (nativeResult.Result != ERROR_SUCCESS)
		{
			RevertToSelf();
			throw gcnew NativeException(nativeResult);
		}

		array<Object^>^ output = gcnew array<Object^>(dwValCount);
		for (DWORD i = 0; i < dwValCount; i++)
		{
			PVOID pvData = (PVOID)pValList[i].ve_valueptr;
			switch (pValList[i].ve_type)
			{
			case REG_BINARY:
			{
				output[i] = gcnew array<byte>(pValList[i].ve_valuelen);
				Marshal::Copy((IntPtr)pvData, (array<byte>^)output[i], 0, pValList[i].ve_valuelen);
			}
			break;

			case REG_DWORD:
			{
				DWORD* dwData = static_cast<DWORD*>(pvData);
				output[i] = gcnew Int32(*dwData);
			}
			break;

			case REG_EXPAND_SZ:
			{
				LPCWSTR strData = reinterpret_cast<LPCWSTR>((LPCWSTR)pvData);
				DWORD charCount = ExpandEnvironmentStrings(strData, NULL, 0);
				if (charCount == 0)
					throw gcnew NativeException(GetLastError());


				DWORD bytesNeeded = charCount * 2;
				wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);
				charCount = ExpandEnvironmentStrings(strData, buffer.get(), bytesNeeded);
				if (charCount == 0)
					throw gcnew NativeException(GetLastError());

				output[i] = gcnew String(buffer.get());
			}
			break;

			case REG_LINK:
				output[i] = gcnew String((LPWSTR)pvData);
				break;

			case REG_MULTI_SZ:
				output[i] = GetStringArrayFromDoubleNullTerminatedCStyleArray((LPWSTR)pvData, pValList[i].ve_valuelen);
				break;

			case REG_QWORD:
			{
				long long* qwData = static_cast<long long*>(pvData);
				output[i] = gcnew Int64(*qwData);
			}
			break;

			case REG_SZ:
				output[i] = gcnew String((LPWSTR)pvData);
				break;

			default:
				{
					RevertToSelf();
					throw gcnew ArgumentException(String::Format("Invalid registry type '{0}'.", pValList[i].ve_type));
				}
				break;
			}
		}
		
		RevertToSelf();

		return output;
	}

	// Expand-Cabinet
	void Wrapper::ExpandArchiveFile(Object^ archiveObject, String^ destination, ArchiveFileType fileType) {
		switch (fileType) {
			case WindowsUtils::Core::ArchiveFileType::Cabinet:
			{
				WuManagedCabinet^ cabinet = (WuManagedCabinet^)archiveObject;
				cabinet->ExpandCabinetFile(destination);
			} break;
		}
	}

	// Start-Tcping
	void Wrapper::StartTcpPing(String^ destination, UInt32 port, CmdletContextBase^ context)
	{
		WWuString wrappedDest = GetWideStringFromSystemString(destination);
		Network::TcpingForm form(wrappedDest, port, 10, 1, Network::IPv4, 10, false, true, false, false, true, false, NULL, false, false, false, Network::GET);
		WuResult result = ntwptr->StartTcpPinging(form, context->GetUnderlyingContext());
	}

	// Utilities
	array<String^>^ Wrapper::GetStringArrayFromDoubleNullTerminatedCStyleArray(const LPWSTR pvNativeArray, DWORD dwszBytes)
	{
		List<String^>^ stringList = gcnew List<String^>();
		LPWSTR lpszNativeArray = pvNativeArray;
		DWORD offset = 0;
		while (true)
		{
			String^ current = gcnew String(lpszNativeArray);
			stringList->Add(current);

			offset += current->Length + 1;
			DWORD remaining = dwszBytes - (offset * 2);
			if (remaining <= 4)
				break;

			lpszNativeArray += current->Length + 1;
		}

		return stringList->ToArray();
	}

	array<String^>^ Wrapper::GetStringArrayFromDoubleNullTerminatedCStyleArray(IntPtr nativeArray, DWORD dwszBytes)
	{
		List<String^>^ stringList = gcnew List<String^>();
		LPWSTR wNativeArray = (LPWSTR)nativeArray.ToPointer();
		DWORD offset = 0;
		while (true)
		{
			String^ current = gcnew String(wNativeArray);
			stringList->Add(current);

			offset += current->Length + 1;
			DWORD remaining = dwszBytes - (offset * 2);
			if (remaining <= 4)
				break;

			wNativeArray += current->Length + 1;
		}

		return stringList->ToArray();
	}

	void Wrapper::LogonAndImpersonateUser(String^ userName, SecureString^ password)
	{
		WWuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		LogonAndImpersonateUser(userName, wuPass);
	}

	// Logs on the given user and impersonates it.
	// You must call 'RevertToSelf()' to revert to the caller.
	void Wrapper::LogonAndImpersonateUser(
		String^ userName,	 // The user name. If the user belongs to a domain, enter the down-level logon name: 'DOMAIN\UserName'.
		WWuString& password	 // The user's password, if any.
	) {
		if (!String::IsNullOrEmpty(userName))
		{
			WWuString wuDomain;
			WWuString wuUserName;
			HANDLE hToken;

			if (userName->Contains(L"\\"))
			{
				wuDomain = GetWideStringFromSystemString(userName->Split('\\')[0]);
				wuUserName = GetWideStringFromSystemString(userName->Split('\\')[1]);
			}
			else
				wuUserName = GetWideStringFromSystemString(userName);

			if (!LogonUser(wuUserName.GetBuffer(), wuDomain.GetBuffer(), password.GetBuffer(), LOGON32_LOGON_NEW_CREDENTIALS, LOGON32_PROVIDER_WINNT50, &hToken))
			{
				password.SecureErase();
				throw gcnew NativeException(GetLastError());
			}

			if (!ImpersonateLoggedOnUser(hToken))
			{
				password.SecureErase();
				CloseHandle(hToken);

				throw gcnew NativeException(GetLastError());
			}

			// Zeroing the memory here instead of Marshal::ZeroFreeGlobalAllocUnicode
			// So the plain text stays less time in memory.
			password.SecureErase();

			CloseHandle(hToken);
		}
	}

	CmdletContextBase::CmdletContextBase(WriteProgressWrapper^ progWrapper, WriteWarningWrapper^ warnWrapper, WriteInformationWrapper^ infoWrapper, IntPtr mappedProgData, IntPtr mappedWarnData, IntPtr mappedInfoData)
	{
		_progressGcHandle = GCHandle::Alloc(progWrapper);
		IntPtr progressDelegatePtr = Marshal::GetFunctionPointerForDelegate(progWrapper);
		auto progressPtr = static_cast<Notification::UnmanagedWriteProgress>(progressDelegatePtr.ToPointer());

		_warningGcHandle = GCHandle::Alloc(warnWrapper);
		IntPtr warningDelegatePtr = Marshal::GetFunctionPointerForDelegate(warnWrapper);
		auto warningPtr = static_cast<Notification::UnmanagedWriteWarning>(warningDelegatePtr.ToPointer());

		_informationGcHandle = GCHandle::Alloc(infoWrapper);
		IntPtr informationDelegatePtr = Marshal::GetFunctionPointerForDelegate(infoWrapper);
		auto infoPtr = static_cast<Notification::UnmanagedWriteInformation>(informationDelegatePtr.ToPointer());

		_nativeContext = new Notification::NATIVE_CONTEXT(progressPtr, warningPtr, infoPtr, (HANDLE)mappedProgData.ToPointer(), (HANDLE)mappedWarnData.ToPointer(), (HANDLE)mappedInfoData);
	}
	CmdletContextBase::~CmdletContextBase()
	{
		_progressGcHandle.Free();
		_warningGcHandle.Free();
		delete _nativeContext;
	}
	CmdletContextBase::!CmdletContextBase()
	{
		_progressGcHandle.Free();
		_warningGcHandle.Free();
		delete _nativeContext;
	}

	WWuString GetWideStringFromSystemString(String^ string)
	{
		pin_ptr<const wchar_t> pinnedString = PtrToStringChars(string);
		return WWuString((LPWSTR)pinnedString);
	}

	Notification::PNATIVE_CONTEXT CmdletContextBase::GetUnderlyingContext()
	{
		return _nativeContext;
	}
}