#pragma once
#pragma managed

#include "CmdletContextProxy.h"

namespace WindowsUtils::Wrappers
{
	using namespace System::Security;
	using namespace System::Collections::Generic;

	public ref class ProcessAndThreadWrapper
	{
	public:
		// Get-ObjectHandle
		List<ObjectHandle^>^ GetProcessObjectHandle(array<ObjectHandleInput^>^ inputList, Boolean closeHandle, Core::CmdletContextProxy^ context);
		void ListProcessHandleInfo(UInt32 processId, Boolean all, Core::CmdletContextProxy^ context);

		// Start-ProcessAsUser
		void StartProcessAsUser(String^ userName, String^ domain, SecureString^ password, String^ commandLine, String^ titleBar);

		// Get-ProcessModule
		void ListProcessModule(array<UInt32>^ processIdList, bool includeVersionInfo, Core::CmdletContextProxy^ context);
		void ListProcessModule(bool includeVersionInfo, Core::CmdletContextProxy^ context);

		// Suspend-Process
		void SuspendProcess(UInt32 processId, Core::CmdletContextProxy^ context);

		// Resume-Process
		void ResumeProcess(UInt32 processId, Core::CmdletContextProxy^ context);

		// TEST
		Dictionary<Int32, String^>^ ListProcesses();
		// ENDTEST

	private:
		Core::ProcessAndThread* m_pat;
	};
}