using System.Reflection;
using System.Management.Automation;

namespace WindowsUtils.Commands
{
    /// <summary>
    /// <para type="synopsis">Returns error information for an error code.</para>
    /// <para type="description">This Cmdlet returns error information about an error code.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Get-ErrorInformation -ErrorCode 5</code>
    ///     <para>Returning information for a given error code.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>[System.Runtime.InteropServices.Marshal]::GetLastWin32Error() | Get-ErrorInformation</code>
    ///     <para>Calling GetLastWin32Error and providing it to the Cmdlet to get the error information.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "ErrorInformation")]
    [OutputType(typeof(ErrorInformation))]
    [Alias("err")]
    public class GetErrorInformationCommand : Cmdlet
    {
        /// <summary>
        /// <para type="description">The error code.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ValueFromPipeline = true,
            HelpMessage = "The error code."
        )]
        public int ErrorCode { get; set; }

        protected override void ProcessRecord()
        {
            // Trick to get the lib location.
            string corePath = Assembly.GetAssembly(typeof(ErrorInformation)).Location;
            string libPath = Path.Combine(Path.GetDirectoryName(corePath), "ErrorLibrary.dll");

            // Creating the binary reader.
            using BinaryReader reader = new(new FileStream(libPath, FileMode.Open, FileAccess.Read, FileShare.Read));

            // Reading the total record count.
            uint count = reader.ReadUInt32();

            HResultInfo hrInfo = new(ErrorCode);
            List<string> foundList = new();
            for (uint i = 0; i < count; i++)
            {
                if (reader.BaseStream.Position >= reader.BaseStream.Length)
                    break;

                int currentCode = reader.ReadInt32();

                string symName = reader.ReadString();
                string source = reader.ReadString();
                string? desc = reader.ReadString();
                if (desc == "N/A")
                    desc = null;

                if (currentCode == ErrorCode && !foundList.Contains(symName))
                {
                    WriteObject(new ErrorInformation(ErrorCode, symName, source, desc, null));
                    foundList.Add(symName);
                }
                
                // Trying with HResult.
                if (currentCode == hrInfo.Code && !foundList.Contains(symName))
                {
                    WriteObject(new ErrorInformation(ErrorCode, symName, source, desc, hrInfo));
                    foundList.Add(symName);
                }
            }
        }
    }
}