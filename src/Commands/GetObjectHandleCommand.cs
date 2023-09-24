using System.Management.Automation;
using WindowsUtils.Core;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    /// <summary>
    /// <para type="synopsis">Returns all processes that have a handle to a file or directory.</para>
    /// <para type="description">This Cmdlet return all processes that have a handle to a file or directory.</para>
    /// <para type="description">This is particularly useful when a process is preventing a file from being read, modified or deleted.</para>
    /// <para type="description">Although working in a different way, this Cmdlet was designed to mimic handle.exe, from Sysinternals suite.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Get-ObjectHandle -Path "$env:windir\System32\kernel32.dll", "$env:windir\System32\ntdll.dll"</code>
    ///     <para>Returning processes that have open handles to a list of files.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-ObjectHandle $env:TEMP\*.tmp</code>
    ///     <para>Listing all .tmp files on the temp folder, and listing the processes with open handles to those files.</para>
    ///     <para>The Cmdlet lists the processes related to the files queried to make it easier identify processes when querying a list of files.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(
        VerbsCommon.Get, "ObjectHandle",
        SupportsShouldProcess = true,
        ConfirmImpact = ConfirmImpact.High
    )]
    [OutputType(typeof(ObjectHandle))]
    [Alias("gethandle")]
    public class GetObjectHandleCommand : PSCmdlet
    {
        private readonly List<ObjectHandleInput> _validInput = new();

        private string[] _path;
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

        /// <summary>
        /// <para type="description">Closes all handles to the object(s).</para>
        /// </summary>
        [Parameter(HelpMessage = "Closes all handles to the object(s) returned.")]
        public SwitchParameter CloseHandle { get; set; }

        /// <summary>
        /// <para type="description">Suppresses confirmation for 'CloseHandle'.</para>
        /// </summary>
        [Parameter(HelpMessage = "Suppresses confirmation for 'CloseHandle'.")]
        public SwitchParameter Force { get; set; }

        protected override void BeginProcessing()
        {
            if (!Utils.IsAdministrator())
                WriteWarning("This PowerShell process is not elevated. 'Get-ObjectHandle' might return incomplete results due the lack of privileges to open certain processes.");

            if (Force && !CloseHandle)
                throw new ArgumentException("'Force' can only be used with 'CloseHandle'.");
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
                    switch (provider.Name)
                    {
                        case "FileSystem":
                            if (File.Exists(singlePath) || Directory.Exists(singlePath))
                                _validInput.Add(new ObjectHandleInput(singlePath, ObjectHandleType.FileSystem));
                            else
                                WriteWarning($"Object '{singlePath}' not found.");

                            break;

                        case "Registry":
                            string? ntPath = Wrapper.GetRegistryNtPath(singlePath);
                            if (ntPath is not null)
                            {
                                // Console.WriteLine(ntPath);
                                _validInput.Add(new ObjectHandleInput(ntPath, ObjectHandleType.Registry));
                            }
                            else
                                WriteWarning($"Object '{singlePath}' not found.");

                            break;

                        default:
                            WriteWarning($"Provider '{provider.Name}' not supported.");
                            break;
                    }
                }
            }

            Wrapper unWrapper = new();
            if (CloseHandle)
            {
                if (Force || ShouldProcess(
                    "",
                    "Are you sure you want to close all handles for the input object(s)?",
                    "ATTENTION! Closing handles can lead to system malfunction.\n"
                    ))
                {
                    unWrapper.GetProcessObjectHandle(_validInput.ToArray(), true);
                    ObjectHandle[] result = unWrapper.GetProcessObjectHandle(_validInput.ToArray(), false);
                    if (result is not null)
                    {
                        WriteWarning("Failed to remove handles for the processes below.");
                        result.OrderBy(p => p.Type).ThenBy(p => p.Name).ToList().ForEach(x => WriteObject(x));
                    }
                }
            }
            else
            {
                ObjectHandle[] result = unWrapper.GetProcessObjectHandle(_validInput.ToArray(), false);
                result?.OrderBy(p => p.Type).ThenBy(p => p.Name).ToList().ForEach(x => WriteObject(x));
            }
        }
    }
}