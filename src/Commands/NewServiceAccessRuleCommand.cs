using System.Security.Principal;
using System.Management.Automation;
using System.Security.AccessControl;
using WindowsUtils.Services;
using WindowsUtils.AccessControl;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">Creates a new service access rule object.</para>
    /// <para type="description">This cmdlet creates a new service access rule object to modify service security attributes.</para>
    /// <example>
    ///     <para></para>
    ///     <code>New-ServiceAccessRule -Identity 'DOMAIN\YourUser' -Rights 'AllAccess' -Type 'Allow'</code>
    ///     <para>Creates a new service access rule with all access allowed for user 'DOMAIN\YourUser'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>              $serviceAccessRuleSplat = @{
    ///    Identity = [System.Security.Principal.NTAccount]'DOMAIN\YourUser
    ///    Rights = [WindowsUtils.Engine.ServiceRights]::GenericRead -bor [WindowsUtils.Engine.ServiceRights]::EnumerateDependents
    ///    Type = [System.Security.AccessControl.AccessControlType]::Deny
    ///    InheritanceFlags = [System.Security.AccessControl.InheritanceFlags]::ContainerInherit
    ///    PropagationFlags = [System.Security.AccessControl.PropagationFlags]::InheritOnly
    ///    Inherited = $true
    ///}
    ///New-ServiceAccessRule @serviceAccessRuleSplat</code>
    ///     <para>Creates a new service access rule with the defined parameters.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsCommon.New, "ServiceAccessRule")]
    [OutputType(typeof(ServiceAccessRule))]
    public class NewServiceAccessRuleCommand : PSCmdlet
    {
        private string _identityAsString;
        private IdentityReference _identityAsIdReference;
        private bool _isString;
        private ServiceRights _rights;
        private InheritanceFlags _inheritance;
        private PropagationFlags _propagation;

        /// <summary>
        /// <para type="description">The identity to be associated to the new access rule.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            HelpMessage = "The identity to be associated to the new access rule."
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
        /// <para type="description">The type of the new access rule.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            HelpMessage = "The type of the new access rule."
        )]
        public AccessControlType Type { get; set; }

        /// <summary>
        /// <para type="description">The inheritance flags to be associated with the new access rule.</para>
        /// </summary>
        [Parameter(HelpMessage = "The inheritance flags to be associated with the new access rule.")]
        public InheritanceFlags InheritanceFlags {
            get { return _inheritance; }
            set { _inheritance = value; }
        }

        /// <summary>
        /// <para type="description">The propagation flags to be associated with the new access rule.</para>
        /// </summary>
        [Parameter(HelpMessage = "The propagation flags to be associated with the new access rule.")]
        public PropagationFlags PropagationFlags {
            get { return _propagation; }
            set { _propagation = value; }
        }

        /// <summary>
        /// <para type="description">Called if the new access rule is inherited.</para>
        /// </summary>
        [Parameter(HelpMessage = "Called if the new access rule is inherited.")]
        public SwitchParameter Inherited { get; set; }

        protected override void ProcessRecord()
            => WriteObject(new ServiceAccessRule(_identityAsIdReference, _rights, Inherited, _inheritance, _propagation, Type));
    }
}