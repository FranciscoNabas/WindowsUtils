using System.ServiceProcess;
using System.Management.Automation;
using System.Runtime.InteropServices;
using System.Runtime.ConstrainedExecution;
using WindowsUtils.Core;
using WindowsUtils.AccessControl;

namespace WindowsUtils.Interop
{
    internal partial class NativeFunctions
    {
        [DllImport("Advapi32.dll", SetLastError = true)]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        internal static extern bool CloseServiceHandle(IntPtr hSCObject);
    }
}

namespace WindowsUtils.Services
{
    [Flags]
    public enum ServiceRights : uint
    {
        QueryConfig = 0x0001,
        ChangeConfig = 0x0002,
        QueryStatus = 0x0004,
        EnumerateDependents = 0x0008,
        Start = 0x0010,
        Stop = 0x0020,
        PauseContinue = 0x0040,
        Interrogate = 0x0080,
        UserDefinedControl = 0x0100,
        AllAccess = 0xF01FF,
        Delete = 0x10000,
        ReadControl = 0x20000,
        WriteDac = 0x40000,
        WriteOwner = 0x80000,
        GenericRead = ReadControl |
                       QueryConfig |
                       QueryStatus |
                       Interrogate |
                       EnumerateDependents,

        GenericWrite = ReadControl | ChangeConfig,
        GenericExecute = ReadControl |
                          Start |
                          Stop |
                          PauseContinue |
                          UserDefinedControl
    }


    public abstract class ServiceCommandBase : PSCmdlet
    {
        private readonly WrapperFunctions _unwrapper = new();

        internal ServiceSecurity GetServiceObjectSecurity(string serviceName, bool getAudit)
        {
            string sddl;
            try
            {
                sddl = _unwrapper.GetServiceSecurityDescriptorString(serviceName, getAudit);
            }
            catch (Core.NativeException ex)
            {
                throw (NativeException)ex;
            }

            return new ServiceSecurity(sddl, serviceName);
        }

        internal ServiceSecurity GetServiceObjectSecurity(ServiceController service, bool getAudit)
        {
            string sddl;
            try
            {
                sddl = _unwrapper.GetServiceSecurityDescriptorString(service.ServiceHandle.DangerousGetHandle(), getAudit);
            }
            catch (Core.NativeException ex)
            {
                throw (NativeException)ex;
            }

            return new ServiceSecurity(sddl, service.ServiceName);
        }

        internal List<string> GetMatchingServicesNameByName(string[] serviceNames)
        {
            ServiceController[] allServices = ServiceController.GetServices();
            List<string> result = new();
            foreach (string name in serviceNames) 
            {
                bool found = false;
                bool isWildcard = WildcardPattern.ContainsWildcardCharacters(name);
                if (isWildcard)
                {
                    WildcardPattern wildcardPattern = WildcardPattern.Get(name, WildcardOptions.IgnoreCase);
                    foreach (ServiceController service in allServices)
                    {
                        if (wildcardPattern.IsMatch(service.ServiceName))
                        {
                            if (!result.Contains(service.ServiceName))
                                result.Add(service.ServiceName);

                            found = true;
                        }
                    }
                }
                else
                {
                    string? singleServiceName = allServices.Where(s => s.ServiceName == name).FirstOrDefault().ServiceName;
                    if (singleServiceName is not null)
                    {
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
            foreach (string name in displayNames) 
            {
                bool found = false;
                bool isWildcard = WildcardPattern.ContainsWildcardCharacters(name);
                if (isWildcard)
                {
                    WildcardPattern wildcardPattern = WildcardPattern.Get(name, WildcardOptions.IgnoreCase);
                    foreach (ServiceController service in allServices)
                    {
                        if (wildcardPattern.IsMatch(service.DisplayName))
                        {
                            if (!result.Contains(service.ServiceName))
                                result.Add(service.ServiceName);

                            found = true;
                        }
                    }
                }
                else
                {
                    string? singleServiceName = allServices.Where(s => s.DisplayName == name).FirstOrDefault().ServiceName;
                    if (singleServiceName is not null)
                    {
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
}