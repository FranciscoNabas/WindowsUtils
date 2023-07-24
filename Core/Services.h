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
		DWORD RemoveService(const WuString& servicename, const WuString& computername, BOOL stopservice, Notification::PNATIVE_CONTEXT context);
		DWORD RemoveService(SC_HANDLE hservice, const WuString& servicename, const WuString& computername, BOOL stopservice, Notification::PNATIVE_CONTEXT context);

		// Get-ServiceSecurity
		DWORD GetServiceSecurity(const WuString& serviceName, const WuString& computerName, PSECURITY_DESCRIPTOR pSvcSecurity, LPDWORD pdwSize, BOOL bAudit = FALSE);
		DWORD GetServiceSecurity(SC_HANDLE serviceName, PSECURITY_DESCRIPTOR pSvcSecurity, LPDWORD pdwSize, BOOL bAudit = FALSE);

		// Set-ServiceSecurity
		DWORD SetServiceSecurity(const WuString& lpszServiceName, const WuString& lpszComputerName, const WuString& lpszSddl, BOOL bChangeAudit, BOOL bChangeOwner);
	};

	DWORD StopDependentServices(SC_HANDLE scm, SC_HANDLE hservice, const WuString& computername, Notification::PNATIVE_CONTEXT context);
	DWORD StopServiceWithWarning(SC_HANDLE hservice, SC_HANDLE scm, const WuString& lpszSvcName, LPSERVICE_STATUS lpsvcstatus, Notification::PNATIVE_CONTEXT context);
}