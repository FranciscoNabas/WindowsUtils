#pragma once
#pragma unmanaged

#include "../Engine/Services.h"
#include "../Stubs/ServicesStub.h"

#pragma managed

#include "WrapperBase.h"
#include "Marshalers.h"
#include "NativeException.h"

namespace WindowsUtils::Wrappers
{
	public ref class ServicesWrapper : public WrapperBase
	{
	public:
		ServicesWrapper(Core::CmdletContextProxy^ context)
			: WrapperBase(context) { }

		// Remove-Service
		void RemoveService(String^ servicename, String^ computerName, bool stopservice, bool noWait);
		void RemoveService(String^ servicename, bool stopservice, bool noWait);

		// Get-ServiceSecurity
		static String^ GetServiceSecurityDescriptorString(String^ serviceName, String^ computerName, bool audit);
		static String^ GetServiceSecurityDescriptorString(String^ serviceName, bool audit);
		static String^ GetServiceSecurityDescriptorString(IntPtr hService, bool audit);

		// Set-ServiceSecurity
		void SetServiceSecurity(String^ serviceName, String^ computerName, String^ sddl, bool audit, bool changeOwner);
		void SetServiceSecurity(String^ serviceName, String^ sddl, bool audit, bool changeOwner);
	};
}