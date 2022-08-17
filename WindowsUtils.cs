using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using System.Runtime.ConstrainedExecution;
using Wrapper;

#nullable enable
namespace WindowsUtils
{
    public class Utilities
    {
        private static TerminalServices.Session sessionInfo = new TerminalServices.Session();

        public static List<Managed.wSessionEnumOutput> GetComputerSession(string computerName, bool onlyActive, bool excludeSystemSessions)
        {
            if (sessionInfo.SessionHandle == null || string.IsNullOrEmpty(sessionInfo.ComputerName)) {
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
            return unWrapper.GetEnumeratedSession(sessionInfo.SessionHandle.ToIntPtr(), onlyActive, excludeSystemSessions);
        }

        public static List<Managed.wSessionEnumOutput> GetComputerSession(string computerName)
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
            return unWrapper.GetEnumeratedSession(sessionInfo.SessionHandle.ToIntPtr(), false, false);
        }

        public static List<Managed.wSessionEnumOutput> GetComputerSession(bool onlyActive, bool excludeSystemSessions)
        {
            Managed unWrapper = new Managed();
            return unWrapper.GetEnumeratedSession(IntPtr.Zero, onlyActive, excludeSystemSessions);
        }

        public static List<Managed.wSessionEnumOutput> GetComputerSession()
        {
            Managed unWrapper = new Managed();
            return unWrapper.GetEnumeratedSession(IntPtr.Zero, false, false);
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

        public static List<int>? InvokeMessage(string? computerName, string title, string message)
        {
            List<int> result = new List<int>();
            if (computerName is null)
            {
                string input = InvokeConfirmationRequest("This will send the message to all sessions on this computer, including disconnected ones.");
                if (string.Equals(input, "N", StringComparison.OrdinalIgnoreCase)) { return null; }
                else
                {
                    Managed unWrapper = new Managed();
                    result = unWrapper.InvokeMessage(IntPtr.Zero, null, title, message, TerminalServices.MessageBoxButton.MB_OK.Value, 0, false);
                }
            }
            else
            {
                string input = InvokeConfirmationRequest("This will send the message to all sessions on the computer " + computerName + ".");
                if (string.Equals(input, "N", StringComparison.OrdinalIgnoreCase)) { return null; }
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
                    result = unWrapper.InvokeMessage(sessionInfo.SessionHandle.ToIntPtr(), null, title, message, TerminalServices.MessageBoxButton.MB_OK.Value, 0, false);
                }
            }
            
            return result;
        }

        public static void WriteWarning(string warning, bool newLine = true, bool prefix = true)
        {
            ConsoleColor currentCollor = Console.ForegroundColor;
            Console.ForegroundColor = ConsoleColor.Yellow;
            
            if (newLine)
            {
                if (prefix) { Console.WriteLine("WARNING: " + warning); }
                else {  Console.WriteLine(warning); }
            }
            else
            {
                if (prefix) { Console.Write("WARNING: " + warning); }
                else { Console.Write(warning); }
            }
            
            Console.ForegroundColor = currentCollor;
        }

        public static string InvokeConfirmationRequest(string message)
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

        public static void Test()
        {
            InvokeConfirmationRequest("This will send the message to all sessions on this computer, including disconnected ones.");
        }
    }
}