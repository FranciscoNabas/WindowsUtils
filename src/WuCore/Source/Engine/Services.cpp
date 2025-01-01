#include "../../pch.h"

#include "../../Headers/Engine/Services.h"

namespace WindowsUtils::Core
{
	void Services::RemoveService(
		const WWuString& serviceName,					// The service name.
		const WWuString& computerName,					// Optional computer name.
		bool stopService,								// Stops the service, if it's running.
		bool noWait,									// Doesn't wait for the services to stop.
		const WuNativeContext* context					// A native representation of the Cmdlet context.
	)
	{
		// Trying to give the least amount of privileges as possible, depending on the request.
		DWORD desiredAccess;
		if (stopService)
			desiredAccess = DELETE | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_STOP;
		else
			desiredAccess = DELETE;

		ScmHandle hScm(computerName, SC_MANAGER_CONNECT);
		ScmHandle hService(hScm, computerName, serviceName, desiredAccess);

		if (stopService == TRUE) {
			SERVICE_STATUS serviceStatus { };

			if (!QueryServiceStatus(hService.Get(), &serviceStatus))
				_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"QueryServiceStatus", WriteErrorCategory::InvalidResult);

			if (serviceStatus.dwCurrentState != SERVICE_STOPPED && serviceStatus.dwCurrentState != SERVICE_STOP_PENDING) {
				StopDependentServices(hScm, hService, computerName, noWait, context);

				if (noWait)
					ControlService(hService.Get(), SERVICE_CONTROL_STOP, &serviceStatus);
				else
					StopServiceWithWarning(hService, hScm, serviceName, serviceStatus, context);
			}
		}

		if (!DeleteService(hService.Get()))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"DeleteService", WriteErrorCategory::InvalidResult);
	}

	void Services::GetServiceSecurity(
		const WWuString& serviceName,
		const WWuString& computerName,
		WWuString& sddl,
		LPDWORD pdwSize,
		bool bAudit
	)
	{
		DWORD dwResult { };
		PrivilegeCookie privilegeCookie;

		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		DWORD dwServiceAccess = READ_CONTROL;
		if (bAudit) {
			secInfo |= SACL_SECURITY_INFORMATION;
			dwServiceAccess |= ACCESS_SYSTEM_SECURITY;
			privilegeCookie = PrivilegeCookie::Ensure(SE_SECURITY_NAME);
		}

		ScmHandle hScm { computerName, SC_MANAGER_CONNECT };
		ScmHandle hService { hScm, computerName, serviceName, dwServiceAccess };

		PSECURITY_DESCRIPTOR secDesc;
		if (dwResult = GetSecurityInfo(hService.Get(), SE_OBJECT_TYPE::SE_SERVICE, secInfo, nullptr, nullptr, nullptr, nullptr, &secDesc) != ERROR_SUCCESS)
			_WU_RAISE_NATIVE_EXCEPTION(dwResult, L"GetSecurityInfo", WriteErrorCategory::InvalidResult);

		LPWSTR cSddl;
		ULONG len;
		ConvertSecurityDescriptorToStringSecurityDescriptorW(secDesc, SDDL_REVISION_1, secInfo, &cSddl, &len);

		sddl = WWuString(cSddl);
		LocalFree(secDesc);
		LocalFree(cSddl);
	}

	void Services::GetServiceSecurity(
		SC_HANDLE hService,
		WWuString& sddl,
		LPDWORD pdwSize,
		bool bAudit
	)
	{
		DWORD bytesNeeded { };
		PrivilegeCookie privilegeCookie;

		SECURITY_INFORMATION secInfo { OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION };
		DWORD dwServiceAccess { READ_CONTROL };
		if (bAudit) {
			secInfo |= SACL_SECURITY_INFORMATION;
			dwServiceAccess |= ACCESS_SYSTEM_SECURITY;
			privilegeCookie = PrivilegeCookie::Ensure(SE_SECURITY_NAME);
		}

		PSECURITY_DESCRIPTOR secDesc;
		if (DWORD dwResult = GetSecurityInfo(hService, SE_SERVICE, secInfo, nullptr, nullptr, nullptr, nullptr, &secDesc) != ERROR_SUCCESS)
			_WU_RAISE_NATIVE_EXCEPTION(dwResult, L"GetSecurityInfo", WriteErrorCategory::InvalidResult);

		LPWSTR cSddl;
		ULONG len;
		ConvertSecurityDescriptorToStringSecurityDescriptorW(secDesc, SDDL_REVISION_1, secInfo, &cSddl, &len);

		sddl = WWuString(cSddl);
		LocalFree(secDesc);
		LocalFree(cSddl);
	}

	void Services::SetServiceSecurity(
		const WWuString& serviceName,		// The service name.
		const WWuString& computerName,		// The computer name where to set the service security.
		const WWuString& sddl,				// The SDDL representation of the security descriptor to set.
		bool changeAudit,					// TRUE to change SACL.
		bool changeOwner					// TRUE to change the owner.
	)
	{
		DWORD dwResult { };
		DWORD dwSzSecDesc;
		PSECURITY_DESCRIPTOR pSecDesc;

		if (!ConvertStringSecurityDescriptorToSecurityDescriptor(sddl.Raw(), SDDL_REVISION_1, &pSecDesc, &dwSzSecDesc))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"ConvertStringSecurityDescriptorToSecurityDescriptor", WriteErrorCategory::InvalidResult);

		LPCWSTR privilegeList[2]{ };
		DWORD dwSvcAccess { WRITE_DAC | WRITE_OWNER };
		DWORD dwSecInfo { DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION };
		if (changeOwner) {
			privilegeList[0] = SE_TAKE_OWNERSHIP_NAME;
			dwSecInfo |= OWNER_SECURITY_INFORMATION;
		}
		if (changeAudit) {
			privilegeList[1] = SE_SECURITY_NAME;
			dwSvcAccess |= ACCESS_SYSTEM_SECURITY;
			dwSecInfo |= SACL_SECURITY_INFORMATION;
		}

		PrivilegeCookie privilegeCookie = PrivilegeCookie::Ensure(privilegeList[0], privilegeList[1]);

		ScmHandle hScm { computerName, SC_MANAGER_CONNECT };
		ScmHandle hService { hScm, computerName, serviceName, dwSvcAccess };

		if (!SetServiceObjectSecurity(hService.Get(), dwSecInfo, pSecDesc))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"SetServiceObjectSecurity", WriteErrorCategory::InvalidResult);

		if (pSecDesc != NULL)
			LocalFree(pSecDesc);
	}

	/*=========================================
	==			 Utility functions			 ==
	===========================================*/

	// Queries and stops services dependent of a given service.
	void Services::StopDependentServices(
		const ScmHandle& hScm,						// Handle to the Service Control Manager. Used to open dependent services.
		const ScmHandle& hService,					// Handle to the service we want to query dependence.
		const WWuString& computerName,				// Computer name. In cases where we inherit the service handle from the pipeline.
		bool noWait,								// Doesn't wait for the services to stop.
		const WuNativeContext* context				// A native representation of the Cmdlet context.
	)
	{
		DWORD bytesNeeded;
		DWORD serviceCount;

		ULONGLONG starttime = GetTickCount64();
		DWORD timeout = 30000; // 30 seconds.

		// Determining buffer size.
		// If this call succeeds, there are no dependent services.
		if (EnumDependentServices(hService.Get(), SERVICE_ACTIVE, NULL, 0, &bytesNeeded, &serviceCount))
			return;

		SERVICE_STATUS serviceStatus { };
		ScopedBuffer buffer{ bytesNeeded };
		if (!EnumDependentServices(hService.Get(), SERVICE_ACTIVE, reinterpret_cast<LPENUM_SERVICE_STATUS>(buffer.Get()), bytesNeeded, &bytesNeeded, &serviceCount))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"EnumDependentServices", WriteErrorCategory::InvalidResult);

		// Going through the service array and closing them.
		auto serviceList = reinterpret_cast<LPENUM_SERVICE_STATUS>(buffer.Get());
		for (DWORD i = 0; i < serviceCount; i++) {
			
			// If we fail to stop a single service, the original will be marked
			// to deletion anyways. No reason to keep up.
			ScmHandle currentService;
			try {
				currentService = ScmHandle{ hScm, computerName, serviceList[i].lpServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS };
			}
			catch (const WuNativeException&) {
				return;
			}

			if (noWait)
				ControlService(currentService.Get(), SERVICE_CONTROL_STOP, &serviceStatus);
			else
				StopServiceWithWarning(currentService, hScm, serviceList[i].lpServiceName, serviceStatus, context);
		}
	}

	void Services::StopServiceWithWarning(const ScmHandle& hService, const ScmHandle& hScm, const WWuString& serviceName, SERVICE_STATUS& serviceStatus, const WuNativeContext* context)
	{
		DWORD result;
		DWORD charCount;

		if (!GetServiceDisplayName(hScm.Get(), serviceName.Raw(), NULL, &charCount)) {
			result = GetLastError();
			if (result != ERROR_INSUFFICIENT_BUFFER)
				_WU_RAISE_NATIVE_EXCEPTION(result, L"GetServiceDisplayName", WriteErrorCategory::InvalidResult);
			else
				result = ERROR_SUCCESS;
		}

		charCount++;
		std::unique_ptr<WCHAR[]> nameBuffer = std::make_unique<WCHAR[]>(charCount);
		if (!GetServiceDisplayName(hScm.Get(), serviceName.Raw(), nameBuffer.get(), &charCount))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"GetServiceDisplayName", WriteErrorCategory::InvalidResult);

		WWuString warningText = WWuString::Format(L"Waiting for service '%ws (%ws)' to stop...", nameBuffer.get(), serviceName.Raw());
		do {
			if (serviceStatus.dwCurrentState != SERVICE_STOP_PENDING && serviceStatus.dwCurrentState != SERVICE_START_PENDING && serviceStatus.dwCurrentState != SERVICE_STOPPED) {
				if (!ControlService(hService.Get(), SERVICE_CONTROL_STOP, &serviceStatus)) {
					result = GetLastError();

					// Checking if the service is already stopped.
					if (result == ERROR_SERVICE_NOT_ACTIVE)
						break;

					if (result != ERROR_SERVICE_CANNOT_ACCEPT_CTRL)
						_WU_RAISE_NATIVE_EXCEPTION(result, L"ControlService", WriteErrorCategory::InvalidResult);
				}
			}

			if (serviceStatus.dwCurrentState == SERVICE_STOP_PENDING) {
				Sleep(2145);
				context->NativeWriteWarning(warningText);
				if (!QueryServiceStatus(hService.Get(), &serviceStatus))
					_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"QueryServiceStatus", WriteErrorCategory::InvalidResult);
			}
			else {
				if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
					break;

				warningText = WWuString::Format(L"Failed to stop service '%ws (%ws)'. Service will be marked to deletion.", nameBuffer.get(), serviceName);
				context->NativeWriteWarning(warningText);
				break;
			}

		} while (serviceStatus.dwCurrentState != SERVICE_STOPPED);
	}
}