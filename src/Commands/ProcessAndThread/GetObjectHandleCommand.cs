using System.Diagnostics;
using System.Management.Automation;
using WindowsUtils.Engine;
using WindowsUtils.Wrappers;

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
    /// <example>
    ///     <para></para>
    ///     <code>Get-ChildItem -Path 'C:\Windows\System32' -Filter '*.dll' | Get-ObjectHandle</code>
    ///     <para>List handles opened to all DLL files under 'C:\Windows\System32'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>gethandle -ProcessId 666</code>
    ///     <para>Lists all File and Registry handles for process '666'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-Process -Id 666 | gethandle -All</code>
    ///     <para>Lists all handles opened for the process with ID '666'.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(
        VerbsCommon.Get, "ObjectHandle",
        SupportsShouldProcess = true,
        ConfirmImpact = ConfirmImpact.High
    )]
    [OutputType(typeof(ObjectHandle), typeof(ObjectHandleInfo))]
    [Alias("gethandle")]
    public class GetObjectHandleCommand : CoreCommandBase
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
        /// <para type="description">Closes all handles to the object(s).</para>
        /// </summary>
        [Parameter(ParameterSetName = "byPath")]
        [Parameter(ParameterSetName = "byLiteral")]
        public SwitchParameter CloseHandle { get; set; }

        /// <summary>
        /// <para type="description">Used with 'CloseHandle' to close handles without prompting for confirmation.</para>
        /// </summary>
        [Parameter(ParameterSetName = "byPath")]
        [Parameter(ParameterSetName = "byLiteral")]
        public SwitchParameter Force { get; set; }

        /// <summary>
        /// <para type="description">A unique process ID. Used to list all handles from a process.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ParameterSetName = "byProcessId"
        )]
        [ValidateRange(0, int.MaxValue)]
        public int ProcessId { get; set; }

        /// <summary>
        /// <para type="description">A input process object. Used to list all handles from a process.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ParameterSetName = "byProcessObject",
            ValueFromPipeline = true
        )]
        public Process InputObject { get; set; }

        /// <summary>
        /// <para type="description">Used with process ID or process object, lists all handles for that process, not only File System or Registry handles.</para>
        /// </summary>
        [Parameter(ParameterSetName = "byProcessId")]
        [Parameter(ParameterSetName = "byProcessObject")]
        public SwitchParameter All { get; set; }

        protected override void BeginProcessing()
        {
            if (!Utils.IsAdministrator())
                WriteWarning("This PowerShell process is not elevated. 'Get-ObjectHandle' might return incomplete results due the lack of privileges to open certain processes.");

            if (Force && !CloseHandle)
                throw new ArgumentException("'Force' can only be used with 'CloseHandle'.");
        }
        protected override void ProcessRecord()
        {
            switch (ParameterSetName) {
                case "byProcessId":
                    try {
                        ProcessAndThread.ListProcessHandleInfo((uint)ProcessId, All);
                    }
                    // Error record already written to the stream.
                    catch (NativeException) { }
                    break;

                case "byProcessObject":
                    try {
                        ProcessAndThread.ListProcessHandleInfo((uint)InputObject.Id, All);
                    }
                    // Error record already written to the stream.
                    catch (NativeException) { }
                    break;

                case "byPath":
                case "byLiteral":
                    List<string> pathList = new();
                    _validInput.Clear();

                    _path ??= new[] { ".\\*" };

                    foreach (string path in _path) {
                        pathList.Clear();

                        ProviderInfo provider;

                        if (_shouldExpandWildcards)
                            pathList.AddRange(GetResolvedProviderPathFromPSPath(path, out provider));

                        else
                            pathList.Add(SessionState.Path.GetUnresolvedProviderPathFromPSPath(path, out provider, out _));


                        foreach (string singlePath in pathList) {
                            switch (provider.Name) {
                                case "FileSystem":
                                    if (File.Exists(singlePath) || Directory.Exists(singlePath))
                                        _validInput.Add(new ObjectHandleInput(singlePath, ObjectHandleType.FileSystem));
                                    else
                                        WriteWarning($"Object '{singlePath}' not found.");

                                    break;

                                case "Registry":
                                    string? ntPath = RegistryWrapper.GetRegistryNtPath(singlePath);
                                    if (ntPath is not null) {
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

                    if (CloseHandle) {
                        if (Force || ShouldProcess(
                            "",
                            "Are you sure you want to close all handles for the input object(s)?",
                            "ATTENTION! Closing handles can lead to system malfunction.\n"
                        )) {
                            try {
                                ProcessAndThread.GetProcessObjectHandle(_validInput.ToArray(), true);
                            }
                            // Error record already written to the stream.
                            catch (NativeException) { }
                        }
                    }
                    else {
                        List<ObjectHandle>? output = null;
                        try {
                            output = ProcessAndThread.GetProcessObjectHandle(_validInput.ToArray(), false);
                        }
                        // Error record already written to the stream.
                        catch (NativeException) { }

                        if (output is not null)
                            WriteObject(output.OrderBy(o => o.Type).ThenBy(o => o.Name), true);
                    }
                    break;
            }
        }
    }
}