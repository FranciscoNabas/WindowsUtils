#pragma once
#pragma managed

#include "Types/WtsTypes.h"

namespace WindowsUtils::Wrappers
{
	using namespace System;

	public ref class TerminalServicesWrapper
	{
	public:
		// Invoke-RemoteMessage
		array<MessageResponse^>^ SendRemoteMessage(IntPtr session, array<Int32>^ sessionid, String^ title, String^ message, UInt32 style, Int32 timeout, Boolean wait);

		// Get-ComputerSession
		array<ComputerSession^>^ GetComputerSession(String^ computername, Boolean onlyactive, Boolean includesystemsession);

		// Disconnect-Session
		Void DisconnectSession(IntPtr session, UInt32^ sessionid, Boolean wait);

	private:
		Core::TerminalServices* m_wts;
	};
}