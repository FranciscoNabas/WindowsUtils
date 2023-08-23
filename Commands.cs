using System.ServiceProcess;
using System.Security.Principal;
using System.Management.Automation;
using System.Security.AccessControl;
using WindowsUtils.Core;
using WindowsUtils.Services;
using WindowsUtils.Attributes;
using WindowsUtils.AccessControl;
using WindowsUtils.TerminalServices;
using WindowsUtils.ArgumentCompletion;
using System.Security;

#pragma warning disable CS8618
namespace WindowsUtils.Commands
{
    /// <summary>
    /// <para type="synopsis">Sends a message to sessions on local or remote computers.</para>
    /// <para type="description">The Invoke-RemoteMessage cmdlet sends a MessageBox-Style message to sessions on local or remote computers.</para>
    ///</summary>
    [Cmdlet(
        VerbsCommunications.Send, "RemoteMessage",
        SupportsShouldProcess = true,
        ConfirmImpact = ConfirmImpact.High,
        DefaultParameterSetName = "NoWait"
    )]
    [OutputType(typeof(MessageResponse))]
    public class SendRemoteMessageCommand : Cmdlet
    {
        private int _timeout = 0;

        /// <summary>
        /// <para type="description">The message box title.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            HelpMessage = "The message box title."
        )]
        [ValidateNotNullOrEmpty]
        public string Title { get; set; }

        /// <summary>
        /// <para type="description">The message text.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 1,
            HelpMessage = "The message text."
        )]
        [ValidateNotNullOrEmpty]
        public string Message { get; set; }

        /// <summary>
        /// <para type="description">The computer name to send the message.</para>
        /// <para type="description">If not specified, sends the message to the local host.</para>
        /// </summary>
        [Parameter(HelpMessage = "The computer name to send the message.")]
        public string ComputerName { get; set; }

        /// <summary>
        /// <para type="description">The session(s) to receive the message.</para>
        /// <para type="description">If not specified, sends the message to all sessions.</para>
        /// </summary>
        [Parameter(HelpMessage = "The session(s) to receive the message.")]
        public int[] SessionId { get; set; }

        /// <summary>
        /// <para type="description">A list of MessageBox style enum objects.</para>
        /// <para type="description">To obtaining a list of available options, call 'Get-RemoteMessageOptions', or see definition for the MessageBox function.</para>
        /// </summary>
        [Parameter(HelpMessage = "A list of MessageBox styles.")]
        [ValidateNotNullOrEmpty]
        public string[] Style { get; set; } = { "MB_OK" };

        /// <summary>
        /// <para type="description">The timeout, in seconds, to wait for a response.</para>
        /// <para type="description">If the timeout expires, the cmdlet returns 'Timeout'. The message is not closed.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ParameterSetName = "WithWait",
            HelpMessage = "The timeout, in seconds, to wait for a response."
        )]
        [Parameter(
            ParameterSetName = "NoWait",
            HelpMessage = "The timeout, in seconds, to wait for a response."
        )]
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
        [Parameter(
            ParameterSetName = "WithWait",
            HelpMessage = "If called, waits for the user response in the specified timeout.")]
        public SwitchParameter Wait { get; set; } = false;

        protected override void ProcessRecord()
        {
            Wrapper unWrapper = new();
            uint nativeStyle = MessageBoxOption.MbOptionsResolver(Style);

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
                        MessageResponseBase[] result = unWrapper.SendRemoteMessage(IntPtr.Zero, null, Title, Message, nativeStyle, Timeout, Wait);
                        result.Where(q => q is not null).ToList().ForEach(x => WriteObject((MessageResponse)x));
                    }
                }
                else
                {
                    MessageResponseBase[] result = unWrapper.SendRemoteMessage(IntPtr.Zero, SessionId, Title, Message, nativeStyle, Timeout, Wait);
                    result.Where(q => q is not null).ToList().ForEach(x => WriteObject((MessageResponse)x));
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
                        MessageResponseBase[] result = unWrapper.SendRemoteMessage(session.SessionHandle.DangerousGetHandle(), null, Title, Message, nativeStyle, Timeout, Wait);
                        result.Where(q => q is not null).ToList().ForEach(x => WriteObject((MessageResponse)x));
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
                            MessageResponseBase[] result = unWrapper.SendRemoteMessage(session.SessionHandle.DangerousGetHandle(), null, Title, Message, nativeStyle, Timeout, Wait);
                            result.Where(q => q is not null).ToList().ForEach(x => WriteObject((MessageResponse)x));
                        }
                    }
                    else
                    {
                        MessageResponseBase[] result = unWrapper.SendRemoteMessage(session.SessionHandle.DangerousGetHandle(), SessionId, Title, Message, nativeStyle, Timeout, Wait);
                        result.Where(q => q is not null).ToList().ForEach(x => WriteObject((MessageResponse)x));
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
    [OutputType(typeof(MessageBoxOption[]))]
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
    [OutputType(typeof(ComputerSession))]
    public class GetComputerSessionCommand : Cmdlet
    {
        /// <summary>
        /// <para type="description">The computer name.</para>
        /// <para type="description">If no computer name is informed, it returns sessions on the current computer.</para>
        /// </summary>
        [Parameter(
            Position = 0,
            HelpMessage = "The computer name."
        )]
        [ValidateNotNullOrEmpty]
        public string ComputerName { get; set; }

        /// <summary>
        /// <para type="description">When called, this Cmdlet returns only active sessions.</para>
        /// </summary>
        [Parameter(HelpMessage = "Returns only active sessions.")]
        public SwitchParameter ActiveOnly { get; set; } = false;

        /// <summary>
        /// <para type="description">When called, this Cmdlet includes sessions without an user name assigned.</para>
        /// <para type="description">Sessions without user name are marked as 'System'.</para>
        /// </summary>
        [Parameter(HelpMessage = "Includes sessions without an user assigned.")]
        public SwitchParameter IncludeSystemSession { get; set; } = false;

        protected override void ProcessRecord()
        {
            Wrapper unWrapper = new();
            if (string.IsNullOrEmpty(ComputerName))
            {
                ComputerSessionBase[] result = unWrapper.GetComputerSession(ComputerName, IntPtr.Zero, ActiveOnly, IncludeSystemSession);
                result.ToList().ForEach(x => WriteObject((ComputerSession)x));
            }
            else
            {
                using WtsSession session = new(ComputerName);
                ComputerSessionBase[] result = unWrapper.GetComputerSession(ComputerName, session.SessionHandle.DangerousGetHandle(), ActiveOnly, IncludeSystemSession);
                result.ToList().ForEach(x => WriteObject((ComputerSession)x));
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
            Wrapper unWrapper = new();
            unWrapper.SendClick();
        }
    }

    /// <summary>
    /// <para type="synopsis">Returns the message table stored in a file.</para>
    /// <para type="description">This Cmdlet lists all messages, and their index from a resource message table.</para>
    /// <para type="description">These message tables are stored into files, like DLL or EXE.</para>
    /// <para type="description">I.E.: on kernel32.dll are stored the Win32 system error messages.</para>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "ResourceMessageTable")]
    [OutputType(typeof(ResourceMessageTable))]
    public class GetResourceMessageTableCommand : Cmdlet
    {
        /// <summary>
        /// <para type="description">The path to the file containing the message table.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ValueFromPipelineByPropertyName = true,
            HelpMessage = "The path to the file containing the messages.")]
        [ValidateNotNullOrEmpty]
        [ValidateFileExists]
        public string Path { get; set; }

        protected override void ProcessRecord()
        {
            Wrapper unWrapper = new();
            ResourceMessageTableCore[] result = unWrapper.GetResourceMessageTable(Path);
            result.ToList().ForEach(x => WriteObject((ResourceMessageTable)x));
        }
    }

    /// <summary>
    /// <para type="synopsis">Returns the error message for a system error code.</para>
    /// <para type="description">This Cmdlet uses FormatMessage to return the message text for a given 'Win32' system error.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Get-ErrorString -ErrorCode 5</code>
    ///     <para>Returning the message for a given error code.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>[System.Runtime.InteropServices.Marshal]::GetLastWin32Error() | Get-ErrorString</code>
    ///     <para>Calling GetLastWin32Error and providing it to the Cmdlet to get the message string.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "ErrorString")]
    [OutputType(typeof(string))]
    public class GetErrorStringCommand : Cmdlet
    {
        /// <summary>
        /// <para type="description">The error code.</para>
        /// <para type="description">This value will be passed to the 'FormatMessage' function, with the system message parameter..</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ValueFromPipeline = true,
            HelpMessage = "The Win32 error code."
        )]
        public int ErrorCode { get; set; }

        protected override void ProcessRecord()
        {
            Wrapper unWrapper = new();
            WriteObject(unWrapper.GetFormattedError(ErrorCode));
        }
    }

    /// <summary>
    /// <para type="synopsis">Returns the last 'Win32' system error message.</para>
    /// <para type="description">This Cmdlet gets the last error thrown by the system, and returns the message string.</para>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "LastWin32Error")]
    [OutputType(typeof(string))]
    public class GetLastWin32ErrorCommand : Cmdlet
    {
        protected override void ProcessRecord()
        {
            Wrapper unWrapper = new();
            WriteObject(unWrapper.GetLastWin32Error());
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
        private List<string> validPaths = new();
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
            ParameterSetName = "byLiteral",
            HelpMessage = "The object(s) literal path."
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

            Wrapper unWrapper = new();
            if (CloseHandle)
            {
                if (Force || ShouldProcess(
                    "",
                    "Are you sure you want to close all handles for the input object(s)?",
                    "ATTENTION! Closing handles can lead to system malfunction.\n"
                    ))
                {
                    unWrapper.GetProcessObjectHandle(validPaths.ToArray(), true);
                    ObjectHandleBase[] result = unWrapper.GetProcessObjectHandle(validPaths.ToArray(), false);
                    if (result is not null)
                    {
                        WriteWarning("Failed to remove handles for the processes below.");
                        result.OrderBy(p => p.Name).ToList().ForEach(x => WriteObject((ObjectHandle)x));
                    }
                }
            }
            else
            {
                ObjectHandleBase[] result = unWrapper.GetProcessObjectHandle(validPaths.ToArray(), false);
                result?.OrderBy(p => p.Name).ToList().ForEach(x => WriteObject((ObjectHandle)x));
            }
        }
    }

    /// <summary>
    /// <para type="synopsis">Returns the installer properties from a MSI file.</para>
    /// <para type="description">This Cmdlet returns the installer properties from the installer database.</para>
    /// <para type="description">Besides standard information, like Product Code, it brings vendor-specific information.</para>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "MsiProperties")]
    [OutputType(typeof(PSObject))]
    public class GetMsiPropertiesCommand : Cmdlet
    {
        /// <summary>
        /// <para type="description">The MSI file path.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ValueFromPipelineByPropertyName = true,
            HelpMessage = "The MSI file path."
        )]
        [ValidateNotNullOrEmpty]
        [ValidateFileExists]
        public string Path { get; set; }

        protected override void ProcessRecord()
        {
            Wrapper unWrapper = new();
            PSObject outObject = new();
            foreach (KeyValuePair<string, string> item in unWrapper.GetMsiProperties(Path).OrderBy(x => x.Key))
                outObject.Members.Add(new PSNoteProperty(item.Key, item.Value));

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
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-ComputerSession -ComputerName 'MYAWESOMEPC.contoso.com' | Where-Object { $PSItem.UserName -eq 'CONTOSO\user.name' } | Disconnect-Session -ComputerName 'MYAWESOMEPC.contoso.com' -SessionId $PSItem.SessionId </code>
    ///     <para>Gets the session from a specific user on a remote computer, and loggs it off.</para>
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
            ValueFromPipelineByPropertyName = true,
            HelpMessage = "The computer name to disconnect the session."
        )]
        [ValidateNotNullOrEmpty]
        public string ComputerName { get; set; }

        /// <summary>
        /// <para type="description">The session id to disconnect.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ParameterSetName = "WithComputerName",
            ValueFromPipelineByPropertyName = true,
            HelpMessage = "The session ID to disconnect."
        )]
        [Parameter(
            Mandatory = false,
            ParameterSetName = "NoComputerName",
            ValueFromPipelineByPropertyName = true,
            HelpMessage = "The session ID to disconnect."
        )]
        public uint SessionId { get; set; }

        /// <summary>
        /// <para type="description">If called, the Cmdlet waits the logoff process to finish before returning.</para>
        /// </summary>
        [Parameter(
            ParameterSetName = "WithComputerName",
            HelpMessage = "Waits for the logon process to finish."
        )]
        [Parameter(
            ParameterSetName = "NoComputerName",
            HelpMessage = "Waits for the logon process to finish."
        )]
        public SwitchParameter Wait { get; set; }

        protected override void ProcessRecord()
        {
            Wrapper unWrapper = new();

            if (string.IsNullOrEmpty(ComputerName))
            {
                if (SessionId == 0)
                {
                    if (ShouldProcess(
                    "Disconnecting current session from this computer.",
                    "Are you sure you want to disconnect the current session on this computer?",
                    "Disconnect session"))
                        unWrapper.DisconnectSession(IntPtr.Zero, SessionId, Wait);
                }
                else
                {
                    if (ShouldProcess(
                   "Disconnecting session ID " + SessionId + " from current computer.",
                   "Are you sure you want to disconnect sesion ID " + SessionId + " on the current computer?",
                   "Disconnect session"))
                        unWrapper.DisconnectSession(IntPtr.Zero, SessionId, Wait);
                }
            }
            else
            {
                using WtsSession session = new(ComputerName);
                if (ShouldProcess(
                   "Disconnecting session ID " + SessionId + " from computer " + ComputerName + ".",
                   "Are you sure you want to disconnect session ID " + SessionId + " on computer " + ComputerName + "?",
                   "Disconnect session"))
                    unWrapper.DisconnectSession(session.SessionHandle.DangerousGetHandle(), SessionId, Wait);
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
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Remove-Service -Name 'MyCoolService' -Stop -Force</code>
    ///     <para>Stops the service, and its dependents, and remove it. 'Force' skips confirmation.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(
        VerbsCommon.Remove, "Service",
        SupportsShouldProcess = true,
        ConfirmImpact = ConfirmImpact.Medium,
        DefaultParameterSetName = "WithServiceName"
    )]
    public class RemoveServiceCommand : CoreCommandBase
    {
        /// <summary>
        /// <para type="description">The service controller input object.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ValueFromPipeline = true,
            ParameterSetName = "WithServiceController",
            HelpMessage = "The service controller input object."
        )]
        public ServiceController InputObject { get; set; }

        /// <summary>
        /// <para type="description">The service name.</para>
        /// </summary>
        [Parameter(
            Position = 0,
            Mandatory = true,
            ValueFromPipeline = false,
            ParameterSetName = "WithServiceName",
            HelpMessage = "The service name."
        )]
        [ValidateNotNullOrEmpty]
        [ArgumentCompleter(typeof(ServiceNameCompleter))]
        public string Name { get; set; }

        /// <summary>
        /// <para type="description">The computer name to remove the service from.</para>
        /// </summary>
        [Parameter(HelpMessage = "The computer name to remove the service from.")]
        [ValidateNotNullOrEmpty]
        public string ComputerName { get; set; }

        /// <summary>
        /// <para type="description">Stops the service and any dependents.</para>
        /// </summary>
        [Parameter(HelpMessage = "Stops the service and any dependents.")]
        public SwitchParameter Stop { get; set; }

        /// <summary>
        /// <para type="description">Suppresses confirmation.</para>
        /// </summary>
        [Parameter(HelpMessage = "Suppresses confirmation.")]
        new public SwitchParameter Force { get; set; }

        protected override void ProcessRecord()
        {
            Wrapper unWrapper = new();

            if (ParameterSetName == "WithServiceController")
            {
                WriteVerbose($"Using service controller for service {InputObject.ServiceName}. Unsafe handle: {InputObject.ServiceHandle.DangerousGetHandle()}");
                if (Force)
                    unWrapper.RemoveService(InputObject.ServiceHandle.DangerousGetHandle(), InputObject.ServiceName, InputObject.MachineName, Stop, (CmdletContextBase)CmdletContext);
                else
                {
                    if (InputObject.MachineName == ".")
                    {
                        if (ShouldProcess(
                           $"Removing service {InputObject.ServiceName} from the local computer.",
                           $"Are you sure you want to remove service {InputObject.ServiceName}?",
                           "Removing Service"))
                            unWrapper.RemoveService(InputObject.ServiceHandle.DangerousGetHandle(), InputObject.ServiceName, InputObject.MachineName, Stop, (CmdletContextBase)CmdletContext);
                    }
                    else
                    {
                        if (ShouldProcess(
                           $"Removing service {InputObject.ServiceName} on computer {InputObject.MachineName}",
                           $"Are you sure you want to remove service {InputObject.ServiceName} on {InputObject.MachineName}?",
                           "Removing Service"))
                            unWrapper.RemoveService(InputObject.ServiceHandle.DangerousGetHandle(), InputObject.ServiceName, InputObject.MachineName, Stop, (CmdletContextBase)CmdletContext);
                    }
                }

                InputObject.Dispose();
            }
            else
            {
                if (string.IsNullOrEmpty(ComputerName))
                {
                    if (Force)
                        unWrapper.RemoveService(Name, Stop, (CmdletContextBase)CmdletContext);
                    else
                    {
                        if (ShouldProcess(
                           $"Removing service {Name} from the local computer.",
                           $"Are you sure you want to remove service {Name}?",
                           "Removing Service"))
                            unWrapper.RemoveService(Name, Stop, (CmdletContextBase)CmdletContext);
                    }
                }
                else
                {
                    if (Force)
                        unWrapper.RemoveService(Name, ComputerName, Stop, (CmdletContextBase)CmdletContext);
                    else
                    {
                        if (ShouldProcess(
                           $"Removing service {Name} on computer {ComputerName}",
                           $"Are you sure you want to remove service {Name} on {ComputerName}?",
                           "Removing Service"))
                            unWrapper.RemoveService(Name, ComputerName, Stop, (CmdletContextBase)CmdletContext);
                    }
                }
            }
        }
    }

    /// <summary>
    /// <para type="synopsis">Gets the service security attributes.</para>
    /// <para type="description">This cmdlet retrieves the service security attributes.</para>
    /// <para type="description">The attributes retrieved are Owner, Group, DACL, and SACL, if used with the 'Audit' parameter.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Get-ServiceSecurity -Name 'MyCoolService'</code>
    ///     <para>Gets the security attributes from the service 'MyCoolService'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-ServiceSecurity -Name 'MyCoolService' -Audit</code>
    ///     <para>Gets the security attributes from the service 'MyCoolService', including System Access Control List (SACL).</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-Service -Name 'MyCoolService' | Get-ServiceSecurity</code>
    ///     <code>$service = Get-Service -Name 'MyCoolService'; Get-ServiceSecurity -InputObject $service</code>
    ///     <para>Gets the security attributes from the service 'MyCoolService', from 'Get-Service'.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(
        VerbsCommon.Get, "ServiceSecurity",
        DefaultParameterSetName = "WithServiceName"
    )]
    [OutputType(typeof(ServiceSecurity))]
    public class GetServiceSecurityCommand : ServiceCommandBase
    {
        /// <summary>
        /// <para type="description">The service(s) name.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ParameterSetName = "WithServiceName",
            HelpMessage = "The service(s) name."
        )]
        [ValidateNotNullOrEmpty]
        [ArgumentCompleter(typeof(ServiceNameCompleter))]
        public string[] Name { get; set; }

        /// <summary>
        /// <para type="description">The service(s) display name.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ParameterSetName = "WithDisplayName",
            HelpMessage = "The service(s) display name."
        )]
        [ValidateNotNullOrEmpty]
        [ArgumentCompleter(typeof(ServiceDisplayNameCompleter))]
        public string[] DisplayName { get; set; }

        /// <summary>
        /// <para type="description">The service controller input object.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ParameterSetName = "WithServiceController",
            ValueFromPipeline = true,
            HelpMessage = "The service controller input object."
        )]
        public ServiceController InputObject { get; set; }

        /// <summary>
        /// <para type="description">Includes SACL to the result.</para>
        /// </summary>
        [Parameter(HelpMessage = "Includes SACL to the result.")]
        public SwitchParameter Audit { get; set; }

        protected override void ProcessRecord()
        {
            List<string> serviceNameList;
            switch (ParameterSetName)
            {
                case "WithServiceName":
                    serviceNameList = GetMatchingServicesNameByName(Name);
                    foreach (string serviceName in serviceNameList)
                        WriteObject(GetServiceObjectSecurity(serviceName, Audit));
                    break;

                case "WithServiceController":
                    // We can't use given handle because we need to use extra privileges to access SACL.
                    if (Audit)
                        WriteObject(GetServiceObjectSecurity(InputObject.ServiceName, Audit));
                    else
                        WriteObject(GetServiceObjectSecurity(InputObject, Audit));
                    break;

                case "WithDisplayName":
                    serviceNameList = GetMatchingServicesNameByDisplayName(DisplayName);
                    foreach (string serviceName in serviceNameList)
                        WriteObject(GetServiceObjectSecurity(serviceName, Audit));
                    break;
            }
        }
    }

    /// <summary>
    /// <para type="synopsis">Creates a new service access rule object.</para>
    /// <para type="description">This cmdlet creates a new service access rule object to modify service security attributes.</para>
    /// <example>
    ///     <para></para>
    ///     <code>New-ServiceAccessRule -Identity 'DOMAIN\YourUser' -Rights 'AllAccess' -Type 'Allow'</code>
    ///     <para>Creates a new service access rule with all access allowed for user 'DOMAIN\YourUser'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>              $serviceAccessRuleSplat = @{
    ///    Identity = [System.Security.Principal.NTAccount]'DOMAIN\YourUser
    ///    Rights = [WindowsUtils.Engine.ServiceRights]::GenericRead -bor [WindowsUtils.Engine.ServiceRights]::EnumerateDependents
    ///    Type = [System.Security.AccessControl.AccessControlType]::Deny
    ///    InheritanceFlags = [System.Security.AccessControl.InheritanceFlags]::ContainerInherit
    ///    PropagationFlags = [System.Security.AccessControl.PropagationFlags]::InheritOnly
    ///    Inherited = $true
    ///}
    ///New-ServiceAccessRule @serviceAccessRuleSplat</code>
    ///     <para>Creates a new service access rule with the defined parameters.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsCommon.New, "ServiceAccessRule")]
    [OutputType(typeof(ServiceAccessRule))]
    public class NewServiceAccessRuleCommand : PSCmdlet
    {
        private string _identityAsString;
        private IdentityReference _identityAsIdReference;
        private bool _isString;
        private ServiceRights _rights;
        private InheritanceFlags _inheritance;
        private PropagationFlags _propagation;

        /// <summary>
        /// <para type="description">The identity to be associated to the new access rule.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            HelpMessage = "The identity to be associated to the new access rule."
        )]
        public object Identity
        {
            get
            {
                if (_isString)
                    return _identityAsString;

                return _identityAsIdReference;
            }
            set
            {
                if (value is string _valueStr)
                {
                    _identityAsString = _valueStr;
                    _isString = true;
                    _identityAsIdReference = new NTAccount(_valueStr);
                }
                else if (value is IdentityReference _valueIdRf)
                    _identityAsIdReference = _valueIdRf;
                else
                    throw new ArgumentException("Wrong identity type. Accepted types are 'System.String' and 'System.Security.Principal.IdentityReference'.");
            }
        }

        /// <summary>
        /// <para type="description">A bitwise OR flag of one or more service rights.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            HelpMessage = "A bitwise OR flag of one or more service rights."
        )]
        public ServiceRights Rights
        {
            get { return _rights; }
            set { _rights = value; }
        }

        /// <summary>
        /// <para type="description">The type of the new access rule.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            HelpMessage = "The type of the new access rule."
        )]
        public AccessControlType Type { get; set; }

        /// <summary>
        /// <para type="description">The inheritance flags to be associated with the new access rule.</para>
        /// </summary>
        [Parameter(HelpMessage = "The inheritance flags to be associated with the new access rule.")]
        public InheritanceFlags InheritanceFlags
        {
            get { return _inheritance; }
            set { _inheritance = value; }
        }

        /// <summary>
        /// <para type="description">The propagation flags to be associated with the new access rule.</para>
        /// </summary>
        [Parameter(HelpMessage = "The propagation flags to be associated with the new access rule.")]
        public PropagationFlags PropagationFlags
        {
            get { return _propagation; }
            set { _propagation = value; }
        }

        /// <summary>
        /// <para type="description">Called if the new access rule is inherited.</para>
        /// </summary>
        [Parameter(HelpMessage = "Called if the new access rule is inherited.")]
        public SwitchParameter Inherited { get; set; }

        protected override void ProcessRecord() => WriteObject(new ServiceAccessRule(_identityAsIdReference, _rights, Inherited, _inheritance, _propagation, Type));
    }

    /// <summary>
    /// <para type="synopsis">Creates a new service audit rule object.</para>
    /// <para type="description">This cmdlet creates a new service audit rule object to modify service security attributes.</para>
    /// <example>
    ///     <para></para>
    ///     <code>New-ServiceAuditRule -Identity 'DOMAIN\YourUser' -Rights 'AllAccess' -Flags 'Failure'</code>
    ///     <para>Creates a new service audit rule for all access and failure for 'DOMAIN\YourUser'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>              $serviceAuditRuleSplat = @{
    ///    Identity = [System.Security.Principal.NTAccount]'DOMAIN\YourUser
    ///    Rights = [WindowsUtils.Engine.ServiceRights]::GenericRead -bor [WindowsUtils.Engine.ServiceRights]::EnumerateDependents
    ///    Flags = [System.Security.AccessControl.AuditFlags]::Success
    ///    InheritanceFlags = [System.Security.AccessControl.InheritanceFlags]::ContainerInherit
    ///    PropagationFlags = [System.Security.AccessControl.PropagationFlags]::InheritOnly
    ///    Inherited = $true
    ///}
    ///New-ServiceAuditRule @serviceAuditRuleSplat</code>
    ///     <para>Creates a new service audit rule with the defined parameters.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsCommon.New, "ServiceAuditRule")]
    [OutputType(typeof(ServiceAuditRule))]
    public class NewServiceSecurityCommand : PSCmdlet
    {
        private string _identityAsString;
        private IdentityReference _identityAsIdReference;
        private bool _isString;
        private ServiceRights _rights;
        private InheritanceFlags _inheritance;
        private PropagationFlags _propagation;

        /// <summary>
        /// <para type="description">The identity to be associated to the new audit rule.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            HelpMessage = "The identity to be associated to the new audit rule."
        )]
        public object Identity
        {
            get
            {
                if (_isString)
                    return _identityAsString;

                return _identityAsIdReference;
            }
            set
            {
                if (value is string _valueStr)
                {
                    _identityAsString = _valueStr;
                    _isString = true;
                    _identityAsIdReference = new NTAccount(_valueStr);
                }
                else if (value is IdentityReference _valueIdRf)
                    _identityAsIdReference = _valueIdRf;
                else
                    throw new ArgumentException("Wrong identity type. Accepted types are 'System.String' and 'System.Security.Principal.IdentityReference'.");
            }
        }

        /// <summary>
        /// <para type="description">A bitwise OR flag of one or more service rights.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            HelpMessage = "A bitwise OR flag of one or more service rights."
        )]
        public ServiceRights Rights
        {
            get { return _rights; }
            set { _rights = value; }
        }

        /// <summary>
        /// <para type="description">The type of the new audit rule.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            HelpMessage = "The type of the new audit rule."
        )]
        public AuditFlags Flags { get; set; }

        /// <summary>
        /// <para type="description">The inheritance flags to be associated with the new audit rule.</para>
        /// </summary>
        [Parameter(HelpMessage = "The inheritance flags to be associated with the new audit rule.")]
        public InheritanceFlags InheritanceFlags
        {
            get { return _inheritance; }
            set { _inheritance = value; }
        }

        /// <summary>
        /// <para type="description">The propagation flags to be associated with the new audit rule.</para>
        /// </summary>
        [Parameter(HelpMessage = "The propagation flags to be associated with the new audit rule.")]
        public PropagationFlags PropagationFlags
        {
            get { return _propagation; }
            set { _propagation = value; }
        }

        /// <summary>
        /// <para type="description">Called if the new audit rule is inherited.</para>
        /// </summary>
        [Parameter(HelpMessage = "Called if the new audit rule is inherited.")]
        public SwitchParameter Inherited { get; set; }

        protected override void ProcessRecord() => WriteObject(new ServiceAuditRule(_identityAsIdReference, _rights, Inherited, _inheritance, _propagation, Flags));
    }

    /// <summary>
    /// <para type="synopsis">Set the service object security.</para>
    /// <para type="description">This cmdlet sets the service object security. You first retrieve the service security calling 'Get-ServiceSecurity', modifies the ACL, and pass it as input object to this cmdlet.</para>
    /// <example>
    ///     <para></para>
    ///     <code>              $serviceSecurity = Get-ServiceSecurity -Name 'MyCoolService'
    ///$serviceSecurity.SetOwner([System.Security.Principal.NTAccount]'User')
    ///$serviceSecurity.AddAccessRule((New-ServiceAccessRule -Identity 'User' -Rights 'AllAccess' -Type 'Allow'))
    ///
    ///Set-ServiceSecurity -Name 'MyCoolService' -SecurityObject $serviceSecurity</code>
    ///     <para>Changes the owner, adds an access rule, and sets the service security attributes.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    /// <para></para>
    /// <code>              $serviceSecurity = Get-ServiceSecurity -Name 'MyCoolService' -Audit
    ///$serviceSecurity.AddAuditRule((New-ServiceAuditRule -Identity 'User' -Rights 'AllAccess' -Flags 'Success'))
    ///
    ///Set-ServiceSecurity -Name 'MyCoolService' -AclObject $serviceSecurity</code>
    ///     <para>Changes the service audit rules.</para>
    /// <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(
        VerbsCommon.Set, "ServiceSecurity",
        SupportsShouldProcess = true,
        ConfirmImpact = ConfirmImpact.Medium,
        DefaultParameterSetName = "ByNameAndSecurityObject"
    )]
    public class SetServiceSecurityCommand : PSCmdlet
    {
        private string _serviceFinalName;
        private ServiceSecurity _finalSecurityObject;
        private Wrapper _unWrapper = new();

        /// <summary>
        /// <para type="description">The service name.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ParameterSetName = "ByNameAndSecurityObject",
            ValueFromPipelineByPropertyName = true,
            HelpMessage = "The service name."
        )]
        [Parameter(
            Mandatory = true,
            Position = 0,
            ParameterSetName = "ByNameAndSddl",
            ValueFromPipelineByPropertyName = true,
            HelpMessage = "The service name."
        )]
        [ValidateNotNullOrEmpty]
        [ArgumentCompleter(typeof(ServiceNameCompleter))]
        public string Name { get; set; }

        /// <summary>
        /// <para type="description">The service display name.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            ParameterSetName = "ByDisplayNameAndSecurityObject",
            HelpMessage = "The service display name."
        )]
        [Parameter(
            Mandatory = true,
            ParameterSetName = "ByDisplayNameAndSddl",
            HelpMessage = "The service display name."
        )]
        [ValidateNotNullOrEmpty]
        [ArgumentCompleter(typeof(ServiceDisplayNameCompleter))]
        public string DisplayName { get; set; }

        /// <summary>
        /// <para type="description">The service controller object.</para>
        /// </summary>
        [Parameter(
            Mandatory = true,
            Position = 0,
            ParameterSetName = "ByInputObjectAndSecurityObject",
            ValueFromPipeline = true,
            HelpMessage = "The service controller object."
        )]
        [Parameter(
            Mandatory = true,
            Position = 0,
            ParameterSetName = "ByInputObjectAndSddl",
            ValueFromPipeline = true,
            HelpMessage = "The service controller object."
        )]
        public ServiceController InputObject { get; set; }

        /// <summary>
        /// <para type="description">The computer name. If empty, looks for the service in the current machine.</para>
        /// </summary>
        [Parameter(HelpMessage = "The computer name. If empty, looks for the service in the current machine.")]
        public string ComputerName { get; set; }

        /// <summary>
        /// <para type="description">The service security object to set.</para>
        /// </summary>
        [Parameter(Mandatory = true, ParameterSetName = "ByNameAndSecurityObject", HelpMessage = "The service security object to set.")]
        [Parameter(Mandatory = true, ParameterSetName = "ByDisplayNameAndSecurityObject", HelpMessage = "The service security object to set.")]
        [Parameter(Mandatory = true, ParameterSetName = "ByInputObjectAndSecurityObject", HelpMessage = "The service security object to set.")]
        [Alias("AclObject")]
        public ServiceSecurity SecurityObject { get; set; }

        /// <summary>
        /// <para type="description">The service security object to set.</para>
        /// </summary>
        [Parameter(Mandatory = true, ParameterSetName = "ByNameAndSddl", HelpMessage = "The SDDL string representing the security object.")]
        [Parameter(Mandatory = true, ParameterSetName = "ByDisplayNameAndSddl", HelpMessage = "The SDDL string representing the security object.")]
        [Parameter(Mandatory = true, ParameterSetName = "ByInputObjectAndSddl", HelpMessage = "The SDDL string representing the security object.")]
        public string Sddl { get; set; }

        /// <summary>
        /// <para type="description">True to modify audit rules (SACL).</para>
        /// <para type="description">Attention! If this parameter is called, and the security object,</para>
        /// <para type="description">or SDDL audit rules are empty it will erase the service's audit rules.</para>
        /// </summary>
        [Parameter(HelpMessage = @"
            True to modify audit rules (SACL).
            Attention! If this parameter is called, and the security object,
            or SDDL audit rules are empty it will erase the service's audit rules."
        )]
        public SwitchParameter SetSacl { get; set; }

        protected override void BeginProcessing()
        {
            if (ParameterSetName == "ByNameAndSddl" || ParameterSetName == "ByInputObjectAndSddl" || ParameterSetName == "ByDisplayNameAndSddl")
            {
                try
                {
                    new RawSecurityDescriptor(Sddl);
                }
                catch (Exception ex)
                {
                    throw new ArgumentException(ex.InnerException.Message);
                }
            }
        }

        protected override void ProcessRecord()
        {
            switch (ParameterSetName)
            {
                case "ByNameAndSecurityObject":
                    _serviceFinalName = Name;
                    _finalSecurityObject = SecurityObject;
                    break;

                case "ByDisplayNameAndSecurityObject":
                    string? serviceName = ServiceController.GetServices().Where(s => s.DisplayName == DisplayName).FirstOrDefault().ServiceName
                        ?? throw new ItemNotFoundException($"No service with display name '{DisplayName}' found.");

                    _serviceFinalName = serviceName;
                    _finalSecurityObject = SecurityObject;
                    break;

                case "ByInputObjectAndSecurityObject":
                    if (InputObject.ServiceHandle is null)
                        throw new ArgumentException("Invalid input object.");

                    if (InputObject.ServiceHandle.IsClosed || InputObject.ServiceHandle.IsInvalid)
                        throw new ArgumentException("Invalid input object service handle.");

                    _serviceFinalName = InputObject.ServiceName;
                    _finalSecurityObject = SecurityObject;
                    break;

                case "ByNameAndSddl":
                    _finalSecurityObject = new(_unWrapper.GetServiceSecurityDescriptorString(Name, SetSacl), Name);
                    _finalSecurityObject.SetSecurityDescriptorSddlForm(Sddl);
                    _serviceFinalName = Name;
                    break;

                case "ByDisplayNameAndSddl":
                    serviceName = ServiceController.GetServices().Where(s => s.DisplayName == DisplayName).FirstOrDefault().ServiceName
                        ?? throw new ItemNotFoundException($"No service with display name '{DisplayName}' found.");

                    _serviceFinalName = serviceName;
                    _finalSecurityObject = new(_unWrapper.GetServiceSecurityDescriptorString(Name, SetSacl), Name);
                    _finalSecurityObject.SetSecurityDescriptorSddlForm(Sddl);
                    break;

                case "ByInputObjectAndSddl":
                    if (InputObject.ServiceHandle is null)
                        throw new ArgumentException("Invalid input object.");

                    if (InputObject.ServiceHandle.IsClosed || InputObject.ServiceHandle.IsInvalid)
                        throw new ArgumentException("Invalid input object service handle.");

                    _serviceFinalName = InputObject.ServiceName;
                    _finalSecurityObject = new(_unWrapper.GetServiceSecurityDescriptorString(Name, SetSacl), Name);
                    _finalSecurityObject.SetSecurityDescriptorSddlForm(Sddl);
                    break;
            }

            if (string.IsNullOrEmpty(ComputerName))
            {
                if (ShouldProcess(
                    $"Setting security on service {Name} on the local computer.",
                    $"Are you sure you want to set security for service {Name} on the local computer?",
                    "Setting Service Security"
                ))
                    _unWrapper.SetServiceSecurity(_serviceFinalName, _finalSecurityObject.Sddl, SetSacl, _finalSecurityObject.OwnerModified);
            }
            else
            {
                if (ShouldProcess(
                    $"Setting security on service {Name} on {ComputerName}.",
                    $"Are you sure you want to set security for service {Name} on {ComputerName}?",
                    "Setting Service Security"
                ))
                    _unWrapper.SetServiceSecurity(_serviceFinalName, ComputerName, _finalSecurityObject.Sddl, SetSacl, _finalSecurityObject.OwnerModified);
            }
        }
    }

    /// <summary>
    /// <para type="synopsis">Extracts files from a cabinet file.</para>
    /// <para type="description">This Cmdlet extracts files from one or multiple cabinet files.</para>
    /// <para type="description">It also manages automatically files that spans through multiple cabinet files.</para>
    /// <para type="description">The Cmdlet creates the same folder structure from within the cabinet, relatively to the destination.</para>
    /// <para type="description">If a file with the same name already exists it's ovewritten by default.</para>
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
        private readonly Wrapper _unwrapper = new();
        private readonly List<string> _validPaths = new();
        private readonly List<string> _processedCabinetPaths = new();
        
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
                WuManagedCabinet cabinet = new(path, (CmdletContextBase)CmdletContext);
                if (!_processedCabinetPaths.Contains(cabinet.BundleCabinetPaths[0]))
                {
                    _processedCabinetPaths.AddRange(cabinet.BundleCabinetPaths);
                    _unwrapper.ExpandArchiveFile(cabinet, Destination, ArchiveFileType.Cabinet);
                }
            }
        }
    }

    [Cmdlet(VerbsCommunications.Send, "Tcping")]
    public class SendTcpingCommand : CoreCommandBase
    {
        private readonly Wrapper _unwrapper = new();

        [Parameter(Mandatory = true)]
        public string Destination { get; set; }

        [Parameter(Mandatory = true)]
        public uint Port { get; set; }

        protected override void ProcessRecord()
        {
            _unwrapper.StartTcpPing(Destination, Port, (CmdletContextBase)CmdletContext);
        }
    }
}