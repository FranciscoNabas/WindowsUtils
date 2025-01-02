using System.Diagnostics;
using System.Management.Automation;
using WindowsUtils.Engine;
using WindowsUtils.Wrappers;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618

    /// <summary>
    /// <para type="synopsis">Suspends one or more processes.</para>
    /// <para type="description">This Cmdlet suspends one or more processes by name or process ID.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Suspend-Process -Name 'notepad'</code>
    ///     <para>Suspends all Notepad processes.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>suspend -Id 666</code>
    ///     <para>Suspends process with ID 666.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(
        VerbsLifecycle.Suspend, "Process",
        DefaultParameterSetName = "byName"
    )]
    [Alias("suspend")]
    public class SuspendProcessCommand : CoreCommandBase
    {
        private readonly List<uint> _processIdList = new();

        /// <summary>
        /// <para type="description">One or more process friendly names.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ParameterSetName = "byName"
        )]
        [ValidateNotNullOrEmpty]
        public string[] Name { get; set; }

        /// <summary>
        /// <para type="description">One or more process IDs.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ParameterSetName = "byProcessId"
        )]
        [ValidateNotNullOrEmpty]
        public uint[] Id { get; set; }

        protected override void BeginProcessing()
        {
            if (!Utils.IsAdministrator())
                throw new InvalidOperationException("Cmdlet needs to run elevated.");

            if (ParameterSetName == "byName") {
                if (Name is not null)
                    foreach (string processName in Name) {
                        Process[]? process = Process.GetProcessesByName(processName);
                        if (process is null || process.Length == 0)
                            WriteWarning($"Process with name '{processName}' not found.");
                        else
                            foreach (Process p in process)
                                _processIdList.Add((uint)p.Id);
                    }
            }
            else
                if (Id is not null)
                _processIdList.AddRange(Id);
        }

        protected override void ProcessRecord()
        {
            foreach (uint processId in _processIdList)
                ProcessAndThread.SuspendProcess(processId);
        }
    }
}
