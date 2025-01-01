using System.Data;
using System.Management.Automation;
using WindowsUtils.Engine;
using WindowsUtils.Wrappers;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618

    /// <summary>
    /// <para type="synopsis">Returns table data from an installer's database.</para>
    /// <para type="description">This Cmdlet lists the contents of a table from an installer's database.</para>
    /// <para type="description">It works on default tables as well as custom ones.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Get-MsiTableData -Path 'C:\Path\To\Installer.msi' -Table 'Feature'</code>
    ///     <para>Returns the contents of the 'Feature' table from 'Installer.msi'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-MsiTableData 'C:\Path\To\Installer.msi' 'Feature', 'Registry'</code>
    ///     <para>Returns the contents of the 'Feature' and 'Registry' tables from 'Installer.msi'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-MsiTableInfo -Path 'C:\Path\To\Installer.msi' -Table 'Feature', 'Registry' | Get-MsiTableData</code>
    ///     <para>Returns the contents fo the 'Feature' and 'Registry' tables from 'Installer.msi', from the results of 'Get-MsiTableInfo'.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "MsiTableData", DefaultParameterSetName = "withPathAndTable")]
    [OutputType(typeof(DataTable))]
    public class GetMsiTableDataCommand : CoreCommandBase
    {
        /// <summary>
        /// <para type="description">The installer path.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ParameterSetName = "withPathAndTable",
            ValueFromPipeline = false,
            HelpMessage = "The installer path."
        )]
        [ValidateNotNullOrEmpty]
        public string Path { get; set; }

        /// <summary>
        /// <para type="description">One or more table names.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 1,
            ParameterSetName = "withPathAndTable",
            ValueFromPipeline = false,
            HelpMessage = "One or more table names."
        )]
        [ValidateNotNullOrEmpty]
        public string[] Table { get; set; }

        /// <summary>
        /// <para type="description">Input object in the 'InstallerTableInfo' type.</para>
        /// </summary>
        [Parameter(
            Mandatory = true, 
            ParameterSetName = "withInputObject",
            ValueFromPipeline = true,
            HelpMessage = "Input object in the 'InstallerTableInfo' type."
        )]
        public InstallerTableInfo InputObject { get; set; }

        protected override void ProcessRecord()
        {
            switch (ParameterSetName) {
                case "withInputObject":
                    try {
                        Installer.GetMsiTableDump(InputObject.InstallerPath, InputObject);
                    }
                    // Error record already written to the stream.
                    catch (NativeException) { }

                    break;
                case "withPathAndTable":
                    try {
                        if (Installer.IsInstallerPackage(Path)) {
                            Installer.GetMsiTableDump(Path, Table);
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
                    
                    break;
            }
        }
    }
}