#pragma once
#pragma unmanaged

#include "../Engine/Services.h"

#pragma managed

#include "CmdletContextProxy.h"

namespace WindowsUtils::Wrappers
{
	public ref class ServicesWrapper
	{
	public:
		// Remove-Service
		void RemoveService(String^ servicename, String^ computerName, bool stopservice, Core::CmdletContextProxy^ context, bool noWait);
		void RemoveService(String^ servicename, bool stopservice, Core::CmdletContextProxy^ context, bool noWait);

		// Get-ServiceSecurity
		String^ GetServiceSecurityDescriptorString(String^ serviceName, String^ computerName, bool audit);
		String^ GetServiceSecurityDescriptorString(String^ serviceName, bool audit);
		String^ GetServiceSecurityDescriptorString(IntPtr hService, bool audit);

		// Set-ServiceSecurity
		void SetServiceSecurity(String^ serviceName, String^ computerName, String^ sddl, bool audit, bool changeOwner);
		void SetServiceSecurity(String^ serviceName, String^ sddl, bool audit, bool changeOwner);

	private:
		Core::Services* m_svc;
	};
}