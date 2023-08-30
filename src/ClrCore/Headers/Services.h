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
		WuResult RemoveService(const WWuString& servicename, const WWuString& computername, BOOL stopservice, WuNativeContext* context);
		WuResult RemoveService(SC_HANDLE hservice, const WWuString& servicename, const WWuString& computername, BOOL stopservice, WuNativeContext* context);

		// Get-ServiceSecurity
		WuResult GetServiceSecurity(const WWuString& serviceName, const WWuString& computerName, WWuString& sddl, LPDWORD pdwSize, BOOL bAudit = FALSE);
		WuResult GetServiceSecurity(SC_HANDLE serviceName, WWuString& sddl, LPDWORD pdwSize, BOOL bAudit = FALSE);

		// Set-ServiceSecurity
		WuResult SetServiceSecurity(const WWuString& lpszServiceName, const WWuString& lpszComputerName, const WWuString& lpszSddl, BOOL bChangeAudit, BOOL bChangeOwner);
	};

	WuResult StopDependentServices(SC_HANDLE scm, SC_HANDLE hservice, const WWuString& computername, WuNativeContext* context);
	WuResult StopServiceWithWarning(SC_HANDLE hservice, SC_HANDLE scm, const WWuString& lpszSvcName, LPSERVICE_STATUS lpsvcstatus, WuNativeContext* context);
}