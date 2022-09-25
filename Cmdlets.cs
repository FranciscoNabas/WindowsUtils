using System.Management.Automation;
using UtilitiesLibrary.Abstraction;
using UtilitiesLibrary;
using Wrapper;

namespace WindowsUtils
{
    [Cmdlet(VerbsLifecycle.Invoke, "RemoteMessage")]
    public class InvokeRemoteMessageCmdlet : Cmdlet
    {
        [Parameter(Mandatory = true, Position = 0)]
        [ValidateNotNullOrEmpty]
        public string Title { get; set; }

        [Parameter(Mandatory = true, Position = 1)]
        [ValidateNotNullOrEmpty]
        public string Message { get; set; }

        [Parameter()]
        public string ComputerName { get; set; }

        [Parameter()]
        public int[] SessionId { get; set; }

        [Parameter()]
        [ValidateNotNullOrEmpty]
        public string[] Style { get; set; } = { "MB_OK" };

        [Parameter()]
        public int Timeout { get; set; } = 0;

        [Parameter()]
        public bool Wait { get; set; } = false;

        [Parameter()]
        public SwitchParameter Force { get; set; } = false;

        protected override void ProcessRecord()
        {
            if(SessionId == null)
            {
                if (Force)
                    Utilities.InvokeRemoteMessage(ComputerName, Title, Message, Style, Timeout, Wait, false);
                else
                    Utilities.InvokeRemoteMessage(ComputerName, Title, Message, Style, Timeout, Wait);
            }
            else
                Utilities.InvokeRemoteMessage(ComputerName, SessionId, Title, Message, Style, Timeout, Wait);
        }
    }
    
    [Cmdlet(VerbsCommon.Get, "RemoteMessageOptions")]
    public class GetRemoteMessageOptionsCmdlet : Cmdlet
    {
        protected override void ProcessRecord()
        {
            WriteObject(MessageBoxOption.GetAvailableOptions(), true);
        }
    }
    
    [Cmdlet(VerbsCommon.Get, "ComputerSession")]
    public class GetComputerSessionCmdlet : Cmdlet
    {
        [Parameter(Position = 0)]
        [ValidateNotNullOrEmpty]
        public string ComputerName { get; set; }

        [Parameter()]
        public SwitchParameter ActiveOnly { get; set; } = false;

        [Parameter()]
        public SwitchParameter IncludeSystemSession { get; set; } = false;

        protected override void ProcessRecord()
        {

            if (string.IsNullOrEmpty(ComputerName))
                WriteObject(Utilities.GetComputerSession(ActiveOnly, IncludeSystemSession), true);
            else
                WriteObject(Utilities.GetComputerSession(ComputerName, ActiveOnly, IncludeSystemSession), true);
        }
    }
    
    [Cmdlet(VerbsCommunications.Send, "Click")]
    public class SendClickCmdlet : Cmdlet
    {
        protected override void ProcessRecord()
        {
            Utilities.SendClick();
        }
    }

    [Cmdlet(VerbsCommon.Get, "ResourceMessageTable")]
    public class GetResourceMessageTableCmdlet : Cmdlet
    {
        [Parameter(Mandatory = true, Position = 0, ValueFromPipeline = true)]
        [ValidateNotNullOrEmpty]
        public string Path { get; set; }

        protected override void ProcessRecord()
        {
            Managed unWrapper = new Managed();
            WriteObject(unWrapper.GetResourceMessageTable(Path));
        }
    }

    [Cmdlet(VerbsCommon.Get, "FormattedError")]
    public class GetFormattedErrorCmdlet : Cmdlet
    {
        [Parameter(Mandatory = true, Position = 0, ValueFromPipeline = true)]
        public int ErrorCode { get; set; }

        protected override void ProcessRecord()
        {
            Managed unWrapper = new Managed();
            WriteObject(unWrapper.GetFormatedError(ErrorCode));
        }
    }

    [Cmdlet(VerbsCommon.Get, "LastWin32Error")]
    public class GetLastWin32ErrorCmdlet : Cmdlet
    {
        protected override void ProcessRecord()
        {
            Managed unWrapper = new Managed();
            WriteObject(unWrapper.GetFormatedWin32Error());
        }
    }

    [Cmdlet(VerbsCommon.Get, "LastWinSockError")]
    public class GetLastWinSockErrorCmdlet : Cmdlet
    {
        protected override void ProcessRecord()
        {
            Managed unWrapper = new Managed();
            WriteObject(unWrapper.GetFormatedWSError());
        }
    }

    [Cmdlet(VerbsCommon.Get, "FileHandle")]
    public class GetFileHandleCmdlet : Cmdlet
    {
        [Parameter(Mandatory = true, Position = 0, ValueFromPipeline = true)]
        [ValidateNotNullOrEmpty]
        public string Path { get; set; }

        protected override void ProcessRecord()
        {
            Managed unWrapper = new Managed();
            WriteObject(unWrapper.GetProcessFileHandle(Path));
        }
    }

    [Cmdlet(VerbsCommon.Get, "MsiProperties")]
    public class GetMsiPropertiesCmdlet : Cmdlet
    {
        [Parameter(Mandatory = true, Position = 0, ValueFromPipeline = true)]
        [ValidateNotNullOrEmpty]
        public string Path { get; set; }

        protected override void ProcessRecord()
        {
            Managed unWrapper = new Managed();
            WriteObject(unWrapper.GetMsiProperties(Path));
        }
    }

    [Cmdlet(VerbsCommunications.Disconnect, "Session", SupportsShouldProcess = true, ConfirmImpact = ConfirmImpact.High, DefaultParameterSetName = "NoComputerName")]
    [Alias("disconnect")]
    public class DisconnectCmdlet : Cmdlet
    {
        [Parameter(Mandatory = true, ParameterSetName = "WithComputerName")]
        [ValidateNotNullOrEmpty()]
        public string? ComputerName { get; set; }

        [Parameter(Mandatory = true, ParameterSetName = "WithComputerName")]
        [ValidateNotNullOrEmpty()]
        public int? SessionId { get; set; }

        [Parameter(ParameterSetName = "WithComputerName")]
        [Parameter(ParameterSetName = "NoComputerName")]
        [ValidateNotNullOrEmpty()]
        public SwitchParameter Wait { get; set; }

        protected override void ProcessRecord()
        {
            if(string.IsNullOrEmpty(ComputerName))
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