#pragma once
#pragma unmanaged

#include "../Support/String.h"
#include "../Support/Notification.h"
#include "../Support/SafeHandle.h"

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
		void GetServiceSecurity(const WWuString& serviceName, const WWuString& computerName, WWuString& sddl, LPDWORD pdwSize, BOOL bAudit = FALSE);
		void GetServiceSecurity(SC_HANDLE serviceName, WWuString& sddl, LPDWORD pdwSize, BOOL bAudit = FALSE);

		// Set-ServiceSecurity
		void SetServiceSecurity(const WWuString& lpszServiceName, const WWuString& lpszComputerName, const WWuString& lpszSddl, BOOL bChangeAudit, BOOL bChangeOwner);
	};

	void StopDependentServices(const ScmHandle& scm, const ScmHandle& hservice, const WWuString& computername, WuNativeContext* context, bool noWait);
	void StopServiceWithWarning(const ScmHandle& hservice, const ScmHandle& scm, const WWuString& lpszSvcName, LPSERVICE_STATUS lpsvcstatus, WuNativeContext* context);
}