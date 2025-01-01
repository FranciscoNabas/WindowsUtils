#pragma once
#pragma unmanaged

#include "../Stubs/ProcessAndThreadStub.h"

#pragma managed

#include "WrapperBase.h"
#include "NativeException.h"

namespace WindowsUtils::Wrappers
{
	using namespace System::Security;
	using namespace System::Collections::Generic;

	public ref class ProcessAndThreadWrapper : public WrapperBase
	{
	public:
		ProcessAndThreadWrapper(Core::CmdletContextProxy^ context)
			: WrapperBase(context) { }

		// Get-ObjectHandle
		List<ObjectHandle^>^ GetProcessObjectHandle(array<ObjectHandleInput^>^ inputList, bool closeHandle);
		void ListProcessHandleInfo(UInt32 processId, bool all);

		// Start-ProcessAsUser
		void StartProcessAsUser(String^ userName, String^ domain, SecureString^ password, String^ commandLine, String^ titleBar);

		// Get-ProcessModule
		void ListProcessModule(array<UInt32>^ processIdList, bool includeVersionInfo);
		void ListProcessModule(bool includeVersionInfo);

		// Suspend-Process
		void SuspendProcess(UInt32 processId);

		// Resume-Process
		void ResumeProcess(UInt32 processId);
	};
}