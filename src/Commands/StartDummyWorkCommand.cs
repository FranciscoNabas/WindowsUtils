using System.Management.Automation;
using WindowsUtils.Engine;
using WindowsUtils.Wrappers;

namespace WindowsUtils.Commands;

[Cmdlet(VerbsLifecycle.Start, "DummyWork")]
public class StartDummyWorkCommand : CoreCommandBase
{
    protected override void ProcessRecord()
    {

    }
}