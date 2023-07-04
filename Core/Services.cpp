#include "pch.h"
#include "Services.h"

namespace WindowsUtils::Core
{
	DWORD Services::RemoveService(
		const LPWSTR& servicename		// The service name.
		, const LPWSTR& computername		// Optional computer name.
		, BOOL stopservice				// Stops the service, if it's running.
	)
	{
		DWORD result = ERROR_SUCCESS;
		SC_HANDLE hscmanager;
		SC_HANDLE hservice;
		LPSERVICE_STATUS lpservicestatus;
		DWORD svcdesaccess;

		WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

		// Trying to give the least amount of privileges as possible, depending on the request.
		if (stopservice)
			svcdesaccess = DELETE | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_STOP;
		else
			svcdesaccess = DELETE;

		hscmanager = ::OpenSCManagerW(computername, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
		if (NULL == hscmanager)
			return ::GetLastError();

		hservice = ::OpenServiceW(hscmanager, servicename, svcdesaccess);
		if (NULL == hservice)
		{
			result = ::GetLastError();
			goto CLEANUP;
		}

		if (stopservice == TRUE)
		{
			lpservicestatus = (LPSERVICE_STATUS)MemoryManager.Allocate(sizeof(SERVICE_STATUS));

			if (!::QueryServiceStatus(hservice, lpservicestatus))
			{
				result = ::GetLastError();
				goto CLEANUP;
			}

			if (lpservicestatus->dwCurrentState != SERVICE_STOPPED && lpservicestatus->dwCurrentState != SERVICE_STOP_PENDING)
			{
				result = StopDependentServices(hscmanager, hservice, NULL);
				if (result != ERROR_SUCCESS)
					goto CLEANUP;

				result = StopServiceWithTimeout(hservice, lpservicestatus);
				if (result != ERROR_SUCCESS)
					goto CLEANUP;
			}
		}

		if (!::DeleteService(hservice))
			result = ::GetLastError();

	CLEANUP:

		if (NULL != hscmanager)
			::CloseServiceHandle(hscmanager);
		if (NULL != hservice)
			::CloseServiceHandle(hservice);
		MemoryManager.Free(lpservicestatus);

		return result;
	}

	DWORD Services::RemoveService(SC_HANDLE& hservice, const LPWSTR& computername, BOOL stopservice)
	{
		DWORD result = ERROR_SUCCESS;
		LPSERVICE_STATUS lpservicestatus;
		SC_HANDLE scm = NULL;

		WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

		if (stopservice == TRUE)
		{
			lpservicestatus = (LPSERVICE_STATUS)MemoryManager.Allocate(sizeof(SERVICE_STATUS));

			if (!::QueryServiceStatus(hservice, lpservicestatus))
			{
				result = GetLastError();
				goto CLEANUP;
			}

			if (lpservicestatus->dwCurrentState != SERVICE_STOPPED && lpservicestatus->dwCurrentState != SERVICE_STOP_PENDING)
			{
				DWORD waittime = 0;

				result = StopDependentServices(scm, hservice, computername);
				if (result != ERROR_SUCCESS)
					goto CLEANUP;

				result = StopServiceWithTimeout(hservice, lpservicestatus);
				if (result != ERROR_SUCCESS)
					goto CLEANUP;
			}
		}

		if (!::DeleteService(hservice))
			result = ::GetLastError();

	CLEANUP:

		if (NULL != hservice)
			::CloseServiceHandle(hservice);
		MemoryManager.Free(lpservicestatus);

		return result;
	}

	DWORD Services::GetServiceSecurity(
		const LPWSTR& serviceName,
		const LPWSTR& computerName,
		PSECURITY_DESCRIPTOR& pSvcSecurity,
		LPDWORD pdwSize,
		BOOL bAudit
	)
	{
		DWORD result = ERROR_SUCCESS;
		SharedVecPtr(SC_HANDLE) sc_handles = MakeVecPtr(SC_HANDLE);
		SharedVecPtr(LPCWSTR) privilegeList = MakeVecPtr(LPCWSTR);
		
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

		SC_HANDLE hScm = ::OpenSCManager(computerName, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
		if (hScm == NULL)
			return ::GetLastError();
		sc_handles->push_back(hScm);

		SC_HANDLE hService = ::OpenServiceW(hScm, serviceName, dwServiceAccess);
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
		SC_HANDLE& hService,
		PSECURITY_DESCRIPTOR& pSvcSecurity,
		LPDWORD pdwSize,
		BOOL bAudit
	)
	{
		DWORD result = ERROR_SUCCESS;
		DWORD bytesNeeded = 0;
		SharedVecPtr(LPCWSTR) privilegeList = MakeVecPtr(LPCWSTR);

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
		const LPWSTR& lpszServiceName,		// The service name.
		const LPWSTR& lpszComputerName,		// The computer name where to set the service security.
		const LPWSTR& lpszSddl,				// The SDDL representation of the security descriptor to set.
		BOOL bChangeAudit,					// TRUE to change SACL.
		BOOL bChangeOwner					// TRUE to change the owner.
	)
	{
		DWORD result = ERROR_SUCCESS;
		DWORD dwSzSecDesc;
		PSECURITY_DESCRIPTOR pSecDesc;
		SharedVecPtr(LPCWSTR) privilegeList = MakeVecPtr(LPCWSTR);
		SharedVecPtr(SC_HANDLE) scHandles = MakeVecPtr(SC_HANDLE);

		if (!ConvertStringSecurityDescriptorToSecurityDescriptor(lpszSddl, SDDL_REVISION_1, &pSecDesc, &dwSzSecDesc))
			return GetLastError();

		DWORD dwSvcAccess = WRITE_DAC | WRITE_OWNER;
		DWORD dwSecInfo = DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION;
		if (bChangeOwner)
		{
			privilegeList->push_back(SE_TAKE_OWNERSHIP_NAME);
			dwSecInfo |= OWNER_SECURITY_INFORMATION;
		}
		if (bChangeAudit)
		{
			privilegeList->push_back(SE_SECURITY_NAME);
			dwSvcAccess |= ACCESS_SYSTEM_SECURITY;
			dwSecInfo |= SACL_SECURITY_INFORMATION;
		}
		AccessControl::AdjustCurrentTokenPrivilege(privilegeList, SE_PRIVILEGE_ENABLED);

		SC_HANDLE hScm = OpenSCManager(lpszComputerName, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
		if (hScm == NULL)
		{
			result = GetLastError();
			goto CLEANUP;
		}
		scHandles->push_back(hScm);

		SC_HANDLE hService = OpenService(hScm, lpszServiceName, dwSvcAccess);
		if (hService == NULL)
		{
			result = GetLastError();
			goto CLEANUP;
		}
		scHandles->push_back(hService);

		if (!SetServiceObjectSecurity(hService, dwSecInfo, pSecDesc))
			result = GetLastError();

	CLEANUP:
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
		SC_HANDLE& scm					// Handle to the Service Control Manager. Used to open dependent services.
		, SC_HANDLE& hservice			// Handle to the service we want to query dependence.
		, const LPWSTR& computername		// Computer name. In cases where we inherit the service handle from the pipeline.
	)
	{
		DWORD result = ERROR_SUCCESS;
		DWORD bytesneeded;
		DWORD svccount;
		SERVICE_STATUS ssp;
		BOOL localscm = FALSE;

		ULONGLONG starttime = GetTickCount64();
		DWORD timeout = 30000; // 30 seconds.

		WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

		/*
		* Determining buffer size.
		* If this call succeeds, there are no dependent services.
		*/
		if (::EnumDependentServicesW(hservice, SERVICE_ACTIVE, NULL, 0, &bytesneeded, &svccount))
			return result;

		if (NULL == scm)
		{
			scm = ::OpenSCManagerW(computername, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
			localscm = TRUE;
		}

		LPENUM_SERVICE_STATUS servicelist = (LPENUM_SERVICE_STATUS)MemoryManager.Allocate(bytesneeded);
		__try
		{
			if (!::EnumDependentServicesW(hservice, SERVICE_ACTIVE, servicelist, bytesneeded, &bytesneeded, &svccount))
			{
				result = GetLastError();
				__leave;
			}

			// Going through the service array and closing them.
			for (DWORD i = 0; i < svccount; i++)
			{
				SC_HANDLE svccurrent = ::OpenServiceW(scm, servicelist[i].lpServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS);
				if (!svccurrent)
				{
					result = GetLastError();
					__leave;
				}

				__try
				{
					result = StopServiceWithTimeout(svccurrent, &ssp);
					if (result == ERROR_SERVICE_NOT_ACTIVE)
					{
						result = ERROR_SUCCESS;
						__leave;
					}
					if (result != ERROR_SUCCESS)
						__leave;

					do
					{
						::Sleep(ssp.dwWaitHint);
						if (!::QueryServiceStatus(svccurrent, &ssp))
						{
							result = GetLastError();
							__leave;
						}

						if (ssp.dwCurrentState == SERVICE_STOPPED)
							break;

						if (GetTickCount64() - starttime > timeout)
						{
							result = ERROR_TIMEOUT;
							__leave;
						}

					} while (ssp.dwCurrentState != SERVICE_STOPPED);
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
			::CloseServiceHandle(scm);

		return result;
	}

	DWORD StopServiceWithTimeout(SC_HANDLE& hservice, LPSERVICE_STATUS lpsvcstatus)
	{
		DWORD result = ERROR_SUCCESS;
		ULONGLONG starttime = GetTickCount64();
		DWORD timeout = 3000;

		do
		{
			if (!::ControlService(hservice, SERVICE_CONTROL_STOP, lpsvcstatus))
			{
				result = GetLastError();

				/*
				* ARGH! Check if the DARN thang is already stopped!
				* That took me a while.
				*/
				if (result == ERROR_SERVICE_NOT_ACTIVE)
					break;

				if (result != ERROR_SERVICE_CANNOT_ACCEPT_CTRL)
					return result;
			}
			else
				break;

			::Sleep(lpsvcstatus->dwWaitHint);

			if (GetTickCount64() - starttime > timeout)
				return ERROR_TIMEOUT;

		} while (result == ERROR_SERVICE_CANNOT_ACCEPT_CTRL);

		return result;
	}
}