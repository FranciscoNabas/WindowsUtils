using System.Management.Automation;
using WindowsUtils.Engine;
using WindowsUtils.Network;
using WindowsUtils.Wrappers;

namespace WindowsUtils.Commands
{
#pragma warning disable CS8618

    /// <summary>
    /// <para type="synopsis">Returns network statistics for the current computer.</para>
    /// <para type="description">This Cmdlet returns network statistics for the current computer.</para>
    /// <para type="description">It works similarly to the 'netstat.exe' application, but with more options and more information. The parameters aliases mimics the application's parameters.</para>
    /// <example>
    ///     <para></para>
    ///     <code>Get-NetworkStatistics -All -IncludeModuleName</code>
    ///     <para>Returns which ports are opened by which process, for TCP and UDP. Including the module name.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>Get-NetworkStatistics -InterfaceStatistics</code>
    ///     <para>Returns ethernet statistics for all interfaces.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>getnetstat -Statistics</code>
    ///     <para>Returns statistics for ethernet, IP, ICMP, TCP, and UDP.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>getnetstat -s -p Ethernet, Tcp -CombineIfStats</code>
    ///     <para>Returns statistics for ethernet, all combined, and TCP.</para>
    ///     <para></para>
    /// </example>
    /// <example>
    ///     <para></para>
    ///     <code>getnetstat -RouteTable</code>
    ///     <para>Returns IPv4 and IPv6 route table, including persistent routes, if any.</para>
    ///     <para></para>
    /// </example>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "NetworkStatistics", DefaultParameterSetName = "byTransportTables")]
    [OutputType(typeof(TransportTableInfo), ParameterSetName = new string[] { "byTransportTables" })]
    [OutputType(typeof(InterfaceStatistics), ParameterSetName = new string[] { "byInterfaceStatistics" })]
    [OutputType(typeof(NetworkStatistics), ParameterSetName = new string[] { "byStatistics" })]
    [OutputType(typeof(IpRoute), ParameterSetName = new string[] { "byRouteTable" })]
    [Alias(new string[] { "getnetstat" })]
    public class GetNetworkStatisticsCommand : CoreCommandBase
    {
        private List<InterfaceStatistics> _ethStats;
        private List<IpStatistics> _ipStats;
        private List<IcmpStatistics> _icmpStats;
        private List<TcpStatistics> _tcpStats;
        private List<UdpStatistics> _udpStats;

        /// <summary>
        /// <para type="description">Includes UDP information to the transport table.</para>
        /// </summary>
        [Parameter(ParameterSetName = "byTransportTables")]
        [Alias(new string[] { "a" })]
        public SwitchParameter All {  get; set; }

        /// <summary>
        /// <para type="description">Includes the owning process module name.</para>
        /// </summary>
        [Parameter(ParameterSetName = "byTransportTables")]
        [Alias(new string[] { "b" })]
        public SwitchParameter IncludeModuleName { get; set; }

        /// <summary>
        /// <para type="description">Returns ethernet statistics for all interfaces.</para>
        /// </summary>
        [Parameter(ParameterSetName = "byInterfaceStatistics")]
        [Alias(new string[] { "e" })]
        public SwitchParameter InterfaceStatistics { get; set; }

        /// <summary>
        /// <para type="description">Combine all interfaces statistics in a single object.</para>
        /// </summary>
        [Parameter(ParameterSetName = "byInterfaceStatistics")]
        [Parameter(ParameterSetName = "byStatistics")]
        public SwitchParameter CombineIfStats { get; set; }

        /// <summary>
        /// <para type="description">Returns interface, IP, ICMP, TCP, and UDP statistics, or what's filtered by the 'Protocol' parameter.</para>
        /// </summary>
        [Parameter(ParameterSetName = "byStatistics")]
        [Alias(new string[] { "s" })]
        public SwitchParameter Statistics { get; set; }

        /// <summary>
        /// <para type="description">Filters the protocols returned by the 'Statistics' parameter.</para>
        /// </summary>
        [Parameter(ParameterSetName = "byStatistics")]
        [ValidateNotNullOrEmpty]
        [Alias(new string[] { "p" })]
        public GetNetStatProtocol[] Protocol { get; set; }

        /// <summary>
        /// <para type="description">Returns the route table.</para>
        /// </summary>
        [Parameter(ParameterSetName = "byRouteTable")]
        [Alias(new string[] { "r" })]
        public SwitchParameter RouteTable { get; set; }


        protected override void BeginProcessing()
        {
            if (Protocol is not null && !Statistics.IsPresent)
                throw new ParameterBindingException("'Protocol' can only be used with 'Statistics'.");
        }

        protected override void ProcessRecord()
        {
            switch (ParameterSetName) {
                case "byTransportTables":
                    try {
                        Network.GetTransportTables(All, IncludeModuleName);
                    }
                    catch (NativeException) { }
                    break;

                case "byInterfaceStatistics":
                    try {
                        if (!CombineIfStats)
                            Network.GetInterfaceStatistics();
                        else
                            WriteObject(GetIfStatisticsCombined());
                    }
                    catch (NativeException) { }
                    break;

                case "byRouteTable":
                    try {
                        Network.GetIpRouteTable();
                    }
                    catch (NativeException) { }
                    break;

                case "byStatistics":
                    _ethStats = new();
                    _ipStats = new();
                    _icmpStats = new();
                    _tcpStats = new();
                    _udpStats = new();
                    
                    try {
                        if (Protocol is not null) {
                            foreach (var proto in Protocol) {
                                switch (proto) {
                                    case GetNetStatProtocol.Tcp:
                                        Network.GetTcpv4Statistics(_tcpStats);
                                        break;
                                    case GetNetStatProtocol.Tcpv6:
                                        Network.GetTcpv6Statistics(_tcpStats);
                                        break;
                                    case GetNetStatProtocol.Udp:
                                        Network.GetUdpv4Statistics(_udpStats);
                                        break;
                                    case GetNetStatProtocol.Udpv6:
                                        Network.GetUdpv6Statistics(_udpStats);
                                        break;
                                    case GetNetStatProtocol.Ipv4:
                                        Network.GetIpv4Statistics(_ipStats);
                                        break;
                                    case GetNetStatProtocol.Ipv6:
                                        Network.GetIpv6Statistics(_ipStats);
                                        break;
                                    case GetNetStatProtocol.Icmp:
                                        Network.GetIcmpv4Statistics(_icmpStats);
                                        break;
                                    case GetNetStatProtocol.Icmpv6:
                                        Network.GetIcmpv6Statistics(_icmpStats);
                                        break;
                                    case GetNetStatProtocol.Ethernet:
                                        if (!CombineIfStats) {
                                            List<InterfaceStatisticsSingle> ifStats = new();
                                            Network.GetInterfaceStatistics(ifStats);
                                            _ethStats.AddRange(ifStats);
                                        }
                                        else
                                            _ethStats.Add(GetIfStatisticsCombined());
                                        break;
                                }
                            }
                        }
                        else {
                            GetAllStatistics();
                        }
                    }
                    catch (NativeException) { }
                    WriteObject(new NetworkStatistics(_ethStats.ToArray(), _ipStats.ToArray(), _icmpStats.ToArray(), _tcpStats.ToArray(), _udpStats.ToArray()));
                    break;
            }
        }

        private void GetAllStatistics()
        {
            Network.GetTcpv4Statistics(_tcpStats);
            Network.GetTcpv6Statistics(_tcpStats);
            Network.GetUdpv4Statistics(_udpStats);
            Network.GetUdpv6Statistics(_udpStats);
            Network.GetIpv4Statistics(_ipStats);
            Network.GetIpv6Statistics(_ipStats);
            Network.GetIcmpv4Statistics(_icmpStats);
            Network.GetIcmpv6Statistics(_icmpStats);
            if (!CombineIfStats) {
                List<InterfaceStatisticsSingle> ifStats = new();
                Network.GetInterfaceStatistics(ifStats);
                _ethStats.AddRange(ifStats);
            }
            else
                _ethStats.Add(GetIfStatisticsCombined());
        }

        private InterfaceStatisticsCombined GetIfStatisticsCombined()
        {
            List<InterfaceStatisticsSingle> allStats = new();
            Network.GetInterfaceStatistics(allStats);
            
            // Thought about using Linq, but I'm concerned with performance. I'm a noob.
            long unknownProtocolsReceived  = 0;
            long errorsReceived            = 0;
            long errorsSent                = 0;
            long discardsReceived          = 0;
            long discardsSent              = 0;
            long nonUnicastReceived        = 0;
            long nonUnicastSent            = 0;
            long unicastReceived           = 0;
            long unicastSent               = 0;
            long bytesReceived             = 0;
            long bytesSent                 = 0;
            long outQueueLength            = 0;
            foreach (InterfaceStatisticsSingle stat in allStats) {
                unknownProtocolsReceived   += stat.UnknownProtocolsReceived;
                errorsReceived             += stat.ErrorsReceived;
                errorsSent                 += stat.ErrorsSent;
                discardsReceived           += stat.DiscardsReceived;
                discardsSent               += stat.DiscardsSent;
                nonUnicastReceived         += stat.NonUnicastReceived;
                nonUnicastSent             += stat.NonUnicastSent;
                unicastReceived            += stat.UnicastReceived;
                unicastSent                += stat.UnicastSent;
                bytesReceived              += stat.BytesReceived;
                bytesSent                  += stat.BytesSent;
                outQueueLength             += stat.OutQueueLength;
            }

            return new InterfaceStatisticsCombined(unknownProtocolsReceived,
                errorsReceived,
                errorsSent,
                discardsReceived,
                discardsSent,
                nonUnicastReceived,
                nonUnicastSent,
                unicastReceived,
                unicastSent,
                bytesReceived,
                bytesSent,
                outQueueLength
            );
        }
    }
}