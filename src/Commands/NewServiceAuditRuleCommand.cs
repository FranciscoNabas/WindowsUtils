using System.Security.Principal;
using System.Management.Automation;
using System.Security.AccessControl;
using WindowsUtils.AccessControl;
using WindowsUtils.Services;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">Creates a new service audit rule object.</para>
    /// <para type="description">This cmdlet creates a new service audit rule object to modify service security attributes.</para>
    /// <example>
    ///     <para></para>
    ///     <code>New-ServiceAuditRule -Identity 'DOMAIN\YourUser' -Rights 'AllAccess' -Flags 'Failure'</code>
    ///     <para>Creates a new service audit rule for all access and failure for 'DOMAIN\YourUser'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>              $serviceAuditRuleSplat = @{
    ///    Identity = [System.Security.Principal.NTAccount]'DOMAIN\YourUser
    ///    Rights = [WindowsUtils.Engine.ServiceRights]::GenericRead -bor [WindowsUtils.Engine.ServiceRights]::EnumerateDependents
    ///    Flags = [System.Security.AccessControl.AuditFlags]::Success
    ///    InheritanceFlags = [System.Security.AccessControl.InheritanceFlags]::ContainerInherit
    ///    PropagationFlags = [System.Security.AccessControl.PropagationFlags]::InheritOnly
    ///    Inherited = $true
    ///}
    ///New-ServiceAuditRule @serviceAuditRuleSplat</code>
    ///     <para>Creates a new service audit rule with the defined parameters.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsCommon.New, "ServiceAuditRule")]
    [OutputType(typeof(ServiceAuditRule))]
    public class NewServiceSecurityCommand : PSCmdlet
    {
        private string _identityAsString;
        private IdentityReference _identityAsIdReference;
        private bool _isString;
        private ServiceRights _rights;
        private InheritanceFlags _inheritance;
        private PropagationFlags _propagation;

        /// <summary>
        /// <para type="description">The identity to be associated to the new audit rule.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            HelpMessage = "The identity to be associated to the new audit rule."
        )]
        public object Identity {
            get {
                if (_isString)
                    return _identityAsString;

                return _identityAsIdReference;
            }
            set {
                if (value is string _valueStr)
                {
                    _identityAsString = _valueStr;
                    _isString = true;
                    _identityAsIdReference = new NTAccount(_valueStr);
                }
                else if (value is IdentityReference _valueIdRf)
                    _identityAsIdReference = _valueIdRf;
                else
                    throw new ArgumentException("Wrong identity type. Accepted types are 'System.String' and 'System.Security.Principal.IdentityReference'.");
            }
        }

        /// <summary>
        /// <para type="description">A bitwise OR flag of one or more service rights.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            HelpMessage = "A bitwise OR flag of one or more service rights."
        )]
        public ServiceRights Rights {
            get { return _rights; }
            set { _rights = value; }
        }

        /// <summary>
        /// <para type="description">The type of the new audit rule.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            HelpMessage = "The type of the new audit rule."
        )]
        public AuditFlags Flags { get; set; }

        /// <summary>
        /// <para type="description">The inheritance flags to be associated with the new audit rule.</para>
        /// </summary>
        [Parameter(HelpMessage = "The inheritance flags to be associated with the new audit rule.")]
        public InheritanceFlags InheritanceFlags {
            get { return _inheritance; }
            set { _inheritance = value; }
        }

        /// <summary>
        /// <para type="description">The propagation flags to be associated with the new audit rule.</para>
        /// </summary>
        [Parameter(HelpMessage = "The propagation flags to be associated with the new audit rule.")]
        public PropagationFlags PropagationFlags {
            get { return _propagation; }
            set { _propagation = value; }
        }

        /// <summary>
        /// <para type="description">Called if the new audit rule is inherited.</para>
        /// </summary>
        [Parameter(HelpMessage = "Called if the new audit rule is inherited.")]
        public SwitchParameter Inherited { get; set; }

        protected override void ProcessRecord()
            => WriteObject(new ServiceAuditRule(_identityAsIdReference, _rights, Inherited, _inheritance, _propagation, Flags));
    }
}