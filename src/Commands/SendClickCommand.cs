using System.Management.Automation;
using WindowsUtils.Core;

namespace WindowsUtils.Commands
{
    /// <summary>
    /// <para type="synopsis">Sends a click.</para>
    /// <para type="description">When called, this cmdlet sends a click on the current desktop.</para>
    /// </summary>
    [Cmdlet(VerbsCommunications.Send, "Click")]
    public class SendClickCommand : Cmdlet
    {
        protected override void ProcessRecord()
        {
            Wrapper unWrapper = new();
            unWrapper.SendClick();
        }
    }
}