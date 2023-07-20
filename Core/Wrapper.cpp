#include "pch.h"
#include "Wrapper.h"

namespace WindowsUtils::Core
{
	/*========================================
	==	    Managed object definition		==
	==========================================*/

	//Invoke-RemoteMessage
	MessageResponseBase::MessageResponseBase() : wrapper(new TerminalServices::WU_MESSAGE_RESPONSE) { }
	MessageResponseBase::MessageResponseBase(TerminalServices::WU_MESSAGE_RESPONSE exmessres)
	{
		wrapper = new TerminalServices::WU_MESSAGE_RESPONSE(
			exmessres.SessionId
			, exmessres.Response
		);
	}
	MessageResponseBase::~MessageResponseBase() { delete wrapper; }
	MessageResponseBase::!MessageResponseBase() { delete wrapper; }

	//Get-ComputerSession
	ComputerSessionBase::ComputerSessionBase() : wrapper(new TerminalServices::WU_COMPUTER_SESSION) { }
	ComputerSessionBase::ComputerSessionBase(TerminalServices::WU_COMPUTER_SESSION excompsess)
	{
		wrapper = new TerminalServices::WU_COMPUTER_SESSION(
			excompsess.SessionId
			, excompsess.UserName
			, excompsess.SessionName
			, excompsess.LastInputTime
			, excompsess.LogonTime
			, excompsess.SessionState
		);
	}
	ComputerSessionBase::ComputerSessionBase(TerminalServices::WU_COMPUTER_SESSION excompsess, String^ inppcname)
	{
		_computername = inppcname;
		wrapper = new TerminalServices::WU_COMPUTER_SESSION(
			excompsess.SessionId
			, excompsess.UserName
			, excompsess.SessionName
			, excompsess.LastInputTime
			, excompsess.LogonTime
			, excompsess.SessionState
		);
	}
	ComputerSessionBase::~ComputerSessionBase() { delete wrapper; }
	ComputerSessionBase::!ComputerSessionBase() { delete wrapper; }

	// Get-ObjectHandle
	ObjectHandleBase::ObjectHandleBase() : wrapper(new ProcessAndThread::WU_OBJECT_HANDLE) { }
	ObjectHandleBase::ObjectHandleBase(ProcessAndThread::WU_OBJECT_HANDLE objhandle)
	{
		wrapper = new ProcessAndThread::WU_OBJECT_HANDLE;
		wrapper->ImagePath = objhandle.ImagePath;
		wrapper->Name = objhandle.Name;
		wrapper->InputObject = objhandle.InputObject;
		wrapper->ProcessId = objhandle.ProcessId;
		wrapper->VersionInfo = objhandle.VersionInfo;
	}
	ObjectHandleBase::~ObjectHandleBase() { delete wrapper; }
	ObjectHandleBase::!ObjectHandleBase() { delete wrapper; }

	// Get-ResourceMessageTable
	ResourceMessageTableCore::ResourceMessageTableCore() :wrapper(new Utilities::WU_RESOURCE_MESSAGE_TABLE) { }
	ResourceMessageTableCore::ResourceMessageTableCore(Utilities::WU_RESOURCE_MESSAGE_TABLE resmes) { wrapper = new Utilities::WU_RESOURCE_MESSAGE_TABLE(resmes.Id, resmes.Message); }
	ResourceMessageTableCore::~ResourceMessageTableCore() { delete wrapper; }
	ResourceMessageTableCore::!ResourceMessageTableCore() { delete wrapper; }

	/*========================================
	==		Wrapper function definition		==
	==========================================*/

	// Invoke-RemoteMessage
	array<MessageResponseBase^>^ Wrapper::SendRemoteMessage(IntPtr session, array<Int32>^ sessionid, String^ title, String^ message, UInt32 style, Int32 timeout, Boolean wait)
	{
		DWORD dwresult = ERROR_SUCCESS;
		SharedVecPtr(TerminalServices::WU_MESSAGE_RESPONSE) presult = MakeVecPtr(TerminalServices::WU_MESSAGE_RESPONSE);
		SharedVecPtr(DWORD) psessid;

		pin_ptr<const wchar_t> wTitle = PtrToStringChars(title);
		pin_ptr<const wchar_t> wMessage = PtrToStringChars(message);

		if (sessionid == nullptr)
			dwresult = wtsptr->SendMessage((LPWSTR)wTitle, (LPWSTR)wMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, *presult, (HANDLE)session);

		else
		{
			psessid = MakeVecPtr(DWORD);
			for (int i = 0; i < sessionid->Length; i++)
				psessid->push_back((DWORD)sessionid[i]);

			dwresult = wtsptr->SendMessage((LPWSTR)wTitle, (LPWSTR)wMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, *psessid, *presult, (HANDLE)session);
		}

		if (ERROR_SUCCESS != dwresult)
		{
			wTitle = nullptr;
			wMessage = nullptr;
			throw gcnew NativeExceptionBase(dwresult);
		}

		array<MessageResponseBase^>^ output = gcnew array<MessageResponseBase^>((int)presult->size());
		
		for (size_t i = 0; i < presult->size(); i++)
			if (presult->at(i).Response != 0)
				output[(int)i] = gcnew MessageResponseBase(presult->at(i));

		wTitle = nullptr;
		wMessage = nullptr;

		return output;
	}

	// Get-ComputerSession
	array<ComputerSessionBase^>^ Wrapper::GetComputerSession(String^ computername, IntPtr session, Boolean onlyactive, Boolean includesystemsession)
	{
		SharedVecPtr(TerminalServices::WU_COMPUTER_SESSION) result = MakeVecPtr(TerminalServices::WU_COMPUTER_SESSION);
		DWORD opresult = wtsptr->GetEnumeratedSession(*result, (HANDLE)session, onlyactive, includesystemsession);
		if (opresult != ERROR_SUCCESS)
			throw gcnew NativeExceptionBase(opresult);

		array<ComputerSessionBase^>^ output = gcnew array<ComputerSessionBase^>((int)result->size());

		if (nullptr != computername)
			for (size_t i = 0; i < result->size(); i++)
				output[(int)i] = gcnew ComputerSessionBase(result->at(i), computername);
		else
			for (size_t i = 0; i < result->size(); i++)
				output[(int)i] = gcnew ComputerSessionBase(result->at(i));

		return output;
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
	String^ Wrapper::GetFormattedError(Int32 errorcode)
	{
		std::shared_ptr<LPWSTR> errormess = std::make_shared<LPWSTR>();
		DWORD result = utlptr->GetFormattedError((DWORD)errorcode, *errormess);
		if (ERROR_SUCCESS != result)
			throw gcnew NativeExceptionBase(result, String::Format("'GetFormattedError' failed with {0}", result));

		return gcnew String(*errormess);
	}
	// Get-LastWin32Error
	String^ Wrapper::GetLastWin32Error()
	{
		std::shared_ptr<LPWSTR> errormess = std::make_shared<LPWSTR>();
		DWORD result = utlptr->GetFormattedWin32Error(*errormess);
		if (ERROR_SUCCESS != result)
			throw gcnew NativeExceptionBase(result);

		return gcnew String(*errormess);
	}

	// Get-ObjectHandle
	array<ObjectHandleBase^>^ Wrapper::GetProcessObjectHandle(array<String^>^ fileName, Boolean closeHandle)
	{
		SharedVecPtr(ProcessAndThread::WU_OBJECT_HANDLE) ppOutput = MakeVecPtr(ProcessAndThread::WU_OBJECT_HANDLE);
		SharedVecPtr(LPCWSTR) reslist = MakeVecPtr(LPCWSTR);

		for (int i = 0; i < fileName->Length; i++)
		{
			pin_ptr<const WCHAR> single = PtrToStringChars(fileName[i]);
			reslist->push_back((LPCWSTR)single);
			single = nullptr;
		}

		UINT result = patptr->GetProcessObjectHandle(*ppOutput, *reslist, closeHandle);
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
	array<ResourceMessageTableCore^>^ Wrapper::GetResourceMessageTable(String^ libpath)
	{
		SharedVecPtr(Utilities::WU_RESOURCE_MESSAGE_TABLE) ppresult = MakeVecPtr(Utilities::WU_RESOURCE_MESSAGE_TABLE);
		pin_ptr<const wchar_t> pplibpath = PtrToStringChars(libpath);

		LPWSTR wlibpath = (LPWSTR)pplibpath;
		DWORD result = utlptr->GetResourceMessageTable(*ppresult, wlibpath);
		if (result != ERROR_SUCCESS)
		{
			if (result == ERROR_BAD_EXE_FORMAT)
			{
				PathStripPathW((LPWSTR)wlibpath);
				String^ errormsg = gcnew String(wlibpath);
				wlibpath = nullptr;
				throw gcnew NativeExceptionBase(result, errormsg + " is not a valid Win32 application.");
			}
			else
			{
				wlibpath = nullptr;
				throw gcnew NativeExceptionBase(result);
			}
		}

		array<ResourceMessageTableCore^>^ output = gcnew array<ResourceMessageTableCore^>((int)ppresult->size());
		for (size_t i = 0; i < ppresult->size(); i++)
			output[(int)i] = gcnew ResourceMessageTableCore(ppresult->at(i));

		wlibpath = nullptr;
		return output;
	}

	// Get-MsiProperties
	Dictionary<String^, String^>^ Wrapper::GetMsiProperties(String^ filepath)
	{
		Dictionary<String^, String^>^ output = gcnew Dictionary<String^, String^>();
		auto ppresult = std::make_shared<std::map<LPWSTR, LPWSTR>>();
		pin_ptr<const wchar_t> wfilename = PtrToStringChars(filepath);

		LPWSTR lpfilename = (LPWSTR)wfilename;
		DWORD result = utlptr->GetMsiProperties(*ppresult, lpfilename);
		if (ERROR_SUCCESS != result && ERROR_NO_MORE_ITEMS != result)
		{
			wfilename = nullptr;
			LPWSTR pextmsierr;
			DWORD inResu = utlptr->GetMsiExtendedError(pextmsierr);
			if (ERROR_SUCCESS == inResu)
				throw gcnew NativeExceptionBase(result, GetFormattedError(result), gcnew SystemException(gcnew String(pextmsierr)));
			else
				throw gcnew NativeExceptionBase(result);
		}

		std::map<LPWSTR, LPWSTR>::iterator itr;
		for (itr = ppresult->begin(); itr != ppresult->end(); itr++)
			output->Add(gcnew String(itr->first), gcnew String(itr->second));

		return output;
	}

	// Remove-Service
	void Wrapper::RemoveService(String^ servicename, String^ computername, bool stopservice, CmdletContextBase^ context)
	{
		pin_ptr<const wchar_t> wcomputername = PtrToStringChars(computername);
		pin_ptr<const wchar_t> wservicename = PtrToStringChars(servicename);

		DWORD result = svcptr->RemoveService((LPWSTR)wservicename, (LPWSTR)wcomputername, stopservice, context->GetUnderlyingContext());
		if (result != ERROR_SUCCESS)
		{
			wcomputername = nullptr;
			wservicename = nullptr;
			throw gcnew NativeExceptionBase(result);
		}
		
		wcomputername = nullptr;
		wservicename = nullptr;
	}
	
	void Wrapper::RemoveService(String^ servicename, bool stopservice, CmdletContextBase^ context)
	{
		pin_ptr<const wchar_t> wservicename = PtrToStringChars(servicename);

		DWORD result = svcptr->RemoveService((LPWSTR)wservicename, NULL, stopservice, context->GetUnderlyingContext());
		if (result != ERROR_SUCCESS)
		{
			wservicename = nullptr;
			throw gcnew NativeExceptionBase(result);
		}

		wservicename = nullptr;
	}

	void Wrapper::RemoveService(IntPtr hservice, String^ servicename, String^ computername, bool stopservice, CmdletContextBase^ context)
	{
		pin_ptr<const wchar_t> wcomputername = PtrToStringChars(computername);
		pin_ptr<const wchar_t> wservicename = PtrToStringChars(servicename);

		HANDLE uhservice = static_cast<HANDLE>(hservice);
		SC_HANDLE uschservice = static_cast<SC_HANDLE>(uhservice);
		DWORD result = svcptr->RemoveService(uschservice, (LPWSTR)wservicename, (LPWSTR)wcomputername, stopservice, context->GetUnderlyingContext());

		if (result != ERROR_SUCCESS)
		{
			wcomputername = nullptr;
			throw gcnew NativeExceptionBase(result);
		}

		wcomputername = nullptr;
	}

	// Get-ServiceSecurity
	String^ Wrapper::GetServiceSecurityDescriptorString(String^ serviceName, String^ computerName, bool audit)
	{
		DWORD result = ERROR_SUCCESS;
		pin_ptr<const wchar_t> wServiceName = PtrToStringChars(serviceName);
		pin_ptr<const wchar_t> wComputerName = PtrToStringChars(computerName);

		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		if (audit)
			secInfo |= SACL_SECURITY_INFORMATION;

		PSECURITY_DESCRIPTOR svcSecurity;
		DWORD size = 0;
		if (audit)
			result = svcptr->GetServiceSecurity((LPWSTR)wServiceName, (LPWSTR)wComputerName, svcSecurity, &size, TRUE);
		else
			result = svcptr->GetServiceSecurity((LPWSTR)wServiceName, (LPWSTR)wComputerName, svcSecurity, &size);

		if (result != ERROR_SUCCESS)
		{
			wServiceName = nullptr;
			wComputerName = nullptr;
			throw gcnew NativeExceptionBase(result);
		}

		LPWSTR sddl;
		ConvertSecurityDescriptorToStringSecurityDescriptorW(svcSecurity, SDDL_REVISION_1, secInfo, &sddl, NULL);

		wServiceName = nullptr;
		wComputerName = nullptr;
		return gcnew String(sddl);
	}

	String^ Wrapper::GetServiceSecurityDescriptorString(String^ serviceName, bool audit)
	{
		DWORD result = ERROR_SUCCESS;
		pin_ptr<const wchar_t> wServiceName = PtrToStringChars(serviceName);
		
		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		if (audit)
			secInfo |= SACL_SECURITY_INFORMATION;

		PSECURITY_DESCRIPTOR svcSecurity;
		DWORD size = 0;
		if (audit)
			result = svcptr->GetServiceSecurity((LPWSTR)wServiceName, L".", svcSecurity, &size, TRUE);
		else
			result = svcptr->GetServiceSecurity((LPWSTR)wServiceName, L".", svcSecurity, &size);

		if (result != ERROR_SUCCESS)
		{
			wServiceName = nullptr;
			throw gcnew NativeExceptionBase(result);
		}

		LPWSTR sddl;
		ConvertSecurityDescriptorToStringSecurityDescriptorW(svcSecurity, SDDL_REVISION_1, secInfo, &sddl, NULL);
		
		wServiceName = nullptr;
		return gcnew String(sddl);
	}

	String^ Wrapper::GetServiceSecurityDescriptorString(IntPtr hService, bool audit)
	{
		DWORD result = ERROR_SUCCESS;
		HANDLE phService = static_cast<HANDLE>(hService);
		SC_HANDLE whService = static_cast<SC_HANDLE>(phService);

		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		if (audit)
			secInfo |= SACL_SECURITY_INFORMATION;

		PSECURITY_DESCRIPTOR svcSecurity;
		DWORD size = 0;
		if (audit)
			result = svcptr->GetServiceSecurity(whService, svcSecurity, &size, TRUE);
		else
			result = svcptr->GetServiceSecurity(whService, svcSecurity, &size);

		if (result != ERROR_SUCCESS)
			throw gcnew NativeExceptionBase(result);

		LPWSTR sddl;
		ConvertSecurityDescriptorToStringSecurityDescriptorW(svcSecurity, SDDL_REVISION_1, secInfo, &sddl, NULL);

		return gcnew String(sddl);
	}

	// Set-ServiceSecurity
	void Wrapper::SetServiceSecurity(String^ serviceName, String^ computerName, String^ sddl, bool audit, bool changeOwner)
	{
		pin_ptr<const wchar_t> wServiceName = PtrToStringChars(serviceName);
		pin_ptr<const wchar_t> wComputerName = PtrToStringChars(computerName);
		pin_ptr<const wchar_t> wSddl = PtrToStringChars(sddl);

		DWORD result = svcptr->SetServiceSecurity((LPWSTR)wServiceName, (LPWSTR)wComputerName, (LPWSTR)wSddl, audit, changeOwner);
		if (result != ERROR_SUCCESS)
		{
			wServiceName = nullptr;
			wComputerName = nullptr;
			wSddl = nullptr;
			throw gcnew NativeExceptionBase(result);
		}

		wServiceName = nullptr;
		wComputerName = nullptr;
		wSddl = nullptr;
	}

	void Wrapper::SetServiceSecurity(String^ serviceName, String^ sddl, bool audit, bool changeOwner)
	{
		pin_ptr<const wchar_t> wServiceName = PtrToStringChars(serviceName);
		pin_ptr<const wchar_t> wSddl = PtrToStringChars(sddl);

		DWORD result = svcptr->SetServiceSecurity((LPWSTR)wServiceName, L".", (LPWSTR)wSddl, audit, changeOwner);
		if (result != ERROR_SUCCESS)
		{
			wServiceName = nullptr;
			wSddl = nullptr;
			throw gcnew NativeExceptionBase(result);
		}

		wServiceName = nullptr;
		wSddl = nullptr;
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
		return GetRegistryValue(L"", L"", L"", hive, subKey, valueName);
	}

	Object^ Wrapper::GetRegistryValue(String^ computerName, RegistryHive hive, String^ subKey, String^ valueName)
	{
		return GetRegistryValue(computerName, L"", L"", hive, subKey, valueName);
	}

	Object^ Wrapper::GetRegistryValue(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, String^ valueName)
	{
		LPWSTR wPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistryValue(L"", userName, wPass, hive, subKey, valueName);
	}

	Object^ Wrapper::GetRegistryValue(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, String^ valueName)
	{
		LPWSTR wPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistryValue(computerName, userName, wPass, hive, subKey, valueName);
	}

	Object^ Wrapper::GetRegistryValue(IntPtr hRegistry, String^ subKey, String^ valueName)
	{
		DWORD dwValueType;
		DWORD dwBytesReturned;
		PVOID pvData;

		WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

		pin_ptr<const wchar_t> wSubKey = PtrToStringChars(subKey);
		pin_ptr<const wchar_t> wValueName = PtrToStringChars(valueName);

		HKEY whReg = (HKEY)hRegistry.ToPointer();
		LSTATUS result = regptr->GetRegistryKeyValue(whReg, (LPWSTR)wSubKey, (LPWSTR)wValueName, dwValueType, pvData, dwBytesReturned);
		if (result != ERROR_SUCCESS)
		{
			wSubKey = nullptr;
			wValueName = nullptr;

			throw gcnew NativeExceptionBase(result);
		}

		wSubKey = nullptr;
		wValueName = nullptr;

		Object^ output;
		switch (dwValueType)
		{
		case REG_BINARY:
		{
			output = gcnew array<byte>(dwBytesReturned);
			Marshal::Copy((IntPtr)pvData, (array<byte>^)output, 0, dwBytesReturned);
		}
		break;

		case REG_DWORD:
		{
			DWORD* dwData = static_cast<DWORD*>(pvData);
			output = gcnew Int32(*dwData);
		}
		break;

		case REG_EXPAND_SZ:
		{
			// Getting the necessary buffer.
			DWORD bytesNeeded = ExpandEnvironmentStrings((LPCWSTR)pvData, NULL, 0);
			if (bytesNeeded == 0)
				throw gcnew NativeExceptionBase(GetLastError());

			LPWSTR buffer = (LPWSTR)MemoryManager.Allocate(bytesNeeded);
			bytesNeeded = ExpandEnvironmentStrings((LPCWSTR)pvData, buffer, bytesNeeded);
			if (bytesNeeded == 0)
				throw gcnew NativeExceptionBase(GetLastError());

			output = gcnew String(buffer);
			MemoryManager.Free(buffer);
		}
		break;

		case REG_LINK:
			output = gcnew String((LPWSTR)pvData);
			break;

		case REG_MULTI_SZ:
			output = GetStringArrayFromDoubleNullTermninatedCStyleArray((LPWSTR)pvData, dwBytesReturned);
			break;

		case REG_QWORD:
		{
			long long* qwData = static_cast<long long*>(pvData);
			output = gcnew Int64(*qwData);
		}
		break;

		case REG_SZ:
			output = gcnew String((LPWSTR)pvData);
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
		const LPWSTR& password,	  // User password.
		RegistryHive hive,		  // The root hive.
		String^ subKey,			  // The subkey path.
		String^ valueName		  // The value property name.
	) {
		DWORD dwValueType;
		DWORD dwBytesReturned;
		PVOID pvData;

		WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();
		pin_ptr<const wchar_t> temp;

		// Managing logon.
		if (!String::IsNullOrEmpty(userName))
			LogonAndImpersonateUser(userName, password);

		LPWSTR wComputerName = NULL;
		if (!String::IsNullOrEmpty(computerName))
		{
			temp = PtrToStringChars(computerName);
			wComputerName = (LPWSTR)temp;
		}

		pin_ptr<const wchar_t> wSubKey = PtrToStringChars(subKey);
		pin_ptr<const wchar_t> wValueName = PtrToStringChars(valueName);

		LSTATUS result = regptr->GetRegistryKeyValue(wComputerName, (HKEY)hive, (LPWSTR)wSubKey, (LPWSTR)wValueName, dwValueType, pvData, dwBytesReturned);
		if (result != ERROR_SUCCESS)
		{
			RevertToSelf();

			temp = nullptr;
			wSubKey = nullptr;
			wValueName = nullptr;

			throw gcnew NativeExceptionBase(result);
		}

		temp = nullptr;
		wSubKey = nullptr;
		wValueName = nullptr;

		Object^ output;
		switch (dwValueType)
		{
		case REG_BINARY:
			{
				output = gcnew array<byte>(dwBytesReturned);
				Marshal::Copy((IntPtr)pvData, (array<byte>^)output, 0, dwBytesReturned);
			}
			break;

		case REG_DWORD:
			{
				DWORD* dwData = static_cast<DWORD*>(pvData);
				output = gcnew Int32(*dwData);
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
				
				LPWSTR buffer = (LPWSTR)MemoryManager.Allocate(bytesNeeded);
				bytesNeeded = ExpandEnvironmentStrings((LPCWSTR)pvData, buffer, bytesNeeded);
				if (bytesNeeded == 0)
				{
					RevertToSelf();
					throw gcnew NativeExceptionBase(GetLastError());
				}
				
				output = gcnew String(buffer);
				MemoryManager.Free(buffer);
			}
			break;

		case REG_LINK:
			output = gcnew String((LPWSTR)pvData);
			break;

		case REG_MULTI_SZ:
			output = GetStringArrayFromDoubleNullTermninatedCStyleArray((LPWSTR)pvData, dwBytesReturned);
			break;

		case REG_QWORD:
			{
				long long* qwData = static_cast<long long*>(pvData);
				output = gcnew Int64(*qwData);
			}
			break;

		case REG_SZ:
			output = gcnew String((LPWSTR)pvData);
			break;
		
		default:
			{
				RevertToSelf();
				throw gcnew ArgumentException(String::Format("Invalid registry type '{0}'.", dwValueType));
			}
			break;
		}

		RevertToSelf();

		return output;
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(RegistryHive hive, String^ subKey)
	{
		return GetRegistrySubKeyNames(L"", L"", L"", hive, subKey);
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(String^ computerName, RegistryHive hive, String^ subKey)
	{
		return GetRegistrySubKeyNames(computerName, L"", L"", hive, subKey);
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey)
	{
		LPWSTR wPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistrySubKeyNames(L"", userName, wPass, hive, subKey);
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey)
	{
		LPWSTR wPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return GetRegistrySubKeyNames(computerName, userName, wPass, hive, subKey);
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(IntPtr hRegistry, String^ subKey)
	{
		LSTATUS result = ERROR_SUCCESS;
		SharedVecPtr(LPWSTR) subkeyNameVec = MakeVecPtr(LPWSTR);

		pin_ptr<const wchar_t> wSubKey = PtrToStringChars(subKey);

		HKEY whReg = (HKEY)hRegistry.ToPointer();
		result = regptr->GetRegistrySubkeyNames(whReg, (LPWSTR)wSubKey, 0, *subkeyNameVec);
		if (result != ERROR_SUCCESS)
		{
			wSubKey = nullptr;
			throw gcnew NativeExceptionBase(result);
		}

		wSubKey = nullptr;

		List<String^>^ output = gcnew List<String^>();
		for (LPWSTR singleName : *subkeyNameVec)
			output->Add(gcnew String(singleName));

		RevertToSelf();

		return output->ToArray();
	}

	array<String^>^ Wrapper::GetRegistrySubKeyNames(String^ computerName, String^ userName, const LPWSTR& password, RegistryHive hive, String^ subKey)
	{
		LSTATUS result = ERROR_SUCCESS;
		SharedVecPtr(LPWSTR) subkeyNameVec = MakeVecPtr(LPWSTR);
		pin_ptr<const wchar_t> temp;

		// Managing logon.
		if (!String::IsNullOrEmpty(userName))
			LogonAndImpersonateUser(userName, password);

		LPWSTR wComputerName = NULL;
		if (!String::IsNullOrEmpty(computerName))
		{
			temp = PtrToStringChars(computerName);
			wComputerName = (LPWSTR)temp;
		}

		pin_ptr<const wchar_t> wSubKey = PtrToStringChars(subKey);

		result = regptr->GetRegistrySubkeyNames(wComputerName, (HKEY)hive, (LPWSTR)wSubKey, 0, *subkeyNameVec);
		if (result != ERROR_SUCCESS)
		{
			RevertToSelf();

			temp = nullptr;
			wSubKey = nullptr;
			
			throw gcnew NativeExceptionBase(result);
		}

		temp = nullptr;
		wSubKey = nullptr;

		List<String^>^ output = gcnew List<String^>();
		for (LPWSTR singleName : *subkeyNameVec)
			output->Add(gcnew String(singleName));

		RevertToSelf();

		return output->ToArray();
	}

	array<Object^>^ Wrapper::GetRegistryValueList(String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		LPWSTR wPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return Wrapper::GetRegistryValueList(L"", userName, wPass, hive, subKey, valueNameList);
	}

	array<Object^>^ Wrapper::GetRegistryValueList(String^ computerName, String^ userName, SecureString^ password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		LPWSTR wPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		return Wrapper::GetRegistryValueList(computerName, userName, wPass, hive, subKey, valueNameList);
	}

	array<Object^>^ Wrapper::GetRegistryValueList(String^ computerName, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		return Wrapper::GetRegistryValueList(computerName, L"", L"", hive, subKey, valueNameList);
	}

	array<Object^>^ Wrapper::GetRegistryValueList(RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		return Wrapper::GetRegistryValueList(L"", L"", L"", hive, subKey, valueNameList);
	}

	array<Object^>^ Wrapper::GetRegistryValueList(IntPtr hRegistry, String^ subKey, array<String^>^ valueNameList)
	{
		LSTATUS nativeResult;
		DWORD dwValCount = valueNameList->Length;
		PVALENT pValList = new VALENT[valueNameList->Length];
		LPWSTR lpDataBuffer;

		WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

		pin_ptr<const wchar_t> wSubKey = PtrToStringChars(subKey);

		for (DWORD i = 0; i < dwValCount; i++)
		{
			pin_ptr<const wchar_t> wValueName = PtrToStringChars(valueNameList[i]);
			size_t sizeValName = wcslen(wValueName) + 1;

			pValList[i].ve_valuename = (LPWSTR)MemoryManager.Allocate(sizeValName * 2);
			wcscpy_s(pValList[i].ve_valuename, sizeValName, wValueName);

			wValueName = nullptr;
		}
		
		HKEY whReg = (HKEY)hRegistry.ToPointer();
		nativeResult = regptr->GetRegistryKeyValueList(whReg, (LPWSTR)wSubKey, pValList, dwValCount, lpDataBuffer);
		if (nativeResult != ERROR_SUCCESS)
		{
			wSubKey = nullptr;
			throw gcnew NativeExceptionBase(nativeResult);
		}

		wSubKey = nullptr;

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
					MemoryManager.Free(lpDataBuffer);
					throw gcnew NativeExceptionBase(GetLastError());
				}

				LPWSTR buffer = (LPWSTR)MemoryManager.Allocate(bytesNeeded);
				bytesNeeded = ExpandEnvironmentStrings((LPCWSTR)pvData, buffer, bytesNeeded);
				if (bytesNeeded == 0)
				{
					MemoryManager.Free(lpDataBuffer);
					throw gcnew NativeExceptionBase(GetLastError());
				}

				output[i] = gcnew String(buffer);
				MemoryManager.Free(buffer);
			}
			break;

			case REG_LINK:
				output[i] = gcnew String((LPWSTR)pvData);
				break;

			case REG_MULTI_SZ:
				output[i] = GetStringArrayFromDoubleNullTermninatedCStyleArray((LPWSTR)pvData, pValList[i].ve_valuelen);
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
				MemoryManager.Free(lpDataBuffer);
				throw gcnew ArgumentException(String::Format("Invalid registry type '{0}'.", pValList[i].ve_type));
			}
			break;
			}
		}

		MemoryManager.Free(lpDataBuffer);

		return output;
	}

	array<Object^>^ Wrapper::GetRegistryValueList(String^ computerName, String^ userName, const LPWSTR& password, RegistryHive hive, String^ subKey, array<String^>^ valueNameList)
	{
		LSTATUS nativeResult;
		DWORD dwValCount = valueNameList->Length;
		PVALENT pValList = new VALENT[valueNameList->Length];
		pin_ptr<const wchar_t> temp;
		LPWSTR lpDataBuffer;

		WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

		// Managing logon.
		if (!String::IsNullOrEmpty(userName))
			LogonAndImpersonateUser(userName, password);

		LPWSTR wComputerName = NULL;
		if (!String::IsNullOrEmpty(computerName))
		{
			temp = PtrToStringChars(computerName);
			wComputerName = (LPWSTR)temp;
		}

		pin_ptr<const wchar_t> wSubKey = PtrToStringChars(subKey);

		for (DWORD i = 0; i < dwValCount; i++)
		{
			pin_ptr<const wchar_t> wValueName = PtrToStringChars(valueNameList[i]);
			size_t sizeValName = wcslen(wValueName) + 1;

			pValList[i].ve_valuename = (LPWSTR)MemoryManager.Allocate(sizeValName * 2);
			wcscpy_s(pValList[i].ve_valuename, sizeValName, wValueName);

			wValueName = nullptr;
		}

		nativeResult = regptr->GetRegistryKeyValueList(wComputerName, (HKEY)hive, (LPWSTR)wSubKey, pValList, dwValCount, lpDataBuffer);
		if (nativeResult != ERROR_SUCCESS)
		{
			RevertToSelf();

			temp = nullptr;
			wSubKey = nullptr;

			throw gcnew NativeExceptionBase(nativeResult);
		}

		temp = nullptr;
		wSubKey = nullptr;

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
					MemoryManager.Free(lpDataBuffer);

					throw gcnew NativeExceptionBase(GetLastError());
				}

				LPWSTR buffer = (LPWSTR)MemoryManager.Allocate(bytesNeeded);
				bytesNeeded = ExpandEnvironmentStrings((LPCWSTR)pvData, buffer, bytesNeeded);
				if (bytesNeeded == 0)
				{
					RevertToSelf();
					MemoryManager.Free(lpDataBuffer);

					throw gcnew NativeExceptionBase(GetLastError());
				}

				output[i] = gcnew String(buffer);
				MemoryManager.Free(buffer);
			}
			break;

			case REG_LINK:
				output[i] = gcnew String((LPWSTR)pvData);
				break;

			case REG_MULTI_SZ:
				output[i] = GetStringArrayFromDoubleNullTermninatedCStyleArray((LPWSTR)pvData, pValList[i].ve_valuelen);
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
					MemoryManager.Free(lpDataBuffer);

					throw gcnew ArgumentException(String::Format("Invalid registry type '{0}'.", pValList[i].ve_type));
				}
				break;
			}
		}
		
		RevertToSelf();
		MemoryManager.Free(lpDataBuffer);

		return output;
	}

	// Utilities
	array<String^>^ Wrapper::GetStringArrayFromDoubleNullTermninatedCStyleArray(const LPWSTR& pvNativeArray, DWORD dwszBytes)
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

	array<String^>^ Wrapper::GetStringArrayFromDoubleNullTermninatedCStyleArray(IntPtr nativeArray, DWORD dwszBytes)
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
		LPWSTR wPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();
		LogonAndImpersonateUser(userName, wPass);
	}

	// Logs on the given user and impersonates it.
	// You must call 'RevertToSelf()' to revert to the caller.
	void Wrapper::LogonAndImpersonateUser(
		String^ userName,			 // The user name. If the user belongs to a domain, enter the down-level logon name: 'DOMAIN\UserName'.
		const LPWSTR& lpszPassword	 // The user's password, if any.
	) {
		if (!String::IsNullOrEmpty(userName))
		{
			LPWSTR wDomain = NULL;
			pin_ptr<const wchar_t> domain;
			pin_ptr<const wchar_t> wUserName;
			HANDLE hToken;

			if (userName->Contains(L"\\"))
			{
				domain = PtrToStringChars(userName->Split('\\')[0]);
				wUserName = PtrToStringChars(userName->Split('\\')[1]);

				wDomain = (LPWSTR)domain;
			}
			else
				wUserName = PtrToStringChars(userName);

			if (!LogonUser((LPWSTR)wUserName, wDomain, lpszPassword, LOGON32_LOGON_NEW_CREDENTIALS, LOGON32_PROVIDER_WINNT50, &hToken))
			{
				if (lpszPassword != NULL)
				{
					size_t sztpl = wcslen(lpszPassword) + 1;
					SecureZeroMemory(lpszPassword, sztpl * 2);
				}

				domain = nullptr;
				wUserName = nullptr;

				throw gcnew NativeExceptionBase(GetLastError());
			}

			if (!ImpersonateLoggedOnUser(hToken))
			{
				if (lpszPassword != NULL)
				{
					size_t sztpl = wcslen(lpszPassword) + 1;
					SecureZeroMemory(lpszPassword, sztpl * 2);
				}

				domain = nullptr;
				wUserName = nullptr;
				CloseHandle(hToken);

				throw gcnew NativeExceptionBase(GetLastError());
			}

			// Zeroing the memory here instead of Marshal::ZeroFreeGlobalAllocUnicode
			// So the plain text stays less time in memory.
			if (lpszPassword != NULL)
			{
				size_t sztpl = wcslen(lpszPassword) + 1;
				SecureZeroMemory(lpszPassword, sztpl * 2);
			}

			domain = nullptr;
			wUserName = nullptr;
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