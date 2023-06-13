using System.ServiceProcess;
using System.Management.Automation;
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
        ConfirmImpact = ConfirmImpact.High,
        DefaultParameterSetName = "NoWait"
    )]
    public class InvokeRemoteMessageCommand : Cmdlet
    {
        private int _timeout = 0;

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
        [Parameter(Mandatory = true, ParameterSetName = "WithWait")]
        [Parameter(ParameterSetName = "NoWait")]
        public int Timeout
        {
            get
            {
                if (true == Wait && _timeout == 0)
                    throw new ArgumentException("'Timeout' cannot be zero when using 'Wait'.");

                return _timeout;
            }
            set
            {
                if (true == Wait && value == 0)
                    throw new ArgumentException("'Timeout' cannot be zero when using 'Wait'.");
                else
                    _timeout = value;
            }
        }

        /// <summary>
        /// <para type="description">If called, waits for the user response in the specified 'Timeout'.</para>
        /// <para type="description">If not, the Cmdlet returns 'AsyncReturn'.</para>
        /// </summary>
        [Parameter(ParameterSetName = "WithWait")]
        public SwitchParameter Wait { get; set; } = false;

        protected override void ProcessRecord()
        {
            WrapperFunctions unwrapper = new();
            uint nativestyle = MessageBoxOption.MbOptionsResolver(Style);

            /*
             * Querying Timeout value, this will throw if Wait is used with Timeout = 0 before showing the warning.
             * TODO: Find a better way?
             */
            int dummy = Timeout;

            if (string.IsNullOrEmpty(ComputerName))
            {
                if (SessionId is null)
                {
                    if (ShouldProcess(
                        "",
                        "Are you sure you want to send the message to all sessions on this computer?",
                        "Sending message to all sessions on the current computer."
                        ))
                    {
                        try
                        {
                            MessageResponseBase[] result = unwrapper.InvokeRemoteMessage(IntPtr.Zero, null, Title, Message, nativestyle, Timeout, Wait);
                            result.Where(q => q is not null).ToList().ForEach(x => WriteObject((MessageResponse)x));
                        }
                        catch (Core.NativeException ex)
                        {
                            throw (NativeException)ex;
                        }
                    }
                }
                else
                {
                    if (SessionId.Contains(0))
                    {
                        if (ShouldProcess(
                        "",
                        "Are you sure you want to send the message to all sessions on this computer?",
                        "Session '0' represent all sessions on the computer"
                        ))
                        {
                            try
                            {
                                MessageResponseBase[] result = unwrapper.InvokeRemoteMessage(IntPtr.Zero, null, Title, Message, nativestyle, Timeout, Wait);
                                result.Where(q => q is not null).ToList().ForEach(x => WriteObject((MessageResponse)x));
                            }
                            catch (Core.NativeException ex)
                            {
                                throw (NativeException)ex;
                            }
                        }
                    }
                    else
                    {
                        try
                        {
                            MessageResponseBase[] result = unwrapper.InvokeRemoteMessage(IntPtr.Zero, SessionId, Title, Message, nativestyle, Timeout, Wait);
                            result.Where(q => q is not null).ToList().ForEach(x => WriteObject((MessageResponse)x));
                        }
                        catch (Core.NativeException ex)
                        {
                            throw (NativeException)ex;
                        }
                    }
                }
            }
            else
            {
                using WtsSession session = new(ComputerName);
                if (SessionId is null)
                {
                    if (ShouldProcess(
                        "",
                        "Are you sure you want to send the message to all sessions on computer " + ComputerName + "?",
                        "Sending message to all sessions on computer \" + ComputerName + \"."
                        ))
                    {
                        try
                        {
                            MessageResponseBase[] result = unwrapper.InvokeRemoteMessage(session.SessionHandle.DangerousGetHandle(), null, Title, Message, nativestyle, Timeout, Wait);
                            result.Where(q => q is not null).ToList().ForEach(x => WriteObject((MessageResponse)x));
                        }
                        catch (Core.NativeException ex)
                        {
                            throw (NativeException)ex;
                        }
                    }
                }
                else
                {
                    if (SessionId.Contains(0))
                    {
                        if (ShouldProcess(
                        "",
                        "Are you sure you want to send the message to all sessions on computer " + ComputerName + "?",
                        "Session '0' represent all sessions on the computer"
                        ))
                        {
                            try
                            {
                                MessageResponseBase[] result = unwrapper.InvokeRemoteMessage(session.SessionHandle.DangerousGetHandle(), null, Title, Message, nativestyle, Timeout, Wait);
                                result.Where(q => q is not null).ToList().ForEach(x => WriteObject((MessageResponse)x));
                            }
                            catch (Core.NativeException ex)
                            {
                                throw (NativeException)ex;
                            }
                        }
                    }
                    else
                    {
                        try
                        {
                            MessageResponseBase[] result = unwrapper.InvokeRemoteMessage(session.SessionHandle.DangerousGetHandle(), SessionId, Title, Message, nativestyle, Timeout, Wait);
                            result.Where(q => q is not null).ToList().ForEach(x => WriteObject((MessageResponse)x));
                        }
                        catch (Core.NativeException ex)
                        {
                            throw (NativeException)ex;
                        }
                    }
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
            WrapperFunctions unwrapper = new();
            try
            {
                if (string.IsNullOrEmpty(ComputerName))
                {
                    ComputerSessionBase[] result = unwrapper.GetComputerSession(ComputerName, IntPtr.Zero, ActiveOnly, IncludeSystemSession);
                    result.ToList().ForEach(x => WriteObject((ComputerSession)x));
                }
                else
                {
                    using WtsSession session = new(ComputerName);
                    ComputerSessionBase[] result = unwrapper.GetComputerSession(ComputerName, session.SessionHandle.DangerousGetHandle(), ActiveOnly, IncludeSystemSession);
                    result.ToList().ForEach(x => WriteObject((ComputerSession)x));
                }
            }
            catch (Core.NativeException ex)
            {
                throw (NativeException)ex;
            }
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
            WrapperFunctions unwrapper = new();
            try
            {
                unwrapper.SendClick();
            }
            catch (Core.NativeException ex)
            {
                throw (NativeException)ex;
            }
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
            WrapperFunctions unWrapper = new();
            try
            {
                ResourceMessageTableCore[] result = unWrapper.GetResourceMessageTable(Path);
                result.ToList().ForEach(x => WriteObject((ResourceMessageTable)x));
            }
            catch (Core.NativeException ex)
            {
                throw (NativeException)ex;
            }
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
            WrapperFunctions unWrapper = new();
            try
            {
                WriteObject(unWrapper.GetFormattedError(ErrorCode));
            }
            catch (Core.NativeException ex)
            {
                throw (NativeException)ex;
            }
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
            WrapperFunctions unWrapper = new();
            try
            {
                WriteObject(unWrapper.GetLastWin32Error());
            }
            catch (Core.NativeException ex)
            {
                throw (NativeException)ex;
            }
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
    [Cmdlet(
        VerbsCommon.Get, "ObjectHandle",
        SupportsShouldProcess = true,
        ConfirmImpact = ConfirmImpact.High
    )]
    [Alias("gethandle")]
    public class GetObjectHandleCommand : PSCmdlet
    {
        private List<string> validPaths = new();
        private string[] _path;
        private bool _shouldExpandWildcards = true;

        /// <summary>
        /// <para type="description">The file system object path.</para>
        /// </summary>
        [Parameter(
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
            set { _path = value; }
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
            set
            {
                _shouldExpandWildcards = false;
                _path = value;
            }
        }

        [Parameter()]
        public SwitchParameter CloseHandle { get; set; }

        [Parameter()]
        public SwitchParameter Force { get; set; }

        protected override void BeginProcessing()
        {
            if (Force && !CloseHandle)
                throw new ArgumentException("'Force' can only be used with 'CloseHandle'.");
        }
        protected override void ProcessRecord()
        {
            List<string> pathlist = new();
            validPaths.Clear();

            _path ??= new[] { ".\\*" };

            foreach (string path in _path)
            {
                ProviderInfo provider;

                if (_shouldExpandWildcards)
                    pathlist.AddRange(GetResolvedProviderPathFromPSPath(path, out provider));

                else
                    pathlist.Add(SessionState.Path.GetUnresolvedProviderPathFromPSPath(path, out provider, out PSDriveInfo drive));

                // FIX THIS!!!!!!
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

            WrapperFunctions unWrapper = new();
            if (CloseHandle)
            {
                if (Force || ShouldProcess(
                    "",
                    "Are you sure you want to close all handles for the input object(s)?",
                    "ATTENTION! Closing handles can lead to system malfunction.\n"
                    ))
                {
                    try
                    {
                        unWrapper.GetProcessObjectHandle(validPaths.ToArray(), true);
                        ObjectHandleBase[] result = unWrapper.GetProcessObjectHandle(validPaths.ToArray(), false);
                        if (result is not null)
                        {
                            WriteWarning("Failed to remove handles for the processes below.");
                            result.ToList().ForEach(x => WriteObject((ObjectHandle)x));
                        }
                    }
                    catch (Core.NativeException ex)
                    {
                        throw (NativeException)ex;
                    }
                }
            }
            else
            {
                try
                {
                    ObjectHandleBase[] result = unWrapper.GetProcessObjectHandle(validPaths.ToArray(), false);
                    result?.ToList().ForEach(x => WriteObject((ObjectHandle)x));
                }
                catch (Core.NativeException ex)
                {
                    throw (NativeException)ex;
                }
            }
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
            WrapperFunctions unWrapper = new();
            PSObject outObject = new();
            try
            {
                foreach (KeyValuePair<string, string> item in unWrapper.GetMsiProperties(Path).OrderBy(x => x.Key))
                    outObject.Members.Add(new PSNoteProperty(item.Key, item.Value));
            }
            catch (Core.NativeException ex)
            {
                throw (NativeException)ex;
            }

            WriteObject(outObject);
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
            Mandatory = false,
            ParameterSetName = "NoComputerName",
            ValueFromPipelineByPropertyName = true
        )]
        public uint SessionId { get; set; }

        /// <summary>
        /// <para type="description">If called, the Cmdlet waits the logoff process to finish before returning.</para>
        /// </summary>
        [Parameter(ParameterSetName = "WithComputerName")]
        [Parameter(ParameterSetName = "NoComputerName")]
        public SwitchParameter Wait { get; set; }

        protected override void ProcessRecord()
        {
            WrapperFunctions unWrapper = new();

            if (string.IsNullOrEmpty(ComputerName))
            {
                if (SessionId == 0)
                {
                    if (ShouldProcess(
                    "Disconnecting current session from this computer.",
                    "Are you sure you want to disconnect the current session on this computer?",
                    "Disconnect session"))
                    {
                        try
                        {
                            unWrapper.DisconnectSession(IntPtr.Zero, SessionId, Wait);
                        }
                        catch (Core.NativeException ex)
                        {
                            throw (NativeException)ex;
                        }
                    }
                }
                else
                {
                    if (ShouldProcess(
                   "Disconnecting session ID " + SessionId + " from current computer.",
                   "Are you sure you want to disconnect sesion ID " + SessionId + " on the current computer?",
                   "Disconnect session"))
                    {
                        try
                        {
                            unWrapper.DisconnectSession(IntPtr.Zero, SessionId, Wait);
                        }
                        catch (Core.NativeException ex)
                        {
                            throw (NativeException)ex;
                        }
                    }
                }
            }
            else
            {
                using WtsSession session = new(ComputerName);
                if (ShouldProcess(
                   "Disconnecting session ID " + SessionId + " from computer " + ComputerName + ".",
                   "Are you sure you want to disconnect session ID " + SessionId + " on computer " + ComputerName + "?",
                   "Disconnect session"))
                {
                    try
                    {
                        unWrapper.DisconnectSession(session.SessionHandle.DangerousGetHandle(), SessionId, Wait);
                    }
                    catch (Core.NativeException ex)
                    {
                        throw (NativeException)ex;
                    }
                }
            }
        }
    }

    /// <summary>
    /// <para type="synopsis">Deletes a service from the local or remote computer.</para>
    /// <para type="description">This cmdlet removes (deletes) a specified service from the local or remote computer.</para>
    /// <para type="description">If the service is running, and you don't use 'Stop', it will be marked to deletion.</para>
    /// <para type="description">If the service have dependents, and 'Stop' is not used, it will be marked for deletion.</para>
    /// <para type="description">If the service have dependents, and you use 'Stop', it will stop all dependent services.</para>
    /// <para type="description"></para>
    /// <para type="description">Attention!</para>
    /// <para type="description">The act of deleting a service is ultimately marking it for deletion.</para>
    /// <para type="description">A service is only permanently deleted when it's stopped and there are no more open handles to it.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Remove-Service -Name 'MyCoolService'</code>
    ///     <para>Removes the service 'MyCoolService'.</para>
    ///     <para></para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Remove-Service -Name 'MyCoolService' -Stop -Force</code>
    ///     <para>Stops the service, and its dependents, and remove it. 'Force' skips confirmation.</para>
    ///     <para></para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(
        VerbsCommon.Remove, "Service",
        SupportsShouldProcess = true,
        ConfirmImpact = ConfirmImpact.High,
        DefaultParameterSetName = "WithServiceName"
    )]
    public class RemoveServiceCommand : PSCmdlet
    {
        [Parameter(
            Mandatory = true
            , ValueFromPipeline = true
            , ParameterSetName = "WithServiceController"
        )]
        public ServiceController InputObject { get; set; }

        [Parameter(
            Position = 0
            , Mandatory = true
            , ValueFromPipeline = false
            , ParameterSetName = "WithServiceName"
        )]
        [ValidateNotNullOrEmpty()]
        public string Name { get; set; }

        [Parameter()]
        [ValidateNotNullOrEmpty()]
        public string ComputerName { get; set; }

        [Parameter()]
        public SwitchParameter Stop { get; set; }

        [Parameter()]
        public SwitchParameter Force { get; set; }

        protected override void ProcessRecord()
        {
            WrapperFunctions unwrapper = new();

            if (ParameterSetName == "WithServiceController")
            {
                WriteVerbose($"Using service controller for service {InputObject.ServiceName}. Unsafe handle: {InputObject.ServiceHandle.DangerousGetHandle()}");
                if (Force)
                    try
                    {
                        unwrapper.RemoveService(InputObject.ServiceHandle.DangerousGetHandle(), InputObject.MachineName, Stop);
                    }
                    catch (Core.NativeException ex)
                    {
                        throw (NativeException)ex;
                    }
                else
                {
                    if (InputObject.MachineName == ".")
                    {
                        if (ShouldProcess(
                           $"Removing service {InputObject.ServiceName} from the local computer.",
                           $"Are you sure you want to remove service {InputObject.ServiceName}?",
                           "Removing Service"))
                            try
                            {
                                unwrapper.RemoveService(InputObject.ServiceHandle.DangerousGetHandle(), InputObject.MachineName, Stop);
                            }
                            catch (Core.NativeException ex)
                            {
                                throw (NativeException)ex;
                            }
                    }
                    else
                    {
                        if (ShouldProcess(
                           $"Removing service {InputObject.ServiceName} on computer {InputObject.MachineName}",
                           $"Are you sure you want to remove service {InputObject.ServiceName} on {InputObject.MachineName}?",
                           "Removing Service"))
                            try
                            {
                                unwrapper.RemoveService(InputObject.ServiceHandle.DangerousGetHandle(), InputObject.MachineName, Stop);
                            }
                            catch (Core.NativeException ex)
                            {
                                throw (NativeException)ex;
                            }
                    }
                }

                InputObject.Dispose();
            }
            else
            {
                if (string.IsNullOrEmpty(ComputerName))
                {
                    if (Force)
                        unwrapper.RemoveService(Name, Stop);
                    else
                    {
                        if (ShouldProcess(
                           $"Removing service {Name} from the local computer.",
                           $"Are you sure you want to remove service {Name}?",
                           "Removing Service"))
                            try
                            {
                                unwrapper.RemoveService(Name, Stop);
                            }
                            catch (Core.NativeException ex)
                            {
                                throw (NativeException)ex;
                            }
                    }
                }
                else
                {
                    if (Force)
                        try
                        {
                            unwrapper.RemoveService(Name, ComputerName, Stop);
                        }
                        catch (Core.NativeException ex)
                        {
                            throw (NativeException)ex;
                        }
                    else
                    {
                        if (ShouldProcess(
                           $"Removing service {Name} on computer {ComputerName}",
                           $"Are you sure you want to remove service {Name} on {ComputerName}?",
                           "Removing Service"))
                            try
                            {
                                unwrapper.RemoveService(Name, ComputerName, Stop);
                            }
                            catch (Core.NativeException ex)
                            {
                                throw (NativeException)ex;
                            }
                    }
                }
            }
        }
    }
}