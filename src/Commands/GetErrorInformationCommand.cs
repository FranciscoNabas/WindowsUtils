using System.Reflection;
using System.Management.Automation;

namespace WindowsUtils.Commands
{
    /// <summary>
    /// <para type="synopsis">Returns the error message for a system error code.</para>
    /// <para type="description">This Cmdlet uses FormatMessage to return the message text for a given 'Win32' system error.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Get-ErrorInformation -ErrorCode 5</code>
    ///     <para>Returning the message for a given error code.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>[System.Runtime.InteropServices.Marshal]::GetLastWin32Error() | Get-ErrorInformation</code>
    ///     <para>Calling GetLastWin32Error and providing it to the Cmdlet to get the message string.</para>
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
        /// <para type="description">This value will be passed to the 'FormatMessage' function, with the system message parameter..</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ValueFromPipeline = true,
            HelpMessage = "The Win32 error code."
        )]
        public int ErrorCode { get; set; }

        [Parameter(
            Position = 1,
            HelpMessage = "Source of the error."
        )]
        public ErrorType Source { get; set; } = ErrorType.SystemError;

        protected override void ProcessRecord()
        {
            // Trick to get the lib location.
            string corePath = Assembly.GetAssembly(typeof(ComputerSession)).Location;
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
                string? desc = reader.ReadString();
                if (desc == "N/A")
                    desc = null;

                if (currentCode == ErrorCode && !foundList.Contains(symName))
                {
                    WriteObject(new ErrorInformation(ErrorCode, symName, desc, null));
                    foundList.Add(symName);
                }
                
                // Trying with HResult.
                if (currentCode == hrInfo.Code && !foundList.Contains(symName))
                {
                    WriteObject(new ErrorInformation(ErrorCode, symName, desc, hrInfo));
                    foundList.Add(symName);
                }
            }
        }
    }
}