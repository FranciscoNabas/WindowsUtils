#pragma unmanaged

#include "../../Headers/Support/Expressions.h"
#include "../../Headers/Support/WuStdException.h"

#pragma managed

#include "../../Headers/Wrappers/UtilitiesWrapper.h"
#include "../../Headers/Wrappers/TerminalServicesWrapper.h"

namespace WindowsUtils::Wrappers
{
	using namespace System::Collections::Generic;

	// Invoke-RemoteMessage
	array<MessageResponse^>^ TerminalServicesWrapper::SendRemoteMessage(IntPtr session, array<Int32>^ sessionid, String^ title, String^ message, UInt32 style, Int32 timeout, Boolean wait)
	{
		wusunique_vector<Core::WU_MESSAGE_RESPONSE> responseList = make_wusunique_vector<Core::WU_MESSAGE_RESPONSE>();
		wusunique_vector<DWORD> sessionIdList;

		WWuString wuTitle { UtilitiesWrapper::GetWideStringFromSystemString(title) };
		WWuString wuMessage { UtilitiesWrapper::GetWideStringFromSystemString(message) };

		try {
			if (sessionid == nullptr)
				m_wts->SendMessage(wuTitle, wuMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, responseList.get(), (HANDLE)session);

			else {
				sessionIdList = make_wusunique_vector<DWORD>();
				for (int i = 0; i < sessionid->Length; i++)
					sessionIdList->push_back((DWORD)sessionid[i]);

				m_wts->SendMessage(wuTitle, wuMessage, (DWORD)style, (DWORD)timeout, (BOOL)wait, sessionIdList.get(), responseList.get(), (HANDLE)session);
			}
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}

		List<MessageResponse^>^ output = gcnew List<MessageResponse^>(0);
		for (auto& response : *responseList)
			if (response.Response != 0)
				output->Add(gcnew MessageResponse(response));

		return output->ToArray();
	}

	// Get-ComputerSession
	array<ComputerSession^>^ TerminalServicesWrapper::GetComputerSession(String^ computerName, Boolean activeOnly, Boolean includeSystemSessions)
	{
		wusunique_vector<Core::WU_COMPUTER_SESSION> sessionList = make_wusunique_vector<Core::WU_COMPUTER_SESSION>();
		WWuString wrappedPcName;
		if (String::IsNullOrEmpty(computerName))
			wrappedPcName = WWuString();
		else
			wrappedPcName = UtilitiesWrapper::GetWideStringFromSystemString(computerName);

		try {
			m_wts->GetEnumeratedSession(wrappedPcName, *sessionList, activeOnly, includeSystemSessions);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}

		List<ComputerSession^>^ output = gcnew List<ComputerSession^>((int)sessionList->size());

		for (auto& sessionInfo : *sessionList)
			output->Add(gcnew ComputerSession(sessionInfo));

		return output->ToArray();
	}

	// Disconnect-Session
	Void TerminalServicesWrapper::DisconnectSession(IntPtr session, UInt32^ sessionid, Boolean wait)
	{
		try {
			if (nullptr == sessionid)
				m_wts->DisconnectSession((HANDLE)session, NULL, wait);
			else {
				m_wts->DisconnectSession((HANDLE)session, (DWORD)sessionid, wait);
			}
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}

	}
}