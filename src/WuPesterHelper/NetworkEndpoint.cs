using System;
using System.Net;
using System.Linq;
using System.Text;
using System.Threading;
using System.Net.Sockets;
using System.Threading.Tasks;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace WuPesterHelper;

public sealed class NetworkEndpoint : IDisposable
{
    private readonly ushort[] m_udpPorts;
    private readonly ushort[] m_tcpPorts;

    private readonly List<Task> m_tasks;
    private readonly CancellationTokenSource m_cts;
    
    private bool m_isDisposed;

    public ReadOnlyCollection<ushort> UdpPorts => m_udpPorts.AsReadOnly();
    public ReadOnlyCollection<ushort> TcpPorts => m_tcpPorts.AsReadOnly();

    internal NetworkEndpoint(IEnumerable<ushort> tcpPorts, IEnumerable<ushort> udpPorts)
    {
        m_tasks = [];
        m_cts = new();

        m_tasks.AddRange(tcpPorts.Select(p => ErectTcpEndpoint(p, m_cts.Token)));
        m_tasks.AddRange(udpPorts.Select(p => ErectUdpEndpoint(p, m_cts.Token)));

        m_udpPorts = udpPorts.ToArray();
        m_tcpPorts = tcpPorts.ToArray();

        m_isDisposed = false;
    }

    ~NetworkEndpoint()
        => Dispose(disposing: false);

    public void Dispose()
    {
        Dispose(disposing: true);
        GC.SuppressFinalize(this);
    }

    private void Dispose(bool disposing)
    {
        if (disposing && !m_isDisposed) {
            m_cts.Cancel();
            
            try { Task.WaitAll(m_tasks); }
            catch { }

            m_tasks.ForEach(t => t.Dispose());
            m_cts.Dispose();

            m_isDisposed = true;
        }
    }

    private static Task ErectUdpEndpoint(int port, CancellationToken cancellationToken) => Task.Run(() => {
        using UdpClient clientV4 = new(new IPEndPoint(IPAddress.Any, port));
        using UdpClient clientV6 = new(new IPEndPoint(IPAddress.IPv6Any, port));

        Task[] tasks = [
            Task.Run(async () => await HandleRequests(clientV4, cancellationToken)),
            Task.Run(async () => await HandleRequests(clientV6, cancellationToken))
        ];

        try {
            Task.WaitAll(tasks);
        }
        finally {
            tasks[0].Dispose();
            tasks[1].Dispose();
        }

        async Task HandleRequests(UdpClient client, CancellationToken cancellationToken)
        {
            SpinWait sw = new();

            do {
                UdpReceiveResult result = await client.ReceiveAsync(cancellationToken);
                Console.WriteLine($"UDP data received on port '{port}': {Encoding.Unicode.GetString(result.Buffer)}");

                sw.SpinOnce();

            } while (!cancellationToken.IsCancellationRequested);
        }

    }, cancellationToken);

    private static Task ErectTcpEndpoint(int port, CancellationToken cancellationToken) => Task.Run(() => {
        using TcpListener listenerV4 = new(IPAddress.Any, port);
        using TcpListener listenerV6 = new(IPAddress.IPv6Any, port);
        listenerV4.Start();
        listenerV6.Start();
        try {
            Task[] tasks = [
                Task.Run(async () => await HandleRequests(listenerV4, cancellationToken)),
                Task.Run(async () => await HandleRequests(listenerV6, cancellationToken))
            ];

            try {
                Task.WaitAll(tasks);
            }
            finally {
                tasks[0].Dispose();
                tasks[1].Dispose();
            }
        }
        finally {
            listenerV4.Stop();
            listenerV6.Stop();
        }

        async Task HandleRequests(TcpListener listener, CancellationToken cancellationToken)
        {
            SpinWait sw = new();

            do {
                using TcpClient handler = await listener.AcceptTcpClientAsync(cancellationToken);
                using NetworkStream stream = handler.GetStream();

                StringBuilder sb = new();
                byte[] buffer = new byte[1024];
                int bytesRead = stream.Read(buffer.AsSpan());
                while (bytesRead > 0) {
                    sb.Append(Encoding.Unicode.GetString(buffer.AsSpan()[bytesRead..]));
                    bytesRead = stream.Read(buffer.AsSpan());
                }

                Console.WriteLine($"TCP data received on port '{port}': {sb}");

                sw.SpinOnce();

            } while (!cancellationToken.IsCancellationRequested);
        }

    }, cancellationToken);
}