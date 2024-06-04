using System.ServiceProcess;
using System.Management.Automation;
using System.Security.AccessControl;
using WindowsUtils.Wrappers;
using WindowsUtils.AccessControl;
using WindowsUtils.ArgumentCompletion;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">Set the service object security.</para>
    /// <para type="description">This cmdlet sets the service object security. You first retrieve the service security calling 'Get-ServiceSecurity', modifies the ACL, and pass it as input object to this cmdlet.</para>
    /// <example>
    ///     <para></para>
    ///     <code>              $serviceSecurity = Get-ServiceSecurity -Name 'MyCoolService'
    ///$serviceSecurity.SetOwner([System.Security.Principal.NTAccount]'User')
    ///$serviceSecurity.AddAccessRule((New-ServiceAccessRule -Identity 'User' -Rights 'AllAccess' -Type 'Allow'))
    ///
    ///Set-ServiceSecurity -Name 'MyCoolService' -SecurityObject $serviceSecurity</code>
    ///     <para>Changes the owner, adds an access rule, and sets the service security attributes.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    /// <para></para>
    /// <code>              $serviceSecurity = Get-ServiceSecurity -Name 'MyCoolService' -Audit
    ///$serviceSecurity.AddAuditRule((New-ServiceAuditRule -Identity 'User' -Rights 'AllAccess' -Flags 'Success'))
    ///
    ///Set-ServiceSecurity -Name 'MyCoolService' -AclObject $serviceSecurity</code>
    ///     <para>Changes the service audit rules.</para>
    /// <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(
        VerbsCommon.Set, "ServiceSecurity",
        SupportsShouldProcess = true,
        ConfirmImpact = ConfirmImpact.Medium,
        DefaultParameterSetName = "ByNameAndSecurityObject"
    )]
    public class SetServiceSecurityCommand : PSCmdlet
    {
        private string _serviceFinalName;
        private ServiceSecurity _finalSecurityObject;
        private ServicesWrapper _unwrapper = new();

        /// <summary>
        /// <para type="description">The service name.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ParameterSetName = "ByNameAndSecurityObject",
            ValueFromPipelineByPropertyName = true,
            HelpMessage = "The service name."
        )]
        [Parameter(
            Mandatory = true,
            Position = 0,
            ParameterSetName = "ByNameAndSddl",
            ValueFromPipelineByPropertyName = true,
            HelpMessage = "The service name."
        )]
        [ValidateNotNullOrEmpty]
        [ArgumentCompleter(typeof(ServiceNameCompleter))]
        public string Name { get; set; }

        /// <summary>
        /// <para type="description">The service display name.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ParameterSetName = "ByDisplayNameAndSecurityObject",
            HelpMessage = "The service display name."
        )]
        [Parameter(
            Mandatory = true,
            ParameterSetName = "ByDisplayNameAndSddl",
            HelpMessage = "The service display name."
        )]
        [ValidateNotNullOrEmpty]
        [ArgumentCompleter(typeof(ServiceDisplayNameCompleter))]
        public string DisplayName { get; set; }

        /// <summary>
        /// <para type="description">The service controller object.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ParameterSetName = "ByInputObjectAndSecurityObject",
            ValueFromPipeline = true,
            HelpMessage = "The service controller object."
        )]
        [Parameter(
            Mandatory = true,
            Position = 0,
            ParameterSetName = "ByInputObjectAndSddl",
            ValueFromPipeline = true,
            HelpMessage = "The service controller object."
        )]
        public ServiceController InputObject { get; set; }

        /// <summary>
        /// <para type="description">The computer name. If empty, looks for the service in the current machine.</para>
        /// </summary>
        [Parameter(HelpMessage = "The computer name. If empty, looks for the service in the current machine.")]
        public string ComputerName { get; set; }

        /// <summary>
        /// <para type="description">The service security object to set.</para>
        /// </summary>
        [Parameter(Mandatory = true, ParameterSetName = "ByNameAndSecurityObject", HelpMessage = "The service security object to set.")]
        [Parameter(Mandatory = true, ParameterSetName = "ByDisplayNameAndSecurityObject", HelpMessage = "The service security object to set.")]
        [Parameter(Mandatory = true, ParameterSetName = "ByInputObjectAndSecurityObject", HelpMessage = "The service security object to set.")]
        [Alias("AclObject")]
        public ServiceSecurity SecurityObject { get; set; }

        /// <summary>
        /// <para type="description">The service security object to set.</para>
        /// </summary>
        [Parameter(Mandatory = true, ParameterSetName = "ByNameAndSddl", HelpMessage = "The SDDL string representing the security object.")]
        [Parameter(Mandatory = true, ParameterSetName = "ByDisplayNameAndSddl", HelpMessage = "The SDDL string representing the security object.")]
        [Parameter(Mandatory = true, ParameterSetName = "ByInputObjectAndSddl", HelpMessage = "The SDDL string representing the security object.")]
        public string Sddl { get; set; }

        /// <summary>
        /// <para type="description">True to modify audit rules (SACL).</para>
        /// <para type="description">Attention! If this parameter is called, and the security object,</para>
        /// <para type="description">or SDDL audit rules are empty it will erase the service's audit rules.</para>
        /// </summary>
        [Parameter(HelpMessage = @"
            True to modify audit rules (SACL).
            Attention! If this parameter is called, and the security object,
            or SDDL audit rules are empty it will erase the service's audit rules."
        )]
        public SwitchParameter SetSacl { get; set; }

        protected override void BeginProcessing()
        {
            if (ParameterSetName == "ByNameAndSddl" || ParameterSetName == "ByInputObjectAndSddl" || ParameterSetName == "ByDisplayNameAndSddl")
            {
                try
                {
                    new RawSecurityDescriptor(Sddl);
                }
                catch (Exception ex)
                {
                    throw new ArgumentException(ex.InnerException.Message);
                }
            }
        }

        protected override void ProcessRecord()
        {
            switch (ParameterSetName)
            {
                case "ByNameAndSecurityObject":
                    _serviceFinalName = Name;
                    _finalSecurityObject = SecurityObject;
                    break;

                case "ByDisplayNameAndSecurityObject":
                    string? serviceName = ServiceController.GetServices().Where(s => s.DisplayName == DisplayName).FirstOrDefault().ServiceName
                        ?? throw new ItemNotFoundException($"No service with display name '{DisplayName}' found.");

                    _serviceFinalName = serviceName;
                    _finalSecurityObject = SecurityObject;
                    break;

                case "ByInputObjectAndSecurityObject":
                    if (InputObject.ServiceHandle is null)
                        throw new ArgumentException("Invalid input object.");

                    if (InputObject.ServiceHandle.IsClosed || InputObject.ServiceHandle.IsInvalid)
                        throw new ArgumentException("Invalid input object service handle.");

                    _serviceFinalName = InputObject.ServiceName;
                    _finalSecurityObject = SecurityObject;
                    break;

                case "ByNameAndSddl":
                    _finalSecurityObject = new(_unwrapper.GetServiceSecurityDescriptorString(Name, SetSacl), Name);
                    _finalSecurityObject.SetSecurityDescriptorSddlForm(Sddl);
                    _serviceFinalName = Name;
                    break;

                case "ByDisplayNameAndSddl":
                    serviceName = ServiceController.GetServices().Where(s => s.DisplayName == DisplayName).FirstOrDefault().ServiceName
                        ?? throw new ItemNotFoundException($"No service with display name '{DisplayName}' found.");

                    _serviceFinalName = serviceName;
                    _finalSecurityObject = new(_unwrapper.GetServiceSecurityDescriptorString(Name, SetSacl), Name);
                    _finalSecurityObject.SetSecurityDescriptorSddlForm(Sddl);
                    break;

                case "ByInputObjectAndSddl":
                    if (InputObject.ServiceHandle is null)
                        throw new ArgumentException("Invalid input object.");

                    if (InputObject.ServiceHandle.IsClosed || InputObject.ServiceHandle.IsInvalid)
                        throw new ArgumentException("Invalid input object service handle.");

                    _serviceFinalName = InputObject.ServiceName;
                    _finalSecurityObject = new(_unwrapper.GetServiceSecurityDescriptorString(Name, SetSacl), Name);
                    _finalSecurityObject.SetSecurityDescriptorSddlForm(Sddl);
                    break;
            }

            if (string.IsNullOrEmpty(ComputerName))
            {
                if (ShouldProcess(
                    $"Setting security on service {Name} on the local computer.",
                    $"Are you sure you want to set security for service {Name} on the local computer?",
                    "Setting Service Security"
                ))
                    _unwrapper.SetServiceSecurity(_serviceFinalName, _finalSecurityObject.Sddl, SetSacl, _finalSecurityObject.OwnerModified);
            }
            else
            {
                if (ShouldProcess(
                    $"Setting security on service {Name} on {ComputerName}.",
                    $"Are you sure you want to set security for service {Name} on {ComputerName}?",
                    "Setting Service Security"
                ))
                    _unwrapper.SetServiceSecurity(_serviceFinalName, ComputerName, _finalSecurityObject.Sddl, SetSacl, _finalSecurityObject.OwnerModified);
            }
        }
    }
}