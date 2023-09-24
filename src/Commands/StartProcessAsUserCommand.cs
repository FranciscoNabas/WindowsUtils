using System.Security;
using System.Management.Automation;
using WindowsUtils.Core;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618
    [Cmdlet(
        VerbsLifecycle.Start, "ProcessAsUser",
        DefaultParameterSetName = "withUserName"
    )]
    [Alias("runas")]
    public class StartProcessAsUserCommand : PSCmdlet
    {
        private Wrapper _unwrapper = new();

        private string _userName;
        private string _domain;
        private SecureString _password;

        private string _titleBar;

        [Parameter(
            Mandatory = true,
            Position = 0,
            ParameterSetName = "withUserName"
        )]
        [ValidateNotNullOrEmpty]
        public string UserName { get; set; }

        [Parameter(
            Mandatory = true,
            Position = 1,
            ParameterSetName = "withUserName"
        )]
        [Parameter(
            Mandatory = true,
            Position = 1,
            ParameterSetName = "withCredentials"
        )]
        [ValidateNotNullOrEmpty]
        public string CommandLine { get; set; }

        [Parameter(
            Mandatory = true,
            ParameterSetName = "withCredentials"
        )]
        public PSCredential Credential { get; set; }

        protected override void BeginProcessing()
        {
            if (ParameterSetName == "withUserName")
            {
                Console.Write($"Enter the password for {UserName}: ");
                _password = Utils.ReadPassword();
                Console.Write("\n");

                if (UserName.Contains('\\'))
                {
                    var userSplit = UserName.Split('\\');
                    _domain = userSplit[0];
                    _userName = userSplit[1];
                }
                else if (UserName.Contains('@'))
                {
                    var userSplit = UserName.Split('@');
                    _domain = userSplit[0].Split('.')[0];
                    _userName = userSplit[1];
                }
                else
                {
                    _userName = UserName;
                    _domain = Environment.GetEnvironmentVariable("COMPUTERNAME");
                }
            }
            else
            {
                _password = Credential.Password;

                if (Credential.UserName.Contains('\\'))
                {
                    var userSplit = Credential.UserName.Split('\\');
                    _domain = userSplit[0];
                    _userName = userSplit[1];
                }
                else if (Credential.UserName.Contains('@'))
                {
                    var userSplit = Credential.UserName.Split('@');
                    _domain = userSplit[0].Split('.')[0];
                    _userName = userSplit[1];
                }
                else
                {
                    _userName = Credential.UserName;
                    _domain = Environment.GetEnvironmentVariable("COMPUTERNAME");
                }
            }

            _titleBar = $"{CommandLine} (running as {_domain}\\{_userName})";
        }

        protected override void ProcessRecord()
            => _unwrapper.StartProcessAsUser(_userName, _domain, _password, CommandLine, _titleBar);
    }
}