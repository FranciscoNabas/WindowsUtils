using System.Management.Automation;
using WindowsUtils.Wrappers;
using WindowsUtils.TerminalServices;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">Logs off an interactive session on the local or remote computer.</para>
    /// <para type="description">This Cmdlet disconnects, or logs off, an interactive session on the local or remote computer.</para>
    /// <para type="description">You can retrieve the SessionId with Get-ComputerSession, and use this Cmdled to logoff users.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Disconnect-Session -SessionId 3</code>
    ///     <para>Disconnects a session with a given session id.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-ComputerSession -ComputerName 'MYAWESOMEPC.contoso.com' | Where-Object { $PSItem.UserName -eq 'CONTOSO\user.name' } | Disconnect-Session -ComputerName 'MYAWESOMEPC.contoso.com' -SessionId $PSItem.SessionId </code>
    ///     <para>Gets the session from a specific user on a remote computer, and loggs it off.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(
        VerbsCommunications.Disconnect, "Session",
        SupportsShouldProcess = true,
        ConfirmImpact = ConfirmImpact.High,
        DefaultParameterSetName = "NoComputerName"
    )]
    [Alias("disconnect")]
    public class DisconnectSessionCommand : Cmdlet
    {
        /// <summary>
        /// <para type="description">The computer name to disconnect a session.</para>
        /// <para type="description">If not informed, it disconnects the session from the local computer.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ParameterSetName = "WithComputerName",
            ValueFromPipelineByPropertyName = true,
            HelpMessage = "The computer name to disconnect the session."
        )]
        [ValidateNotNullOrEmpty]
        public string ComputerName { get; set; }

        /// <summary>
        /// <para type="description">The session id to disconnect.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ParameterSetName = "WithComputerName",
            ValueFromPipelineByPropertyName = true,
            HelpMessage = "The session ID to disconnect."
        )]
        [Parameter(
            Mandatory = false,
            ParameterSetName = "NoComputerName",
            ValueFromPipelineByPropertyName = true,
            HelpMessage = "The session ID to disconnect."
        )]
        public uint SessionId { get; set; }

        /// <summary>
        /// <para type="description">If called, the Cmdlet waits the logoff process to finish before returning.</para>
        /// </summary>
        [Parameter(
            ParameterSetName = "WithComputerName",
            HelpMessage = "Waits for the logon process to finish."
        )]
        [Parameter(
            ParameterSetName = "NoComputerName",
            HelpMessage = "Waits for the logon process to finish."
        )]
        public SwitchParameter Wait { get; set; }

        protected override void ProcessRecord()
        {
            TerminalServicesWrapper unWrapper = new();

            if (string.IsNullOrEmpty(ComputerName))
            {
                if (SessionId == 0)
                {
                    if (ShouldProcess(
                    "Disconnecting current session from this computer.",
                    "Are you sure you want to disconnect the current session on this computer?",
                    "Disconnect session"))
                        unWrapper.DisconnectSession(IntPtr.Zero, SessionId, Wait);
                }
                else
                {
                    if (ShouldProcess(
                   "Disconnecting session ID " + SessionId + " from current computer.",
                   "Are you sure you want to disconnect sesion ID " + SessionId + " on the current computer?",
                   "Disconnect session"))
                        unWrapper.DisconnectSession(IntPtr.Zero, SessionId, Wait);
                }
            }
            else
            {
                using WtsSession session = new(ComputerName);
                if (ShouldProcess(
                   "Disconnecting session ID " + SessionId + " from computer " + ComputerName + ".",
                   "Are you sure you want to disconnect session ID " + SessionId + " on computer " + ComputerName + "?",
                   "Disconnect session"))
                    unWrapper.DisconnectSession(session.SessionHandle.DangerousGetHandle(), SessionId, Wait);
            }
        }
    }
}