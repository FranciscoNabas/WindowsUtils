using System;
using System.Threading;
using System.Diagnostics;
using System.Runtime.InteropServices;
using WuPesterHelper.Interop;
using WuPesterHelper.Utilities;

namespace WuPesterHelper;

public sealed class ManagedService : IDisposable
{
    private readonly nint m_hScm;
    private readonly nint m_hService;
    private readonly string m_name;
    private readonly string m_computerName;

    private bool m_isDisposed;

    public ManagedService(string name, string computerName)
    {
        m_hScm = NativeServices.OpenSCManager(computerName, Constants.SERVICES_ACTIVE_DATABASE, ScmAccess.CONNECT | ScmAccess.ENUMERATE_SERVICE);
        if (m_hScm == 0)
            throw new NativeException(Marshal.GetLastWin32Error());

        try {
            m_hService = NativeServices.OpenService(m_hScm, name, ServiceAccess.ALL_ACCESS);
            if (m_hService == 0)
                throw new NativeException(Marshal.GetLastWin32Error());
        }
        catch {
            NativeServices.CloseServiceHandle(m_hScm);
            throw;
        }

        m_name = name;
        m_computerName = computerName;

        m_isDisposed = false;
    }

    internal ManagedService(string name, string computerName, nint hScm, nint hService)
    {
        m_hScm = hScm;
        m_hService = hService;
        m_name = name;
        m_computerName = computerName;

        m_isDisposed = false;
    }

    ~ManagedService()
        => Dispose(disposing: false);

    public static void StartService(string serviceName, string computerName = "")
    {
        using ManagedService service = new(serviceName, computerName);
        service.Start();
    }

    public static void StopService(string serviceName, string computerName = "")
    {
        using ManagedService service = new(serviceName, computerName);
        service.Stop();
    }

    public static void DeleteService(string serviceName, string computerName = "", bool stop = false)
    {
        using ManagedService service = new(serviceName, computerName);
        service.Delete(stop);
    }

    public void Start(string[]? args = null)
        => NativeServices.StartService(m_name, m_hService, args);

    public void Stop()
    {
        SERVICE_STATUS_PROCESS status = new();
        NativeServices.GetServiceStatus(m_hService, ref status);
        if (status.dwCurrentState == ServiceState.STOPPED)
            return;

        Stopwatch sw = Stopwatch.StartNew();
        while (status.dwCurrentState == ServiceState.STOP_PENDING) {
            General.WriteLineInPlace($"Waiting for service '{m_name}' to stop...");

            uint waitTime = status.dwWaitHint / 10;
            if (waitTime < 1000)
                waitTime = 1000;
            else if (waitTime > 10000)
                waitTime = 10000;

            Thread.Sleep((int)waitTime);
            NativeServices.GetServiceStatus(m_hService, ref status);

            if (status.dwCurrentState == ServiceState.STOPPED) {
                sw.Stop();
                return;
            }

            if (sw.Elapsed.TotalSeconds > 30) {
                sw.Stop();
                throw new TimeoutException($"Timed out while waiting for service '{m_name}' to stop.");
            }
        }

        StopDependentServices();

        sw.Restart();
        _ = NativeServices.ControlService(m_hService, ServiceControl.STOP);
        while (status.dwCurrentState != ServiceState.STOPPED) {
            General.WriteLineInPlace($"Waiting for service '{m_name}' to stop...");

            uint waitTime = status.dwWaitHint / 10;
            if (waitTime < 1000)
                waitTime = 1000;
            else if (waitTime > 10000)
                waitTime = 10000;

            Thread.Sleep((int)waitTime);
            NativeServices.GetServiceStatus(m_hService, ref status);

            if (status.dwCurrentState == ServiceState.STOPPED) {
                sw.Stop();
                return;
            }

            if (sw.Elapsed.TotalSeconds > 30) {
                sw.Stop();
                throw new TimeoutException($"Timed out while waiting for service '{m_name}' to stop.");
            }
        }
    }

    public void Delete(bool stop)
    {
        if (stop)
            Stop();

        NativeServices.DeleteService(m_hService);
        Dispose();
    }

    public void Dispose()
    {
        Dispose(disposing: true);
        GC.SuppressFinalize(this);
    }

    private void Dispose(bool disposing)
    {
        if (disposing && !m_isDisposed) {
            NativeServices.CloseServiceHandle(m_hService);
            NativeServices.CloseServiceHandle(m_hScm);

            m_isDisposed = true;
        }
    }

    private void StopDependentServices()
    {
        foreach (string serviceName in NativeServices.GetActiveDependentServices(m_hService)) {
            using ManagedService service = new(serviceName, m_computerName);
            service.Stop();
        }
    }
}