﻿using System.Management.Automation;
using WindowsUtils.Wrappers;

namespace WindowsUtils.Commands
{
    /// <summary>
    /// <para type="synopsis">Returns the last 'Win32' system error message.</para>
    /// <para type="description">This Cmdlet gets the last error thrown by the system, and returns the message string.</para>
    /// </summary>
    [Cmdlet(VerbsCommon.Get, "LastWin32Error")]
    [OutputType(typeof(string))]
    public class GetLastWin32ErrorCommand : Cmdlet
    {
        protected override void ProcessRecord()
            => WriteObject(UtilitiesWrapper.GetLastWin32Error());
    }
}