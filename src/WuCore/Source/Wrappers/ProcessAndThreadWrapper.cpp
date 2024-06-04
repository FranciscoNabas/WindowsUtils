#pragma unmanaged

#include "../../Headers/Support/SafeHandle.h"
#include "../../Headers/Support/WuStdException.h"
#include "../../Headers/Support/NtUtilities.h"

#pragma managed

#include "../../Headers/Wrappers/ProcessAndThreadWrapper.h"
#include "../../Headers/Wrappers/UtilitiesWrapper.h"

namespace WindowsUtils::Wrappers
{
	using namespace System::Collections::Generic;
	using namespace System::Management::Automation;
	using namespace System::Runtime::InteropServices;

	// Get-ObjectHandle
	List<ObjectHandle^>^ ProcessAndThreadWrapper::GetProcessObjectHandle(array<ObjectHandleInput^>^ inputList, Boolean closeHandle, Core::CmdletContextProxy^ context)
	{
		wuvector<Core::WU_OBJECT_HANDLE> ppOutput;
		wuvector<Core::OBJECT_INPUT> reslist;

		for each (ObjectHandleInput ^ input in inputList) {
			Core::OBJECT_INPUT nativeInput = {
				UtilitiesWrapper::GetWideStringFromSystemString(input->Path),
				static_cast<Core::SupportedHandleType>(input->Type)
			};
			reslist.push_back(nativeInput);
		}

		try {
			m_pat->GetProcessObjectHandle(ppOutput, reslist, closeHandle, context->GetUnderlyingContext());
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}

		if (ppOutput.size() == 0)
			return nullptr;

		List<ObjectHandle^>^ output = gcnew List<ObjectHandle^>(0);
		for (const auto& handleInfo : ppOutput) {
			if (!WWuString::IsNullOrEmpty(handleInfo.Name))
				output->Add(gcnew ObjectHandle(handleInfo));
		}

		return output;
	}

	void ProcessAndThreadWrapper::ListProcessHandleInfo(UInt32 processId, Boolean all, Core::CmdletContextProxy^ context)
	{
		if (processId == 0)
			throw gcnew ArgumentException("The 'Idle' process (0) is not a valid process");

		wuvector<Core::WU_OBJECT_HANDLE_INFO> handleInfo;

		try {
			Core::ProcessHandle hProcess { PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE, FALSE, processId };
			try {
				Core::NtUtilities::ListProcessHandleInformation(hProcess, handleInfo, all, context->GetUnderlyingContext());
			}
			// Here the error record was already sent to the stream.
			catch (const Core::WuStdException& ex) {
				throw gcnew NativeException(ex);
			}
		}
		catch (const Core::WuStdException& ex) {
			context->WriteError(gcnew ErrorRecord(
				gcnew NativeException(ex),
				"ErrorOpenProcess",
				ErrorCategory::InvalidResult,
				processId
			));
		}

		for (const Core::WU_OBJECT_HANDLE_INFO& info : handleInfo)
			context->WriteObject(gcnew ObjectHandleInfo(info));
	}

	// Start-ProcessAsUser
	void ProcessAndThreadWrapper::StartProcessAsUser(String^ userName, String^ domain, SecureString^ password, String^ commandLine, String^ titleBar)
	{
		WWuString wrappedUser = UtilitiesWrapper::GetWideStringFromSystemString(userName);
		WWuString wrappedDomain = UtilitiesWrapper::GetWideStringFromSystemString(domain);
		WWuString wrappedCommandLine = UtilitiesWrapper::GetWideStringFromSystemString(commandLine);
		WWuString wrappedTitleBar = UtilitiesWrapper::GetWideStringFromSystemString(titleBar);

		// This string will be erased by the native function.
		WWuString wrappedPass = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();

		try {
			m_pat->RunAs(wrappedUser, wrappedDomain, wrappedPass, wrappedCommandLine, wrappedTitleBar);
		}
		catch (const Core::WuStdException& ex) {
			password->Clear();
			throw gcnew NativeException(ex);
		}

		password->Clear();
	}

	// Get-ProcessModule
	void ProcessAndThreadWrapper::ListProcessModule(array<UInt32>^ processIdList, bool includeVersionInfo, Core::CmdletContextProxy^ context)
	{
		wuvector<DWORD> wrappedProcIdList;
		for each (UInt32 procId in processIdList)
			wrappedProcIdList.push_back(procId);

		m_pat->GetProcessLoadedModuleInformation(wrappedProcIdList, includeVersionInfo, false, context->GetUnderlyingContext());
	}

	void ProcessAndThreadWrapper::ListProcessModule(bool includeVersionInfo, Core::CmdletContextProxy^ context)
	{
		wuvector<DWORD> procIdList;
		Core::NtUtilities::GetRunnningProcessIdList(procIdList);

		wuvector<Core::PROCESS_MODULE_INFO> moduleInfo;
		m_pat->GetProcessLoadedModuleInformation(procIdList, includeVersionInfo, true, context->GetUnderlyingContext());
	}

	// Suspend-Process
	void ProcessAndThreadWrapper::SuspendProcess(UInt32 processId, Core::CmdletContextProxy^ context)
	{
		try {
			m_pat->SuspendProcess(processId, context->GetUnderlyingContext());
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}
	}

	// Resume-Process
	void ProcessAndThreadWrapper::ResumeProcess(UInt32 processId, Core::CmdletContextProxy^ context)
	{
		try {
			m_pat->ResumeProcess(processId, context->GetUnderlyingContext());
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}
	}

	// TEST
	Dictionary<Int32, String^>^ ProcessAndThreadWrapper::ListProcesses()
	{
		std::unordered_map<DWORD, WWuString> procMap;
		Dictionary<Int32, String^>^ output = gcnew Dictionary<Int32, String^>();
		try {
			Core::NtUtilities::ListRunningProcessesWithImageName(procMap);
			for (const auto& [key, value] : procMap)
				output->Add(key, gcnew String(value.GetBuffer()));
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}

		return output;
	}
	// ENDTEST
}