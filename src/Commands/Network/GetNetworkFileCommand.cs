using System.Management.Automation;
using WindowsUtils.Wrappers;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">Returns files opened by remote sessions.</para>
    /// <para type="description">This cmdlet returns files opened in a server remotely by a user session.</para>
    /// <para type="description">It was inspired by 'psfile.exe' from Sysinternals.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Get-NetworkFile -ComputerName SUPERSERVER</code>
    ///     <para>Returns open remotely opened files on computer 'SUPERSERVER'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>psfile SUPERSERVER -UserConnectionFilter 'francisco.nabas'</code>
    ///     <para>Returns files opened remotely on computer 'SUPERSERVER' by user 'francisco.nabas'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>getnetfile 'SUPERSERVER' -IncludeConnectionName</code>
    ///     <para>Returns files opened remotely on computer 'SUPERSERVER' and includes the origin session name.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-NetworkFile -UserConnectionFilter \\10.1.1.15</code>
    ///     <para>Returns files opened remotely on the current computer filtered by session connection name '10.1.1.15'.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "NetworkFile")]
    [OutputType(typeof(NetworkFileInfo))]
    [Alias(new string[] { "psfile", "getnetfile" })]
    public class GetNetworkFileCommand : PSCmdlet
    {
        private readonly NetworkWrapper _unwrapper = new();

        /// <summary>
        /// <para type="description">The computer name. If not present, the current computer is used.</para>
        /// </summary>
        [Parameter(
            Position = 0,
            HelpMessage = "The computer name. If not present, the current computer is used."
        )]
        public string ComputerName { get; set; }

        /// <summary>
        /// <para type="description">A path prefix filter for the results.</para>
        /// </summary>
        [Parameter(HelpMessage = "A path prefix.")]
        public string BasePath { get; set; }

        /// <summary>
        /// <para type="description">An user name or connection name filter.</para>
        /// <para type="description">For user name, input the user name, for connection name, input the connection name prepended by two back slashes '\\'.</para>
        /// <para type="description">For more information, see documentation for 'NetFileEnum'.</para>
        /// </summary>
        [Parameter(HelpMessage = "A user name filter or connection name filter.")]
        public string UserConnectionFilter { get; set; }

        /// <summary>
        /// <para type="description">Includes the session connection name. This implies a cost because a call to 'NtSessionEnum' is made.</para>
        /// </summary>
        [Parameter(HelpMessage = "Include connection name.")]
        public SwitchParameter IncludeConnectionName { get; set; }

        protected override void ProcessRecord()
        {
            if (string.IsNullOrWhiteSpace(ComputerName)) { ComputerName = string.Empty; }
            if (string.IsNullOrWhiteSpace(BasePath)) { BasePath = string.Empty; }
            if (string.IsNullOrWhiteSpace(UserConnectionFilter)) { UserConnectionFilter = string.Empty; }

            WriteObject(_unwrapper.GetNetworkFile(ComputerName, BasePath, UserConnectionFilter, IncludeConnectionName), true);
        }
    }
}