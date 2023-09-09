#pragma once
#pragma unmanaged

#include "Utilities.h"
#include "SafeHandle.h"
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
		void RemoveService(const WWuString& servicename, const WWuString& computername, BOOL stopservice, WuNativeContext* context, bool noWait);

		// Get-ServiceSecurity
		WuResult GetServiceSecurity(const WWuString& serviceName, const WWuString& computerName, WWuString& sddl, LPDWORD pdwSize, BOOL bAudit = FALSE);
		WuResult GetServiceSecurity(SC_HANDLE serviceName, WWuString& sddl, LPDWORD pdwSize, BOOL bAudit = FALSE);

		// Set-ServiceSecurity
		WuResult SetServiceSecurity(const WWuString& lpszServiceName, const WWuString& lpszComputerName, const WWuString& lpszSddl, BOOL bChangeAudit, BOOL bChangeOwner);
	};

	void StopDependentServices(const ScmHandle& scm, const ScmHandle& hservice, const WWuString& computername, WuNativeContext* context, bool noWait);
	void StopServiceWithWarning(const ScmHandle& hservice, const ScmHandle& scm, const WWuString& lpszSvcName, LPSERVICE_STATUS lpsvcstatus, WuNativeContext* context);
}