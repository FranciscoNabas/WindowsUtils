using System.Management.Automation;
using WindowsUtils.Engine;
using WindowsUtils.Wrappers;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">Returns the installer properties from a MSI file.</para>
    /// <para type="description">This Cmdlet returns the installer properties from the installer database.</para>
    /// <para type="description">Besides standard information, like Product Code, it brings vendor-specific information.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Get-MsiProperties -Path 'C:\Path\To\Installer.msi'</code>
    ///     <para>Lists the contents of the 'Property' table for 'Installer.msi'.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "MsiProperties")]
    [OutputType(typeof(PSObject))]
    public class GetMsiPropertiesCommand : CoreCommandBase
    {
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
                if (Installer.IsInstallerPackage(Path)) {
                    WriteObject(Utils.PSObjectFactory(Installer.GetMsiProperties(Path).OrderBy(x => x.Key)));
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