using System.ServiceProcess;
using System.Management.Automation;
using WindowsUtils.Wrappers;
using WindowsUtils.Engine.AccessControl;

namespace WindowsUtils.Commands;

public abstract class ServiceCommandBase : PSCmdlet
{
    internal ServiceSecurity GetServiceObjectSecurity(string serviceName, bool getAudit)
    {
        string sddl;
        sddl = ServicesWrapper.GetServiceSecurityDescriptorString(serviceName, getAudit);

        return new ServiceSecurity(sddl, serviceName);
    }

    internal ServiceSecurity GetServiceObjectSecurity(ServiceController service, bool getAudit)
    {
        string sddl;
        sddl = ServicesWrapper.GetServiceSecurityDescriptorString(service.ServiceHandle.DangerousGetHandle(), getAudit);

        return new ServiceSecurity(sddl, service.ServiceName);
    }

    internal List<string> GetMatchingServicesNameByName(string[] serviceNames)
    {
        ServiceController[] allServices = ServiceController.GetServices();
        List<string> result = new();
        foreach (string name in serviceNames) {
            bool found = false;
            bool isWildcard = WildcardPattern.ContainsWildcardCharacters(name);
            if (isWildcard) {
                WildcardPattern wildcardPattern = WildcardPattern.Get(name, WildcardOptions.IgnoreCase);
                foreach (ServiceController service in allServices) {
                    if (wildcardPattern.IsMatch(service.ServiceName)) {
                        if (!result.Contains(service.ServiceName))
                            result.Add(service.ServiceName);

                        found = true;
                    }
                }
            }
            else {
                string? singleServiceName = allServices.Where(s => s.ServiceName == name).FirstOrDefault().ServiceName;
                if (singleServiceName is not null) {
                    if (!result.Contains(singleServiceName))
                        result.Add(singleServiceName);

                    found = true;
                }
            }

            if (!found && !isWildcard)
                WriteError(new ErrorRecord(
                    new ItemNotFoundException($"No service with name '{name}' found."),
                    "NoServiceFoundForGivenName",
                    ErrorCategory.ObjectNotFound,
                    name
                ));
        }

        return result;
    }

    internal List<string> GetMatchingServicesNameByDisplayName(string[] displayNames)
    {
        ServiceController[] allServices = ServiceController.GetServices();
        List<string> result = new();
        foreach (string name in displayNames) {
            bool found = false;
            bool isWildcard = WildcardPattern.ContainsWildcardCharacters(name);
            if (isWildcard) {
                WildcardPattern wildcardPattern = WildcardPattern.Get(name, WildcardOptions.IgnoreCase);
                foreach (ServiceController service in allServices) {
                    if (wildcardPattern.IsMatch(service.DisplayName)) {
                        if (!result.Contains(service.ServiceName))
                            result.Add(service.ServiceName);

                        found = true;
                    }
                }
            }
            else {
                string? singleServiceName = allServices.Where(s => s.DisplayName == name).FirstOrDefault().ServiceName;
                if (singleServiceName is not null) {
                    if (!result.Contains(singleServiceName))
                        result.Add(singleServiceName);

                    found = true;
                }
            }

            if (!found && !isWildcard)
                WriteError(new ErrorRecord(
                    new ItemNotFoundException($"No service with name '{name}' found."),
                    "NoServiceFoundForGivenName",
                    ErrorCategory.ObjectNotFound,
                    name
                ));
        }

        return result;
    }
}
