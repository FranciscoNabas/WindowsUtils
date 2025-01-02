#pragma once

#pragma unmanaged

#include "../Stubs/TerminalServicesStub.h"

#pragma managed

#include "WrapperBase.h"
#include "NativeException.h"
#include "Types/WtsTypes.h"

namespace WindowsUtils::Wrappers
{
	using namespace System;

	public ref class TerminalServicesWrapper : public WrapperBase
	{
	public:
		TerminalServicesWrapper(Core::CmdletContextProxy^ context)
			: WrapperBase(context) { }

		// Invoke-RemoteMessage
		array<MessageResponse^>^ SendRemoteMessage(IntPtr session, array<Int32>^ sessionid, String^ title, String^ message, UInt32 style, Int32 timeout, bool wait);

		// Get-ComputerSession
		array<ComputerSession^>^ GetComputerSession(String^ computername, bool onlyactive, bool includesystemsession);

		// Disconnect-Session
		Void DisconnectSession(IntPtr session, UInt32^ sessionid, bool wait);
	};
}