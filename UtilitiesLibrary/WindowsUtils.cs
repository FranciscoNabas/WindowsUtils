using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Management.Automation;
using UtilitiesLibrary.Abstraction;
using UtilitiesLibrary.TerminalServices;
using Wrapper;

#nullable enable
namespace UtilitiesLibrary
{
    public class Utilities
    {
        private static Session sessionInfo = new();

        public static List<Managed.SessionEnumOutput> GetComputerSession(string computerName, bool onlyActive, bool IncludeSystemSession)
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
            Managed unWrapper = new Managed();
            List<Managed.SessionEnumOutput> result = unWrapper.GetEnumeratedSession(IntPtr.Zero, onlyActive, IncludeSystemSession);
            foreach (Managed.SessionEnumOutput session in result)
            {
                if (session.IdleTime == TimeSpan.Zero)
                    session.IdleTime = null;
                if (session.LogonDate == DateTime.FromFileTime(0))
                    session.LogonDate = null;
            }
            return result;
        }
        public static List<Managed.SessionEnumOutput> GetComputerSession(string computerName)
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
            Managed unWrapper = new Managed();
            List<Managed.SessionEnumOutput> result = unWrapper.GetEnumeratedSession(IntPtr.Zero, false, false);
            foreach (Managed.SessionEnumOutput session in result)
            {
                if (session.IdleTime == TimeSpan.Zero)
                    session.IdleTime = null;
                if (session.LogonDate == DateTime.FromFileTime(0))
                    session.LogonDate = null;
            }
            return result;
        }
        public static List<Managed.SessionEnumOutput> GetComputerSession(bool onlyActive, bool IncludeSystemSession)
        {
            Managed unWrapper = new Managed();
            if (IncludeSystemSession == false)
                return unWrapper.GetEnumeratedSession(IntPtr.Zero, onlyActive, IncludeSystemSession);
            
            List<Managed.SessionEnumOutput> result = unWrapper.GetEnumeratedSession(IntPtr.Zero, onlyActive, IncludeSystemSession);
            foreach (Managed.SessionEnumOutput session in result)
            {
                if (string.IsNullOrEmpty(session.UserName))
                {
                    session.LogonDate = null;
                    session.IdleTime = null;
                }
            }
            return result;
        }
        public static List<Managed.SessionEnumOutput> GetComputerSession()
        {
            Managed unWrapper = new Managed();
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
                    Managed unWrapper = new Managed();
                    List<int> result = unWrapper.InvokeMessage(IntPtr.Zero, null, title, message, 0, 0, false);
                    result.ForEach(x => output.Add((MessageBoxReturn)x));
                }
            }
            else
            {
                Managed unWrapper = new Managed();
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
                        Managed unWrapper = new Managed();
                        List<int> result = unWrapper.InvokeMessage(IntPtr.Zero, null, title, message, 0, 0, false);
                        result.ForEach(x => output.Add((MessageBoxReturn)x));
                    }
                }
                else
                {
                    Managed unWrapper = new Managed();
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
                    Managed unWrapper = new Managed();
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
                        Managed unWrapper = new Managed();
                        List<int> result = unWrapper.InvokeMessage(IntPtr.Zero, null, title, message, unStyle, timeout, wait);
                        result.ForEach(x => output.Add((MessageBoxReturn)x));
                    }
                }
                else
                {
                    Managed unWrapper = new Managed();
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
                        Managed unWrapper = new Managed();
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
                    Managed unWrapper = new Managed();
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
                Managed unWrapper = new Managed();
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
                Managed unWrapper = new Managed();
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
                Managed unWrapper = new Managed();
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
                Managed unWrapper = new Managed();
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
                Managed unWrapper = new Managed();
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
                Managed unWrapper = new Managed();
                List<int> result = unWrapper.InvokeMessage(sessionInfo.SessionHandle.ToIntPtr(), sessionId, title, message, unStyle, timeout, wait);
                result.ForEach(x => output.Add((MessageBoxReturn)x));
            }

            return output;
        }

        public static List<Managed.RpcMapperOutput> MapRpcEndpoints()
        {
            Managed unWrapper = new Managed();
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
        public static List<Managed.MessageDumpOutput> GetResourceMessageTable(string libPath)
        {
            Managed unWrapper = new Managed();
            List<Managed.MessageDumpOutput> output = unWrapper.GetResourceMessageTable(libPath);
            return output;
        }

        public static string GetFormattedError(int errorCode)
        {
            Managed unWrapper = new Managed();
            return unWrapper.GetFormatedError(errorCode);
        }
        public static string GetFormattedWin32Error()
        {
            Managed unWrapper = new Managed();
            return unWrapper.GetFormatedWin32Error();
        }
        public static string GetFormattedWSError()
        {
            Managed unWrapper = new Managed();
            return unWrapper.GetFormatedWSError();
        }

        public static List<Managed.FileHandleOutput>? GetProcessFileHandle(string fileName)
        {
            Managed unWrapper = new Managed();
            List<Managed.FileHandleOutput> output = unWrapper.GetProcessFileHandle(fileName);
            return output;
        }

        public static PSObject GetMsiProperties(string fileName)
        {
            Managed unWrapper = new Managed();
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