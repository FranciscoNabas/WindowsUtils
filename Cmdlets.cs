using System.Management.Automation;
using System.Runtime.InteropServices;
using WindowsUtils.Core;

namespace WindowsUtils.Commands
{
    /// <summary>
    /// <para type="synopsis">Sends a message to sessions on local or remote computers.</para>
    /// <para type="description">The Invoke-RemoteMessage cmdlet sends a MessageBox-Style message to sessions on local or remote computers.</para>
    ///</summary>
    [Cmdlet(
        VerbsLifecycle.Invoke, "RemoteMessage",
        SupportsShouldProcess = true,
        ConfirmImpact = ConfirmImpact.High
    )]
    public class InvokeRemoteMessageCommand : Cmdlet
    {
        /// <summary>
        /// <para type="description">The message box title.</para>
        /// </summary>
        [Parameter(Mandatory = true, Position = 0)]
        [ValidateNotNullOrEmpty]
        public string Title { get; set; }

        /// <summary>
        /// <para type="description">The message text.</para>
        /// </summary>
        [Parameter(Mandatory = true, Position = 1)]
        [ValidateNotNullOrEmpty]
        public string Message { get; set; }

        /// <summary>
        /// <para type="description">The computer name to send the nessage.</para>
        /// <para type="description">If not specified, sends the message to the local host.</para>
        /// </summary>
        [Parameter()]
        public string ComputerName { get; set; }

        /// <summary>
        /// <para type="description">The session(s) to receive the message.</para>
        /// <para type="description">If not specified, sends the message to all sessions.</para>
        /// </summary>
        [Parameter()]
        public int[] SessionId { get; set; }

        /// <summary>
        /// <para type="description">A list of MessageBox style enum objects.</para>
        /// <para type="description">To obtaing a list of available options, call 'Get-RemoteMessageOptions', or see definition for the MessageBox function.</para>
        /// </summary>
        [Parameter()]
        [ValidateNotNullOrEmpty]
        public string[] Style { get; set; } = { "MB_OK" };

        /// <summary>
        /// <para type="description">The timeout, in seconds, to wait for a response.</para>
        /// <para type="description">If the timeout expires, the cmdlet returns 'Timeout'. The message is not closed.</para>
        /// </summary>
        [Parameter()]
        public int Timeout { get; set; } = 0;

        /// <summary>
        /// <para type="description">If called, waits for the user response in the specified 'Timeout'.</para>
        /// <para type="description">If not, the Cmdlet returns 'AsyncReturn'.</para>
        /// </summary>
        [Parameter()]
        public SwitchParameter Wait { get; set; } = false;

        protected override void ProcessRecord()
        {
            WrappedFunctions unwrapper = new();
            uint nativestyle = MessageBoxOption.MbOptionsResolver(Style);
            List<int> result = new();

            if (string.IsNullOrEmpty(ComputerName))
            {
                if (SessionId.Length == 0)
                {
                    if (ShouldProcess(
                        "Sending message to all sessions on the current computer.",
                        "Are you sure you want to send the message to all sessions on this computer?",
                        "Send message"
                        ))
                    {
                        result = unwrapper.InvokeMessage(IntPtr.Zero, null, Title, Message, nativestyle, Timeout, Wait);
                        result.ForEach(x => WriteObject((MessageBoxReturn)x));
                    }
                }
                else
                {
                    result = unwrapper.InvokeMessage(IntPtr.Zero, SessionId, Title, Message, nativestyle, Timeout, Wait);
                    result.ForEach(x => WriteObject((MessageBoxReturn)x));
                }
            }
            else
            {
                if (SessionId.Length == 0)
                {
                    if (ShouldProcess(
                        "Sending message to all sessions on computer " + ComputerName + ".",
                        "Are you sure you want to send the message to all sessions on computer " + ComputerName + "?",
                        "Send message"
                        ))
                    {
                        WtsSession.StageComputerSession(ComputerName);
                        if (WtsSession.SessionHandle is not null)
                            result = unwrapper.InvokeMessage(WtsSession.SessionHandle.ToIntPtr(), null, Title, Message, nativestyle, Timeout, Wait);
                        
                        result.ForEach(x => WriteObject((MessageBoxReturn)x));
                    }
                }
                else
                {
                    WtsSession.StageComputerSession(ComputerName);
                    if (WtsSession.SessionHandle is not null)
                        result = unwrapper.InvokeMessage(WtsSession.SessionHandle.ToIntPtr(), SessionId, Title, Message, nativestyle, Timeout, Wait);

                    result.ForEach(x => WriteObject((MessageBoxReturn)x));
                }
            }
        }
    }

    /// <summary>
    /// <para type="synopsis">Retrieves the options to be used with Invoke-RemoteMessage.</para>
    /// <para type="description">This Cmdlet returns a list of options available to be used with the parameter 'Style', from Invoke-RemoteMessage.</para>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "RemoteMessageOptions")]
    public class GetRemoteMessageOptionsCommand : Cmdlet
    {
        protected override void ProcessRecord()
        {
            WriteObject(MessageBoxOption.GetAvailableOptions(), true);
        }
    }

    /// <summary>
    /// <para type="synopsis">Returns the user sessions on the local or remote computer.</para>
    /// <para type="description">This Cmdlet returns the sessions on the local or remote computer.</para>
    /// <para type="description">By default, only brings sessions that has an user name assigned.</para>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "ComputerSession")]
    public class GetComputerSessionCommand : Cmdlet
    {
        /// <summary>
        /// <para type="description">The computer name.</para>
        /// <para type="description">If no computer name is informed, it returns sessions on the current computer.</para>
        /// </summary>
        [Parameter(Position = 0)]
        [ValidateNotNullOrEmpty]
        public string ComputerName { get; set; }

        /// <summary>
        /// <para type="description">When called, this Cmdlet returns only active sessions.</para>
        /// </summary>
        [Parameter()]
        public SwitchParameter ActiveOnly { get; set; } = false;

        /// <summary>
        /// <para type="description">When called, this Cmdlet includes sessions without an user name assigned.</para>
        /// <para type="description">Sessions without user name are marked as 'System'.</para>
        /// </summary>
        [Parameter()]
        public SwitchParameter IncludeSystemSession { get; set; } = false;

        protected override void ProcessRecord()
        {
            WrappedFunctions unwrapper = new();
            
            if (string.IsNullOrEmpty(ComputerName))
            {
                WtsSession.StageComputerSession(ComputerName);
                if (WtsSession.SessionHandle is not null)
                    WriteObject((ComputerSession[])unwrapper.GetEnumeratedSession(ComputerName, WtsSession.SessionHandle.ToIntPtr(), ActiveOnly, IncludeSystemSession));
            }
            else
                WriteObject((ComputerSession[])unwrapper.GetEnumeratedSession(ComputerName, IntPtr.Zero, ActiveOnly, IncludeSystemSession));

        }
    }

    /// <summary>
    /// <para type="synopsis">Sends a click.</para>
    /// <para type="description">When called, this cmdlet sends a click on the current desktop.</para>
    /// </summary>
    [Cmdlet(VerbsCommunications.Send, "Click")]
    public class SendClickCommand : Cmdlet
    {
        protected override void ProcessRecord()
        {
            WrappedFunctions unwrapper = new();
            unwrapper.SendClick();
        }
    }

    /// <summary>
    /// <para type="synopsis">Returns the message table stored in a file.</para>
    /// <para type="description">This Cmdlet lists all messages, and their index from a resource message table.</para>
    /// <para type="description">These message tables are stored into files, like DLL or EXE.</para>
    /// <para type="description">I.E.: on kernel32.dll are stored the Win32 system error messages.</para>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "ResourceMessageTable")]
    public class GetResourceMessageTableCommand : Cmdlet
    {
        /// <summary>
        /// <para type="description">The path to the file containing the message table.</para>
        /// </summary>
        [Parameter(Mandatory = true, Position = 0, ValueFromPipeline = true)]
        [ValidateNotNullOrEmpty]
        public string Path { get; set; }

        protected override void ProcessRecord()
        {
            WrappedFunctions unWrapper = new();
            WriteObject(unWrapper.GetResourceMessageTable(Path));
        }
    }

    /// <summary>
    /// <para type="synopsis">Returns the error message for a system error code.</para>
    /// <para type="description">This Cmdlet uses FormatMessage to return the message text for a given 'Win32' system error.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Get-FormattedMessage -ErrorCode 5</code>
    ///     <para>Returning the message for a given error code.</para>
    ///     <para></para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>[System.Runtime.InteropServices.Marshal]::GetLastWin32Error() | Get-FormattedError</code>
    ///     <para>Calling GetLastWin32Error and providing it to the Cmdlet to get the message string.</para>
    ///     <para></para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "FormattedError")]
    public class GetFormattedErrorCommand : Cmdlet
    {
        /// <summary>
        /// <para type="description">The error code.</para>
        /// </summary>
        [Parameter(Mandatory = true, Position = 0, ValueFromPipeline = true)]
        public int ErrorCode { get; set; }

        protected override void ProcessRecord()
        {
            WrappedFunctions unWrapper = new();
            WriteObject(unWrapper.GetFormatedError(ErrorCode));
        }
    }

    /// <summary>
    /// <para type="synopsis">Returns the last 'Win32' system error message.</para>
    /// <para type="description">This Cmdlet gets the last error thrown by the system, and returns the message string.</para>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "LastWin32Error")]
    public class GetLastWin32ErrorCommand : Cmdlet
    {
        protected override void ProcessRecord()
        {
            WrappedFunctions unWrapper = new();
            WriteObject(unWrapper.GetFormatedWin32Error());
        }
    }

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
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-ObjectHandle $env:TEMP\*.tmp</code>
    ///     <para>Listing all .tmp files on the temp folder, and listing the processes with open handles to those files.</para>
    ///     <para>The Cmdlet lists the processes related to the files queried to make it easier identify processes when querying a list of files.</para>
    ///     <para></para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "ObjectHandle")]
    public class GetObjectHandleCommand : PSCmdlet
    {
        private List<string> validPaths = new List<string>();
        private string[] _path;
        private bool _shouldExpandWildcards;

        /// <summary>
        /// <para type="description">The file system object path.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true,
            ParameterSetName = "byPath"
        )]
        [ValidateNotNullOrEmpty]
        [SupportsWildcards()]
        public string[] Path
        {
            get { return _path; }
            set
            {
                _shouldExpandWildcards = true;
                _path = value;
            }
        }

        /// <summary>
        /// <para Type="description">Provider-aware file system object path.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ValueFromPipeline = false,
            ValueFromPipelineByPropertyName = true,
            ParameterSetName = "byLiteral"
        )]
        [Alias("PSPath")]
        [ValidateNotNullOrEmpty]
        public string[] LiteralPath
        {
            get { return _path; }
            set { _path = value; }
        }

        protected override void ProcessRecord()
        {
            List<string> pathlist = new();
            validPaths.Clear();
            foreach (string path in _path)
            {
                ProviderInfo provider;
                PSDriveInfo drive;

                if (_shouldExpandWildcards)
                    pathlist.AddRange(this.GetResolvedProviderPathFromPSPath(path, out provider));

                else
                    pathlist.Add(this.SessionState.Path.GetUnresolvedProviderPathFromPSPath(path, out provider, out drive));

                foreach (string filepath in pathlist)
                {
                    if (File.Exists(filepath) || Directory.Exists(filepath))
                        validPaths.Add(filepath);
                    else
                        WriteWarning("Object '" + filepath + "' not found.");
                }

            }

            if (validPaths.Count == 0)
                throw new ItemNotFoundException("No object found for the specified path(s).");

            WrappedFunctions unWrapper = new();
            WriteObject(unWrapper.GetProcessObjectHandle(validPaths.ToArray()), true);
        }
    }

    /// <summary>
    /// <para type="synopsis">Returns the installer properties from a MSI file.</para>
    /// <para type="description">This Cmdlet returns the installer properties from the installer database.</para>
    /// <para type="description">Besides standard information, like Product Code, it brings vendor-specific information.</para>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "MsiProperties")]
    public class GetMsiPropertiesCommand : Cmdlet
    {
        /// <summary>
        /// <para type="description">The MSI file path.</para>
        /// </summary>
        [Parameter(Mandatory = true, Position = 0, ValueFromPipeline = true)]
        [ValidateNotNullOrEmpty]
        public string Path { get; set; }

        protected override void ProcessRecord()
        {
            WrappedFunctions unWrapper = new();
            WriteObject(unWrapper.GetMsiProperties(Path));
        }
    }

    /// <summary>
    /// <para type="synopsis">Logs off an interactive session on the local or remote computer.</para>
    /// <para type="description">This Cmdlet disconnects, or logs off, an interactive session on the local or remote computer.</para>
    /// <para type="description">You can retrieve the SessionId with Get-ComputerSession, and use this Cmdled to logoff users.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Disconnect-Session -SessionId 3</code>
    ///     <para>Disconnects a session with a given session id.</para>
    ///     <para></para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-ComputerSession -ComputerName 'MYAWESOMEPC.contoso.com' | Where-Object { $PSItem.UserName -eq 'CONTOSO\user.name' } | Disconnect-Session -ComputerName 'MYAWESOMEPC.contoso.com' -SessionId $PSItem.SessionId </code>
    ///     <para>Gets the session from a specific user on a remote computer, and loggs it off.</para>
    ///     <para></para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(
        VerbsCommunications.Disconnect, "Session",
        SupportsShouldProcess = true,
        ConfirmImpact = ConfirmImpact.High,
        DefaultParameterSetName = "NoComputerName"
    )]
    [Alias("disconnect")]
    public class DisconnectSessionCommand : Cmdlet
    {
        /// <summary>
        /// <para type="description">The computer name to disconnect a session.</para>
        /// <para type="description">If not informed, it disconnects the session from the local computer.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ParameterSetName = "WithComputerName",
            ValueFromPipelineByPropertyName = true
        )]
        [ValidateNotNullOrEmpty()]
        public string ComputerName { get; set; }

        /// <summary>
        /// <para type="description">The session id to disconnect.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ParameterSetName = "WithComputerName",
            ValueFromPipelineByPropertyName = true
        )]
        [Parameter(
            Mandatory = true,
            ParameterSetName = "NoComputerName",
            ValueFromPipelineByPropertyName = true
        )]
        public int SessionId { get; set; }

        /// <summary>
        /// <para type="description">If called, the Cmdlet waits the logoff process to finish before returning.</para>
        /// </summary>
        [Parameter(ParameterSetName = "WithComputerName")]
        [Parameter(ParameterSetName = "NoComputerName")]
        [ValidateNotNullOrEmpty()]
        public SwitchParameter Wait { get; set; }

        protected override void ProcessRecord()
        {
            if (string.IsNullOrEmpty(ComputerName))
            {
                if (SessionId == null)
                {
                    if (ShouldProcess(
                        "Disconnecting current session from this computer.",
                        "Are you sure you want to disconnect the current session on this computer?",
                        "Disconnect session"))
                    {
                        Utilities.DisconnectSession(false);
                    }
                }
                else
                {
                    if (ShouldProcess(
                        "Disconnecting session ID " + SessionId + " from current computer.",
                        "Are you sure you want to disconnect sesion ID " + SessionId + " on the current computer?",
                        "Disconnect session"))
                    {
                        Utilities.DisconnectSession(null, (int)SessionId, Wait, false);
                    }
                }
            }
            else
            {
                if (ShouldProcess(
                        "Disconnecting session ID " + SessionId + " from computer " + ComputerName + ".",
                        "Are you sure you want to disconnect sesion ID " + SessionId + " on computer " + ComputerName + "?",
                        "Disconnect session"))
                {
                    Utilities.DisconnectSession(ComputerName, (int)SessionId, Wait, false);
                }
            }
        }
    }
}