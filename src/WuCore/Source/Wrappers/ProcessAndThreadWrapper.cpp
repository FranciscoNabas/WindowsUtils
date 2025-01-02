#pragma unmanaged

#include "../../Headers/Support/SafeHandle.h"
#include "../../Headers/Support/WuException.h"
#include "../../Headers/Support/Nt/NtUtilities.h"

#pragma managed

#include "../../Headers/Wrappers/ProcessAndThreadWrapper.h"
#include "../../Headers/Wrappers/UtilitiesWrapper.h"

namespace WindowsUtils::Wrappers
{
	using namespace System::Collections::Generic;
	using namespace System::Management::Automation;
	using namespace System::Runtime::InteropServices;

	// Get-ObjectHandle
	List<ObjectHandle^>^ ProcessAndThreadWrapper::GetProcessObjectHandle(array<ObjectHandleInput^>^ inputList, bool closeHandle)
	{
		WuList<Core::WU_OBJECT_HANDLE> ppOutput;
		WuList<Core::OBJECT_INPUT> reslist;

		for each (ObjectHandleInput ^ input in inputList) {
			reslist.Add(
				UtilitiesWrapper::GetWideStringFromSystemString(input->Path),
				static_cast<Core::SupportedHandleType>(input->Type)
			);
		}

		_WU_START_TRY
			Stubs::ProcessAndThread::Dispatch<PatOperation::GetHandle>(Context->GetUnderlyingContext(), reslist, static_cast<bool>(closeHandle), ppOutput);
		_WU_MANAGED_CATCH
		
		if (!ppOutput.Count())
			return nullptr;

		List<ObjectHandle^>^ output = gcnew List<ObjectHandle^>(0);
		for (const auto& handleInfo : ppOutput) {
			if (!WWuString::IsNullOrEmpty(handleInfo.Name))
				output->Add(gcnew ObjectHandle(handleInfo));
		}

		return output;
	}

	void ProcessAndThreadWrapper::ListProcessHandleInfo(UInt32 processId, bool all)
	{
		if (processId == 0)
			throw gcnew ArgumentException("The 'Idle' process (0) is not a valid process");

		WuList<Core::WU_OBJECT_HANDLE_INFO> handleInfo(200);
		Core::ProcessHandle hProcess{ processId, PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE, false };
		if (!hProcess.IsValid())
			throw gcnew NativeException(_WU_NEW_NATIVE_EXCEPTION(ERROR_INVALID_HANDLE, L"OpenProcess", Core::WriteErrorCategory::OpenError));
		
		_WU_START_TRY
			Stubs::ProcessAndThread::Dispatch<PatOperation::ListHandleInfo>(Context->GetUnderlyingContext(), hProcess, all, handleInfo);
		_WU_MANAGED_CATCH
		
		for (const Core::WU_OBJECT_HANDLE_INFO& info : handleInfo)
			Context->WriteObject(gcnew ObjectHandleInfo(info));
	}

	// Start-ProcessAsUser
	void ProcessAndThreadWrapper::StartProcessAsUser(String^ userName, String^ domain, SecureString^ password, String^ commandLine, String^ titleBar)
	{
		WWuString wrappedUser         = UtilitiesWrapper::GetWideStringFromSystemString(userName);
		WWuString wrappedDomain       = UtilitiesWrapper::GetWideStringFromSystemString(domain);
		WWuString wrappedCommandLine  = UtilitiesWrapper::GetWideStringFromSystemString(commandLine);
		WWuString wrappedTitleBar     = UtilitiesWrapper::GetWideStringFromSystemString(titleBar);

		// This string will be erased by the native function.
		WWuString wrappedPass		  = (LPWSTR)Marshal::SecureStringToCoTaskMemUnicode(password).ToPointer();

		try {
			Stubs::ProcessAndThread::Dispatch<PatOperation::RunAs>(Context->GetUnderlyingContext(),
				wrappedUser, wrappedDomain, wrappedPass, wrappedCommandLine, wrappedTitleBar);
		}
		catch (NativeException^ ex) {
			password->Clear();
			Context->WriteError(ex->Record);

			throw;
		}

		password->Clear();
	}

	// Get-ProcessModule
	void ProcessAndThreadWrapper::ListProcessModule(array<UInt32>^ processIdList, bool includeVersionInfo)
	{
		WuList<DWORD> wrappedProcIdList(static_cast<size_t>(processIdList->Length));
		for each (UInt32 procId in processIdList)
			wrappedProcIdList.Add(static_cast<DWORD>(procId));

		_WU_START_TRY
			Stubs::ProcessAndThread::Dispatch<PatOperation::GetModuleInfo>(Context->GetUnderlyingContext(),
				wrappedProcIdList, includeVersionInfo, false);
		_WU_MANAGED_CATCH
	}

	void ProcessAndThreadWrapper::ListProcessModule(bool includeVersionInfo)
	{
		WuList<DWORD> procIdList = Core::NtUtilities::ListRunningProcesses();

		_WU_START_TRY
			Stubs::ProcessAndThread::Dispatch<PatOperation::GetModuleInfo>(Context->GetUnderlyingContext(),
				procIdList, includeVersionInfo, true);
		_WU_MANAGED_CATCH
	}

	// Suspend-Process
	void ProcessAndThreadWrapper::SuspendProcess(UInt32 processId)
	{
		_WU_START_TRY
			Stubs::ProcessAndThread::Dispatch<PatOperation::Suspend>(Context->GetUnderlyingContext(), static_cast<DWORD>(processId));
		_WU_MANAGED_CATCH
	}

	// Resume-Process
	void ProcessAndThreadWrapper::ResumeProcess(UInt32 processId)
	{
		_WU_START_TRY
			Stubs::ProcessAndThread::Dispatch<PatOperation::Resume>(Context->GetUnderlyingContext(), static_cast<DWORD>(processId));
		_WU_MANAGED_CATCH
	}
}