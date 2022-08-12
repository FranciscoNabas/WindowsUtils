﻿using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using System.Security;
using System.Runtime.ConstrainedExecution;
using System.ComponentModel;
using Wrapper;

namespace WindowsUtils
{
    internal class SystemSafeHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        private SystemSafeHandle() : base(true) { }

        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        override protected bool ReleaseHandle() { return Utilities.CloseHandle(handle); }
    }

    public enum SessionState : uint
    {
        Active = 0,
        Connected = 1,
        ConnectQuery = 2,
        Shadow = 3,
        Disconnected = 4,
        Idle = 5,
        Listen = 6,
        Reset = 7,
        Down = 8,
        Init = 9
    }
    public class ComputerSessionOutput
    {
        public string UserName { get; internal set; }
        public string SessionName { get; internal set; }
        public SessionState SessionState { get; internal set; }

    }
    public class Utilities
    {
        [DllImport("kernel32", SetLastError = true)]
        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        internal extern static bool CloseHandle(IntPtr handle);

        [DllImport("Wtsapi32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        internal extern static SystemSafeHandle WTSOpenServerW(string pServerName);

        [StructLayout(LayoutKind.Sequential)]
        internal struct WTS_SESSION_INFO
        {
            internal Int32 SessionID;

            [MarshalAs(UnmanagedType.LPStr)]
            internal String pWinStationName;

            internal WTS_CONNECTSTATE_CLASS State;
        }

        private enum MouseEvent : uint
        {
            MOUSEEVENTF_LEFTDOWN = 2,
            MOUSEEVENTF_LEFTUP = 4,
            MOUSEEVENTF_RIGHTDOWN = 8,
            MOUSEEVENTF_RIGHTUP = 16
        }

        internal enum WTS_CONNECTSTATE_CLASS
        {
            WTSActive,
            WTSConnected,
            WTSConnectQuery,
            WTSShadow,
            WTSDisconnected,
            WTSIdle,
            WTSListen,
            WTSReset,
            WTSDown,
            WTSInit
        }
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern IntPtr GetModuleHandle(
            string lpModuleName
        );
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern IntPtr LoadLibrary(
            string lpLibFileName
        );
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern int FreeLibrary(
            IntPtr hLibModule
        );
        [DllImport("wtsapi32.dll", SetLastError = true)]
        private static extern int WTSEnumerateSessions (
            IntPtr hServer,
            int Reserved,
            int Version,
            ref IntPtr ppSessionInfo,
            ref int pCount
        );
        [DllImport("wtsapi32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern bool WTSSendMessage (
            IntPtr hServer,
            [MarshalAs(UnmanagedType.I4)] int SessionId,
            string pTitle,
            [MarshalAs(UnmanagedType.U4)] int TitleLength,
            string pMessage,
            [MarshalAs(UnmanagedType.U4)] int MessageLength,
            [MarshalAs(UnmanagedType.U4)] int Style,
            [MarshalAs(UnmanagedType.U4)] int Timeout,
            [MarshalAs(UnmanagedType.U4)] out int pResponse,
            bool bWait
        );

        [DllImport("user32.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        private static extern void mouse_event (
            uint dwFlags,
            uint dx,
            uint dy,
            uint cButtons,
            uint dwExtraInfo
        );

        public static List<Managed.wSessionEnumOutput> GetComputerSession(string computerName, bool onlyActive, bool excludeSystemSessions)
        {
            Managed unWrapper = new Managed();
            return unWrapper.GetEnumeratedSession(computerName, onlyActive, excludeSystemSessions);
        }

        public static List<Managed.wSessionEnumOutput> GetComputerSession(string computerName)
        {
            Managed unWrapper = new Managed();
            return unWrapper.GetEnumeratedSession(computerName, false, false);
        }

        public static List<Managed.wSessionEnumOutput> GetComputerSession(bool onlyActive, bool excludeSystemSessions)
        {
            Managed unWrapper = new Managed();
            return unWrapper.GetEnumeratedSession(null, onlyActive, excludeSystemSessions);
        }

        public static List<Managed.wSessionEnumOutput> GetComputerSession()
        {
            Managed unWrapper = new Managed();
            return unWrapper.GetEnumeratedSession(null, false, false);
        }

        public static void SendClick()
        {
            //Call the imported function with the cursor's current position
            uint X = (uint)Cursor.Position.X;
            uint Y = (uint)Cursor.Position.Y;
            mouse_event(
                ((uint)MouseEvent.MOUSEEVENTF_LEFTDOWN) | ((uint)MouseEvent.MOUSEEVENTF_LEFTUP),
                X,
                Y,
                0,
                0
            );
        }

        public static List<int> InvokeWarning(string message, string title, int style, int timeout, bool bWait)
        {
            try
            {
                IntPtr sessionInfo = new IntPtr();
                int sessionCount = new int();
                WTSEnumerateSessions(IntPtr.Zero, 0, 1, ref sessionInfo, ref sessionCount);
                int dataSize = Marshal.SizeOf(typeof(WTS_SESSION_INFO));

                List<int> allSResponse = new List<int>();
                for (int i = 0; i < sessionCount; i++)
                {
                    int response;
                    object alloc = (Marshal.PtrToStructure(sessionInfo + (dataSize * i), typeof(WTS_SESSION_INFO)));
                    if (alloc != null)
                    {
                        WTS_SESSION_INFO unSessionInfo = ((WTS_SESSION_INFO)alloc);
                        WTSSendMessage(IntPtr.Zero, unSessionInfo.SessionID, title, (title.Length * 2), message, (message.Length * 2), style, timeout, out response, bWait);
                        allSResponse.Add(response);
                    }
                }

                return allSResponse;
            }
            catch (Exception ex) { throw ex; }
        }
    }
}