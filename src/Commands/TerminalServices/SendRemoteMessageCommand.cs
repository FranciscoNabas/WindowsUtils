using System.Management.Automation;
using WindowsUtils.Wrappers;
using WindowsUtils.TerminalServices;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
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
        private readonly TerminalServicesWrapper _unwrapper = new();

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
        public int Timeout {
            get {
                if (true == Wait && _timeout == 0)
                    throw new ArgumentException("'Timeout' cannot be zero when using 'Wait'.");

                return _timeout;
            }
            set {
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
                        MessageResponse[] result = _unwrapper.SendRemoteMessage(IntPtr.Zero, null, Title, Message, nativeStyle, Timeout, Wait);
                        result.Where(q => q is not null).ToList().ForEach(WriteObject);
                    }
                }
                else
                {
                    MessageResponse[] result = _unwrapper.SendRemoteMessage(IntPtr.Zero, SessionId, Title, Message, nativeStyle, Timeout, Wait);
                    result.Where(q => q is not null).ToList().ForEach(WriteObject);
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
                        MessageResponse[] result = _unwrapper.SendRemoteMessage(session.SessionHandle.DangerousGetHandle(), null, Title, Message, nativeStyle, Timeout, Wait);
                        result.Where(q => q is not null).ToList().ForEach(WriteObject);
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
                            MessageResponse[] result = _unwrapper.SendRemoteMessage(session.SessionHandle.DangerousGetHandle(), null, Title, Message, nativeStyle, Timeout, Wait);
                            result.Where(q => q is not null).ToList().ForEach(WriteObject);
                        }
                    }
                    else
                    {
                        MessageResponse[] result = _unwrapper.SendRemoteMessage(session.SessionHandle.DangerousGetHandle(), SessionId, Title, Message, nativeStyle, Timeout, Wait);
                        result.Where(q => q is not null).ToList().ForEach(WriteObject);
                    }
                }
            }
        }
    }
}