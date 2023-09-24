using System.Management.Automation;
using WindowsUtils.Core;

namespace WindowsUtils.Commands
{
    /// <summary>
    /// <para type="synopsis">Returns the error message for a system error code.</para>
    /// <para type="description">This Cmdlet uses FormatMessage to return the message text for a given 'Win32' system error.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Get-ErrorString -ErrorCode 5</code>
    ///     <para>Returning the message for a given error code.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>[System.Runtime.InteropServices.Marshal]::GetLastWin32Error() | Get-ErrorString</code>
    ///     <para>Calling GetLastWin32Error and providing it to the Cmdlet to get the message string.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "ErrorString")]
    [OutputType(typeof(string))]
    public class GetErrorStringCommand : Cmdlet
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
            Wrapper unWrapper = new();
            WriteObject(unWrapper.GetFormattedError(ErrorCode, Source));
        }
    }
}