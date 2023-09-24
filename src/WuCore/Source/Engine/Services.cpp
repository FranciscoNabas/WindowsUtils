#include "../../pch.h"

#include "../../Headers/Engine/Services.h"
#include "../../Headers/Engine/AccessControl.h"
#include "../../Headers/Support/WuStdException.h"

#include <AclAPI.h>
#include <sddl.h>

namespace WindowsUtils::Core
{
	void Services::RemoveService(
		const WWuString& serviceName,					// The service name.
		const WWuString& computerName,					// Optional computer name.
		BOOL stopService,								// Stops the service, if it's running.
		WuNativeContext* context,						// A native representation of the Cmdlet context.
		bool noWait										// Doesn't wait for the services to stop.
	)
	{
		DWORD desiredAccess;
		std::shared_ptr<SERVICE_STATUS> serviceStatus;

		// Trying to give the least amount of privileges as possible, depending on the request.
		if (stopService)
			desiredAccess = DELETE | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_STOP;
		else
			desiredAccess = DELETE;

		ScmHandle hScm(computerName, SC_MANAGER_CONNECT);
		ScmHandle hService(hScm, computerName, serviceName, desiredAccess);

		if (stopService == TRUE) {
			serviceStatus = std::make_shared<SERVICE_STATUS>();

			if (!QueryServiceStatus(hService.get(), serviceStatus.get()))
				throw WuStdException(GetLastError(), __FILEW__, __LINE__);

			if (serviceStatus.get()->dwCurrentState != SERVICE_STOPPED && serviceStatus.get()->dwCurrentState != SERVICE_STOP_PENDING) {
				StopDependentServices(hScm, hService, computerName, context, noWait);

				if (noWait)
					ControlService(hService.get(), SERVICE_CONTROL_STOP, serviceStatus.get());
				else
					StopServiceWithWarning(hService, hScm, serviceName, serviceStatus.get(), context);
			}
		}

		if (!DeleteService(hService.get()))
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);
	}

	void Services::GetServiceSecurity(
		const WWuString& serviceName,
		const WWuString& computerName,
		WWuString& sddl,
		LPDWORD pdwSize,
		BOOL bAudit
	)
	{
		DWORD dwResult { };
		SC_HANDLE hService { };
		wusunique_vector<SC_HANDLE> sc_handles = make_wusunique_vector<SC_HANDLE>();
		wusunique_vector<WWuString> privilegeList = make_wusunique_vector<WWuString>();

		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		DWORD dwServiceAccess = READ_CONTROL;
		if (bAudit) {
			secInfo |= SACL_SECURITY_INFORMATION;
			privilegeList->push_back(SE_SECURITY_NAME);

			try {
				AccessControl::AdjustCurrentTokenPrivilege(*privilegeList, SE_PRIVILEGE_ENABLED);
			}
			catch (const WuStdException& ex) {
				throw WuStdException(ex.ErrorCode(), __FILEW__, __LINE__);
			}


			dwServiceAccess |= ACCESS_SYSTEM_SECURITY;
		}

		SC_HANDLE hScm = ::OpenSCManager(computerName.GetBuffer(), SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
		if (hScm == NULL) {
			dwResult = GetLastError();
			goto CLEANUP;
		}
		sc_handles->push_back(hScm);

		hService = ::OpenServiceW(hScm, serviceName.GetBuffer(), dwServiceAccess);
		if (hService == NULL) {
			dwResult = GetLastError();
			goto CLEANUP;
		}
		sc_handles->push_back(hService);

		PSECURITY_DESCRIPTOR secDesc;
		dwResult = GetSecurityInfo(hService, SE_SERVICE, secInfo, NULL, NULL, NULL, NULL, &secDesc);
		if (dwResult != ERROR_SUCCESS)
			dwResult = GetLastError();

		LPWSTR cSddl;
		ULONG len;
		ConvertSecurityDescriptorToStringSecurityDescriptorW(secDesc, SDDL_REVISION_1, secInfo, &cSddl, &len);

		sddl = WWuString(cSddl);
		LocalFree(secDesc);
		LocalFree(cSddl);

	CLEANUP:
		if (bAudit)
			AccessControl::AdjustCurrentTokenPrivilege(*privilegeList, SE_PRIVILEGE_DISABLED);

		for (SC_HANDLE handle : *sc_handles)
			CloseServiceHandle(handle);

		if (dwResult != ERROR_SUCCESS)
			throw WuStdException(dwResult, __FILEW__, __LINE__);
	}

	void Services::GetServiceSecurity(
		SC_HANDLE hService,
		WWuString& sddl,
		LPDWORD pdwSize,
		BOOL bAudit
	)
	{
		DWORD bytesNeeded { };
		wusunique_vector<WWuString> privilegeList { make_wusunique_vector<WWuString>() };

		SECURITY_INFORMATION secInfo { OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION };
		DWORD dwServiceAccess { READ_CONTROL };
		if (bAudit) {
			secInfo |= SACL_SECURITY_INFORMATION;
			privilegeList->push_back(SE_SECURITY_NAME);

			AccessControl::AdjustCurrentTokenPrivilege(*privilegeList, SE_PRIVILEGE_ENABLED);

			dwServiceAccess |= ACCESS_SYSTEM_SECURITY;
		}

		PSECURITY_DESCRIPTOR secDesc;
		DWORD dwResult = GetSecurityInfo(hService, SE_SERVICE, secInfo, NULL, NULL, NULL, NULL, &secDesc);
		if (dwResult != ERROR_SUCCESS)
			throw WuStdException { static_cast<int>(dwResult), __FILEW__, __LINE__ };

		LPWSTR cSddl;
		ULONG len;
		ConvertSecurityDescriptorToStringSecurityDescriptorW(secDesc, SDDL_REVISION_1, secInfo, &cSddl, &len);

		sddl = WWuString(cSddl);
		LocalFree(secDesc);
		LocalFree(cSddl);

		if (bAudit)
			AccessControl::AdjustCurrentTokenPrivilege(*privilegeList, SE_PRIVILEGE_DISABLED);
	}

	void Services::SetServiceSecurity(
		const WWuString& serviceName,		// The service name.
		const WWuString& computerName,		// The computer name where to set the service security.
		const WWuString& sddl,				// The SDDL representation of the security descriptor to set.
		BOOL changeAudit,					// TRUE to change SACL.
		BOOL changeOwner					// TRUE to change the owner.
	)
	{
		DWORD dwResult { };
		DWORD dwSzSecDesc;
		PSECURITY_DESCRIPTOR pSecDesc;
		SC_HANDLE hService { };

		wusunique_vector<SC_HANDLE> scHandles = make_wusunique_vector<SC_HANDLE>();
		wusunique_vector<WWuString> privilegeList = make_wusunique_vector<WWuString>();

		if (!ConvertStringSecurityDescriptorToSecurityDescriptor(sddl.GetBuffer(), SDDL_REVISION_1, &pSecDesc, &dwSzSecDesc))
			throw WuStdException { static_cast<int>(GetLastError()), __FILEW__, __LINE__ };

		DWORD dwSvcAccess { WRITE_DAC | WRITE_OWNER };
		DWORD dwSecInfo { DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION };
		if (changeOwner) {
			privilegeList->push_back(SE_TAKE_OWNERSHIP_NAME);
			dwSecInfo |= OWNER_SECURITY_INFORMATION;
		}
		if (changeAudit) {
			privilegeList->push_back(SE_SECURITY_NAME);
			dwSvcAccess |= ACCESS_SYSTEM_SECURITY;
			dwSecInfo |= SACL_SECURITY_INFORMATION;
		}

		AccessControl::AdjustCurrentTokenPrivilege(*privilegeList, SE_PRIVILEGE_ENABLED);

		SC_HANDLE hScm = OpenSCManager(computerName.GetBuffer(), SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
		if (hScm == NULL) {
			dwResult = GetLastError();
			goto CLEANUP;
		}
		scHandles->push_back(hScm);

		hService = OpenService(hScm, serviceName.GetBuffer(), dwSvcAccess);
		if (hService == NULL) {
			dwResult = GetLastError();
			goto CLEANUP;
		}
		scHandles->push_back(hService);

		if (!SetServiceObjectSecurity(hService, dwSecInfo, pSecDesc))
			dwResult = GetLastError();

	CLEANUP:
		if (pSecDesc != NULL)
			LocalFree(pSecDesc);

		AccessControl::AdjustCurrentTokenPrivilege(*privilegeList, SE_PRIVILEGE_DISABLED);
		for (SC_HANDLE hScObject : *scHandles)
			CloseServiceHandle(hScObject);

		if (dwResult != ERROR_SUCCESS)
			throw WuStdException { static_cast<int>(dwResult), __FILEW__, __LINE__ };
	}

	/*=========================================
	==			 Utility functions			 ==
	===========================================*/

	// Queries and stops services dependent of a given service.
	void StopDependentServices(
		const ScmHandle& hScm,						// Handle to the Service Control Manager. Used to open dependent services.
		const ScmHandle& hService,					// Handle to the service we want to query dependence.
		const WWuString& computerName,				// Computer name. In cases where we inherit the service handle from the pipeline.
		WuNativeContext* context,					// A native representation of the Cmdlet context.
		bool noWait									// Doesn't wait for the services to stop.
	)
	{
		DWORD bytesNeeded;
		DWORD serviceCount;

		ULONGLONG starttime = GetTickCount64();
		DWORD timeout = 30000; // 30 seconds.

		/*
		* Determining buffer size.
		* If this call succeeds, there are no dependent services.
		*/
		if (EnumDependentServices(hService.get(), SERVICE_ACTIVE, NULL, 0, &bytesNeeded, &serviceCount))
			return;

		wuunique_ptr<SERVICE_STATUS> serviceStatus = make_wuunique<SERVICE_STATUS>();
		wuunique_ha_ptr<ENUM_SERVICE_STATUS> serviceList = make_wuunique_ha<ENUM_SERVICE_STATUS>(bytesNeeded);

		if (!EnumDependentServices(hService.get(), SERVICE_ACTIVE, serviceList.get(), bytesNeeded, &bytesNeeded, &serviceCount))
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		// Going through the service array and closing them.
		for (DWORD i = 0; i < serviceCount; i++) {
			// If we fail to stop a single service, the original will be marked
			// to deletion anyways. No reason to keep up.
			ScmHandle currentService;
			try {
				currentService = ScmHandle(hScm, computerName, serviceList.get()[i].lpServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS);
			}
			catch (const WuStdException&) {
				return;
			}

			if (noWait)
				ControlService(currentService.get(), SERVICE_CONTROL_STOP, serviceStatus.get());
			else
				StopServiceWithWarning(currentService, hScm, serviceList.get()[i].lpServiceName, serviceStatus.get(), context);
		}
	}

	void StopServiceWithWarning(const ScmHandle& hService, const ScmHandle& hScm, const WWuString& serviceName, LPSERVICE_STATUS serviceStatus, WuNativeContext* context)
	{
		DWORD dwResult;
		DWORD dwChars = 0;

		if (!GetServiceDisplayName(hScm.get(), serviceName.GetBuffer(), NULL, &dwChars)) {
			dwResult = GetLastError();
			if (dwResult != ERROR_INSUFFICIENT_BUFFER)
				throw WuStdException(dwResult, __FILEW__, __LINE__);
			else
				dwResult = ERROR_SUCCESS;
		}

		dwChars += 1;
		DWORD bytesNeeded = dwChars * 2;
		wuunique_ha_ptr<WCHAR> nameBuffer = make_wuunique_ha<WCHAR>(bytesNeeded);
		if (!GetServiceDisplayName(hScm.get(), serviceName.GetBuffer(), nameBuffer.get(), &dwChars))
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		WWuString displayName(nameBuffer.get());
		WWuString warningText = WWuString::Format(L"Waiting for service '%ws (%ws)' to stop...", displayName.GetBuffer(), serviceName.GetBuffer());

		do {
			if (serviceStatus->dwCurrentState != SERVICE_STOP_PENDING && serviceStatus->dwCurrentState != SERVICE_START_PENDING && serviceStatus->dwCurrentState != SERVICE_STOPPED) {
				if (!ControlService(hService.get(), SERVICE_CONTROL_STOP, serviceStatus)) {
					dwResult = GetLastError();

					// Checking if the service is already stopped.
					if (dwResult == ERROR_SERVICE_NOT_ACTIVE)
						break;

					if (dwResult != ERROR_SERVICE_CANNOT_ACCEPT_CTRL)
						throw WuStdException(dwResult, __FILEW__, __LINE__);;
				}
			}

			if (serviceStatus->dwCurrentState == SERVICE_STOP_PENDING) {
				// '2145' is the closes I could get to the waiting warning in
				// 'Stop-Service' without getting into the code.
				Sleep(2145);
				context->NativeWriteWarning(warningText);
				if (!QueryServiceStatus(hService.get(), serviceStatus))
					throw WuStdException(GetLastError(), __FILEW__, __LINE__);
			}
			else {
				if (serviceStatus->dwCurrentState == SERVICE_STOPPED)
					break;

				warningText = WWuString::Format(L"Failed to stop service '%ws (%ws)'. Service will be marked to deletion.", displayName, serviceName);
				context->NativeWriteWarning(warningText);
				break;
			}

		} while (serviceStatus->dwCurrentState != SERVICE_STOPPED);
	}
}