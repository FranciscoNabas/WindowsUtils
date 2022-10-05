using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Management.Automation;
using System.IO;

#nullable enable
namespace WindowsUtils.Core
{
    public class Utilities
    {
        private static WtsSession sessionInfo = new();
        private static RestartManagerSession rmSessionHandle = new();
        
        public static ComputerSession[] GetComputerSession(string computerName, bool onlyActive, bool IncludeSystemSession)
        {
            try { IPHostEntry hostEntry = Dns.GetHostEntry(computerName); }
            catch (SocketException ex) { throw ex; }

            if (sessionInfo.SessionHandle == null || string.IsNullOrEmpty(sessionInfo.ComputerName))
            {
                sessionInfo.SessionHandle = null;
                sessionInfo.SessionHandle = Interop.WTSOpenServerW(computerName);
                if (null == sessionInfo.SessionHandle || sessionInfo.SessionHandle.ToIntPtr() == IntPtr.Zero)
                    throw new SystemException(GetFormattedWin32Error());

                sessionInfo.ComputerName = computerName;
            }
            else
            {
                if (sessionInfo.ComputerName != computerName)
                {
                    sessionInfo.SessionHandle = null;
                    sessionInfo.SessionHandle = Interop.WTSOpenServerW(computerName);
                    if (null == sessionInfo.SessionHandle || sessionInfo.SessionHandle.ToIntPtr() == IntPtr.Zero)
                        throw new SystemException(GetFormattedWin32Error());

                    sessionInfo.ComputerName = computerName;
                }
            }
            WrappedFunctions unWrapper = new();
            ComputerSession[] result = unWrapper.GetEnumeratedSession(sessionInfo.SessionHandle.ToIntPtr(), onlyActive, IncludeSystemSession);
            return result;
        }
        public static ComputerSession[] GetComputerSession(string computerName)
        {
            try { IPHostEntry hostEntry = Dns.GetHostEntry(computerName); }
            catch (SocketException ex) { throw ex; }

            if (sessionInfo.SessionHandle == null || string.IsNullOrEmpty(sessionInfo.ComputerName))
            {
                sessionInfo.SessionHandle = null;
                sessionInfo.SessionHandle = Interop.WTSOpenServerW(computerName);
                if (null == sessionInfo.SessionHandle || sessionInfo.SessionHandle.ToIntPtr() == IntPtr.Zero)
                    throw new SystemException(GetFormattedWin32Error());

                sessionInfo.ComputerName = computerName;
            }
            else
            {
                if (sessionInfo.ComputerName != computerName)
                {
                    sessionInfo.SessionHandle = null;
                    sessionInfo.SessionHandle = Interop.WTSOpenServerW(computerName);
                    if (null == sessionInfo.SessionHandle || sessionInfo.SessionHandle.ToIntPtr() == IntPtr.Zero)
                        throw new SystemException(GetFormattedWin32Error());

                    sessionInfo.ComputerName = computerName;
                }
            }
            WrappedFunctions unWrapper = new();
            ComputerSession[] result = unWrapper.GetEnumeratedSession(sessionInfo.SessionHandle.ToIntPtr(), false, false);
            return result;
        }
        public static ComputerSession[] GetComputerSession(bool onlyActive, bool IncludeSystemSession)
        {
            WrappedFunctions unWrapper = new();
            if (IncludeSystemSession == false)
                return unWrapper.GetEnumeratedSession(IntPtr.Zero, onlyActive, IncludeSystemSession);

            ComputerSession[] result = unWrapper.GetEnumeratedSession(IntPtr.Zero, onlyActive, IncludeSystemSession);
            return result;
        }
        public static ComputerSession[] GetComputerSession()
        {
            WrappedFunctions unWrapper = new();
            return unWrapper.GetEnumeratedSession(IntPtr.Zero, false, false);
        }

        public static List<MessageBoxReturn>? InvokeRemoteMessage(string title, string message, bool confirm = true)
        {
            List<MessageBoxReturn> output = new();
            if (confirm)
            {
                string input = InvokeConfirmationRequest("This will send the message to all sessions on this computer, including disconnected ones.");
                if (string.Equals(input, "N", StringComparison.OrdinalIgnoreCase)) { return null; }
                else
                {
                    WrappedFunctions unWrapper = new();
                    List<int> result = unWrapper.InvokeMessage(IntPtr.Zero, null, title, message, 0, 0, false);
                    result.ForEach(x => output.Add((MessageBoxReturn)x));
                }
            }
            else
            {
                WrappedFunctions unWrapper = new();
                List<int> result = unWrapper.InvokeMessage(IntPtr.Zero, null, title, message, 0, 0, false);
                result.ForEach(x => output.Add((MessageBoxReturn)x));
            }

            return output;
        }
        public static List<MessageBoxReturn>? InvokeRemoteMessage(string? computerName, string title, string message, bool confirm = true)
        {
            List<MessageBoxReturn> output = new();
            if (string.IsNullOrEmpty(computerName))
            {
                if (confirm)
                {
                    string input = InvokeConfirmationRequest("This will send the message to all sessions on this computer, including disconnected ones.");
                    if (string.Equals(input, "N", StringComparison.OrdinalIgnoreCase))
                        return null;
                    else
                    {
                        WrappedFunctions unWrapper = new();
                        List<int> result = unWrapper.InvokeMessage(IntPtr.Zero, null, title, message, 0, 0, false);
                        result.ForEach(x => output.Add((MessageBoxReturn)x));
                    }
                }
                else
                {
                    WrappedFunctions unWrapper = new();
                    List<int> result = unWrapper.InvokeMessage(IntPtr.Zero, null, title, message, 0, 0, false);
                    result.ForEach(x => output.Add((MessageBoxReturn)x));
                }

            }
            else
            {
                string input = InvokeConfirmationRequest("This will send the message to all sessions on the computer " + computerName + ".");
                if (string.Equals(input, "N", StringComparison.OrdinalIgnoreCase))
                    return null;
                else
                {
                    if (sessionInfo.SessionHandle == null || string.IsNullOrEmpty(sessionInfo.ComputerName))
                    {
                        sessionInfo.SessionHandle = Interop.WTSOpenServerW(computerName);
                        sessionInfo.ComputerName = computerName;
                    }
                    else
                    {
                        if (sessionInfo.ComputerName != computerName)
                        {
                            sessionInfo.SessionHandle = Interop.WTSOpenServerW(computerName);
                            sessionInfo.ComputerName = computerName;
                        }
                    }
                    WrappedFunctions unWrapper = new();
                    List<int> result = unWrapper.InvokeMessage(sessionInfo.SessionHandle.ToIntPtr(), null, title, message, 0, 0, false);
                    result.ForEach(x => output.Add((MessageBoxReturn)x));
                }
            }

            return output;
        }
        public static List<MessageBoxReturn>? InvokeRemoteMessage(string? computerName, string title, string message, string[] style, int timeout, bool wait, bool confirm = true)
        {
            List<MessageBoxReturn> output = new List<MessageBoxReturn>();
            uint unStyle = MessageBoxOption.MbOptionsResolver(style);

            if (string.IsNullOrEmpty(computerName))
            {
                if (confirm)
                {
                    string input = InvokeConfirmationRequest("This will send the message to all sessions on this computer, including disconnected ones.");
                    if (string.Equals(input, "N", StringComparison.OrdinalIgnoreCase))
                        return null;
                    else
                    {
                        WrappedFunctions unWrapper = new();
                        List<int> result = unWrapper.InvokeMessage(IntPtr.Zero, null, title, message, unStyle, timeout, wait);
                        result.ForEach(x => output.Add((MessageBoxReturn)x));
                    }
                }
                else
                {
                    WrappedFunctions unWrapper = new();
                    List<int> result = unWrapper.InvokeMessage(IntPtr.Zero, null, title, message, unStyle, timeout, wait);
                    result.ForEach(x => output.Add((MessageBoxReturn)x));
                }

            }
            else
            {
                if (confirm)
                {
                    string input = InvokeConfirmationRequest("This will send the message to all sessions on the computer " + computerName + ".");
                    if (string.Equals(input, "N", StringComparison.OrdinalIgnoreCase))
                        return null;
                    else
                    {
                        if (sessionInfo.SessionHandle == null || string.IsNullOrEmpty(sessionInfo.ComputerName))
                        {
                            sessionInfo.SessionHandle = Interop.WTSOpenServerW(computerName);
                            sessionInfo.ComputerName = computerName;
                        }
                        else
                        {
                            if (sessionInfo.ComputerName != computerName)
                            {
                                sessionInfo.SessionHandle = Interop.WTSOpenServerW(computerName);
                                sessionInfo.ComputerName = computerName;
                            }
                        }
                        WrappedFunctions unWrapper = new();
                        List<int> result = unWrapper.InvokeMessage(sessionInfo.SessionHandle.ToIntPtr(), null, title, message, unStyle, timeout, wait);
                        result.ForEach(x => output.Add((MessageBoxReturn)x));
                    }
                }
                else
                {
                    if (sessionInfo.SessionHandle == null || string.IsNullOrEmpty(sessionInfo.ComputerName))
                    {
                        sessionInfo.SessionHandle = Interop.WTSOpenServerW(computerName);
                        sessionInfo.ComputerName = computerName;
                    }
                    else
                    {
                        if (sessionInfo.ComputerName != computerName)
                        {
                            sessionInfo.SessionHandle = Interop.WTSOpenServerW(computerName);
                            sessionInfo.ComputerName = computerName;
                        }
                    }
                    WrappedFunctions unWrapper = new();
                    List<int> result = unWrapper.InvokeMessage(sessionInfo.SessionHandle.ToIntPtr(), null, title, message, unStyle, timeout, wait);
                    result.ForEach(x => output.Add((MessageBoxReturn)x));
                }
            }

            return output;
        }
        public static List<MessageBoxReturn>? InvokeRemoteMessage(string? computerName, int[] sessionId, string title, string message)
        {
            List<MessageBoxReturn> output = new List<MessageBoxReturn>();
            if (string.IsNullOrEmpty(computerName))
            {
                WrappedFunctions unWrapper = new();
                List<int> result = unWrapper.InvokeMessage(IntPtr.Zero, sessionId, title, message, 0, 0, false);
                result.ForEach(x => output.Add((MessageBoxReturn)x));
            }
            else
            {
                if (sessionInfo.SessionHandle == null || string.IsNullOrEmpty(sessionInfo.ComputerName))
                {
                    sessionInfo.SessionHandle = Interop.WTSOpenServerW(computerName);
                    sessionInfo.ComputerName = computerName;
                }
                else
                {
                    if (sessionInfo.ComputerName != computerName)
                    {
                        sessionInfo.SessionHandle = Interop.WTSOpenServerW(computerName);
                        sessionInfo.ComputerName = computerName;
                    }
                }
                WrappedFunctions unWrapper = new();
                List<int> result = unWrapper.InvokeMessage(sessionInfo.SessionHandle.ToIntPtr(), sessionId, title, message, 0, 0, false);
                result.ForEach(x => output.Add((MessageBoxReturn)x));
            }

            return output;
        }
        public static List<MessageBoxReturn>? InvokeRemoteMessage(string? computerName, int[] sessionId, string title, string message, MessageBoxOption[] style, int timeout, bool wait)
        {
            List<MessageBoxReturn> output = new List<MessageBoxReturn>();
            uint unStyle = 0;
            foreach (uint item in style.Select(x => x.Value)) { unStyle = unStyle | item; }

            if (string.IsNullOrEmpty(computerName))
            {
                WrappedFunctions unWrapper = new();
                List<int> result = unWrapper.InvokeMessage(IntPtr.Zero, sessionId, title, message, unStyle, timeout, wait);
                result.ForEach(x => output.Add((MessageBoxReturn)x));
            }
            else
            {
                if (sessionInfo.SessionHandle == null || string.IsNullOrEmpty(sessionInfo.ComputerName))
                {
                    sessionInfo.SessionHandle = Interop.WTSOpenServerW(computerName);
                    sessionInfo.ComputerName = computerName;
                }
                else
                {
                    if (sessionInfo.ComputerName != computerName)
                    {
                        sessionInfo.SessionHandle = Interop.WTSOpenServerW(computerName);
                        sessionInfo.ComputerName = computerName;
                    }
                }
                WrappedFunctions unWrapper = new();
                List<int> result = unWrapper.InvokeMessage(sessionInfo.SessionHandle.ToIntPtr(), sessionId, title, message, unStyle, timeout, wait);
                result.ForEach(x => output.Add((MessageBoxReturn)x));
            }

            return output;
        }
        public static List<MessageBoxReturn>? InvokeRemoteMessage(string? computerName, int[] sessionId, string title, string message, string[] style, int timeout, bool wait)
        {
            List<MessageBoxReturn> output = new List<MessageBoxReturn>();
            uint unStyle = MessageBoxOption.MbOptionsResolver(style);

            if (string.IsNullOrEmpty(computerName))
            {
                WrappedFunctions unWrapper = new();
                List<int> result = unWrapper.InvokeMessage(IntPtr.Zero, sessionId, title, message, unStyle, timeout, wait);
                result.ForEach(x => output.Add((MessageBoxReturn)x));
            }
            else
            {
                if (sessionInfo.SessionHandle == null || string.IsNullOrEmpty(sessionInfo.ComputerName))
                {
                    sessionInfo.SessionHandle = Interop.WTSOpenServerW(computerName);
                    sessionInfo.ComputerName = computerName;
                }
                else
                {
                    if (sessionInfo.ComputerName != computerName)
                    {
                        sessionInfo.SessionHandle = Interop.WTSOpenServerW(computerName);
                        sessionInfo.ComputerName = computerName;
                    }
                }
                WrappedFunctions unWrapper = new();
                List<int> result = unWrapper.InvokeMessage(sessionInfo.SessionHandle.ToIntPtr(), sessionId, title, message, unStyle, timeout, wait);
                result.ForEach(x => output.Add((MessageBoxReturn)x));
            }

            return output;
        }

        public static void DisconnectSession(string? computername, int sessionid, bool wait, bool confirm = true)
        {
            if (string.IsNullOrEmpty(computername))
            {
                if (confirm)
                {
                    string input = InvokeConfirmationRequest("This will disconnect session " + sessionid + " on the current computer.");
                    if (string.Equals(input, "N", StringComparison.OrdinalIgnoreCase)) { return; }
                    else
                    {
                        WrappedFunctions unwrapper = new();
                        unwrapper.DisconnectSession(IntPtr.Zero, sessionid, wait);
                    }
                }
                else
                {
                    WrappedFunctions unwrapper = new();
                    unwrapper.DisconnectSession(IntPtr.Zero, sessionid, wait);
                }
            }
            else
            {
                try { IPHostEntry hostEntry = Dns.GetHostEntry(computername); }
                catch (SocketException ex) { throw ex; }

                if (sessionInfo.SessionHandle == null || string.IsNullOrEmpty(sessionInfo.ComputerName))
                {
                    sessionInfo.SessionHandle = null;
                    sessionInfo.SessionHandle = Interop.WTSOpenServerW(computername);
                    if (null == sessionInfo.SessionHandle || sessionInfo.SessionHandle.ToIntPtr() == IntPtr.Zero)
                        throw new SystemException(GetFormattedWin32Error());

                    sessionInfo.ComputerName = computername;
                }
                else
                {
                    if (sessionInfo.ComputerName != computername)
                    {
                        sessionInfo.SessionHandle = null;
                        sessionInfo.SessionHandle = Interop.WTSOpenServerW(computername);
                        if (null == sessionInfo.SessionHandle || sessionInfo.SessionHandle.ToIntPtr() == IntPtr.Zero)
                            throw new SystemException(GetFormattedWin32Error());

                        sessionInfo.ComputerName = computername;
                    }
                }
                if (confirm)
                {
                    string input = InvokeConfirmationRequest("This will disconnect session " + sessionid + " on computer " + computername +".");
                    if (string.Equals(input, "N", StringComparison.OrdinalIgnoreCase)) { return; }
                    else
                    {
                        WrappedFunctions unwrapper = new();
                        unwrapper.DisconnectSession(sessionInfo.SessionHandle.ToIntPtr(), sessionid, wait);
                    }
                }
                else
                {
                    WrappedFunctions unwrapper = new();
                    unwrapper.DisconnectSession(sessionInfo.SessionHandle.ToIntPtr(), sessionid, wait);
                }
            }
        }
        public static void DisconnectSession(bool confirm = true)
        {
            if (confirm)
            {
                string input = InvokeConfirmationRequest("This will disconnect the current session on this computer.");
                if (string.Equals(input, "N", StringComparison.OrdinalIgnoreCase)) { return; }
                else
                {
                    WrappedFunctions unwrapper = new();
                    unwrapper.DisconnectSession(IntPtr.Zero, false);
                }
            }
            else
            {
                WrappedFunctions unwrapper = new();
                unwrapper.DisconnectSession(IntPtr.Zero, false);
            }
        }

        public static RpcEndpoint[] MapRpcEndpoints()
        {
            WrappedFunctions unWrapper = new();
            return unWrapper.MapRpcEndpoints();
        }

        public static void SendClick()
        {
            //Call the imported function with the cursor's current position
            uint X = (uint)Cursor.Position.X;
            uint Y = (uint)Cursor.Position.Y;
            Interop.mouse_event(
                ((uint)Interop.MouseEvent.MOUSEEVENTF_LEFTDOWN) | ((uint)Interop.MouseEvent.MOUSEEVENTF_LEFTUP),
                X,
                Y,
                0,
                0
            );
        }
        public static ResourceMessageTable[] GetResourceMessageTable(string libPath)
        {
            WrappedFunctions unWrapper = new();
            return unWrapper.GetResourceMessageTable(libPath);
        }

        public static string GetFormattedError(int errorCode)
        {
            WrappedFunctions unWrapper = new();
            return unWrapper.GetFormatedError(errorCode);
        }
        public static string GetFormattedWin32Error()
        {
            WrappedFunctions unWrapper = new();
            return unWrapper.GetFormatedWin32Error();
        }
        public static string GetFormattedWSError()
        {
            WrappedFunctions unWrapper = new();
            return unWrapper.GetFormatedWSError();
        }

        public static FileHandle[]? GetProcessFileHandle(string[] fileName)
        {
            List<string> validPaths = new List<string>();
            for (int i = 0; i < fileName.Length; i++)
            {
                string path = fileName[i];
                if (File.Exists(path.Trim()) || Directory.Exists(path.Trim()))
                    validPaths.Add(path);
                else
                    WriteWarning("Object '" + path + "' not found.");
            }

            if (validPaths.Count == 0)
                throw new ItemNotFoundException();

            string[] validsent = new string[validPaths.Count];
            for (int i = 0; i < validPaths.Count; i++)
                validsent[i] = validPaths[i].Trim();

            WrappedFunctions unWrapper = new();
            return unWrapper.GetProcessFileHandle(validsent);
        }

        public static PSObject GetMsiProperties(string fileName)
        {
            WrappedFunctions unWrapper = new();
            return unWrapper.GetMsiProperties(fileName);
        }

        internal static void WriteWarning(string warning, bool newLine = true, bool prefix = true)
        {
            ConsoleColor currentCollor = Console.ForegroundColor;
            Console.ForegroundColor = ConsoleColor.Yellow;

            if (newLine)
            {
                if (prefix) { Console.WriteLine("WARNING: " + warning); }
                else { Console.WriteLine(warning); }
            }
            else
            {
                if (prefix) { Console.Write("WARNING: " + warning); }
                else { Console.Write(warning); }
            }

            Console.ForegroundColor = currentCollor;
        }
        internal static string InvokeConfirmationRequest(string message)
        {
            Console.WriteLine(message + "\nDo you want to continue?");
            WriteWarning("[Y] Yes ", false, false);
            Console.Write("[N] No(default is \"Y\"): ");
            string input = Console.ReadLine();
            if (!string.IsNullOrEmpty(input) && !string.Equals(input, "Y", StringComparison.OrdinalIgnoreCase) && !string.Equals(input, "N", StringComparison.OrdinalIgnoreCase))
            {
                Console.WriteLine("Invalid option '" + input + "'.\n");
                input = InvokeConfirmationRequest(message);
            }
            return input;
        }
    }
}