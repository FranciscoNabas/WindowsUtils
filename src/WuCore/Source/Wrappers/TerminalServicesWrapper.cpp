#pragma unmanaged

#include "../../Headers/Support/Expressions.h"
#include "../../Headers/Support/WuException.h"

#pragma managed

#include "../../Headers/Wrappers/UtilitiesWrapper.h"
#include "../../Headers/Wrappers/TerminalServicesWrapper.h"

namespace WindowsUtils::Wrappers
{
	using namespace System::Collections::Generic;

	// Invoke-RemoteMessage
	array<MessageResponse^>^ TerminalServicesWrapper::SendRemoteMessage(IntPtr session, array<Int32>^ sessionid, String^ title, String^ message, UInt32 style, Int32 timeout, bool wait)
	{
		std::vector<Core::WU_MESSAGE_RESPONSE> responseList;
		std::vector<DWORD> sessionIdList;

		WWuString wuTitle { UtilitiesWrapper::GetWideStringFromSystemString(title) };
		WWuString wuMessage { UtilitiesWrapper::GetWideStringFromSystemString(message) };

		_WU_START_TRY
			if (sessionid == nullptr)
				Stubs::TerminalServices::Dispatch<WtsOperation::SendMessage>(Context->GetUnderlyingContext(),
					wuTitle, wuMessage, static_cast<DWORD>(style), static_cast<DWORD>(timeout), wait, responseList, reinterpret_cast<HANDLE>(session.ToPointer()));

			else {
				for (int i = 0; i < sessionid->Length; i++)
					sessionIdList.push_back(static_cast<DWORD>(sessionid[i]));

				Stubs::TerminalServices::Dispatch<WtsOperation::SendMessage>(Context->GetUnderlyingContext(),
					wuTitle, wuMessage, static_cast<DWORD>(style), static_cast<DWORD>(timeout), wait, sessionIdList, responseList, reinterpret_cast<HANDLE>(session.ToPointer()));
			}
		_WU_MANAGED_CATCH

		List<MessageResponse^>^ output = gcnew List<MessageResponse^>(0);
		for (auto& response : responseList)
			if (response.Response != 0)
				output->Add(gcnew MessageResponse(response));

		return output->ToArray();
	}

	// Get-ComputerSession
	array<ComputerSession^>^ TerminalServicesWrapper::GetComputerSession(String^ computerName, bool activeOnly, bool includeSystemSessions)
	{
		std::vector<Core::WU_COMPUTER_SESSION> sessionList;
		WWuString wrappedPcName;
		if (String::IsNullOrEmpty(computerName))
			wrappedPcName = WWuString();
		else
			wrappedPcName = UtilitiesWrapper::GetWideStringFromSystemString(computerName);

		_WU_START_TRY
			Stubs::TerminalServices::Dispatch<WtsOperation::EnumSessions>(Context->GetUnderlyingContext(),
				wrappedPcName, sessionList, activeOnly, includeSystemSessions);
		_WU_MANAGED_CATCH

		List<ComputerSession^>^ output = gcnew List<ComputerSession^>((int)sessionList.size());

		for (auto& sessionInfo : sessionList)
			output->Add(gcnew ComputerSession(sessionInfo));

		return output->ToArray();
	}

	// Disconnect-Session
	Void TerminalServicesWrapper::DisconnectSession(IntPtr session, UInt32^ sessionid, bool wait)
	{
		_WU_START_TRY
			if (!sessionid)
				Stubs::TerminalServices::Dispatch<WtsOperation::Disconnect>(Context->GetUnderlyingContext(), reinterpret_cast<HANDLE>(session.ToPointer()), 0, wait);
			else
				Stubs::TerminalServices::Dispatch<WtsOperation::Disconnect>(Context->GetUnderlyingContext(), reinterpret_cast<HANDLE>(session.ToPointer()), static_cast<DWORD>(*sessionid), wait);
		_WU_MANAGED_CATCH
	}
}