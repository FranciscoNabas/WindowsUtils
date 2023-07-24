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

	/*========================================
	==		Wrapper function definition		==
	==========================================*/

	// Invoke-RemoteMessage
	array<MessageResponseBase^>^ Wrapper::SendRemoteMessage(IntPtr session, array<Int32>^ sessionid, String^ title, String^ message, UInt32 style, Int32 timeout, Boolean wait)
	{
		DWORD result = ERROR_SUCCESS;
		
		wusunique_vector<TerminalServices::WU_MESSAGE_RESPONSE> responseList = make_wusunique_vector<TerminalServices::WU_MESSAGE_RESPONSE>();
		wusunique_vector<DWORD> sessionIdList;

		WuString wuTitle = GetWuStringFromSystemString(title);
		WuString wuMessage = GetWuStringFromSystemString(message);

		if (sessionid == nullptr)
			result = wtsptr->SendMessage(wuTitle, wuMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, responseList.get(), (HANDLE)session);

		else
		{
			sessionIdList = make_wusunique_vector<DWORD>();
			for (int i = 0; i < sessionid->Length; i++)
				sessionIdList->push_back((DWORD)sessionid[i]);

			result = wtsptr->SendMessage(wuTitle, wuMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, sessionIdList.get(), responseList.get(), (HANDLE)session);
		}

		if (ERROR_SUCCESS != result)
			throw gcnew NativeExceptionBase(result);

		List<MessageResponseBase^>^ output = gcnew List<MessageResponseBase^>();
		for(TerminalServices::WU_MESSAGE_RESPONSE response : *responseList)
			if (response.Response != 0)
				output->Add(gcnew MessageResponseBase(response));

		return output->ToArray();
	}

	// Get-ComputerSession
	array<ComputerSessionBase^>^ Wrapper::GetComputerSession(String^ computerName, IntPtr session, Boolean activeOnly, Boolean includeSystemSessions)
	{
		wusunique_vector<TerminalServices::WU_COMPUTER_SESSION> sessionList = make_wusunique_vector<TerminalServices::WU_COMPUTER_SESSION>();
		DWORD result = wtsptr->GetEnumeratedSession(sessionList.get(), (HANDLE)session, activeOnly, includeSystemSessions);
		if (result != ERROR_SUCCESS)
			throw gcnew NativeExceptionBase(result);

		List<ComputerSessionBase^>^ output = gcnew List<ComputerSessionBase^>((int)sessionList->size());

		if (computerName != nullptr)
			for (TerminalServices::WU_COMPUTER_SESSION sessionInfo : *sessionList)
				output->Add(gcnew ComputerSessionBase(sessionInfo, computerName));
		else
			for (TerminalServices::WU_COMPUTER_SESSION sessionInfo : *sessionList)
				output->Add(gcnew ComputerSessionBase(sessionInfo));

		return output->ToArray();
	}

	// Disconnect-Session
	Void Wrapper::DisconnectSession(IntPtr session, UInt32^ sessionid, Boolean wait)
	{
		DWORD result = ERROR_SUCCESS;
		if (nullptr == sessionid)
			result = wtsptr->DisconnectSession((HANDLE)session, NULL, wait);
		else
		{
			result = wtsptr->DisconnectSession((HANDLE)session, (DWORD)sessionid, wait);
			if (result != ERROR_SUCCESS)
				throw gcnew NativeExceptionBase(result);
		}
	}

	// Get-FormattedError
	String^ Wrapper::GetFormattedError(Int32 errorCode)
	{
		WuString errorMessage;
		DWORD result = utlptr->GetFormattedError((DWORD)errorCode, errorMessage);
		if (result != ERROR_SUCCESS)
			throw gcnew NativeExceptionBase(result, String::Format("'GetFormattedError' failed with {0}", result));

		return gcnew String(errorMessage.GetBuffer());
	}
	// Get-LastWin32Error
	String^ Wrapper::GetLastWin32Error()
	{
		WuString errorMessage;
		DWORD result = utlptr->GetFormattedWin32Error(errorMessage);
		if (result != ERROR_SUCCESS)
			throw gcnew NativeExceptionBase(result, String::Format("'GetFormattedError' failed with {0}", result));

		return gcnew String(errorMessage.GetBuffer());
	}

	// Get-ObjectHandle
	array<ObjectHandleBase^>^ Wrapper::GetProcessObjectHandle(array<String^>^ fileName, Boolean closeHandle)
	{
		wusunique_vector<ProcessAndThread::WU_OBJECT_HANDLE> ppOutput = make_wusunique_vector<ProcessAndThread::WU_OBJECT_HANDLE>();
		wusunique_vector<WuString> reslist = make_wusunique_vector<WuString>();

		for each (String^ name in fileName)
		{
			WuString wuName = GetWuStringFromSystemString(name);
			reslist->push_back(wuName);
		}

		UINT result = patptr->GetProcessObjectHandle(ppOutput.get(), reslist.get(), closeHandle);
		if (result != ERROR_SUCCESS)
			throw gcnew NativeExceptionBase(result);

		if (ppOutput->size() == 0)
			return nullptr;

		array<ObjectHandleBase^>^ output = gcnew array<ObjectHandleBase^>((int)ppOutput->size());
		for (size_t i = 0; i < ppOutput->size(); i++)
			output[(int)i] = gcnew ObjectHandleBase(ppOutput->at(i));

		return output;
	}

	// Send-Click
	Void Wrapper::SendClick()
	{
		DWORD result = utlptr->SendClick();
		if (ERROR_SUCCESS != result)
			throw gcnew NativeExceptionBase(result);
	}

	// Get-ResourceMessageTable
	array<ResourceMessageTableCore^>^ Wrapper::GetResourceMessageTable(String^ libPath)
	{
		wusunique_vector<Utilities::WU_RESOURCE_MESSAGE_TABLE> messageTable = make_wusunique_vector<Utilities::WU_RESOURCE_MESSAGE_TABLE>();
		
		WuString wuLibPath = GetWuStringFromSystemString(libPath);
		
		DWORD result = utlptr->GetResourceMessageTable(messageTable.get(), wuLibPath);
		if (result != ERROR_SUCCESS)
		{
			if (result == ERROR_BAD_EXE_FORMAT)
			{
				PathStripPathW(wuLibPath.GetBuffer());
				String^ errormsg = gcnew String(wuLibPath.GetBuffer());
				throw gcnew NativeExceptionBase(result, errormsg + " is not a valid Win32 application.");
			}
			else
				throw gcnew NativeExceptionBase(result);
		}

		List<ResourceMessageTableCore^>^ output = gcnew List<ResourceMessageTableCore^>();
		for (Utilities::WU_RESOURCE_MESSAGE_TABLE tableEntry : *messageTable)
			output->Add(gcnew ResourceMessageTableCore(tableEntry));

		return output->ToArray();
	}

	// Get-MsiProperties
	Dictionary<String^, String^>^ Wrapper::GetMsiProperties(String^ filePath)
	{
		Dictionary<String^, String^>^ output = gcnew Dictionary<String^, String^>();
		wusunique_map<WuString, WuString> propertyMap = make_wusunique_map<WuString, WuString>();
		WuString wuFileName = GetWuStringFromSystemString(filePath);

		DWORD result = utlptr->GetMsiProperties(propertyMap.get(), wuFileName);
		if (ERROR_SUCCESS != result && ERROR_NO_MORE_ITEMS != result)
		{
			WuString msiError;
			DWORD inResult = utlptr->GetMsiExtendedError(msiError);
			if (ERROR_SUCCESS == inResult)
				throw gcnew NativeExceptionBase(result, GetFormattedError(result), gcnew SystemException(gcnew String(msiError.GetBuffer())));
			else
				throw gcnew NativeExceptionBase(result);
		}

		std::map<WuString, WuString>::iterator itr;
		for (itr = propertyMap->begin(); itr != propertyMap->end(); itr++)
			output->Add(gcnew String(itr->first.GetBuffer()), gcnew String(itr->second.GetBuffer()));

		return output;
	}

	// Remove-Service
	void Wrapper::RemoveService(String^ serviceName, String^ computerName, bool stopService, CmdletContextBase^ context)
	{
		WuString wuComputerName = GetWuStringFromSystemString(computerName);
		WuString wuServiceName = GetWuStringFromSystemString(serviceName);

		DWORD result = svcptr->RemoveService(wuServiceName, wuComputerName, stopService, context->GetUnderlyingContext());
		if (result != ERROR_SUCCESS)
			throw gcnew NativeExceptionBase(result);
	}
	
	void Wrapper::RemoveService(String^ serviceName, bool stopService, CmdletContextBase^ context)
	{
		WuString wuServiceName = GetWuStringFromSystemString(serviceName);

		DWORD result = svcptr->RemoveService(wuServiceName, L"", stopService, context->GetUnderlyingContext());
		if (result != ERROR_SUCCESS)
			throw gcnew NativeExceptionBase(result);
	}

	void Wrapper::RemoveService(IntPtr hService, String^ serviceName, String^ computerName, bool stopService, CmdletContextBase^ context)
	{
		WuString wuComputerName = GetWuStringFromSystemString(computerName);
		WuString wuServiceName = GetWuStringFromSystemString(serviceName);


		HANDLE wService = static_cast<HANDLE>(hService);
		SC_HANDLE schService = static_cast<SC_HANDLE>(wService);
		DWORD result = svcptr->RemoveService(schService, wuServiceName, wuComputerName, stopService, context->GetUnderlyingContext());

		if (result != ERROR_SUCCESS)
			throw gcnew NativeExceptionBase(result);
	}

	// Get-ServiceSecurity
	String^ Wrapper::GetServiceSecurityDescriptorString(String^ serviceName, String^ computerName, bool audit)
	{
		DWORD result = ERROR_SUCCESS;

		WuString wuServiceName = GetWuStringFromSystemString(computerName);
		WuString wuComputerName = GetWuStringFromSystemString(serviceName);

		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		if (audit)
			secInfo |= SACL_SECURITY_INFORMATION;

		PSECURITY_DESCRIPTOR svcSecurity = NULL;
		DWORD size = 0;
		if (audit)
			result = svcptr->GetServiceSecurity(wuServiceName, wuComputerName, svcSecurity, &size, TRUE);
		else
			result = svcptr->GetServiceSecurity(wuServiceName, wuComputerName, svcSecurity, &size);

		if (result != ERROR_SUCCESS)
		{
			if (svcSecurity != NULL)
				LocalFree(svcSecurity);
			
			throw gcnew NativeExceptionBase(result);
		}

		LPWSTR sddl;
		ConvertSecurityDescriptorToStringSecurityDescriptorW(svcSecurity, SDDL_REVISION_1, secInfo, &sddl, NULL);

		if (svcSecurity != NULL)
			LocalFree(svcSecurity);

		String^ manSddl = gcnew String(sddl);
		LocalFree(sddl);

		return manSddl;
	}

	String^ Wrapper::GetServiceSecurityDescriptorString(String^ serviceName, bool audit)
	{
		DWORD result = ERROR_SUCCESS;

		WuString wuServiceName = GetWuStringFromSystemString(serviceName);
		
		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		if (audit)
			secInfo |= SACL_SECURITY_INFORMATION;

		PSECURITY_DESCRIPTOR svcSecurity = NULL;
		DWORD size = 0;
		if (audit)
			result = svcptr->GetServiceSecurity(wuServiceName, L"", svcSecurity, &size, TRUE);
		else
			result = svcptr->GetServiceSecurity(wuServiceName, L"", svcSecurity, &size);

		if (result != ERROR_SUCCESS)
		{
			if (svcSecurity != NULL)
				LocalFree(svcSecurity);
			throw gcnew NativeExceptionBase(result);
		}

		LPWSTR sddl;
		ConvertSecurityDescriptorToStringSecurityDescriptorW(svcSecurity, SDDL_REVISION_1, secInfo, &sddl, NULL);
		
		if (svcSecurity != NULL)
			LocalFree(svcSecurity);

		String^ manSddl = gcnew String(sddl);
		LocalFree(sddl);

		return manSddl;
	}

	String^ Wrapper::GetServiceSecurityDescriptorString(IntPtr hService, bool audit)
	{
		DWORD result = ERROR_SUCCESS;
		HANDLE phService = static_cast<HANDLE>(hService);
		SC_HANDLE whService = static_cast<SC_HANDLE>(phService);

		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		if (audit)
			secInfo |= SACL_SECURITY_INFORMATION;

		PSECURITY_DESCRIPTOR svcSecurity = NULL;
		DWORD size = 0;
		if (audit)
			result = svcptr->GetServiceSecurity(whService, svcSecurity, &size, TRUE);
		else
			result = svcptr->GetServiceSecurity(whService, svcSecurity, &size);

		if (result != ERROR_SUCCESS)
		{
			if (svcSecurity != NULL)
				LocalFree(svcSecurity);

			throw gcnew NativeExceptionBase(result);
		}

		LPWSTR sddl;
		ConvertSecurityDescriptorToStringSecurityDescriptorW(svcSecurity, SDDL_REVISION_1, secInfo, &sddl, NULL);

		if (svcSecurity != NULL)
			LocalFree(svcSecurity);

		String^ manSddl = gcnew String(sddl);
		LocalFree(sddl);

		return manSddl;
	}

	// Set-ServiceSecurity
	void Wrapper::SetServiceSecurity(String^ serviceName, String^ computerName, String^ sddl, bool audit, bool changeOwner)
	{
		WuString wuServiceName = GetWuStringFromSystemString(serviceName);
		WuString wuComputerName = GetWuStringFromSystemString(computerName);
		WuString wuSddl = GetWuStringFromSystemString(sddl);
		
		DWORD result = svcptr->SetServiceSecurity(wuServiceName, wuComputerName, wuSddl, audit, changeOwner);
		if (result != ERROR_SUCCESS)
			throw gcnew NativeExceptionBase(result);
	}

	void Wrapper::SetServiceSecurity(String^ serviceName, String^ sddl, bool audit, bool changeOwner)
	{
		WuString wuServiceName = GetWuStringFromSystemString(serviceName);
		WuString wuSddl = GetWuStringFromSystemString(sddl);

		DWORD result = svcptr->SetServiceSecurity(wuServiceName, L"", wuSddl, audit, changeOwner);
		if (result != ERROR_SUCCESS)
			throw gcnew NativeExceptionBase(result);
	}

	// Expand-File
	void Wrapper::ExpandFile(String^ fileFullName, String^ destination, ArchiveFileType fileType, CmdletContextBase^ context)
	{
		LPSTR wFileName = static_cast<LPSTR>(Marshal::StringToHGlobalAnsi(Path::GetFileName(fileFullName)).ToPointer());
		LPSTR wFilePath = static_cast<LPSTR>(Marshal::StringToHGlobalAnsi(Path::GetDirectoryName(fileFullName)).ToPointer());
		LPSTR wDestination = static_cast<LPSTR>(Marshal::StringToHGlobalAnsi(destination).ToPointer());

		DWORD result = ctnptr->ExpandArchiveFile(wFileName, wFilePath, wDestination, (Containers::ARCHIVE_FILE_TYPE)fileType, context->GetUnderlyingContext());
		if (result != ERROR_SUCCESS)
			throw FDIErrorToException((FDIERROR)result);
	}

	// Registry operations
	Object^ Wrapper::GetRegistryValue(RegistryHive hive, String^ subKey, String^ valueName)
	{
		WuString wuPass = L"";
		return GetRegistryValue(L"", L"", wuPass, hive, subKey, valueName);
	}

	Object^ Wrapper::GetRegistryValue(String^ computerName, RegistryHive hive, String^ subKey, String^ valueName)
	{
		WuString wuPass = L"";
		return GetRegistryValue(computerName, L"", wuPass, hive, subKey, valueName);
	}

	Object^ Wrapper::GetRegistryValue(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, String^ valueName)
	{
		WuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistryValue(L"", userName, wuPass, hive, subKey, valueName);
	}

	Object^ Wrapper::GetRegistryValue(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, String^ valueName)
	{
		WuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistryValue(computerName, userName, wuPass, hive, subKey, valueName);
	}

	Object^ Wrapper::GetRegistryValue(IntPtr hRegistry, String^ subKey, String^ valueName)
	{
		DWORD dwValueType = 0;
		DWORD dwBytesReturned = 0;
		wuunique_ha_ptr<void> data;

		WuString wuSubKey = GetWuStringFromSystemString(subKey);
		WuString wuValueName = GetWuStringFromSystemString(valueName);

		HKEY whReg = (HKEY)hRegistry.ToPointer();
		LSTATUS result = regptr->GetRegistryKeyValue(whReg, wuSubKey, wuValueName, dwValueType, data, dwBytesReturned);
		if (result != ERROR_SUCCESS)
			throw gcnew NativeExceptionBase(result);

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
			DWORD bytesNeeded = ExpandEnvironmentStrings(strData, NULL, 0);
			if (bytesNeeded == 0)
				throw gcnew NativeExceptionBase(GetLastError());

			WuString buffer(bytesNeeded);
			bytesNeeded = ExpandEnvironmentStrings(strData, buffer.GetBuffer(), bytesNeeded);
			if (bytesNeeded == 0)
				throw gcnew NativeExceptionBase(GetLastError());

			output = gcnew String(buffer.GetBuffer());
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
		WuString& password,		  // User password.
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
		
		WuString wuComputerName;
		if (!String::IsNullOrEmpty(computerName))
			wuComputerName = GetWuStringFromSystemString(computerName);

		WuString wuSubKey = GetWuStringFromSystemString(subKey);
		WuString wuValueName = GetWuStringFromSystemString(valueName);

		LSTATUS result = regptr->GetRegistryKeyValue(wuComputerName, (HKEY)hive, wuSubKey, wuValueName, dwValueType, data, dwBytesReturned);
		if (result != ERROR_SUCCESS)
		{
			RevertToSelf();
			throw gcnew NativeExceptionBase(result);
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
			DWORD bytesNeeded = ExpandEnvironmentStrings(strData, NULL, 0);
			if (bytesNeeded == 0)
				throw gcnew NativeExceptionBase(GetLastError());

			WuString buffer(bytesNeeded);
			bytesNeeded = ExpandEnvironmentStrings(strData, buffer.GetBuffer(), bytesNeeded);
			if (bytesNeeded == 0)
				throw gcnew NativeExceptionBase(GetLastError());

			output = gcnew String(buffer.GetBuffer());
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
		WuString wuPass = L"";
		return GetRegistrySubKeyNames(L"", L"", wuPass, hive, subKey);
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(String^ computerName, RegistryHive hive, String^ subKey)
	{
		WuString wuPass = L"";
		return GetRegistrySubKeyNames(computerName, L"", wuPass, hive, subKey);
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey)
	{
		WuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistrySubKeyNames(L"", userName, wuPass, hive, subKey);
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey)
	{
		WuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistrySubKeyNames(computerName, userName, wuPass, hive, subKey);
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(IntPtr hRegistry, String^ subKey)
	{
		LSTATUS result = ERROR_SUCCESS;
		wusunique_vector<WuString> subkeyNameVec = make_wusunique_vector<WuString>();

		WuString wuSubKey = GetWuStringFromSystemString(subKey);

		HKEY whReg = (HKEY)hRegistry.ToPointer();
		result = regptr->GetRegistrySubkeyNames(whReg, wuSubKey, 0, subkeyNameVec.get());
		if (result != ERROR_SUCCESS)
			throw gcnew NativeExceptionBase(result);

		List<String^>^ output = gcnew List<String^>();
		for (WuString singleName : *subkeyNameVec)
			output->Add(gcnew String(singleName.GetBuffer()));

		RevertToSelf();

		return output->ToArray();
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(String^ computerName, String^ userName, WuString& password, RegistryHive hive, String^ subKey)
	{
		LSTATUS result = ERROR_SUCCESS;
		wusunique_vector<WuString> subkeyNameVec = make_wusunique_vector<WuString>();

		// Managing logon.
		if (!String::IsNullOrEmpty(userName))
			LogonAndImpersonateUser(userName, password);

		WuString wuComputerName;
		if (!String::IsNullOrEmpty(computerName))
			wuComputerName = GetWuStringFromSystemString(computerName);

		WuString wuSubKey = GetWuStringFromSystemString(subKey);

		result = regptr->GetRegistrySubkeyNames(wuComputerName, (HKEY)hive, wuSubKey, 0, subkeyNameVec.get());
		if (result != ERROR_SUCCESS)
		{
			RevertToSelf();
			throw gcnew NativeExceptionBase(result);
		}

		List<String^>^ output = gcnew List<String^>();
		for (WuString singleName : *subkeyNameVec)
			output->Add(gcnew String(singleName.GetBuffer()));

		RevertToSelf();

		return output->ToArray();
	}

	array<Object^>^ Wrapper::GetRegistryValueList(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		WuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return Wrapper::GetRegistryValueList(L"", userName, wuPass, hive, subKey, valueNameList);
	}

	array<Object^>^ Wrapper::GetRegistryValueList(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		WuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return Wrapper::GetRegistryValueList(computerName, userName, wuPass, hive, subKey, valueNameList);
	}

	array<Object^>^ Wrapper::GetRegistryValueList(String^ computerName, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		WuString wuPass = L"";
		return Wrapper::GetRegistryValueList(computerName, L"", wuPass, hive, subKey, valueNameList);
	}

	array<Object^>^ Wrapper::GetRegistryValueList(RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		WuString wuPass = L"";
		return Wrapper::GetRegistryValueList(L"", L"", wuPass, hive, subKey, valueNameList);
	}

	array<Object^>^ Wrapper::GetRegistryValueList(IntPtr hRegistry, String^ subKey, array<String^>^ valueNameList)
	{
		LSTATUS nativeResult;
		DWORD dwValCount = valueNameList->Length;
		PVALENT pValList = new VALENT[valueNameList->Length];
		WuString* wuValueNames = new WuString[valueNameList->Length];
		
		wuunique_ha_ptr<void> buffer;
		WuString wuSubKey = GetWuStringFromSystemString(subKey);

		for (DWORD i = 0; i < dwValCount; i++)
		{
			wuValueNames[i] = GetWuStringFromSystemString(valueNameList[i]);
			pValList[i].ve_valuename = wuValueNames[i].GetBuffer();
		}
		
		HKEY whReg = (HKEY)hRegistry.ToPointer();
		nativeResult = regptr->GetRegistryKeyValueList(whReg, wuSubKey, pValList, dwValCount, buffer);
		if (nativeResult != ERROR_SUCCESS)
			throw gcnew NativeExceptionBase(nativeResult);

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
				// Getting the necessary buffer.
				DWORD bytesNeeded = ExpandEnvironmentStrings((LPCWSTR)pvData, NULL, 0);
				if (bytesNeeded == 0)
					throw gcnew NativeExceptionBase(GetLastError());

				WuString expBuffer(bytesNeeded);
				bytesNeeded = ExpandEnvironmentStrings((LPCWSTR)pvData, expBuffer.GetBuffer(), bytesNeeded);
				if (bytesNeeded == 0)
					throw gcnew NativeExceptionBase(GetLastError());

				output[i] = gcnew String(expBuffer.GetBuffer());
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

	array<Object^>^ Wrapper::GetRegistryValueList(String^ computerName, String^ userName, WuString& password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		LSTATUS nativeResult;
		DWORD dwValCount = valueNameList->Length;
		PVALENT pValList = new VALENT[valueNameList->Length];
		WuString* wuValueNames = new WuString[valueNameList->Length];
		wuunique_ha_ptr<void> buffer;

		// Managing logon.
		if (!String::IsNullOrEmpty(userName))
			LogonAndImpersonateUser(userName, password);

		WuString wuComputerName;
		if (!String::IsNullOrEmpty(computerName))
			wuComputerName = GetWuStringFromSystemString(computerName);

		WuString wuSubKey = GetWuStringFromSystemString(subKey);

		for (DWORD i = 0; i < dwValCount; i++)
		{
			wuValueNames[i] = GetWuStringFromSystemString(valueNameList[i]);
			pValList[i].ve_valuename = wuValueNames[i].GetBuffer();
		}

		nativeResult = regptr->GetRegistryKeyValueList(wuComputerName, (HKEY)hive, wuSubKey, pValList, dwValCount, buffer);
		if (nativeResult != ERROR_SUCCESS)
		{
			RevertToSelf();
			throw gcnew NativeExceptionBase(nativeResult);
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
				// Getting the necessary buffer.
				DWORD bytesNeeded = ExpandEnvironmentStrings((LPCWSTR)pvData, NULL, 0);
				if (bytesNeeded == 0)
				{
					RevertToSelf();
					throw gcnew NativeExceptionBase(GetLastError());
				}

				WuString expBuffer(bytesNeeded);
				bytesNeeded = ExpandEnvironmentStrings((LPCWSTR)pvData, expBuffer.GetBuffer(), bytesNeeded);
				if (bytesNeeded == 0)
				{
					RevertToSelf();
					throw gcnew NativeExceptionBase(GetLastError());
				}

				output[i] = gcnew String(expBuffer.GetBuffer());
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
		WuString wuPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		LogonAndImpersonateUser(userName, wuPass);
	}

	// Logs on the given user and impersonates it.
	// You must call 'RevertToSelf()' to revert to the caller.
	void Wrapper::LogonAndImpersonateUser(
		String^ userName,	 // The user name. If the user belongs to a domain, enter the down-level logon name: 'DOMAIN\UserName'.
		WuString& password	 // The user's password, if any.
	) {
		if (!String::IsNullOrEmpty(userName))
		{
			WuString wuDomain;
			WuString wuUserName;
			HANDLE hToken;

			if (userName->Contains(L"\\"))
			{
				wuDomain = GetWuStringFromSystemString(userName->Split('\\')[0]);
				wuUserName = GetWuStringFromSystemString(userName->Split('\\')[1]);
			}
			else
				wuUserName = GetWuStringFromSystemString(userName);

			if (!LogonUser(wuUserName.GetBuffer(), wuDomain.GetBuffer(), password.GetBuffer(), LOGON32_LOGON_NEW_CREDENTIALS, LOGON32_PROVIDER_WINNT50, &hToken))
			{
				password.SecureErase();
				throw gcnew NativeExceptionBase(GetLastError());
			}

			if (!ImpersonateLoggedOnUser(hToken))
			{
				password.SecureErase();
				CloseHandle(hToken);

				throw gcnew NativeExceptionBase(GetLastError());
			}

			// Zeroing the memory here instead of Marshal::ZeroFreeGlobalAllocUnicode
			// So the plain text stays less time in memory.
			password.SecureErase();

			CloseHandle(hToken);
		}
	}

	CmdletContextBase::CmdletContextBase(WriteProgressWrapper^ progWrapper, WriteWarningWrapper^ warnWrapper, IntPtr mappedProgFile, IntPtr mappedWarnFile)
	{
		_progressGcHandle = GCHandle::Alloc(progWrapper);
		IntPtr progressDelegatePtr = Marshal::GetFunctionPointerForDelegate(progWrapper);
		auto progressPtr = static_cast<Notification::UnmanagedWriteProgress>(progressDelegatePtr.ToPointer());

		_warningGcHandle = GCHandle::Alloc(warnWrapper);
		IntPtr WarningDelegatePtr = Marshal::GetFunctionPointerForDelegate(warnWrapper);
		auto warningPtr = static_cast<Notification::UnmanagedWriteWarning>(WarningDelegatePtr.ToPointer());

		_nativeContext = new Notification::NATIVE_CONTEXT(progressPtr, warningPtr, (HANDLE)mappedProgFile.ToPointer(), (HANDLE)mappedWarnFile.ToPointer());
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

	WuString GetWuStringFromSystemString(String^ string)
	{
		pin_ptr<const wchar_t> pinnedString = PtrToStringChars(string);
		return WuString((LPWSTR)pinnedString);
	}

	Notification::PNATIVE_CONTEXT CmdletContextBase::GetUnderlyingContext()
	{
		return _nativeContext;
	}

	Exception^ FDIErrorToException(FDIERROR err)
	{
		switch (err)
		{
		case FDIERROR_CABINET_NOT_FOUND:
			return gcnew FileNotFoundException("Cabinet not found");

		case FDIERROR_NOT_A_CABINET:
			return gcnew ArgumentException("File is not a cabinet");

		case FDIERROR_UNKNOWN_CABINET_VERSION:
			return gcnew ArgumentException("Unknown cabinet version");

		case FDIERROR_CORRUPT_CABINET:
			return gcnew ArgumentException("Corrupt cabinet");

		case FDIERROR_ALLOC_FAIL:
			return gcnew OutOfMemoryException("Memory allocation failed");

		case FDIERROR_BAD_COMPR_TYPE:
			return gcnew ArgumentException("Unknown compression type");

		case FDIERROR_MDI_FAIL:
			return gcnew ApplicationException("Failure decompressing data");

		case FDIERROR_TARGET_FILE:
			return gcnew ApplicationException("Failure writing to target file");

		case FDIERROR_RESERVE_MISMATCH:
			return gcnew ArgumentException("Cabinets in set have different RESERVE sizes");

		case FDIERROR_WRONG_CABINET:
			return gcnew InvalidOperationException("Cabinet returned on fdintNEXT_CABINET is incorrect");

		case FDIERROR_USER_ABORT:
			return gcnew OperationCanceledException("Application aborted");

		default:
			return gcnew SystemException("Unknown error");
		}
	}
}