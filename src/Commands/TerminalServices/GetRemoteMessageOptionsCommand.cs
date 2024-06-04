using System.Management.Automation;

namespace WindowsUtils.Commands
{
    /// <summary>
    /// <para type="synopsis">Retrieves the options to be used with Invoke-RemoteMessage.</para>
    /// <para type="description">This Cmdlet returns a list of options available to be used with the parameter 'Style', from Invoke-RemoteMessage.</para>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "RemoteMessageOptions")]
    [OutputType(typeof(MessageBoxOption[]))]
    public class GetRemoteMessageOptionsCommand : Cmdlet
    {
        protected override void ProcessRecord()
        {
            WriteObject(MessageBoxOption.GetAvailableOptions(), true);
        }
    }
}