using System.Management.Automation;
using WindowsUtils.Core;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">Returns the installer properties from a MSI file.</para>
    /// <para type="description">This Cmdlet returns the installer properties from the installer database.</para>
    /// <para type="description">Besides standard information, like Product Code, it brings vendor-specific information.</para>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "MsiProperties")]
    [OutputType(typeof(PSObject))]
    public class GetMsiPropertiesCommand : PSCmdlet
    {
        private string[] _path;
        private bool _shouldExpandWildcards = true;

        private readonly List<string> _validInput = new();

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
        /// <para Type="description">Provider-aware file system object path.</para>
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

        protected override void ProcessRecord()
        {
            List<string> pathList = new();
            _validInput.Clear();

            _path ??= new[] { ".\\*" };

            foreach (string path in _path)
            {
                pathList.Clear();

                ProviderInfo provider;

                if (_shouldExpandWildcards)
                    pathList.AddRange(GetResolvedProviderPathFromPSPath(path, out provider));

                else
                    pathList.Add(SessionState.Path.GetUnresolvedProviderPathFromPSPath(path, out provider, out _));


                foreach (string singlePath in pathList)
                {
                    if (provider.Name == "FileSystem")
                    {
                        if (File.Exists(singlePath))
                            _validInput.Add(singlePath);
                        else
                        {
                            if (Directory.Exists(singlePath))
                                WriteWarning("Path must be a valid MSI file.");
                            else
                                WriteWarning($"File '{singlePath}' not found.");
                        }
                    }
                    else
                        WriteWarning("Only 'FileSystem' provider is supported.");
                }
            }

            Wrapper unWrapper = new();
            PSObject outObject = new();
            foreach (string path in _validInput)
                WriteObject(Utils.PSObjectFactory(unWrapper.GetMsiProperties(path).OrderBy(x => x.Key)));
        }
    }
}