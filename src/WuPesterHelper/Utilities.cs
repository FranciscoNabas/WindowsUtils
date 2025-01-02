using System;
using System.Net;
using System.Text;
using System.Net.Sockets;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using WuPesterHelper.Interop;

namespace WuPesterHelper.Utilities;

public static class General
{
    /// <summary>
    /// Writes a line to the console without changing the cursor position.
    /// </summary>
    /// <param name="value">The value to write.</param>
    public static void WriteLineInPlace(string? value)
    {
        var (Left, Top) = Console.GetCursorPosition();
        Console.WriteLine(value);
        Console.SetCursorPosition(Left, Top);
    }
}

public static class Network
{
    /// <summary>
    /// Erects a network endpoint with the specified ports open.
    /// </summary>
    /// <param name="tcpPorts">An <see cref="IEnumerable{ushort}"/> of TCP ports.</param>
    /// <param name="udpPorts">An <see cref="IEnumerable{ushort}"/> of UDP ports.</param>
    /// <returns>A <see cref="NetworkEndpoint"/> representing the open and listening ports.</returns>
    public static NetworkEndpoint ErectEndpoint(IEnumerable<ushort> tcpPorts, IEnumerable<ushort> udpPorts)
        => new(tcpPorts, udpPorts);

    /// <summary>
    /// Erects a network endpoint with a number of ports open.
    /// </summary>
    /// <param name="tcpPortCount">The number of TCP ports to open.</param>
    /// <param name="udpPortCount">The number of UDP ports to open.</param>
    /// <returns>A <see cref="NetworkEndpoint"/> representing the open and listening ports.</returns>
    /// <exception cref="OverflowException">We couldn't find the requested number of free ports.</exception>
    public static NetworkEndpoint ErectEndpoint(uint tcpPortCount, uint udpPortCount)
    {
        // The port lists.
        List<ushort> tcpPorts = [];
        List<ushort> udpPorts = [];

        // Starting port values.
        ushort tcpPort = 666;
        ushort udpPort = 6969;

        // Looking for free TCP ports.
        for (int i = 0; i < tcpPortCount;) {
            if (!IsPortOpen(tcpPort, false)) {
                tcpPorts.Add(tcpPort);
                i++;
            }

            tcpPort++;

            // Overflow.
            if (tcpPort == 666)
                throw new OverflowException("There are no TCP ports left.");
        }

        // Looking for free UDP ports.
        for (int i = 0; i < udpPortCount;) {
            if (!IsPortOpen(udpPort, true)) {
                udpPorts.Add(udpPort);
                i++;
            }

            udpPort++;

            // Overflow.
            if (udpPort == 6969)
                throw new OverflowException("There are no UDP ports left.");
        }

        return new(tcpPorts, udpPorts);
    }

    /// <summary>
    /// Checks if a network port is being used in the current computer.
    /// </summary>
    /// <param name="port">The port.</param>
    /// <param name="udp">True if it's a UDP port.</param>
    /// <returns>True if the port is open and in use.</returns>
    private static bool IsPortOpen(ushort port, bool udp)
    {
        if (udp) {
            using UdpClient client = new();
            client.Connect("localhost", port);
            client.Send(Encoding.ASCII.GetBytes("Tits!"), 5);
            try {
                IPEndPoint endpoint = new(IPAddress.Any, 0);
                client.Receive(ref endpoint);

                return true;
            }
            catch (SocketException ex) {
                if (ex.NativeErrorCode == 10054)
                    return false;
            }
        }
        else {
            //
            // This can be used to speed up the connection test
            // and should be enough, since we're connecting to
            // localhost, but we lose the ability to track exceptions.
            //
            // using TcpClient test = new();
            // try {
            //     if (!test.ConnectAsync("localhost", port).Wait(500))
            //         return false;
               
            //     return true;
            // }
            // catch (Exception) {
            //     return false;
            // }
            //

            using TcpClient client = new();
            try {
                client.Connect("localhost", port);

                return true;
            }
            catch (SocketException ex) {
                if (ex.NativeErrorCode == 10061)
                    return false;
            }
        }

        return true;
    }
}

public static class ProcessAndThread
{
    /// <summary>
    /// Loads a module in another process.
    /// </summary>
    /// <param name="modulePath">The module path.</param>
    /// <param name="processId">The target process unique identifier.</param>
    /// <remarks>
    /// Run-off-the-mill DLL injection technique.
    /// Uses 'LoadLibrary' and 'CreateRemoteThread' to load a module in the target process.
    /// </remarks>
    /// <seealso href="https://learn.microsoft.com/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryw">LoadLibraryW function (libloaderapi.h)</seealso>
    /// <seealso href="https://learn.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-createremotethread">CreateRemoteThread function (processthreadsapi.h)</seealso>
    public static void LoadModuleInProcess(string modulePath, uint processId)
    {
        // Opening a handle to the target process.
        using SafeProcessHandle hProcess = NativeProcess.OpenProcess(
            processId,
            ProcessAccess.CREATE_THREAD      |  // Allows us to create threads for this process handle.
            ProcessAccess.QUERY_INFORMATION  |  // Allows us to query information about the process. Used implicitly by the system.
            ProcessAccess.VM_OPERATION       |  // Required to perform operations in the process virtual memory space.
            ProcessAccess.VM_READ            |  // Allows us to read the process virtual memory.
            ProcessAccess.DUP_HANDLE         |  // Required to duplicate handles from this process.
            ProcessAccess.VM_WRITE,             // Allows us to write into the process virtual memory space.
            false
        );

        // Getting a handle to kernel32.dll. This will take care of releasing the module only if needed.
        using SafeModuleHandle hKernel32 = new(@"C:\Windows\System32\kernel32.dll");

        // Getting the address of LoadLibraryW.
        nint procAddr = Common.GetProcAddress(hKernel32, "LoadLibraryW");

        // Allocating memory in the target process space.
        int buffLen = (modulePath.Length + 1) * UnicodeEncoding.CharSize;
        nint buffer = Common.VirtualAllocReadWrite(hProcess, 0, buffLen);
        try {
            // Getting a buffer from our module path.
            nint strBuffer = Marshal.StringToHGlobalUni(modulePath);
            try {
                // Writing the buffer in the target process space.
                NativeProcess.WriteProcessMemory(hProcess, buffer, strBuffer, buffLen);

                // Creating the remote thread.
                var threadInfo = NativeProcess.CreateRemoteThread(
                    hProcess,                   // The handle to the target process.
                    procAddr,                   // The 'ThreadProc', in this case the address of 'LoadLibraryW'.
                    buffer,                     // The argument to pass to the 'ThreadProc', in this case our module buffer allocated in the target process space.
                    ThreadCreationFlags.NONE    // Thread creation flags.
                );

                // Waiting for the thread to exit, if not already.
                Common.WaitForSingleObject((SafeSystemHandle)threadInfo.Item2, Constants.INFINITE);

                // Disposing of the created thread handle.
                threadInfo.Item2.Dispose();
            }
            finally {
                // Freeing our buffer.
                Marshal.FreeHGlobal(strBuffer);
            }
        }
        finally {
            // Freeing the target process buffer.
            Common.VirtualFree(hProcess, buffer);
        }
    }
}