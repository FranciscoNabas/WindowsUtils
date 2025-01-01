#pragma once
#pragma unmanaged

#include "AccessControl.h"

#include "../Support/WuString.h"
#include "../Support/Notification.h"
#include "../Support/SafeHandle.h"
#include "../Support/WuException.h"

#include <AclAPI.h>
#include <sddl.h>
#include <memory>

namespace WindowsUtils::Core
{
	class Services
	{
	public:
		// Remove-Service
		static void RemoveService(const WWuString& servicename, const WWuString& computername, bool stopservice, bool noWait, const WuNativeContext* context);

		// Get-ServiceSecurity
		static void GetServiceSecurity(const WWuString& serviceName, const WWuString& computerName, WWuString& sddl, LPDWORD pdwSize, bool bAudit = false);
		static void GetServiceSecurity(SC_HANDLE serviceHandle, WWuString& sddl, LPDWORD pdwSize, bool bAudit = FALSE);

		// Set-ServiceSecurity
		static void SetServiceSecurity(const WWuString& lpszServiceName, const WWuString& lpszComputerName, const WWuString& lpszSddl, bool bChangeAudit, bool bChangeOwner);

	private:
		static void StopDependentServices(const ScmHandle& scm, const ScmHandle& hservice, const WWuString& computername, bool noWait, const WuNativeContext* context);
		static void StopServiceWithWarning(const ScmHandle& hservice, const ScmHandle& scm, const WWuString& lpszSvcName, SERVICE_STATUS& lpsvcstatus, const WuNativeContext* context);
	};
}