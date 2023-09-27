using System.ServiceProcess;
using System.Management.Automation;
using WindowsUtils.Services;
using WindowsUtils.AccessControl;
using WindowsUtils.ArgumentCompletion;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">Gets the service security attributes.</para>
    /// <para type="description">This cmdlet retrieves the service security attributes.</para>
    /// <para type="description">The attributes retrieved are Owner, Group, DACL, and SACL, if used with the 'Audit' parameter.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Get-ServiceSecurity -Name 'MyCoolService'</code>
    ///     <para>Gets the security attributes from the service 'MyCoolService'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-ServiceSecurity -Name 'MyCoolService' -Audit</code>
    ///     <para>Gets the security attributes from the service 'MyCoolService', including System Access Control List (SACL).</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-Service -Name 'MyCoolService' | Get-ServiceSecurity</code>
    ///     <code>$service = Get-Service -Name 'MyCoolService'; Get-ServiceSecurity -InputObject $service</code>
    ///     <para>Gets the security attributes from the service 'MyCoolService', from 'Get-Service'.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(
        VerbsCommon.Get, "ServiceSecurity",
        DefaultParameterSetName = "WithServiceName"
    )]
    [OutputType(typeof(ServiceSecurity))]
    public class GetServiceSecurityCommand : ServiceCommandBase
    {
        /// <summary>
        /// <para type="description">The service(s) name.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ParameterSetName = "WithServiceName",
            HelpMessage = "The service(s) name."
        )]
        [ValidateNotNullOrEmpty]
        [ArgumentCompleter(typeof(ServiceNameCompleter))]
        public string[] Name { get; set; }

        /// <summary>
        /// <para type="description">The service(s) display name.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ParameterSetName = "WithDisplayName",
            HelpMessage = "The service(s) display name."
        )]
        [ValidateNotNullOrEmpty]
        [ArgumentCompleter(typeof(ServiceDisplayNameCompleter))]
        public string[] DisplayName { get; set; }

        /// <summary>
        /// <para type="description">The service controller input object.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ParameterSetName = "WithServiceController",
            ValueFromPipeline = true,
            HelpMessage = "The service controller input object."
        )]
        public ServiceController InputObject { get; set; }

        /// <summary>
        /// <para type="description">Includes SACL to the result.</para>
        /// </summary>
        [Parameter(HelpMessage = "Includes SACL to the result.")]
        public SwitchParameter Audit { get; set; }

        protected override void ProcessRecord()
        {
            List<string> serviceNameList;
            switch (ParameterSetName)
            {
                case "WithServiceName":
                    serviceNameList = GetMatchingServicesNameByName(Name);
                    foreach (string serviceName in serviceNameList)
                        WriteObject(GetServiceObjectSecurity(serviceName, Audit));
                    break;

                case "WithServiceController":
                    // We can't use given handle because we need to use extra privileges to access SACL.
                    if (Audit)
                        WriteObject(GetServiceObjectSecurity(InputObject.ServiceName, Audit));
                    else
                        WriteObject(GetServiceObjectSecurity(InputObject, Audit));
                    break;

                case "WithDisplayName":
                    serviceNameList = GetMatchingServicesNameByDisplayName(DisplayName);
                    foreach (string serviceName in serviceNameList)
                        WriteObject(GetServiceObjectSecurity(serviceName, Audit));
                    break;
            }
        }
    }
}