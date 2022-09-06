using System.Management.Automation;
using Wrapper;

namespace WindowsUtils.PowershellCoreModule
{
    [Cmdlet(VerbsLifecycle.Invoke, "RemoteMessage")]
    public class InvokeRemoteMessageCmdlet : PSCmdlet
    {
        [Parameter(Mandatory = true, Position = 0)]
        [ValidateNotNullOrEmpty]
        public string Title { get; set; }

        [Parameter(Mandatory = true, Position = 1)]
        [ValidateNotNullOrEmpty]
        public string Message { get; set; }

        [Parameter()]
        public string? ComputerName { get; set; }

        [Parameter()]
        public int[]? SessionId { get; set; }

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
    public class GetRemoteMessageOptionsCmdlet : PSCmdlet
    {
        protected override void ProcessRecord()
        {
            WriteObject(Abstraction.MessageBoxOption.GetAvailableOptions());
        }
    }
    
    [Cmdlet(VerbsCommon.Get, "ComputerSession")]
    public class GetComputerSessionCmdlet : PSCmdlet
    {
        [Parameter(Position = 0)]
        [ValidateNotNullOrEmpty]
        public string? ComputerName { get; set; }

        [Parameter()]
        public SwitchParameter ActiveOnly { get; set; } = false;

        [Parameter()]
        public SwitchParameter ExcludeSystemSession { get; set; } = false;

        protected override void ProcessRecord()
        {
            if (string.IsNullOrEmpty(ComputerName))
                WriteObject(Utilities.GetComputerSession(ActiveOnly, ExcludeSystemSession));
            else
                WriteObject(Utilities.GetComputerSession(ComputerName, ActiveOnly, ExcludeSystemSession));
        }
    }
    
    [Cmdlet(VerbsCommunications.Send, "Click")]
    public class SendClickCmdlet : PSCmdlet
    {
        protected override void ProcessRecord()
        {
            Utilities.SendClick();
        }
    }

    [Cmdlet(VerbsCommon.Get, "ResourceMessageTable")]
    public class GetResourceMessageTableCmdlet : PSCmdlet
    {
        [Parameter(Mandatory = true, Position = 0, ValueFromPipeline = true)]
        [ValidateNotNullOrEmpty]
        public string Path { get; set; }

        protected override void ProcessRecord()
        {
            Managed unWrapper = new();
            WriteObject(unWrapper.GetResourceMessageTable(Path));
        }
    }

    [Cmdlet(VerbsCommon.Get, "FormattedError")]
    public class GetFormattedErrorCmdlet : PSCmdlet
    {
        [Parameter(Mandatory = true, Position = 0, ValueFromPipeline = true)]
        public int ErrorCode { get; set; }

        protected override void ProcessRecord()
        {
            Managed unWrapper = new();
            WriteObject(unWrapper.GetFormatedError(ErrorCode));
        }
    }

    [Cmdlet(VerbsCommon.Get, "LastWin32Error")]
    public class GetLastWin32ErrorCmdlet : PSCmdlet
    {
        protected override void ProcessRecord()
        {
            Managed unWrapper = new();
            WriteObject(unWrapper.GetFormatedWin32Error());
        }
    }

    [Cmdlet(VerbsCommon.Get, "LastWinSockError")]
    public class GetLastWinSockErrorCmdlet : PSCmdlet
    {
        protected override void ProcessRecord()
        {
            Managed unWrapper = new();
            WriteObject(unWrapper.GetFormatedWSError());
        }
    }

    [Cmdlet(VerbsCommon.Get, "FileHandle")]
    public class GetFileHandleCmdlet : PSCmdlet
    {
        [Parameter(Mandatory = true, Position = 0, ValueFromPipeline = true)]
        [ValidateNotNullOrEmpty]
        public string Path { get; set; }

        protected override void ProcessRecord()
        {
            Managed unWrapper = new();
            WriteObject(unWrapper.GetProcessFileHandle(Path));
        }
    }

    [Cmdlet(VerbsCommon.Get, "MsiProperties")]
    public class GetMsiPropertiesCmdlet : PSCmdlet
    {
        [Parameter(Mandatory = true, Position = 0, ValueFromPipeline = true)]
        [ValidateNotNullOrEmpty]
        public string Path { get; set; }

        protected override void ProcessRecord()
        {
            Managed unWrapper = new();
            WriteObject(unWrapper.GetMsiProperties(Path));
        }
    }
}