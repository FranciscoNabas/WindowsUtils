using System.Management.Automation;
using WindowsUtils.Installer;
using WindowsUtils.Wrappers;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618

    /// <summary>
    /// <para type="synopsis">Returns information about one or more tables in an installer's database.</para>
    /// <para type="description">This Cmdlet gets information about one or more tables from an installer's database.</para>
    /// <para type="description">You can use it to list all tables in a database, and get column specific information like type, max. length, position, etc.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Get-MsiTableInfo -Path 'C:\Path\To\Installer.msi' -Table 'Feature', 'Registry'</code>
    ///     <para>Returns information about tables 'Feature' and 'Registry', from 'Installer.msi'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-MsiTableInfo 'C:\Path\To\Installer.msi'</code>
    ///     <para>Lists the information about all tables in 'Installer.msi'.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "MsiTableInfo")]
    [OutputType(typeof(InstallerTableInfo))]
    public class GetMsiTableInfoCommand : CoreCommandBase
    {
        private static readonly InstallerWrapper _unwrapper = new();

        /// <summary>
        /// <para type="description">The installer's path.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true,
            HelpMessage = "The installer's path."
        )]
        [ValidateNotNullOrEmpty]
        public string Path { get; set; }

        /// <summary>
        /// <para type="description">One or more table names.</para>
        /// </summary>
        [Parameter(
            Position = 1,
            HelpMessage = "One or more table names."
        )]
        public string[] Table { get; set; }

        protected override void ProcessRecord()
        {
            try {
                if (InstallerWrapper.IsInstallerPackage(Path, CmdletContext)) {
                    // If 'Table' is null or empty we list all the tables.
                    foreach (InstallerTableInfoBase info in _unwrapper.GetMsiTableInfo(Path, Table, CmdletContext))
                        WriteObject(new InstallerTableInfo(Path, info));
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
            catch (ArgumentException) { }
        }
    }
}
