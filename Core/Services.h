#pragma once
#pragma unmanaged

#include "Utilities.h"
#include "AccessControl.h"

namespace WindowsUtils::Core
{
	extern "C" public class __declspec(dllexport) Services
	{
	public:

		/*=========================================
		==		 Function identification		 ==
		===========================================*/

		// Remove-Service
		DWORD RemoveService(const LPWSTR& servicename, const LPWSTR& computername, BOOL stopservice);
		DWORD RemoveService(SC_HANDLE& hservice, const LPWSTR& computername, BOOL stopservice);

		// Get-ServiceSecurity
		DWORD GetServiceSecurity(const LPWSTR& serviceName, const LPWSTR& computerName, PSECURITY_DESCRIPTOR& pSvcSecurity, LPDWORD pdwSize, BOOL bAudit = FALSE);
		DWORD GetServiceSecurity(SC_HANDLE& serviceName, PSECURITY_DESCRIPTOR& pSvcSecurity, LPDWORD pdwSize, BOOL bAudit = FALSE);

		// Set-ServiceSecurity
		DWORD SetServiceSecurity(const LPWSTR& lpszServiceName, const LPWSTR& lpszComputerName, const LPWSTR& lpszSddl, BOOL bChangeAudit, BOOL bChangeOwner);
	};

	DWORD StopDependentServices(SC_HANDLE& scm, SC_HANDLE& hservice, const LPWSTR& computername);
	DWORD StopServiceWithTimeout(SC_HANDLE& hservice, LPSERVICE_STATUS lpsvcstatus);
}