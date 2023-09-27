using System.Diagnostics;
using System.Management.Automation;
using WindowsUtils.Core;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618

    /// <summary>
    /// <para type="synopsis">Resumes a suspended process.</para>
    /// <para type="description">This Cmdlet resumes a process previously suspended. It does nothing in a process not suspended.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Resume-Process -Name 'notepad'</code>
    ///     <para>Resumes all notepad processes opened.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Suspend-Process -Id 666; resume -Id 666</code>
    ///     <para>Suspends and resumes process with ID 666.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(
        VerbsLifecycle.Resume, "Process",
        DefaultParameterSetName = "byName"
    )]
    [Alias("resume")]
    public class ResumeProcessCommand : CoreCommandBase
    {
        private readonly Wrapper _unwrapper = new();
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
                _unwrapper.ResumeProcess(processId, (CmdletContextBase)CmdletContext);
        }
    }
}
