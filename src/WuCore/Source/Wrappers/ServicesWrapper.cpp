#pragma unmanaged

#include "../../Headers/Support/WuStdException.h"

#pragma managed

#include "../../Headers/Wrappers/ServicesWrapper.h"
#include "../../Headers/Wrappers/UtilitiesWrapper.h"

namespace WindowsUtils::Wrappers
{
	// Remove-Service
	void ServicesWrapper::RemoveService(String^ serviceName, String^ computerName, bool stopService, Core::CmdletContextProxy^ context, bool noWait)
	{
		WWuString wuComputerName = UtilitiesWrapper::GetWideStringFromSystemString(computerName);
		WWuString wuServiceName = UtilitiesWrapper::GetWideStringFromSystemString(serviceName);

		try {
			m_svc->RemoveService(wuServiceName, wuComputerName, stopService, context->GetUnderlyingContext(), noWait);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}
	}

	void ServicesWrapper::RemoveService(String^ serviceName, bool stopService, Core::CmdletContextProxy^ context, bool noWait)
	{
		WWuString wuServiceName = UtilitiesWrapper::GetWideStringFromSystemString(serviceName);

		try {
			m_svc->RemoveService(wuServiceName, L"", stopService, context->GetUnderlyingContext(), noWait);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}
	}

	// Get-ServiceSecurity
	String^ ServicesWrapper::GetServiceSecurityDescriptorString(String^ serviceName, String^ computerName, bool audit)
	{
		WWuString wuServiceName = UtilitiesWrapper::GetWideStringFromSystemString(computerName);
		WWuString wuComputerName = UtilitiesWrapper::GetWideStringFromSystemString(serviceName);

		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		if (audit)
			secInfo |= SACL_SECURITY_INFORMATION;

		WWuString sddl;
		DWORD size = 0;
		try {
			if (audit)
				m_svc->GetServiceSecurity(wuServiceName, wuComputerName, sddl, &size, TRUE);
			else
				m_svc->GetServiceSecurity(wuServiceName, wuComputerName, sddl, &size);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}

		String^ manSddl = gcnew String(sddl.GetBuffer());

		return manSddl;
	}

	String^ ServicesWrapper::GetServiceSecurityDescriptorString(String^ serviceName, bool audit)
	{
		WWuString wuServiceName = UtilitiesWrapper::GetWideStringFromSystemString(serviceName);

		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		if (audit)
			secInfo |= SACL_SECURITY_INFORMATION;

		WWuString sddl;
		DWORD size = 0;
		try {
			if (audit)
				m_svc->GetServiceSecurity(wuServiceName, L"", sddl, &size, TRUE);
			else
				m_svc->GetServiceSecurity(wuServiceName, L"", sddl, &size);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}

		String^ manSddl = gcnew String(sddl.GetBuffer());

		return manSddl;
	}

	String^ ServicesWrapper::GetServiceSecurityDescriptorString(IntPtr hService, bool audit)
	{
		HANDLE phService = static_cast<HANDLE>(hService);
		SC_HANDLE whService = static_cast<SC_HANDLE>(phService);

		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		if (audit)
			secInfo |= SACL_SECURITY_INFORMATION;

		WWuString sddl;
		DWORD size = 0;
		try {
			if (audit)
				m_svc->GetServiceSecurity(whService, sddl, &size, TRUE);
			else
				m_svc->GetServiceSecurity(whService, sddl, &size);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}

		String^ manSddl = gcnew String(sddl.GetBuffer());

		return manSddl;
	}

	// Set-ServiceSecurity
	void ServicesWrapper::SetServiceSecurity(String^ serviceName, String^ computerName, String^ sddl, bool audit, bool changeOwner)
	{
		WWuString wuServiceName = UtilitiesWrapper::GetWideStringFromSystemString(serviceName);
		WWuString wuComputerName = UtilitiesWrapper::GetWideStringFromSystemString(computerName);
		WWuString wuSddl = UtilitiesWrapper::GetWideStringFromSystemString(sddl);

		try {
			m_svc->SetServiceSecurity(wuServiceName, wuComputerName, wuSddl, audit, changeOwner);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}
	}

	void ServicesWrapper::SetServiceSecurity(String^ serviceName, String^ sddl, bool audit, bool changeOwner)
	{
		WWuString wuServiceName = UtilitiesWrapper::GetWideStringFromSystemString(serviceName);
		WWuString wuSddl = UtilitiesWrapper::GetWideStringFromSystemString(sddl);

		try {
			m_svc->SetServiceSecurity(wuServiceName, L"", wuSddl, audit, changeOwner);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}
	}
}