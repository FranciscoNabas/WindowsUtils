﻿//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version:4.0.30319.42000
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

namespace WindowsUtils.Properties {
    using System;
    
    
    /// <summary>
    ///   A strongly-typed resource class, for looking up localized strings, etc.
    /// </summary>
    // This class was auto-generated by the StronglyTypedResourceBuilder
    // class via a tool like ResGen or Visual Studio.
    // To add or remove a member, edit your .ResX file then rerun ResGen
    // with the /str option, or rebuild your VS project.
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("System.Resources.Tools.StronglyTypedResourceBuilder", "17.0.0.0")]
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
    [global::System.Runtime.CompilerServices.CompilerGeneratedAttribute()]
    internal class Resources {
        
        private static global::System.Resources.ResourceManager resourceMan;
        
        private static global::System.Globalization.CultureInfo resourceCulture;
        
        [global::System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
        internal Resources() {
        }
        
        /// <summary>
        ///   Returns the cached ResourceManager instance used by this class.
        /// </summary>
        [global::System.ComponentModel.EditorBrowsableAttribute(global::System.ComponentModel.EditorBrowsableState.Advanced)]
        internal static global::System.Resources.ResourceManager ResourceManager {
            get {
                if (object.ReferenceEquals(resourceMan, null)) {
                    global::System.Resources.ResourceManager temp = new global::System.Resources.ResourceManager("WindowsUtils.Properties.Resources", typeof(Resources).Assembly);
                    resourceMan = temp;
                }
                return resourceMan;
            }
        }
        
        /// <summary>
        ///   Overrides the current thread's CurrentUICulture property for all
        ///   resource lookups using this strongly typed resource class.
        /// </summary>
        [global::System.ComponentModel.EditorBrowsableAttribute(global::System.ComponentModel.EditorBrowsableState.Advanced)]
        internal static global::System.Globalization.CultureInfo Culture {
            get {
                return resourceCulture;
            }
            set {
                resourceCulture = value;
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to .
        /// </summary>
        internal static string DisconnectWMessageCleanRegPS1 {
            get {
                return ResourceManager.GetString("DisconnectWMessageCleanRegPS1", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to &lt;?xml version=&quot;1.0&quot; encoding=&quot;UTF-16&quot;?&gt;
        ///&lt;Task version=&quot;1.2&quot; xmlns=&quot;http://schemas.microsoft.com/windows/2004/02/mit/task&quot;&gt;
        ///    &lt;RegistrationInfo /&gt;
        ///    &lt;Principals&gt;
        ///        &lt;Principal&gt;
        ///            &lt;UserId&gt;S-1-5-18&lt;/UserId&gt;
        ///            &lt;RunLevel&gt;HighestAvailable&lt;/RunLevel&gt;
        ///        &lt;/Principal&gt;
        ///    &lt;/Principals&gt;
        ///    &lt;Settings&gt;
        ///        &lt;AllowStartOnDemand&gt;false&lt;/AllowStartOnDemand&gt;
        ///        &lt;DisallowStartIfOnBatteries&gt;false&lt;/DisallowStartIfOnBatteries&gt;
        ///        &lt;StopIfGoingOnBatteries&gt;true&lt;/StopIfGoi [rest of string was truncated]&quot;;.
        /// </summary>
        internal static string DisconnectWMessageCleanRegTask {
            get {
                return ResourceManager.GetString("DisconnectWMessageCleanRegTask", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Set-ExecutionPolicy Unrestricted -Scope &apos;Process&apos;
        ///Add-Type -MemberDefinition @&apos;
        ///[DllImport(&quot;Wtsapi32.dll&quot;, CharSet = CharSet.Unicode, SetLastError = true)]
        ///public static extern bool WTSSendMessageW(
        ///    IntPtr hserver
        ///    ,uint sessionid
        ///    ,string title
        ///    ,uint titlelength
        ///    ,string message
        ///    ,uint messagelength
        ///    ,uint style
        ///    ,uint timeout
        ///    ,out uint response
        ///    ,bool wait
        ///);
        ///[DllImport(&quot;Wtsapi32.dll&quot;, CharSet = CharSet.Unicode, SetLastError = true)]
        ///public static extern bo [rest of string was truncated]&quot;;.
        /// </summary>
        internal static string DisconnectWMessagePwshP1 {
            get {
                return ResourceManager.GetString("DisconnectWMessagePwshP1", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to $errorcanceledbyuser = 1223
        ///$start = Get-Date
        ///[System.UInt32]$response = 0
        ///if ($allowcancel) {
        ///    [CiscoWonders.Interop]::WTSSendMessageW(0, $sessionid, &apos;Disconnecting session.&apos;, 44, $Message, $Message.Length * 2, 4145, ([System.Int32]$Timeout * 60), [ref]$response, $true)
        ///    switch ($response.Response) {
        ///        1 {
        ///            $elapsed = [math]::Ceiling(((Get-Date) - $start).TotalSeconds)
        ///            $remaining = [math]::Ceiling(([System.Int32]$Timeout * 60) - $elapsed)
        ///
        ///            Start-Slee [rest of string was truncated]&quot;;.
        /// </summary>
        internal static string DisconnectWMessagePwshP2 {
            get {
                return ResourceManager.GetString("DisconnectWMessagePwshP2", resourceCulture);
            }
        }
    }
}