#include "..\pch.h"
#include "..\Headers\Services.h"

namespace WindowsUtils::Core
{
	WuResult Services::RemoveService(
		const WWuString& serviceName,					// The service name.
		const WWuString& computerName,					// Optional computer name.
		BOOL stopService,								// Stops the service, if it's running.
		WuNativeContext* context						// A native representation of the Cmdlet context.
	)
	{
		SC_HANDLE hScm;
		SC_HANDLE hService;
		DWORD desiredAccess;
		std::shared_ptr<SERVICE_STATUS> serviceStatus;

		// Trying to give the least amount of privileges as possible, depending on the request.
		if (stopService)
			desiredAccess = DELETE | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_STOP;
		else
			desiredAccess = DELETE;

		hScm = OpenSCManagerW(computerName.GetBuffer(), SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
		if (NULL == hScm)
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		hService = OpenServiceW(hScm, serviceName.GetBuffer(), desiredAccess);
		if (NULL == hService)
		{
			CloseServiceHandle(hScm);
			return WuResult(GetLastError(), __FILEW__, __LINE__);
		}

		if (stopService == TRUE)
		{
			serviceStatus = std::make_shared<SERVICE_STATUS>();

			if (!QueryServiceStatus(hService, serviceStatus.get()))
			{
				CloseServiceHandle(hService);
				CloseServiceHandle(hScm);
				return WuResult(GetLastError(), __FILEW__, __LINE__);
			}

			if (serviceStatus.get()->dwCurrentState != SERVICE_STOPPED && serviceStatus.get()->dwCurrentState != SERVICE_STOP_PENDING)
			{
				WuResult result = StopDependentServices(hScm, hService, L"", context);
				if (result.Result != ERROR_SUCCESS) {
					CloseServiceHandle(hService);
					CloseServiceHandle(hScm);
					return result;
				}

				result = StopServiceWithWarning(hService, hScm, serviceName, serviceStatus.get(), context);
				if (result.Result != ERROR_SUCCESS) {
					CloseServiceHandle(hService);
					CloseServiceHandle(hScm);
					return result;
				}
			}
		}

		if (!DeleteService(hService)) {
			CloseServiceHandle(hService);
			CloseServiceHandle(hScm);
			return WuResult(GetLastError(), __FILEW__, __LINE__);
		}

		if (NULL != hScm)
			CloseServiceHandle(hScm);
		if (NULL != hService)
			CloseServiceHandle(hService);

		return WuResult();
	}

	WuResult Services::RemoveService(SC_HANDLE hService, const WWuString& serviceName, const WWuString& computerName, BOOL stopService, WuNativeContext* context)
	{
		SC_HANDLE hScm = NULL;

		hScm = OpenSCManager(computerName.GetBuffer(), SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
		if (hScm == NULL)
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		std::shared_ptr<SERVICE_STATUS> serviceStatus;
		if (stopService)
		{
			serviceStatus = std::make_shared<SERVICE_STATUS>();
			if (!QueryServiceStatus(hService, serviceStatus.get()))
			{
				CloseServiceHandle(hScm);
				return WuResult(GetLastError(), __FILEW__, __LINE__);
			}

			if (serviceStatus.get()->dwCurrentState != SERVICE_STOPPED && serviceStatus.get()->dwCurrentState != SERVICE_STOP_PENDING)
			{
				WuResult result = StopDependentServices(hScm, hService, computerName, context);
				if (result.Result != ERROR_SUCCESS) {
					CloseServiceHandle(hScm);
					return result;
				}

				result = StopServiceWithWarning(hService, hScm, serviceName, serviceStatus.get(), context);
				if (result.Result != ERROR_SUCCESS) {
					CloseServiceHandle(hScm);
					return result;
				}
			}
		}

		if (!DeleteService(hService)) {
			CloseServiceHandle(hScm);
			return WuResult(GetLastError(), __FILEW__, __LINE__);
		}


		if (NULL != hScm)
			CloseServiceHandle(hScm);

		return WuResult();
	}

	WuResult Services::GetServiceSecurity(
		const WWuString& serviceName,
		const WWuString& computerName,
		WWuString& sddl,
		LPDWORD pdwSize,
		BOOL bAudit
	) {
		WuResult result;
		wusunique_vector<SC_HANDLE> sc_handles = make_wusunique_vector<SC_HANDLE>();
		wusunique_vector<WWuString> privilegeList = make_wusunique_vector<WWuString>();
		
		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		DWORD dwServiceAccess = READ_CONTROL;
		if (bAudit)
		{
			secInfo |= SACL_SECURITY_INFORMATION;
			privilegeList->push_back(SE_SECURITY_NAME);
			
			result = AccessControl::AdjustCurrentTokenPrivilege(privilegeList.get(), SE_PRIVILEGE_ENABLED);
			if (result.Result != ERROR_SUCCESS)
				return result;

			dwServiceAccess |= ACCESS_SYSTEM_SECURITY;
		}

		SC_HANDLE hScm = ::OpenSCManager(computerName.GetBuffer(), SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
		if (hScm == NULL) {
			result = WuResult(GetLastError(), __FILEW__, __LINE__);
			goto CLEANUP;
		}
		sc_handles->push_back(hScm);

		SC_HANDLE hService = ::OpenServiceW(hScm, serviceName.GetBuffer(), dwServiceAccess);
		if (hService == NULL)
		{
			result = WuResult(GetLastError(), __FILEW__, __LINE__);
			goto CLEANUP;
		}
		sc_handles->push_back(hService);

		PSECURITY_DESCRIPTOR secDesc;
		DWORD dwResult = GetSecurityInfo(hService, SE_SERVICE, secInfo, NULL, NULL, NULL, NULL, &secDesc);
		if (dwResult != ERROR_SUCCESS)
			result = WuResult(dwResult, __FILEW__, __LINE__);

		LPWSTR cSddl;
		ULONG len;
		ConvertSecurityDescriptorToStringSecurityDescriptorW(secDesc, SDDL_REVISION_1, secInfo, &cSddl, &len);

		sddl = WWuString(cSddl);
		LocalFree(secDesc);
		LocalFree(cSddl);

	CLEANUP:
		if (bAudit)
			AccessControl::AdjustCurrentTokenPrivilege(privilegeList.get(), SE_PRIVILEGE_DISABLED);

		for (SC_HANDLE handle : *sc_handles)
			CloseServiceHandle(handle);

		return result;
	}

	WuResult Services::GetServiceSecurity(
		SC_HANDLE hService,
		WWuString& sddl,
		LPDWORD pdwSize,
		BOOL bAudit
	) {
		WuResult result;
		DWORD bytesNeeded = 0;
		wusunique_vector<WWuString> privilegeList = make_wusunique_vector<WWuString>();

		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		DWORD dwServiceAccess = READ_CONTROL;
		if (bAudit)
		{
			secInfo |= SACL_SECURITY_INFORMATION;
			privilegeList->push_back(SE_SECURITY_NAME);
			
			result = AccessControl::AdjustCurrentTokenPrivilege(privilegeList.get(), SE_PRIVILEGE_ENABLED);
			if (result.Result != ERROR_SUCCESS)
				return result;

			dwServiceAccess |= ACCESS_SYSTEM_SECURITY;
		}

		PSECURITY_DESCRIPTOR secDesc;
		DWORD dwResult = GetSecurityInfo(hService, SE_SERVICE, secInfo, NULL, NULL, NULL, NULL, &secDesc);
		if (dwResult != ERROR_SUCCESS)
			result = WuResult(dwResult, __FILEW__, __LINE__);

		LPWSTR cSddl;
		ULONG len;
		ConvertSecurityDescriptorToStringSecurityDescriptorW(secDesc, SDDL_REVISION_1, secInfo, &cSddl, &len);

		sddl = WWuString(cSddl);
		LocalFree(secDesc);
		LocalFree(cSddl);

		if (bAudit)
			AccessControl::AdjustCurrentTokenPrivilege(privilegeList.get(), SE_PRIVILEGE_DISABLED);

		return result;
	}

	WuResult Services::SetServiceSecurity(
		const WWuString& serviceName,		// The service name.
		const WWuString& computerName,		// The computer name where to set the service security.
		const WWuString& sddl,				// The SDDL representation of the security descriptor to set.
		BOOL changeAudit,					// TRUE to change SACL.
		BOOL changeOwner					// TRUE to change the owner.
	) {
		WuResult result;
		DWORD dwSzSecDesc;
		PSECURITY_DESCRIPTOR pSecDesc;
		
		wusunique_vector<SC_HANDLE> scHandles = make_wusunique_vector<SC_HANDLE>();
		wusunique_vector<WWuString> privilegeList = make_wusunique_vector<WWuString>();

		if (!ConvertStringSecurityDescriptorToSecurityDescriptor(sddl.GetBuffer(), SDDL_REVISION_1, &pSecDesc, &dwSzSecDesc))
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		DWORD dwSvcAccess = WRITE_DAC | WRITE_OWNER;
		DWORD dwSecInfo = DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION;
		if (changeOwner)
		{
			privilegeList->push_back(SE_TAKE_OWNERSHIP_NAME);
			dwSecInfo |= OWNER_SECURITY_INFORMATION;
		}
		if (changeAudit)
		{
			privilegeList->push_back(SE_SECURITY_NAME);
			dwSvcAccess |= ACCESS_SYSTEM_SECURITY;
			dwSecInfo |= SACL_SECURITY_INFORMATION;
		}
		result = AccessControl::AdjustCurrentTokenPrivilege(privilegeList.get(), SE_PRIVILEGE_ENABLED);
		if (result.Result != ERROR_SUCCESS)
			return result;

		SC_HANDLE hScm = OpenSCManager(computerName.GetBuffer(), SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
		if (hScm == NULL)
		{
			result = WuResult(GetLastError(), __FILEW__, __LINE__);
			goto CLEANUP;
		}
		scHandles->push_back(hScm);

		SC_HANDLE hService = OpenService(hScm, serviceName.GetBuffer(), dwSvcAccess);
		if (hService == NULL)
		{
			result = WuResult(GetLastError(), __FILEW__, __LINE__);
			goto CLEANUP;
		}
		scHandles->push_back(hService);

		if (!SetServiceObjectSecurity(hService, dwSecInfo, pSecDesc))
			result = WuResult(GetLastError(), __FILEW__, __LINE__);

	CLEANUP:
		if (pSecDesc != NULL)
			LocalFree(pSecDesc);

		AccessControl::AdjustCurrentTokenPrivilege(privilegeList.get(), SE_PRIVILEGE_DISABLED);
		for (SC_HANDLE hScObject : *scHandles)
			CloseServiceHandle(hScObject);

		return result;
	}

	/*=========================================
	==			 Utility functions			 ==
	===========================================*/

	// Queries and stops services dependent of a given service.
	WuResult StopDependentServices(
		SC_HANDLE hScm,								// Handle to the Service Control Manager. Used to open dependent services.
		SC_HANDLE hService,							// Handle to the service we want to query dependence.
		const WWuString& computerName,				// Computer name. In cases where we inherit the service handle from the pipeline.
		WuNativeContext* context					// A native representation of the Cmdlet context.
	)
	{
		WuResult result;
		DWORD bytesNeeded;
		DWORD serviceCount;
		BOOL localScm = FALSE;

		ULONGLONG starttime = GetTickCount64();
		DWORD timeout = 30000; // 30 seconds.

		/*
		* Determining buffer size.
		* If this call succeeds, there are no dependent services.
		*/
		if (EnumDependentServices(hService, SERVICE_ACTIVE, NULL, 0, &bytesNeeded, &serviceCount))
			return result;

		if (hScm == NULL)
		{
			hScm = OpenSCManager(computerName.GetBuffer(), SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
			if (hScm == NULL)
				return WuResult(GetLastError(), __FILEW__, __LINE__);
			
			localScm = TRUE;
		}

		wuunique_ptr<SERVICE_STATUS> serviceStatus = make_wuunique<SERVICE_STATUS>();
		wuunique_ha_ptr<ENUM_SERVICE_STATUS> serviceList = make_wuunique_ha<ENUM_SERVICE_STATUS>(bytesNeeded);
		
		if (!EnumDependentServices(hService, SERVICE_ACTIVE, serviceList.get(), bytesNeeded, &bytesNeeded, &serviceCount))
		{
			CloseServiceHandle(hScm);
			return WuResult(GetLastError(), __FILEW__, __LINE__);
		}

		// Going through the service array and closing them.
		for (DWORD i = 0; i < serviceCount; i++)
		{
			SC_HANDLE currentService = OpenServiceW(hScm, serviceList.get()[i].lpServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS);
			
			// If we fail to stop a single service, the original will be marked
			// to deletion anyways. No reason to keep up.
			if (currentService == NULL)
			{
				CloseServiceHandle(hScm);
				return WuResult(GetLastError(), __FILEW__, __LINE__);
			}

			result = StopServiceWithWarning(currentService, hScm, serviceList.get()[i].lpServiceName, serviceStatus.get(), context);
			if (result.Result == ERROR_SERVICE_NOT_ACTIVE)
				result = WuResult();
			
			if (result.Result != ERROR_SUCCESS)
			{
				CloseServiceHandle(currentService);
				CloseServiceHandle(hScm);
				return result;
			}

			CloseServiceHandle(currentService);
		}

		if (hScm)
			CloseServiceHandle(hScm);

		return result;
	}

	WuResult StopServiceWithWarning(SC_HANDLE hService, SC_HANDLE hScm, const WWuString& serviceName, LPSERVICE_STATUS serviceStatus, WuNativeContext* context)
	{
		WuResult result;
		DWORD dwResult;
		DWORD dwChars = 0;

		if (!GetServiceDisplayName(hScm, serviceName.GetBuffer(), NULL, &dwChars))
		{
			dwResult = GetLastError();
			if (dwResult != ERROR_INSUFFICIENT_BUFFER)
				return WuResult(dwResult, __FILEW__, __LINE__);
			else
				dwResult = ERROR_SUCCESS;
		}

		dwChars += 1;
		DWORD bytesNeeded = dwChars * 2;
		wuunique_ha_ptr<WCHAR> dnameBuffer = make_wuunique_ha<WCHAR>(bytesNeeded);
		if (!GetServiceDisplayName(hScm, serviceName.GetBuffer(), dnameBuffer.get(), &dwChars))
			return WuResult(GetLastError(), __FILEW__, __LINE__);

		WWuString displayName(dnameBuffer.get());
		WWuString warningText = WWuString::Format(L"Waiting for service '%ws (%ws)' to stop...", displayName, serviceName.GetBuffer());

		do
		{
			if (serviceStatus->dwCurrentState != SERVICE_STOP_PENDING && serviceStatus->dwCurrentState != SERVICE_START_PENDING && serviceStatus->dwCurrentState != SERVICE_STOPPED)
			{
				if (!ControlService(hService, SERVICE_CONTROL_STOP, serviceStatus))
				{
					dwResult = GetLastError();

					// Checking if the service is already stopped.
					if (dwResult == ERROR_SERVICE_NOT_ACTIVE)
						break;

					if (dwResult != ERROR_SERVICE_CANNOT_ACCEPT_CTRL)
						return WuResult(dwResult, __FILEW__, __LINE__);;
				}
			}

			if (serviceStatus->dwCurrentState == SERVICE_STOP_PENDING)
			{
				// '2145' is the closes I could get to the waiting warning in
				// 'Stop-Service' without getting into the code.
				Sleep(2145);
				context->NativeWriteWarning(warningText);
				if (!QueryServiceStatus(hService, serviceStatus))
				{
					result = WuResult(GetLastError(), __FILEW__, __LINE__);
					break;
				}
			}
			else
			{
				if (serviceStatus->dwCurrentState == SERVICE_STOPPED)
					break;

				warningText = WWuString::Format(L"Failed to stop service '%ws (%ws)'. Service will be marked to deletion.", displayName, serviceName);
				context->NativeWriteWarning(warningText);
				break;
			}

		} while (serviceStatus->dwCurrentState != SERVICE_STOPPED);

		return result;
	}
}