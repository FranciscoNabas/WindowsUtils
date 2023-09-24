using System.Diagnostics;
using System.Management.Automation;
using WindowsUtils.Core;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618

    [Cmdlet(
        VerbsLifecycle.Resume, "Process",
        DefaultParameterSetName = "byName"
    )]
    [Alias("resume")]
    public class ResumeProcessCommand : CoreCommandBase
    {
        private readonly Wrapper _unwrapper = new();
        private readonly List<uint> _processIdList = new();

        [Parameter(
            Mandatory = true,
            Position = 0,
            ParameterSetName = "byName"
        )]
        [ValidateNotNullOrEmpty]
        public string[] Name { get; set; }

        [Parameter(
            Mandatory = true,
            ParameterSetName = "byProcessId"
        )]
        [ValidateNotNullOrEmpty]
        public uint[] ProcessId { get; set; }

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
                if (ProcessId is not null)
                _processIdList.AddRange(ProcessId);
        }

        protected override void ProcessRecord()
        {
            foreach (uint processId in _processIdList)
                _unwrapper.ResumeProcess(processId, (CmdletContextBase)CmdletContext);
        }
    }
}
