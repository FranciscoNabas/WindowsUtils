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
	ObjectHandleBase::ObjectHandleBase() : wrapper(new Utilities::WU_OBJECT_HANDLE) { }
	ObjectHandleBase::ObjectHandleBase(Utilities::WU_OBJECT_HANDLE objhandle)
	{
		wrapper = new Utilities::WU_OBJECT_HANDLE;
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
	array<MessageResponseBase^>^ WrapperFunctions::InvokeRemoteMessage(IntPtr session, array<Int32>^ sessionid, String^ title, String^ message, UInt32 style, Int32 timeout, Boolean wait)
	{
		DWORD dwresult = ERROR_SUCCESS;
		SharedVecPtr(TerminalServices::WU_MESSAGE_RESPONSE) presult = MakeVecPtr(TerminalServices::WU_MESSAGE_RESPONSE);
		SharedVecPtr(DWORD) psessid = MakeVecPtr(DWORD);

		pin_ptr<const wchar_t> wTitle = PtrToStringChars(title);
		pin_ptr<const wchar_t> wMessage = PtrToStringChars(message);

		if (sessionid == nullptr)
			dwresult = wtsptr->InvokeMessage((LPWSTR)wTitle, (LPWSTR)wMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, *psessid, *presult, (HANDLE)session);

		else
		{
			for (int i = 0; i < sessionid->Length; i++)
				psessid->push_back((DWORD)sessionid[i]);

			dwresult = wtsptr->InvokeMessage((LPWSTR)wTitle, (LPWSTR)wMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, *psessid, *presult, (HANDLE)session);
		}

		if (ERROR_SUCCESS != dwresult)
			throw gcnew NativeException(dwresult);

		array<MessageResponseBase^>^ output = gcnew array<MessageResponseBase^>((int)presult->size());

		for (size_t i = 0; i < presult->size(); i++)
			if (presult->at(i).Response != 0)
				output[(int)i] = gcnew MessageResponseBase(presult->at(i));

		return output;
	}

	// Get-ComputerSession
	array<ComputerSessionBase^>^ WrapperFunctions::GetComputerSession(String^ computername, IntPtr session, Boolean onlyactive, Boolean includesystemsession)
	{
		SharedVecPtr(TerminalServices::WU_COMPUTER_SESSION) result = MakeVecPtr(TerminalServices::WU_COMPUTER_SESSION);
		DWORD opresult = wtsptr->GetEnumeratedSession(*result, (HANDLE)session, onlyactive, includesystemsession);
		if (opresult != ERROR_SUCCESS)
			throw gcnew NativeException(opresult);

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
	Void WrapperFunctions::DisconnectSession(IntPtr session, UInt32^ sessionid, Boolean wait)
	{
		DWORD result = ERROR_SUCCESS;
		if (nullptr == sessionid)
			result = wtsptr->DisconnectSession((HANDLE)session, NULL, wait);
		else
		{
			result = wtsptr->DisconnectSession((HANDLE)session, (DWORD)sessionid, wait);
			if (result != ERROR_SUCCESS)
				throw gcnew NativeException(result);
		}
	}

	// Get-FormattedError
	String^ WrapperFunctions::GetFormattedError(Int32 errorcode)
	{
		std::shared_ptr<LPWSTR> errormess = std::make_shared<LPWSTR>();
		DWORD result = utlptr->GetFormattedError((DWORD)errorcode, *errormess);
		if (ERROR_SUCCESS != result)
			throw gcnew NativeException(result, String::Format("'GetFormattedError' failed with {0}", result));

		return gcnew String(*errormess);
	}
	// Get-LastWin32Error
	String^ WrapperFunctions::GetLastWin32Error()
	{
		std::shared_ptr<LPWSTR> errormess = std::make_shared<LPWSTR>();
		DWORD result = utlptr->GetFormattedWin32Error(*errormess);
		if (ERROR_SUCCESS != result)
			throw gcnew NativeException(result);

		return gcnew String(*errormess);
	}

	// Get-ObjectHandle
	array<ObjectHandleBase^>^ WrapperFunctions::GetProcessObjectHandle(array<String^>^ fileName, Boolean closeHandle)
	{
		SharedVecPtr(Utilities::WU_OBJECT_HANDLE) ppOutput = MakeVecPtr(Utilities::WU_OBJECT_HANDLE);
		SharedVecPtr(LPCWSTR) reslist = MakeVecPtr(LPCWSTR);

		for (int i = 0; i < fileName->Length; i++)
		{
			pin_ptr<const WCHAR> single = PtrToStringChars(fileName[i]);
			reslist->push_back((LPCWSTR)single);
			single = nullptr;
		}

		UINT result = utlptr->GetProcessObjectHandle(*ppOutput, *reslist, closeHandle);
		if (result != ERROR_SUCCESS)
			throw gcnew NativeException(result);

		if (ppOutput->size() == 0)
			return nullptr;

		array<ObjectHandleBase^>^ output = gcnew array<ObjectHandleBase^>((int)ppOutput->size());
		for (size_t i = 0; i < ppOutput->size(); i++)
			output[(int)i] = gcnew ObjectHandleBase(ppOutput->at(i));

		return output;
	}

	// Send-Click
	Void WrapperFunctions::SendClick()
	{
		DWORD result = utlptr->SendClick();
		if (ERROR_SUCCESS != result)
			throw gcnew NativeException(result);
	}

	// Get-ResourceMessageTable
	array<ResourceMessageTableCore^>^ WrapperFunctions::GetResourceMessageTable(String^ libpath)
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
				throw gcnew NativeException(result, errormsg + " is not a valid Win32 application.");
			}
			else
			{
				wlibpath = nullptr;
				throw gcnew NativeException(result);
			}
		}

		array<ResourceMessageTableCore^>^ output = gcnew array<ResourceMessageTableCore^>((int)ppresult->size());
		for (size_t i = 0; i < ppresult->size(); i++)
			output[(int)i] = gcnew ResourceMessageTableCore(ppresult->at(i));

		wlibpath = nullptr;
		return output;
	}

	// Get-MsiProperties
	Dictionary<String^, String^>^ WrapperFunctions::GetMsiProperties(String^ filepath)
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
				throw gcnew NativeException(result, GetFormattedError(result), gcnew SystemException(gcnew String(pextmsierr)));
			else
				throw gcnew NativeException(result);
		}

		std::map<LPWSTR, LPWSTR>::iterator itr;
		for (itr = ppresult->begin(); itr != ppresult->end(); itr++)
			output->Add(gcnew String(itr->first), gcnew String(itr->second));

		return output;
	}

	// Remove-Service
	void WrapperFunctions::RemoveService(String^ servicename, String^ computername, bool stopservice)
	{
		pin_ptr<const wchar_t> wcomputername = PtrToStringChars(computername);
		pin_ptr<const wchar_t> wservicename = PtrToStringChars(servicename);

		DWORD result = svcptr->RemoveService((LPWSTR)wservicename, (LPWSTR)wcomputername, stopservice);
		if (result != ERROR_SUCCESS)
			throw gcnew NativeException(result);
	}
	void WrapperFunctions::RemoveService(String^ servicename, bool stopservice)
	{
		pin_ptr<const wchar_t> wservicename = PtrToStringChars(servicename);

		DWORD result = svcptr->RemoveService((LPWSTR)wservicename, NULL, stopservice);
		if (result != ERROR_SUCCESS)
			throw gcnew NativeException(result);
	}

	void WrapperFunctions::RemoveService(IntPtr hservice, String^ computername, bool stopservice)
	{
		pin_ptr<const wchar_t> wcomputername = PtrToStringChars(computername);

		HANDLE uhservice = static_cast<HANDLE>(hservice);
		SC_HANDLE uschservice = static_cast<SC_HANDLE>(uhservice);
		DWORD result = svcptr->RemoveService(uschservice, (LPWSTR)wcomputername, stopservice);

		if (result != ERROR_SUCCESS)
			throw gcnew NativeException(result);
	}

	// Get-ServiceSecurity
	String^ WrapperFunctions::GetServiceSecurityDescriptorString(String^ serviceName, String^ computerName, bool audit)
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
			throw gcnew NativeException(result);

		LPWSTR sddl;
		ConvertSecurityDescriptorToStringSecurityDescriptorW(svcSecurity, SDDL_REVISION_1, secInfo, &sddl, NULL);

		return gcnew String(sddl);
	}

	String^ WrapperFunctions::GetServiceSecurityDescriptorString(String^ serviceName, bool audit)
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
			throw gcnew NativeException(result);

		LPWSTR sddl;
		ConvertSecurityDescriptorToStringSecurityDescriptorW(svcSecurity, SDDL_REVISION_1, secInfo, &sddl, NULL);
		
		return gcnew String(sddl);
	}

	String^ WrapperFunctions::GetServiceSecurityDescriptorString(IntPtr hService, bool audit)
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
			throw gcnew NativeException(result);

		LPWSTR sddl;
		ConvertSecurityDescriptorToStringSecurityDescriptorW(svcSecurity, SDDL_REVISION_1, secInfo, &sddl, NULL);

		return gcnew String(sddl);
	}

	// Set-ServiceSecurity
	void WrapperFunctions::SetServiceSecurity(String^ serviceName, String^ computerName, String^ sddl, bool audit, bool changeOwner)
	{
		pin_ptr<const wchar_t> wServiceName = PtrToStringChars(serviceName);
		pin_ptr<const wchar_t> wComputerName = PtrToStringChars(computerName);
		pin_ptr<const wchar_t> wSddl = PtrToStringChars(sddl);

		DWORD result = svcptr->SetServiceSecurity((LPWSTR)wServiceName, (LPWSTR)wComputerName, (LPWSTR)wSddl, audit, changeOwner);
		if (result != ERROR_SUCCESS)
			throw gcnew NativeException(result);
	}

	void WrapperFunctions::SetServiceSecurity(String^ serviceName, String^ sddl, bool audit, bool changeOwner)
	{
		pin_ptr<const wchar_t> wServiceName = PtrToStringChars(serviceName);
		pin_ptr<const wchar_t> wSddl = PtrToStringChars(sddl);

		DWORD result = svcptr->SetServiceSecurity((LPWSTR)wServiceName, L".", (LPWSTR)wSddl, audit, changeOwner);
		if (result != ERROR_SUCCESS)
			throw gcnew NativeException(result);
	}

	/*========================================
	==		Utility function definition		==
	==========================================*/
}