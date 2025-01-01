namespace WindowsUtils.Network;

public sealed class NetworkStatistics
{
    public InterfaceStatistics[] InterfaceStatistics { get; }
    public IpStatistics[] IpStatistics { get; }
    public IcmpStatistics[] IcmpStatistics { get; }
    public TcpStatistics[] TcpStatistics { get; }
    public UdpStatistics[] UdpStatistics { get; }

    internal NetworkStatistics(
        InterfaceStatistics[] ethStats,
        IpStatistics[] ipStats,
        IcmpStatistics[] icmpStats,
        TcpStatistics[] tcpStats,
        UdpStatistics[] udpStats
    ) => (InterfaceStatistics, IpStatistics, IcmpStatistics, TcpStatistics, UdpStatistics) =
        (ethStats, ipStats, icmpStats, tcpStats, udpStats);
}

public sealed class InterfaceStatisticsCombined : InterfaceStatistics
{
    public long OutQueueLength { get; }
    public long BytesSent { get; }
    public long BytesReceived { get; }
    public long UnicastSent { get; }
    public long UnicastReceived { get; }
    public long NonUnicastSent { get; }
    public long NonUnicastReceived { get; }
    public long DiscardsSent { get; }
    public long DiscardsReceived { get; }
    public long ErrorsSent { get; }
    public long ErrorsReceived { get; }
    public long UnknownProtocolsReceived { get; }

    internal InterfaceStatisticsCombined(long unknownProtocolsReceived, long errorsReceived, long errorsSent, long discardsReceived,
        long discardsSent, long nonUnicastReceived, long nonUnicastSent, long unicastReceived, long unicastSent, long bytesReceived,
        long bytesSent, long outQueueLength)
    {
        UnknownProtocolsReceived = unknownProtocolsReceived;
        ErrorsReceived = errorsReceived;
        ErrorsSent = errorsSent;
        DiscardsReceived = discardsReceived;
        DiscardsSent = discardsSent;
        NonUnicastReceived = nonUnicastReceived;
        NonUnicastSent = nonUnicastSent;
        UnicastReceived = unicastReceived;
        UnicastSent = unicastSent;
        BytesReceived = bytesReceived;
        BytesSent = bytesSent;
        OutQueueLength = outQueueLength;
    }
}