using System.Management.Automation;
using WindowsUtils.Engine;
using WindowsUtils.Wrappers;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">Extracts files from a cabinet file.</para>
    /// <para type="description">This Cmdlet extracts files from one or multiple cabinet files.</para>
    /// <para type="description">It also manages automatically files that spans through multiple cabinet files.</para>
    /// <para type="description">The Cmdlet creates the same folder structure from within the cabinet, relatively to the destination.</para>
    /// <para type="description">If a file with the same name already exists it's overwritten by default.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Expand-Cabinet -Path "$env:SystemDrive\Path\To\Cabinet.cab" -Destination "$env:SystemDrive\Path\To\Destination"</code>
    ///     <para>Extracts files from 'Cabinet.cab' to the 'Destination' folder.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-ChildItem -Path 'C:\CabinetSource\MultipleCab*' | Expand-Cabinet -Destination 'C:\Path\To\Destination'</code>
    ///     <para>Extract files from all cabinet files from 'C:\CabinetSource' that matches 'MultipleCab*'.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsData.Expand, "Cabinet")]
    public class ExpandCabinetCommand : CoreCommandBase
    {
        private readonly List<string> _validPaths = new();

        private string[] _path;
        private string _destination;
        private bool _shouldExpandWildcards = true;

        /// <summary>
        /// <para type="description">The object path.</para>
        /// </summary>
        [Parameter(
            Position = 0,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true,
            ParameterSetName = "byPath",
            HelpMessage = "The object(s) path."
        )]
        [ValidateNotNullOrEmpty]
        [SupportsWildcards]
        public string[] Path {
            get { return _path; }
            set { _path = value; }
        }

        /// <summary>
        /// <para type="description">Provider-aware file system object path.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ValueFromPipeline = false,
            ValueFromPipelineByPropertyName = true,
            ParameterSetName = "byLiteral",
            HelpMessage = "The object(s) literal path."
        )]
        [Alias("PSPath")]
        [ValidateNotNullOrEmpty]
        public string[] LiteralPath {
            get { return _path; }
            set {
                _shouldExpandWildcards = false;
                _path = value;
            }
        }

        /// <summary>
        /// <para type="description">The destination folder.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 1
        )]
        [ValidateNotNullOrEmpty]
        public string Destination {
            get { return _destination; }
            set {
                DirectoryInfo fsInfo = new(value);
                if (!fsInfo.Exists)
                    throw new ArgumentException($"Directory '{value}' not found.");

                if ((fsInfo.Attributes & FileAttributes.Directory) != FileAttributes.Directory)
                    throw new ArgumentException("Destination must be a directory.");

                _destination = value;
            }
        }

        protected override void ProcessRecord()
        {
            _path ??= new[] { ".\\*" };
            List<string> resolvedPaths = new();

            foreach (string path in _path)
            {
                ProviderInfo providerInfo;
                if (_shouldExpandWildcards)
                {
                    resolvedPaths.AddRange(GetResolvedProviderPathFromPSPath(path, out providerInfo));
                    if (providerInfo.Name != "FileSystem")
                        throw new InvalidOperationException("Only the file system provider is allowed with this Cmdlet.");
                }
                else
                {
                    resolvedPaths.Add(SessionState.Path.GetUnresolvedProviderPathFromPSPath(path, out providerInfo, out _));
                    if (providerInfo.Name != "FileSystem")
                        throw new InvalidOperationException("Only the file system provider is allowed with this Cmdlet.");
                }
            }

            foreach (string path in resolvedPaths)
            {
                if (File.Exists(path))
                    _validPaths.Add(path);
                else
                    WriteWarning($"File '{path}' not found.");
            }

            if (_validPaths.Count == 0)
                return;

            foreach (string path in _validPaths)
            {
                try {
                    Containers.ExpandArchiveFile(path, Destination, ArchiveFileType.Cabinet);
                }
                // Error already written to the stream.
                catch (NativeException) { }
            }
        }
    }
}