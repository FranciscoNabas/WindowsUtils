#pragma once
#pragma unmanaged

#include "Utilities.h"
#include "Notification.h"
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
		DWORD RemoveService(const WWuString& servicename, const WWuString& computername, BOOL stopservice, Notification::PNATIVE_CONTEXT context);
		DWORD RemoveService(SC_HANDLE hservice, const WWuString& servicename, const WWuString& computername, BOOL stopservice, Notification::PNATIVE_CONTEXT context);

		// Get-ServiceSecurity
		DWORD GetServiceSecurity(const WWuString& serviceName, const WWuString& computerName, PSECURITY_DESCRIPTOR pSvcSecurity, LPDWORD pdwSize, BOOL bAudit = FALSE);
		DWORD GetServiceSecurity(SC_HANDLE serviceName, PSECURITY_DESCRIPTOR pSvcSecurity, LPDWORD pdwSize, BOOL bAudit = FALSE);

		// Set-ServiceSecurity
		DWORD SetServiceSecurity(const WWuString& lpszServiceName, const WWuString& lpszComputerName, const WWuString& lpszSddl, BOOL bChangeAudit, BOOL bChangeOwner);
	};

	DWORD StopDependentServices(SC_HANDLE scm, SC_HANDLE hservice, const WWuString& computername, Notification::PNATIVE_CONTEXT context);
	DWORD StopServiceWithWarning(SC_HANDLE hservice, SC_HANDLE scm, const WWuString& lpszSvcName, LPSERVICE_STATUS lpsvcstatus, Notification::PNATIVE_CONTEXT context);
}