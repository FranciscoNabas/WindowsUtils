using System.Management.Automation;
using WindowsUtils.Core;
using WindowsUtils.Commands;

namespace WindowsUtils
{
    [Cmdlet("Start", "LabProcedure")]
    public class StartLabProcedureCommand : CoreCommandBase
    {
        private readonly LabWrapper _unwrapper = new();

        protected override void ProcessRecord()
        {
            _unwrapper.WriteVectorAsObject((CmdletContextBase)CmdletContext);
        }
    }
}
