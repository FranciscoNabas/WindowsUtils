#include "pch.h"
#include "Services.h"

namespace WindowsUtils::Core
{
	DWORD Services::RemoveService(
		const LPWSTR& servicename,						// The service name.
		const LPWSTR& computername,						// Optional computer name.
		BOOL stopservice,								// Stops the service, if it's running.
		Notification::PNATIVE_CONTEXT context			// A native representation of the Cmdlet context.
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
				result = StopDependentServices(hscmanager, hservice, NULL, context);
				if (result != ERROR_SUCCESS)
					goto CLEANUP;

				result = StopServiceWithWarning(hservice, hscmanager, servicename, lpservicestatus, context);
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

	DWORD Services::RemoveService(SC_HANDLE& hservice, const LPWSTR& servicename, const LPWSTR& computername, BOOL stopservice, Notification::PNATIVE_CONTEXT context)
	{
		DWORD result = ERROR_SUCCESS;
		LPSERVICE_STATUS lpservicestatus;
		SC_HANDLE scm = NULL;

		WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

		scm = ::OpenSCManager(computername, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
		if (scm == NULL)
			return ::GetLastError();

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

				result = StopDependentServices(scm, hservice, computername, context);
				if (result != ERROR_SUCCESS)
					goto CLEANUP;

				result = StopServiceWithWarning(hservice, scm, servicename, lpservicestatus, context);
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
		SC_HANDLE& scm,									// Handle to the Service Control Manager. Used to open dependent services.
		SC_HANDLE& hservice,							// Handle to the service we want to query dependence.
		const LPWSTR& computername,						// Computer name. In cases where we inherit the service handle from the pipeline.
		Notification::PNATIVE_CONTEXT context			// A native representation of the Cmdlet context.
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
					result = StopServiceWithWarning(svccurrent, scm, servicelist[i].lpServiceName , &ssp, context);
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

	DWORD StopServiceWithWarning(SC_HANDLE& hservice, SC_HANDLE& scm, const LPWSTR& lpszSvcName, LPSERVICE_STATUS lpsvcstatus, Notification::PNATIVE_CONTEXT context)
	{
		DWORD result = ERROR_SUCCESS;
		DWORD dwChars = 0;

		LPWSTR lpszDisplayName;
		if (!GetServiceDisplayName(scm, lpszSvcName, NULL, &dwChars))
		{
			result = GetLastError();
			if (result != ERROR_INSUFFICIENT_BUFFER)
				return result;
			else
				result = ERROR_SUCCESS;
		}

		lpszDisplayName = new WCHAR[dwChars + 1];
		dwChars += 1;
		if (!GetServiceDisplayName(scm, lpszSvcName, lpszDisplayName, &dwChars))
		{
			delete[] lpszDisplayName;
			return GetLastError();
		}

		size_t svcNameSz = wcslen(lpszSvcName) + 1;
		size_t svcDisplayNameSz = wcslen(lpszDisplayName) + 1;
		size_t totalSize = svcDisplayNameSz + svcNameSz + 38;

		LPWSTR warningString = new WCHAR[totalSize];
		_snwprintf_s(warningString, totalSize, _TRUNCATE, L"Waiting for service '%ws (%ws)' to stop...", lpszDisplayName, lpszSvcName);

		do
		{
			if (lpsvcstatus->dwCurrentState != SERVICE_STOP_PENDING && lpsvcstatus->dwCurrentState != SERVICE_START_PENDING && lpsvcstatus->dwCurrentState != SERVICE_STOPPED)
			{
				if (!::ControlService(hservice, SERVICE_CONTROL_STOP, lpsvcstatus))
				{
					result = GetLastError();

					// Checking if the service is already stopped.
					if (result == ERROR_SERVICE_NOT_ACTIVE)
						break;

					if (result != ERROR_SERVICE_CANNOT_ACCEPT_CTRL)
						return result;
				}
			}

			if (lpsvcstatus->dwCurrentState == SERVICE_STOP_PENDING)
			{
				::Sleep(2145);
				NativeWriteWarning(context, warningString);
				if (!QueryServiceStatus(hservice, lpsvcstatus))
				{
					result = GetLastError();
					break;
				}
			}
			else
			{
				if (lpsvcstatus->dwCurrentState == SERVICE_STOPPED)
					break;

				totalSize = svcDisplayNameSz + svcNameSz + 66;
				LPWSTR warningString2 = new WCHAR[totalSize];
				_snwprintf_s(warningString2, totalSize, _TRUNCATE, L"Failed to stop service '%ws (%ws)'. Service will be marked to deletion.", lpszDisplayName, lpszSvcName);
				NativeWriteWarning(context, warningString2);
			}

		} while (lpsvcstatus->dwCurrentState != SERVICE_STOPPED);

		delete[] warningString;
		delete[] lpszDisplayName;

		return result;
	}
}