using System.Management.Automation;
using WindowsUtils.Core;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">Returns the user sessions on the local or remote computer.</para>
    /// <para type="description">This Cmdlet returns the sessions on the local or remote computer.</para>
    /// <para type="description">By default, only brings sessions that has an user name assigned.</para>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "ComputerSession")]
    [OutputType(typeof(ComputerSession))]
    public class GetComputerSessionCommand : Cmdlet
    {
        /// <summary>
        /// <para type="description">The computer name.</para>
        /// <para type="description">If no computer name is informed, it returns sessions on the current computer.</para>
        /// </summary>
        [Parameter(
            Position = 0,
            HelpMessage = "The computer name."
        )]
        [ValidateNotNullOrEmpty]
        public string ComputerName { get; set; }

        /// <summary>
        /// <para type="description">When called, this Cmdlet returns only active sessions.</para>
        /// </summary>
        [Parameter(HelpMessage = "Returns only active sessions.")]
        public SwitchParameter ActiveOnly { get; set; } = false;

        /// <summary>
        /// <para type="description">When called, this Cmdlet includes sessions without an user name assigned.</para>
        /// <para type="description">Sessions without user name are marked as 'System'.</para>
        /// </summary>
        [Parameter(HelpMessage = "Includes sessions without an user assigned.")]
        public SwitchParameter IncludeSystemSession { get; set; } = false;

        protected override void ProcessRecord()
        {
            Wrapper unWrapper = new();
            if (string.IsNullOrEmpty(ComputerName))
            {
                ComputerSession[] result = unWrapper.GetComputerSession(ComputerName, ActiveOnly, IncludeSystemSession);
                result.ToList().ForEach(x => WriteObject(x));
            }
            else
            {
                ComputerSession[] result = unWrapper.GetComputerSession(ComputerName, ActiveOnly, IncludeSystemSession);
                result.ToList().ForEach(x => WriteObject(x));
            }
        }
    }
}