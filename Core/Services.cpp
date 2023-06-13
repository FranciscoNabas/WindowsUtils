#include "pch.h"
#include "Services.h"
#include "Utilities.h"

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

	DWORD Services::GetServiceSecurity(const LPWSTR& serviceName, const LPWSTR& computerName, PSECURITY_DESCRIPTOR& pSvcSecurity, LPDWORD pdwSize)
	{
		DWORD result = ERROR_SUCCESS;
		SharedVecPtr(SC_HANDLE) sc_handles = MakeVecPtr(SC_HANDLE);

		WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

		SC_HANDLE hScm = ::OpenSCManager(computerName, L"ServicesActive", SC_MANAGER_CONNECT);
		if (hScm == NULL)
			return ::GetLastError();
		sc_handles->push_back(hScm);

		SC_HANDLE hService = ::OpenService(hScm, serviceName, READ_CONTROL);
		if (hService == NULL)
		{
			result = ::GetLastError();
			goto CLEANUP;
		}
		sc_handles->push_back(hService);

		if (!::QueryServiceObjectSecurity(hService, DACL_SECURITY_INFORMATION, NULL, 0, pdwSize))
		{
			result = ::GetLastError();
			if (result != ERROR_INSUFFICIENT_BUFFER)
				goto CLEANUP;
			else
				result = ERROR_SUCCESS;
		}

		pSvcSecurity = (PSECURITY_DESCRIPTOR)MemoryManager.Allocate(*pdwSize);
		if (!::QueryServiceObjectSecurity(hService, DACL_SECURITY_INFORMATION, pSvcSecurity, *pdwSize, pdwSize))
		{
			result = ::GetLastError();
			goto CLEANUP;
		}

	CLEANUP:
		for (SC_HANDLE handle : *sc_handles)
			::CloseServiceHandle(handle);

		return result;
	}

	DWORD Services::GetServiceSecurity(SC_HANDLE& hService, PSECURITY_DESCRIPTOR& pSvcSecurity, LPDWORD pdwSize)
	{
		DWORD result = ERROR_SUCCESS;
		DWORD bytesNeeded = 0;

		WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

		if (!::QueryServiceObjectSecurity(hService, DACL_SECURITY_INFORMATION, NULL, 0, &bytesNeeded))
		{
			result = ::GetLastError();
			if (result != ERROR_INSUFFICIENT_BUFFER)
				return result;
			else
				result = ERROR_SUCCESS;
		}

		pSvcSecurity = (PSECURITY_DESCRIPTOR)MemoryManager.Allocate(bytesNeeded);
		if (!::QueryServiceObjectSecurity(hService, DACL_SECURITY_INFORMATION, pSvcSecurity, bytesNeeded, &bytesNeeded))
			return ::GetLastError();

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