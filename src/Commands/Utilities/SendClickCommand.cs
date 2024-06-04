using System.Management.Automation;
using WindowsUtils.Wrappers;

namespace WindowsUtils.Commands
{
    /// <summary>
    /// <para type="synopsis">Sends a click.</para>
    /// <para type="description">When called, this cmdlet sends a click on the current desktop.</para>
    /// </summary>
    [Cmdlet(VerbsCommunications.Send, "Click")]
    public class SendClickCommand : Cmdlet
    {
        private readonly UtilitiesWrapper _unwrapper = new();

        protected override void ProcessRecord()
            => _unwrapper.SendClick();
    }
}