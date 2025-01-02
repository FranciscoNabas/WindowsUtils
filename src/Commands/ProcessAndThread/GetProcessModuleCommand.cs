using System.Diagnostics;
using System.Management.Automation;
using WindowsUtils.Engine;
using WindowsUtils.Wrappers;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">List processes modules.</para>
    /// <para type="description">This Cmdlet lists modules loaded into processes.</para>
    /// <para type="description">You can list specific processes, or all processes. You can also include module version information.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Get-ProcessModule -Name 'explorer'</code>
    ///     <para>Gets loaded modules for the explorer process.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>listdlls -ProcessId 666, 667 -IncludeVersionInfo</code>
    ///     <para>Gets loaded modules for processes ID 666 and 667, including module version info.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>listdlls</code>
    ///     <para>Gets loaded modules for all processes.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(
        VerbsCommon.Get, "ProcessModule",
        DefaultParameterSetName = "byName"
    )]
    [Alias("listdlls")]
    public class GetProcessModuleCommand : CoreCommandBase
    {
        private readonly List<uint> _processIdList = new();

        /// <summary>
        /// <para type="description">One or more process friendly names.</para>
        /// </summary>
        [Parameter(
            Position = 0,
            ParameterSetName = "byName"
        )]
        [ValidateNotNullOrEmpty]
        public string[] Name { get; set; }

        /// <summary>
        /// <para type="description">One or more process IDs.</para>
        /// </summary>
        [Parameter(ParameterSetName = "byProcessId")]
        [ValidateNotNullOrEmpty]
        public uint[] ProcessId { get; set; }

        /// <summary>
        /// <para type="description">Include module file version information.</para>
        /// </summary>
        [Parameter()]
        public SwitchParameter IncludeVersionInfo { get; set; }

        protected override void BeginProcessing()
        {
            if (!Utils.IsAdministrator())
                throw new InvalidOperationException("Cmdlet needs to run elevated.");

            if (ParameterSetName == "byName")
            {
                if (Name is not null)
                    foreach (string processName in Name)
                    {
                        Process[]? process = Process.GetProcessesByName(processName);
                        if (process is null || process.Length == 0)
                            WriteWarning($"Process with name '{processName}' not found.");
                        else
                            foreach (Process p in process)
                                _processIdList.Add((uint)p.Id);
                    }
            }
            else
                if (ProcessId is not null)
                _processIdList.AddRange(ProcessId);
        }

        protected override void ProcessRecord()
        {
            if (!_processIdList.Any())
                ProcessAndThread.ListProcessModule(IncludeVersionInfo);
            else
                ProcessAndThread.ListProcessModule(_processIdList.ToArray(), IncludeVersionInfo);
        }
    }
}