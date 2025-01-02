using System.Data;
using System.Management.Automation;
using WindowsUtils.Engine;
using WindowsUtils.Wrappers;
using WindowsUtils.Installer;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618

    /// <summary>
    /// <para type="synopsis">Executes a query in a Windows Installer's database.</para>
    /// <para type="description">This Cmdlet executes a query in a Windows Installer MSI database.</para>
    /// <para type="description">ATTENTION! The SQL language used in the installer database is quite finicky. Be sure to check the notes, and related links for more information.</para>
    /// <para type="link">Installer SQL Syntax</para>
    /// <para type="link" uri="https://learn.microsoft.com/windows/win32/msi/sql-syntax" />
    /// <para type="link"></para>
    /// <para type="link">Example of Database Queries</para>
    /// <para type="link" uri="https://learn.microsoft.com/windows/win32/msi/examples-of-database-queries-using-sql-and-script" />
    /// <para type="link"></para>
    /// <para type="link">MsiViewExecute Function (For Marker Usage Details)</para>
    /// <para type="link" uri="https://learn.microsoft.com/windows/win32/api/msiquery/nf-msiquery-msiviewexecute" />
    /// <para type="link"></para>
    /// <para type="link">OLE Limitations on Streams</para>
    /// <para type="link" uri="https://learn.microsoft.com/windows/win32/msi/ole-limitations-on-streams" />
    /// <example>
    ///     <para></para>
    ///     <code>Invoke-MsiQuery -Path 'C:\SuperInstaller.msi' -Query 'SELECT * FROM Registry' | Format-Table -AutoSize</code>
    ///     <para>Queries all entries from the 'Registry' table, and formats the output as table.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Invoke-MsiQuery 'C:\SuperInstaller.msi' "INSERT INTO Registry (Registry, Root, ``Key``, Name, Value, Component_) VALUES ('NeatNewKey', 0, 'SOFTWARE\NeatKey', 'NeatKey', 'KeyValue', 'KeyComponent')"</code>
    ///     <para>Inserts a new row in the 'Registry' table. Note the escape in the 'Key' word.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>$parameter = [WindowsUtils.Installer.InstallerCommandParameter]::new('String', 'ProductCode'); imsisql 'C:\SuperInstaller.msi' 'Select * From Property Where Property = ?' -Parameters $parameter</code>
    ///     <para>Queries the 'ProductCode' from the 'Property' table using a parameter.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>$parameter = [WindowsUtils.Installer.InstallerCommandParameter]::new('File', 'C:\Path\To\InstallerIcon.ico'); imsisql 'C:\SuperInstaller.msi' "INSERT INTO Icon (Name, Data) VALUES ('IconName', ?)" -Parameters $parameter</code>
    ///     <para>Writes binary data into the 'Icon' table.</para>
    ///     <para></para>
    /// </example>
    /// <list type="alertSet">
    ///     <item>
    ///         <term>General Notes</term>
    ///         <description>
    ///             <para>As stated in the description, the SQL query language for the Microsoft Installer database is quite finicky.</para>
    ///             <para>The main points will be described here, but be sure to read the documentation listed in the related links.</para>
    ///             <para></para>
    ///             <para>- The errors are not always clear. They come from the MSI API engine, and it's the best we can do without writing a full parser.</para>
    ///             <para>- The character used to escape table names, and other words is the backtick `, which it's also a PowerShell escape character. When escaping words review the necessity of doubling them ``.</para>
    ///             <para>- The number of parameters in the 'Parameters' input needs to be the same as the number of markers '?' in the query. Read more about markers in the MSI documentation.</para>
    ///             <para>- You can write binary data from a file and from a byte array, but there are limitations. Read more about data stream limitations in the MSI documentation.</para>
    ///             <para>- The SQL statement has to be written in one line, unfortunately the MSI database doesn't support here-strings. It's the best we can do without writing a parser.</para>
    ///             <para>- Querying data using this Cmdlet, although faster, doesn't return column key information. To get more complete data use the 'Get-MsiTableData' Cmdlet.</para>
    ///             <para></para>
    ///             <para>Good luck.</para>
    ///         </description>
    ///     </item>
    /// </list>
    /// </summary>
    [Cmdlet(VerbsLifecycle.Invoke, "MsiQuery")]
    [OutputType(typeof(DataTable))]
    [Alias(new string[] {"imsisql"})]
    public class InvokeMsiQueryCommand : CoreCommandBase
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

        /// <summary>
        /// <para type="description">The query to be executed in the installer database.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 1,
            HelpMessage = "The query to be executed in the installer database."
        )]
        public string Query { get; set; }

        /// <summary>
        /// <para type="description">One or more parameters to be used with the query. Check notes for more information.</para>
        /// </summary>
        [Parameter(HelpMessage = "One or more parameters to be used with the query. Check notes for more information.")]
        public InstallerCommandParameter[] Parameters { get; set; }

        protected override void ProcessRecord()
        {
            var command = InstallerSqlParser.Parse(Query);

            List<InstallerCommandParameter> parameters;
            if (Parameters is not null && Parameters.Length > 0)
                parameters = new(Parameters);
            else
                parameters = new();

            try {
                Installer.InvokeMsiQuery(Path, command, parameters);
            }
            // Error record already written to the stream.
            catch (ArgumentException) { }
            catch (NativeException) { }
        }
    }
}