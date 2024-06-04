#pragma unmanaged

#include "../../Headers/Support/WuStdException.h"

#pragma managed

#include "../../Headers/Wrappers/NetworkWrapper.h"
#include "../../Headers/Wrappers/UtilitiesWrapper.h"

namespace WindowsUtils::Wrappers
{
	// Start-Tcping
	void NetworkWrapper::StartTcpPing(String^ destination, Int32 port, Int32 count, Int32 timeout, Int32 interval, Int32 failThreshold, bool continuous,
		bool jitter, bool fqdn, bool force, bool single, String^ outFile, bool append, Core::CmdletContextProxy^ context, [Out] bool% isCancel)
	{
		bool isFile = false;
		WWuString wrappedOutFile;
		if (!String::IsNullOrEmpty(outFile)) {
			wrappedOutFile = UtilitiesWrapper::GetWideStringFromSystemString(outFile);
			isFile = true;
		}

		WWuString wrappedDest = UtilitiesWrapper::GetWideStringFromSystemString(destination);

		Core::TcpingForm form { wrappedDest, static_cast<DWORD>(port), static_cast<DWORD>(count), static_cast<DWORD>(timeout), static_cast<DWORD>(interval), static_cast<DWORD>(failThreshold),
			continuous, jitter, fqdn, force, single, isFile, wrappedOutFile, append };

		try {
			m_ntw->StartTcpPing(form, context->GetUnderlyingContext());
		}
		catch (const Core::WuStdException& ex) {
			if (ex.ErrorCode() != ERROR_CANCELLED)
				throw gcnew NativeException(ex);
		}

		isCancel = form.IsCtrlCHit();
	}

	// Get-NetworkFile
	List<NetworkFileInfo^>^ NetworkWrapper::GetNetworkFile(String^ computerName, String^ basePath, String^ userName, bool includeSessionName)
	{
		WWuString wrappedPcName = UtilitiesWrapper::GetWideStringFromSystemString(computerName);
		WWuString wrappedBasePath = UtilitiesWrapper::GetWideStringFromSystemString(basePath);
		WWuString wrappedUserName = UtilitiesWrapper::GetWideStringFromSystemString(userName);

		wuvector<Core::NETWORK_FILE_INFO> result;
		auto output = gcnew List<NetworkFileInfo^>(0);

		if (includeSessionName) {
			wuvector<Core::NETWORK_SESSION_INFO> sessionInfo;
			try {
				m_ntw->ListNetworkFiles(wrappedPcName, wrappedBasePath, wrappedUserName, result, sessionInfo);
			}
			catch (const Core::WuStdException& ex) {
				throw gcnew NativeException(ex);
			}

			for (Core::NETWORK_FILE_INFO& info : result) {
				WWuString sessName;
				for (Core::NETWORK_SESSION_INFO& sessInfo : sessionInfo) {
					if (sessInfo.UserName == info.UserName) {
						sessName = sessInfo.ComputerSessionName;
						break;
					}
				}

				output->Add(gcnew NetworkFileInfo(info, sessName, computerName));
			}
		}
		else {
			try {
				m_ntw->ListNetworkFiles(wrappedPcName, wrappedBasePath, wrappedUserName, result);
				for (Core::NETWORK_FILE_INFO& info : result)
					output->Add(gcnew NetworkFileInfo(info, computerName));
			}
			catch (const Core::WuStdException& ex) {
				throw gcnew NativeException(ex);
			}
		}

		return output;
	}

	// Close-NetworkFile
	void NetworkWrapper::CloseNetworkFile(String^ computerName, Int32 fileId)
	{
		WWuString wrappedPcName = UtilitiesWrapper::GetWideStringFromSystemString(computerName);
		try {
			m_ntw->CloseNetworkFile(wrappedPcName, fileId);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}
	}

	// Test-Port
	void NetworkWrapper::TestNetworkPort(String^ destination, UInt32 port, TransportProtocol protocol, UInt32 timeout, Core::CmdletContextProxy^ context)
	{
		WWuString wrappedDest = UtilitiesWrapper::GetWideStringFromSystemString(destination);
		Core::TestPortForm workForm(
			wrappedDest,
			port,
			static_cast<Core::TestPortProtocol>(protocol),
			timeout
		);

		m_ntw->TestNetworkPort(workForm, context->GetUnderlyingContext());
	}
}