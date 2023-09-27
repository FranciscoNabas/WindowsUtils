using System.Management.Automation;
using WindowsUtils.Core;

namespace WindowsUtils.Commands
{
    /// <summary>
    /// <para type="synopsis">Test if a TCP or UDP port is open.</para>
    /// <para type="description">This Cmdlet tests if TCP or UPD ports are opened in a given destination.</para>
    /// <para type="description">Attention! Testing UDP can return false positives due the nature of the protocol. If the server doesn't refuse the connection we consider the port open.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Test-Port -ComputerName 'google.com'</code>
    ///     <para>Tests if the TCP port '80' is opened at 'google.com'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>testport 'SUPERSERVER.contoso.com' -UdpPort 69</code>
    ///     <para>Tests if the UDP port '69' is opened at 'SUPERSERVER.contoso.com'.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>testport 'SUPERSERVER.contoso.com' -TcpPort 80, 443 -UdpPort 67, 68, 69</code>
    ///     <para>Tests if the TCP ports 80, 443, and UDP ports 67, 68, and 69 are opened at 'SUPERSERVER.contoso.com'.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsDiagnostic.Test, "Port")]
    [Alias("testport")]
    public class TestPortCommand : CoreCommandBase
    {
        private struct SingleTestInfo
        {
            public int Port;
            public TransportProtocol Protocol;
        }

        private readonly Wrapper _unwrapper = new();
        private readonly List<SingleTestInfo> _portList = new();

        private string[] _destination = new string[] { "localhost" };

        /// <summary>
        /// <para type="description">One or more destination.</para>
        /// </summary>
        [Parameter(Position = 0)]
        [ValidateNotNullOrEmpty]
        public string[] ComputerName {
            get { return _destination; }
            set { _destination = value; }
        }

        /// <summary>
        /// <para type="description">One or more TCP ports.</para>
        /// </summary>
        [Parameter(Position = 1)]
        [ValidateNotNullOrEmpty]
        public int[] TcpPort {
            get {
                return _portList
                    .Where(p => p.Protocol == TransportProtocol.Tcp)
                    .Select(p => p.Port)
                    .ToArray();
            }
            set {
                foreach (int port in value)
                    _portList.Add(new() { Port = port, Protocol = TransportProtocol.Tcp });
            }
        }

        /// <summary>
        /// <para type="description">One or more UDP ports.</para>
        /// </summary>
        [Parameter()]
        public int[] UdpPort {
            get {
                return _portList
                    .Where(p => p.Protocol == TransportProtocol.Udp)
                    .Select(p => p.Port)
                    .ToArray();
            }
            set {
                foreach (int port in value)
                    _portList.Add(new() { Port = port, Protocol = TransportProtocol.Udp });
            }
        }

        /// <summary>
        /// <para type="description">The test timeout, in seconds.</para>
        /// </summary>
        [Parameter()]
        [ValidateRange(1, int.MaxValue)]
        public int Timeout { get; set; } = 2;

        protected override void ProcessRecord()
        {
            if (_portList.Count == 0)
                throw new ArgumentException("You need to input at least one port.");

            foreach (string destination in _destination)
            {
                foreach (SingleTestInfo port in _portList)
                {
                    if (port.Port < 0 || port.Port > ushort.MaxValue)
                    {
                        WriteError(new(
                            new ArgumentOutOfRangeException($"Port cannot be smaller than 0, or bigger than 65535. Was '{port.Port}'."),
                            "PortOutOfRange",
                            ErrorCategory.InvalidArgument,
                            port
                        ));

                        continue;
                    }

                    _unwrapper.TestNetworkPort(destination, (uint)port.Port, port.Protocol, (uint)Timeout, (CmdletContextBase)CmdletContext);
                }
            }
        }
    }
}