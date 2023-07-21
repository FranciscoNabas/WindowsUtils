#include "pch.h"
#include "Services.h"

namespace WindowsUtils::Core
{
	DWORD Services::RemoveService(
		const WuString& serviceName,					// The service name.
		const WuString& computerName,					// Optional computer name.
		BOOL stopService,								// Stops the service, if it's running.
		Notification::PNATIVE_CONTEXT context			// A native representation of the Cmdlet context.
	)
	{
		DWORD result = ERROR_SUCCESS;
		SC_HANDLE hScm;
		SC_HANDLE hService;
		DWORD svcdesaccess;

		// Trying to give the least amount of privileges as possible, depending on the request.
		if (stopService)
			svcdesaccess = DELETE | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_STOP;
		else
			svcdesaccess = DELETE;

		hScm = ::OpenSCManagerW(computerName.GetWideBuffer(), SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
		if (NULL == hScm)
			return ::GetLastError();

		hService = ::OpenServiceW(hScm, serviceName.GetWideBuffer(), svcdesaccess);
		if (NULL == hService)
		{
			result = ::GetLastError();
			goto CLEANUP;
		}

		std::shared_ptr<SERVICE_STATUS> serviceStatus;
		if (stopService == TRUE)
		{
			serviceStatus = std::make_shared<SERVICE_STATUS>();

			if (!::QueryServiceStatus(hService, serviceStatus.get()))
			{
				result = ::GetLastError();
				goto CLEANUP;
			}

			if (serviceStatus.get()->dwCurrentState != SERVICE_STOPPED && serviceStatus.get()->dwCurrentState != SERVICE_STOP_PENDING)
			{
				result = StopDependentServices(hScm, hService, L"", context);
				if (result != ERROR_SUCCESS)
					goto CLEANUP;

				result = StopServiceWithWarning(hService, hScm, serviceName, serviceStatus, context);
				if (result != ERROR_SUCCESS)
					goto CLEANUP;
			}
		}

		if (!::DeleteService(hService))
			result = ::GetLastError();

	CLEANUP:

		if (NULL != hScm)
			::CloseServiceHandle(hScm);
		if (NULL != hService)
			::CloseServiceHandle(hService);

		return result;
	}

	DWORD Services::RemoveService(SC_HANDLE hService, const WuString& serviceName, const WuString& computerName, BOOL stopService, Notification::PNATIVE_CONTEXT context)
	{
		DWORD result = ERROR_SUCCESS;
		SC_HANDLE hScm = NULL;

		hScm = ::OpenSCManager(computerName.GetWideBuffer(), SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
		if (hScm == NULL)
			return ::GetLastError();

		std::shared_ptr<SERVICE_STATUS> serviceStatus;
		if (stopService)
		{
			serviceStatus = std::make_shared<SERVICE_STATUS>();
			if (!::QueryServiceStatus(hService, serviceStatus.get()))
			{
				result = GetLastError();
				goto CLEANUP;
			}

			if (serviceStatus.get()->dwCurrentState != SERVICE_STOPPED && serviceStatus.get()->dwCurrentState != SERVICE_STOP_PENDING)
			{
				DWORD waittime = 0;

				result = StopDependentServices(hScm, hService, computerName, context);
				if (result != ERROR_SUCCESS)
					goto CLEANUP;

				result = StopServiceWithWarning(hService, hScm, serviceName, serviceStatus, context);
				if (result != ERROR_SUCCESS)
					goto CLEANUP;
			}
		}

		if (!::DeleteService(hService))
			result = ::GetLastError();

	CLEANUP:

		if (NULL != hService)
			::CloseServiceHandle(hService);

		return result;
	}

	DWORD Services::GetServiceSecurity(
		const WuString& serviceName,
		const WuString& computerName,
		PSECURITY_DESCRIPTOR pSvcSecurity,
		LPDWORD pdwSize,
		BOOL bAudit
	)
	{
		DWORD result = ERROR_SUCCESS;
		SharedVecPtr(SC_HANDLE) sc_handles = MakeVecPtr(SC_HANDLE);
		SharedVecPtr(WuString) privilegeList = MakeVecPtr(WuString);
		
		WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();
		
		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		DWORD dwServiceAccess = READ_CONTROL;
		if (bAudit)
		{
			secInfo |= SACL_SECURITY_INFORMATION;
			privilegeList->push_back(SE_SECURITY_NAME);
			
			result = AccessControl::AdjustCurrentTokenPrivilege(privilegeList, SE_PRIVILEGE_ENABLED);
			if (result != ERROR_SUCCESS)
				return result;

			dwServiceAccess |= ACCESS_SYSTEM_SECURITY;
		}

		SC_HANDLE hScm = ::OpenSCManager(computerName.GetWideBuffer(), SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
		if (hScm == NULL)
			return ::GetLastError();
		sc_handles->push_back(hScm);

		SC_HANDLE hService = ::OpenServiceW(hScm, serviceName.GetWideBuffer(), dwServiceAccess);
		if (hService == NULL)
		{
			result = ::GetLastError();
			goto CLEANUP;
		}
		sc_handles->push_back(hService);

		result = GetSecurityInfo(hService, SE_SERVICE, secInfo, NULL, NULL, NULL, NULL, &pSvcSecurity);
		if (result != ERROR_SUCCESS)
			goto CLEANUP;

	CLEANUP:
		if (bAudit)
			AccessControl::AdjustCurrentTokenPrivilege(privilegeList, SE_PRIVILEGE_DISABLED);

		for (SC_HANDLE handle : *sc_handles)
			::CloseServiceHandle(handle);

		return result;
	}

	DWORD Services::GetServiceSecurity(
		SC_HANDLE hService,
		PSECURITY_DESCRIPTOR pSvcSecurity,
		LPDWORD pdwSize,
		BOOL bAudit
	)
	{
		DWORD result = ERROR_SUCCESS;
		DWORD bytesNeeded = 0;
		SharedVecPtr(WuString) privilegeList = MakeVecPtr(WuString);

		WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

		SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
		DWORD dwServiceAccess = READ_CONTROL;
		if (bAudit)
		{
			secInfo |= SACL_SECURITY_INFORMATION;
			privilegeList->push_back(SE_SECURITY_NAME);
			
			result = AccessControl::AdjustCurrentTokenPrivilege(privilegeList, SE_PRIVILEGE_ENABLED);
			if (result != ERROR_SUCCESS)
				return result;

			dwServiceAccess |= ACCESS_SYSTEM_SECURITY;
		}

		result = GetSecurityInfo(hService, SE_SERVICE, secInfo, NULL, NULL, NULL, NULL, &pSvcSecurity);
		if (result != ERROR_SUCCESS)
			return result;

		if (bAudit)
			result = AccessControl::AdjustCurrentTokenPrivilege(privilegeList, SE_PRIVILEGE_DISABLED);

		return result;
	}

	DWORD Services::SetServiceSecurity(
		const WuString& serviceName,		// The service name.
		const WuString& computerName,		// The computer name where to set the service security.
		const WuString& sddl,				// The SDDL representation of the security descriptor to set.
		BOOL changeAudit,					// TRUE to change SACL.
		BOOL changeOwner					// TRUE to change the owner.
	)
	{
		DWORD result = ERROR_SUCCESS;
		DWORD dwSzSecDesc;
		PSECURITY_DESCRIPTOR pSecDesc;
		SharedVecPtr(WuString) privilegeList = MakeVecPtr(WuString);
		SharedVecPtr(SC_HANDLE) scHandles = MakeVecPtr(SC_HANDLE);

		if (!ConvertStringSecurityDescriptorToSecurityDescriptor(sddl.GetWideBuffer(), SDDL_REVISION_1, &pSecDesc, &dwSzSecDesc))
			return GetLastError();

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
		AccessControl::AdjustCurrentTokenPrivilege(privilegeList, SE_PRIVILEGE_ENABLED);

		SC_HANDLE hScm = OpenSCManager(computerName.GetWideBuffer(), SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
		if (hScm == NULL)
		{
			result = GetLastError();
			goto CLEANUP;
		}
		scHandles->push_back(hScm);

		SC_HANDLE hService = OpenService(hScm, serviceName.GetWideBuffer(), dwSvcAccess);
		if (hService == NULL)
		{
			result = GetLastError();
			goto CLEANUP;
		}
		scHandles->push_back(hService);

		if (!SetServiceObjectSecurity(hService, dwSecInfo, pSecDesc))
			result = GetLastError();

	CLEANUP:
		if (pSecDesc != NULL)
			LocalFree(pSecDesc);

		AccessControl::AdjustCurrentTokenPrivilege(privilegeList, SE_PRIVILEGE_DISABLED);
		for (SC_HANDLE hScObject : *scHandles)
			CloseServiceHandle(hScObject);

		return result;
	}

	/*=========================================
	==			 Utility functions			 ==
	===========================================*/

	// Queries and stops services dependent of a given service.
	DWORD StopDependentServices(
		SC_HANDLE hScm,								// Handle to the Service Control Manager. Used to open dependent services.
		SC_HANDLE hService,							// Handle to the service we want to query dependence.
		const WuString& computerName,				// Computer name. In cases where we inherit the service handle from the pipeline.
		Notification::PNATIVE_CONTEXT context		// A native representation of the Cmdlet context.
	)
	{
		DWORD result = ERROR_SUCCESS;
		DWORD bytesneeded;
		DWORD svccount;
		BOOL localscm = FALSE;

		ULONGLONG starttime = GetTickCount64();
		DWORD timeout = 30000; // 30 seconds.

		WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

		/*
		* Determining buffer size.
		* If this call succeeds, there are no dependent services.
		*/
		if (::EnumDependentServicesW(hService, SERVICE_ACTIVE, NULL, 0, &bytesneeded, &svccount))
			return result;

		if (NULL == hScm)
		{
			hScm = ::OpenSCManagerW(computerName.GetWideBuffer(), SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
			localscm = TRUE;
		}

		std::shared_ptr<SERVICE_STATUS> serviceStatus = std::make_shared<SERVICE_STATUS>();
		LPENUM_SERVICE_STATUS servicelist = (LPENUM_SERVICE_STATUS)MemoryManager.Allocate(bytesneeded);
		__try
		{
			if (!::EnumDependentServicesW(hService, SERVICE_ACTIVE, servicelist, bytesneeded, &bytesneeded, &svccount))
			{
				result = GetLastError();
				__leave;
			}

			// Going through the service array and closing them.
			for (DWORD i = 0; i < svccount; i++)
			{
				SC_HANDLE svccurrent = ::OpenServiceW(hScm, servicelist[i].lpServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS);
				if (!svccurrent)
				{
					result = GetLastError();
					__leave;
				}

				__try
				{
					result = StopServiceWithWarning(svccurrent, hScm, servicelist[i].lpServiceName , serviceStatus, context);
					if (result == ERROR_SERVICE_NOT_ACTIVE)
					{
						result = ERROR_SUCCESS;
						__leave;
					}
					if (result != ERROR_SUCCESS)
						__leave;

					do
					{
						::Sleep(serviceStatus->dwWaitHint);
						if (!::QueryServiceStatus(svccurrent, serviceStatus.get()))
						{
							result = GetLastError();
							__leave;
						}

						if (serviceStatus->dwCurrentState == SERVICE_STOPPED)
							break;

						if (GetTickCount64() - starttime > timeout)
						{
							result = ERROR_TIMEOUT;
							__leave;
						}

					} while (serviceStatus->dwCurrentState != SERVICE_STOPPED);
				}
				__finally
				{
					::CloseServiceHandle(svccurrent);
				}
			}
		}
		__finally
		{
			MemoryManager.Free(servicelist);
		}

		if (localscm)
			::CloseServiceHandle(hScm);

		return result;
	}

	DWORD StopServiceWithWarning(SC_HANDLE hService, SC_HANDLE hScm, const WuString& serviceName, std::shared_ptr<SERVICE_STATUS> serviceStatus, Notification::PNATIVE_CONTEXT context)
	{
		DWORD result = ERROR_SUCCESS;
		DWORD dwChars = 0;

		if (!GetServiceDisplayName(hScm, serviceName.GetWideBuffer(), NULL, &dwChars))
		{
			result = GetLastError();
			if (result != ERROR_INSUFFICIENT_BUFFER)
				return result;
			else
				result = ERROR_SUCCESS;
		}

		dwChars += 1;
		WuString displayName(dwChars);
		if (!GetServiceDisplayName(hScm, serviceName.GetWideBuffer(), displayName.GetWideBuffer(), &dwChars))
			return GetLastError();

		WuString warningText;
		warningText.Format(L"Waiting for service '%ws (%ws)' to stop...", displayName, serviceName.GetWideBuffer());

		do
		{
			if (serviceStatus->dwCurrentState != SERVICE_STOP_PENDING && serviceStatus->dwCurrentState != SERVICE_START_PENDING && serviceStatus->dwCurrentState != SERVICE_STOPPED)
			{
				if (!::ControlService(hService, SERVICE_CONTROL_STOP, serviceStatus.get()))
				{
					result = GetLastError();

					// Checking if the service is already stopped.
					if (result == ERROR_SERVICE_NOT_ACTIVE)
						break;

					if (result != ERROR_SERVICE_CANNOT_ACCEPT_CTRL)
						return result;
				}
			}

			if (serviceStatus->dwCurrentState == SERVICE_STOP_PENDING)
			{
				::Sleep(2145);
				NativeWriteWarning(context, warningText);
				if (!QueryServiceStatus(hService, serviceStatus.get()))
				{
					result = GetLastError();
					break;
				}
			}
			else
			{
				if (serviceStatus->dwCurrentState == SERVICE_STOPPED)
					break;

				warningText.Format(L"Failed to stop service '%ws (%ws)'. Service will be marked to deletion.", displayName, serviceName);
				NativeWriteWarning(context, warningText);
				break;
			}

		} while (serviceStatus->dwCurrentState != SERVICE_STOPPED);

		return result;
	}
}