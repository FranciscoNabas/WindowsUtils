using System.Management.Automation;
using WindowsUtils.Attributes;
using WindowsUtils.Core;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">Returns the message table stored in a file.</para>
    /// <para type="description">This Cmdlet lists all messages, and their index from a resource message table.</para>
    /// <para type="description">These message tables are stored into files, like DLL or EXE.</para>
    /// <para type="description">I.E.: on kernel32.dll are stored the Win32 system error messages.</para>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "ResourceMessageTable")]
    [OutputType(typeof(ResourceMessageTable))]
    public class GetResourceMessageTableCommand : Cmdlet
    {
        /// <summary>
        /// <para type="description">The path to the file containing the message table.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ValueFromPipelineByPropertyName = true,
            HelpMessage = "The path to the file containing the messages.")]
        [ValidateNotNullOrEmpty]
        [ValidateFileExists]
        public string Path { get; set; }

        protected override void ProcessRecord()
        {
            Wrapper unWrapper = new();
            ResourceMessageTable[] result = unWrapper.GetResourceMessageTable(Path);
            result.ToList().ForEach(x => WriteObject(x));
        }
    }
}