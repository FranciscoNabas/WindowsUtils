#pragma once

#pragma unmanaged

#include "..\pch.h"

#include "TerminalServices.h"

#pragma managed

using namespace System;

namespace WindowsUtils
{
	// Get-ComputerSession
	public ref class ComputerSession
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
		property String^ ComputerName { String^ get() { return gcnew String(wrapper->ComputerName.GetBuffer()); } }

		ComputerSession()
			: wrapper(new Core::TerminalServices::WU_COMPUTER_SESSION) { }

		ComputerSession(const Core::TerminalServices::WU_COMPUTER_SESSION& computerSession)
			: wrapper(new Core::TerminalServices::WU_COMPUTER_SESSION(computerSession)) { }
		
		~ComputerSession() { delete wrapper; }

	protected:
		!ComputerSession() { delete wrapper; }

	private:
		Core::TerminalServices::PWU_COMPUTER_SESSION wrapper;
	};
}