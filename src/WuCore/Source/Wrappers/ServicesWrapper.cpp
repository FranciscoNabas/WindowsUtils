#pragma unmanaged

#include "../../Headers/Support/WuException.h"

#pragma managed

#include "../../Headers/Wrappers/ServicesWrapper.h"
#include "../../Headers/Wrappers/UtilitiesWrapper.h"

namespace WindowsUtils::Wrappers
{
	// Remove-Service
	void ServicesWrapper::RemoveService(String^ serviceName, String^ computerName, bool stopService, bool noWait)
	{
		WWuString wuComputerName = UtilitiesWrapper::GetWideStringFromSystemString(computerName);
		WWuString wuServiceName = UtilitiesWrapper::GetWideStringFromSystemString(serviceName);
		_WU_START_TRY
			Stubs::Services::Dispatch<ServicesOperation::RemoveService>(Context->GetUnderlyingContext(),
				wuServiceName, wuComputerName, stopService, noWait);
		_WU_MANAGED_CATCH
	}

	void ServicesWrapper::RemoveService(String^ serviceName, bool stopService, bool noWait)
	{
		WWuString wuServiceName = UtilitiesWrapper::GetWideStringFromSystemString(serviceName);

		_WU_START_TRY
			Stubs::Services::Dispatch<ServicesOperation::RemoveService>(Context->GetUnderlyingContext(),
				wuServiceName, L"", stopService, noWait);
		_WU_MANAGED_CATCH
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

		Stubs::Services::Dispatch<ServicesOperation::GetSecurity>(Core::ExceptionMarshaler::NativePtr, wuServiceName, wuComputerName, sddl, &size, audit);

		String^ manSddl = gcnew String(sddl.Raw());

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

		Stubs::Services::Dispatch<ServicesOperation::GetSecurity>(Core::ExceptionMarshaler::NativePtr, wuServiceName, L"", sddl, &size, audit);

		String^ manSddl = gcnew String(sddl.Raw());

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

		Stubs::Services::Dispatch<ServicesOperation::GetSecurity>(Core::ExceptionMarshaler::NativePtr, whService, sddl, &size, audit);

		String^ manSddl = gcnew String(sddl.Raw());

		return manSddl;
	}

	// Set-ServiceSecurity
	void ServicesWrapper::SetServiceSecurity(String^ serviceName, String^ computerName, String^ sddl, bool audit, bool changeOwner)
	{
		WWuString wuServiceName = UtilitiesWrapper::GetWideStringFromSystemString(serviceName);
		WWuString wuComputerName = UtilitiesWrapper::GetWideStringFromSystemString(computerName);
		WWuString wuSddl = UtilitiesWrapper::GetWideStringFromSystemString(sddl);

		_WU_START_TRY
			Stubs::Services::Dispatch<ServicesOperation::SetSecurity>(Context->GetUnderlyingContext(),
				wuServiceName, wuComputerName, wuSddl, audit, changeOwner);
		_WU_MANAGED_CATCH
	}

	void ServicesWrapper::SetServiceSecurity(String^ serviceName, String^ sddl, bool audit, bool changeOwner)
	{
		WWuString wuServiceName = UtilitiesWrapper::GetWideStringFromSystemString(serviceName);
		WWuString wuSddl = UtilitiesWrapper::GetWideStringFromSystemString(sddl);

		_WU_START_TRY
			Stubs::Services::Dispatch<ServicesOperation::SetSecurity>(Context->GetUnderlyingContext(),
				wuServiceName, L"", wuSddl, audit, changeOwner);
		_WU_MANAGED_CATCH
	}
}