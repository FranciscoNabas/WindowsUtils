using System.Management.Automation;
using WindowsUtils.Wrappers;
using WindowsUtils.Installer;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618

    /// <summary>
    /// <para type="synopsis">Returns the installer's summary information.</para>
    /// <para type="description">This Cmdlet returns the summary information for a Windows Installer MSI file.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Get-MsiSummaryInfo -Path 'C:\Path\To\Installer.msi'</code>
    ///     <para>Gets the summary information for the 'Installer.msi' file.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "MsiSummaryInfo")]
    [OutputType(typeof(InstallerSummaryInfo))]
    public class GetMsiSummaryInfoCommand : CoreCommandBase
    {
        private readonly InstallerWrapper _unwrapper = new();

        /// <summary>
        /// <para type="description">The installer path.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true,
            HelpMessage = "The installer path."
        )]
        [ValidateNotNullOrEmpty]
        public string Path { get; set; }

        protected override void ProcessRecord()
        {
            try {
                if (InstallerWrapper.IsInstallerPackage(Path, CmdletContext)) {
                    WriteObject(_unwrapper.GetMsiSummaryInfo(Path, CmdletContext));
                }
                else
                    WriteError(new ErrorRecord(
                        new ArgumentException($"'{Path}' is not a valid Windows Installer package."),
                        "FileNotMsiError",
                        ErrorCategory.InvalidArgument,
                        Path
                    ));
            }
            catch (FileNotFoundException ex) {
                WriteError(new ErrorRecord(
                    ex,
                    "FileNotFoundError",
                    ErrorCategory.ObjectNotFound,
                    Path
                ));
            }
            // Error record already written to the stream.
            catch (NativeException) { }
        }
    }
}