using System.Security;
using System.Security.Principal;
using System.Security.AccessControl;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using WindowsUtils.Engine;

namespace WindowsUtils.Interop
{
    #region Enumerations
    internal enum PRIVILEGE_ATTRIBUTE : uint
    {
        SE_PRIVILEGE_NONE = 0x00000000,
        SE_PRIVILEGE_ENABLED_BY_DEFAULT = 0x00000001,
        SE_PRIVILEGE_ENABLED = 0x00000002,
        SE_PRIVILEGE_REMOVED = 0X00000004,
        SE_PRIVILEGE_USED_FOR_ACCESS = 0x80000000,
        SE_PRIVILEGE_VALID_ATTRIBUTES = SE_PRIVILEGE_ENABLED_BY_DEFAULT |
                                        SE_PRIVILEGE_ENABLED |
                                        SE_PRIVILEGE_REMOVED |
                                        SE_PRIVILEGE_USED_FOR_ACCESS
    }

    internal enum TOKEN_ACCESS_RIGHT : uint
    {
        TOKEN_ASSIGN_PRIMARY = 0x0001,
        TOKEN_DUPLICATE = 0x0002,
        TOKEN_IMPERSONATE = 0x0004,
        TOKEN_QUERY = 0x0008,
        TOKEN_QUERY_SOURCE = 0x0010,
        TOKEN_ADJUST_PRIVILEGES = 0x0020,
        TOKEN_ADJUST_GROUPS = 0x0040,
        TOKEN_ADJUST_DEFAULT = 0x0080,
        TOKEN_ADJUST_SESSIONID = 0x0100,
        TOKEN_ALL_ACCESS_P = 0x000F0000 |
                             TOKEN_ASSIGN_PRIMARY |
                             TOKEN_DUPLICATE |
                             TOKEN_IMPERSONATE |
                             TOKEN_QUERY |
                             TOKEN_QUERY_SOURCE |
                             TOKEN_ADJUST_PRIVILEGES |
                             TOKEN_ADJUST_GROUPS |
                             TOKEN_ADJUST_DEFAULT,

        TOKEN_ALL_ACCESS = TOKEN_ALL_ACCESS_P | TOKEN_ADJUST_SESSIONID,
        TOKEN_READ = 0x00020000 | TOKEN_QUERY,
        TOKEN_WRITE = 0x00020000 |
                      TOKEN_ADJUST_PRIVILEGES |
                      TOKEN_ADJUST_GROUPS |
                      TOKEN_ADJUST_DEFAULT,

        TOKEN_EXECUTE = 0x00020000,
        TOKEN_TRUST_CONSTRAINT_MASK = 0x00020000 |
                                      TOKEN_QUERY |
                                      TOKEN_QUERY_SOURCE,

        TOKEN_TRUST_ALLOWED_MASK = TOKEN_TRUST_CONSTRAINT_MASK |
                                   TOKEN_DUPLICATE |
                                   TOKEN_IMPERSONATE,

        TOKEN_ACCESS_PSEUDO_HANDLE_WIN8 = TOKEN_QUERY | TOKEN_QUERY_SOURCE,
        TOKEN_ACCESS_PSEUDO_HANDLE = TOKEN_ACCESS_PSEUDO_HANDLE_WIN8
    }

    internal enum SECURITY_IMPERSONATION_LEVEL : uint
    {
        SecurityAnonymous,
        SecurityIdentification,
        SecurityImpersonation,
        SecurityDelegation
    }

    internal enum TOKEN_TYPE : uint
    {
        TokenPrimary = 1,
        TokenImpersonation
    }
    #endregion

    #region Structures
    [StructLayout(LayoutKind.Sequential)]
    internal struct LUID
    {
        internal uint LowPart;
        internal long HighPart;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct LUID_AND_ATTRIBUTES
    {
        internal LUID Luid;
        internal PRIVILEGE_ATTRIBUTE Attributes;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct TOKEN_PRIVILEGE
    {
        internal uint PrivilegeCount;
        internal LUID_AND_ATTRIBUTES Privilege;
    }
    #endregion

    internal partial class NativeFunctions
    {
        [DllImport("Advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode, EntryPoint = "LookupPrivilegeValueW")]
        [SecurityCritical]
        internal static extern bool LookupPrivilegeValue(
            string lpSystemName,
            string lpName,
            ref LUID lpLuid
        );

        [DllImport("Advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode, EntryPoint = "LookupPrivilegeValueW")]
        [SecurityCritical]
        internal static extern bool LookupPrivilegeValue(
            IntPtr lpSystemName,
            string lpName,
            ref LUID lpLuid
        );

        [DllImport("Advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        [SecurityCritical]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool AdjustTokenPrivileges(
            [In] SafeAccessTokenHandle TokenHandle,
            [In] bool DisableAllPrivileges,
            [In] ref TOKEN_PRIVILEGE NewState,
            [In] uint BufferLength,
            [In][Out] ref TOKEN_PRIVILEGE PreviousState,
            [In][Out] ref uint ReturnLength
        );

        [DllImport("Advapi32.dll", SetLastError = true)]
        [SecurityCritical]
        internal static extern bool OpenProcessToken(
            SafeSystemHandle ProcessHandle,
            TOKEN_ACCESS_RIGHT DesiredAccess,
            out SafeAccessTokenHandle pHandle
        );

        [DllImport("Advapi32.dll", SetLastError = true)]
        [SecurityCritical]
        internal static extern int OpenThreadToken(
            SafeSystemHandle ThreadHandle,
            TOKEN_ACCESS_RIGHT DesiredAccess,
            bool OpenAsSelf,
            out SafeAccessTokenHandle pHandle
        );

        [DllImport("Advapi32.dll", SetLastError = true)]
        [SecurityCritical]
        internal static extern bool DuplicateTokenEx(
            SafeAccessTokenHandle hExistingToken,
            TOKEN_ACCESS_RIGHT dwDesiredAccess,
            IntPtr lpTokenAttributes,
            SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
            TOKEN_TYPE TokenType,
            ref SafeAccessTokenHandle phNewHandle
        );

        [DllImport("Advapi32.dll", SetLastError = true)]
        [SecurityCritical]
        internal static extern int SetThreadToken(
            SafeSystemHandle hThread,
            SafeAccessTokenHandle Token
        );

        [DllImport("Advapi32.dll", SetLastError = true)]
        [SecurityCritical]
        internal static extern bool RevertToSelf();
    }
}

namespace WindowsUtils.AccessControl
{
    public sealed class ServiceSecurity : ObjectSecurity
    {
        private readonly RawSecurityDescriptor _rawSecDescriptor;
        private readonly CommonSecurityDescriptor _securityDescriptor;
        private readonly string _name;

        public string Name { get { return _name; } }
        public string AccessToString
        {
            get
            {
                bool first = true;
                string output = string.Empty;
                if (Access is null)
                    return string.Empty;
                
                foreach (ServiceAccessRule ace in Access)
                {
                    if (first)
                        first = false;
                    else
                        output += "\n";

                    output += $"{ace.IdentityReference} ";
                    output += $"{ace.AccessControlType} ";
                    output += ace.ServiceRights.ToString();
                }

                return output;
            }
        }
        public string AuditToString
        {
            get
            {
                bool first = true;
                string output = string.Empty;
                if (Access is null)
                    return string.Empty;
                
                foreach (ServiceAuditRule ace in Access)
                {
                    if (first)
                        first = false;
                    else
                        output += "\n";

                    output += $"{ace.IdentityReference} ";
                    output += $"{ace.AuditFlags} ";
                    output += ace.ServiceRights.ToString();
                }

                return output;
            }
        }
        public string Sddl => GetSecurityDescriptorSddlForm(AccessControlSections.All);
        public AuthorizationRuleCollection Access => GetAccessRules(true, true, typeof(NTAccount));

        public override Type AccessRightType => typeof(ServiceRights);
        public override Type AccessRuleType => typeof(ServiceAccessRule);
        public override Type AuditRuleType => typeof(ServiceAuditRule);
        
        internal ServiceSecurity(byte[] descriptorBytes, string name)
            : base(new(false, false, new RawSecurityDescriptor(descriptorBytes, 0)))
        {
            _name = name;
            _rawSecDescriptor = new RawSecurityDescriptor(descriptorBytes, 0);
            _securityDescriptor = new(false, false, _rawSecDescriptor);
        }
        internal ServiceSecurity(RawSecurityDescriptor rawSecDescriptor, string name)
            : base(new(false, false, rawSecDescriptor))
        {
            _name = name;
            _rawSecDescriptor = rawSecDescriptor;
            _securityDescriptor = new(false, false, rawSecDescriptor);
        }

        public sealed override AccessRule AccessRuleFactory(IdentityReference identityReference, int accessMask, bool isInherited, InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AccessControlType type)
            => new ServiceAccessRule(identityReference, accessMask, isInherited, inheritanceFlags, propagationFlags, type);
        public sealed override AuditRule AuditRuleFactory(IdentityReference identityReference, int accessMask, bool isInherited, InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AuditFlags flags)
            => new ServiceAuditRule(identityReference, accessMask, isInherited, inheritanceFlags, propagationFlags, flags);

        public void AddAccessRule(ServiceAccessRule rule)
        {
            if (rule == null)
            {
                throw new ArgumentNullException("rule");
            }
            WriteLock();
            try
            {
                ModifyAccess(AccessControlModification.Add, rule, out var _);
            }
            finally
            {
                WriteUnlock();
            }
        }
        public void SetAccessRule(ServiceAccessRule rule)
        {
            if (rule == null)
            {
                throw new ArgumentNullException("rule");
            }
            WriteLock();
            try
            {
                ModifyAccess(AccessControlModification.Set, rule, out var _);
            }
            finally
            {
                WriteUnlock();
            }
        }
        public void ResetAccessRule(ServiceAccessRule rule)
        {
            if (rule == null)
            {
                throw new ArgumentNullException("rule");
            }
            WriteLock();
            try
            {
                ModifyAccess(AccessControlModification.Reset, rule, out var _);
            }
            finally
            {
                WriteUnlock();
            }
        }
        public bool RemoveAccessRule(ServiceAccessRule rule)
        {
            if (rule == null)
            {
                throw new ArgumentNullException("rule");
            }
            WriteLock();
            try
            {
                if (_securityDescriptor == null)
                {
                    return true;
                }
                return ModifyAccess(AccessControlModification.Remove, rule, out bool _);
            }
            finally
            {
                WriteUnlock();
            }
        }
        public void RemoveAccessRuleAll(ServiceAccessRule rule)
        {
            if (rule == null)
            {
                throw new ArgumentNullException("rule");
            }
            WriteLock();
            try
            {
                if (_securityDescriptor != null)
                {
                    ModifyAccess(AccessControlModification.RemoveAll, rule, out var _);
                }
            }
            finally
            {
                WriteUnlock();
            }
        }
        public void RemoveAccessRuleSpecific(ServiceAccessRule rule)
        {
            if (rule == null)
            {
                throw new ArgumentNullException("rule");
            }
            WriteLock();
            try
            {
                if (_securityDescriptor != null)
                {
                    ModifyAccess(AccessControlModification.RemoveSpecific, rule, out var _);
                }
            }
            finally
            {
                WriteUnlock();
            }
        }
        public void AddAuditRule(ServiceAuditRule rule)
        {
            if (rule == null)
            {
                throw new ArgumentNullException("rule");
            }
            WriteLock();
            try
            {
                ModifyAudit(AccessControlModification.Add, rule, out var _);
            }
            finally
            {
                WriteUnlock();
            }
        }
        public void SetAuditRule(ServiceAuditRule rule)
        {
            if (rule == null)
            {
                throw new ArgumentNullException("rule");
            }
            WriteLock();
            try
            {
                ModifyAudit(AccessControlModification.Set, rule, out var _);
            }
            finally
            {
                WriteUnlock();
            }
        }
        public bool RemoveAuditRule(ServiceAuditRule rule)
        {
            if (rule == null)
            {
                throw new ArgumentNullException("rule");
            }
            WriteLock();
            try
            {
                bool modified;
                return ModifyAudit(AccessControlModification.Remove, rule, out modified);
            }
            finally
            {
                WriteUnlock();
            }
        }
        public void RemoveAuditRuleAll(ServiceAuditRule rule)
        {
            if (rule == null)
            {
                throw new ArgumentNullException("rule");
            }
            WriteLock();
            try
            {
                ModifyAudit(AccessControlModification.RemoveAll, rule, out var _);
            }
            finally
            {
                WriteUnlock();
            }
        }
        public void RemoveAuditRuleSpecific(ServiceAuditRule rule)
        {
            if (rule == null)
            {
                throw new ArgumentNullException("rule");
            }
            WriteLock();
            try
            {
                ModifyAudit(AccessControlModification.RemoveSpecific, rule, out var _);
            }
            finally
            {
                WriteUnlock();
            }
        }
        public AuthorizationRuleCollection GetAccessRules(bool includeExplicit, bool includeInherited, Type type) => GetRules(access: true, includeExplicit, includeInherited, type);
        public AuthorizationRuleCollection GetAuditRules(bool includeExplicit, bool includeInherited, Type type) => GetRules(access: false, includeExplicit, includeInherited, type);

        protected override bool ModifyAudit(AccessControlModification modification, AuditRule rule, out bool modified) => ModifyAudit(modification, (ServiceAuditRule)rule, out modified);
        protected override bool ModifyAccess(AccessControlModification modification, AccessRule rule, out bool modified) => ModifyAccess(modification, (ServiceAccessRule)rule, out modified);
        private bool ModifyAudit(AccessControlModification modification, ServiceAuditRule rule, out bool modified)
        {
            if (rule == null)
            {
                throw new ArgumentNullException("rule");
            }
            WriteLock();
            try
            {
                bool flag = true;
                if (_securityDescriptor.SystemAcl == null)
                {
                    if (modification == AccessControlModification.Remove || modification == AccessControlModification.RemoveAll || modification == AccessControlModification.RemoveSpecific)
                    {
                        modified = false;
                        return flag;
                    }
                    _securityDescriptor.SystemAcl = new SystemAcl(base.IsContainer, base.IsDS, GenericAcl.AclRevision, 1);
                    AddControlFlags(ControlFlags.SystemAclPresent);
                }
                SecurityIdentifier? sid = rule.IdentityReference.Translate(typeof(SecurityIdentifier)) as SecurityIdentifier;
                if (sid is not null)
                    switch (modification)
                    {
                        case AccessControlModification.Add:
                            _securityDescriptor.SystemAcl.AddAudit(rule.AuditFlags, sid, rule.AccessMask, rule.InheritanceFlags, rule.PropagationFlags);
                            break;
                        case AccessControlModification.Set:
                            _securityDescriptor.SystemAcl.SetAudit(rule.AuditFlags, sid, rule.AccessMask, rule.InheritanceFlags, rule.PropagationFlags);
                            break;
                        case AccessControlModification.Reset:
                            _securityDescriptor.SystemAcl.SetAudit(rule.AuditFlags, sid, rule.AccessMask, rule.InheritanceFlags, rule.PropagationFlags);
                            break;
                        case AccessControlModification.Remove:
                            flag = _securityDescriptor.SystemAcl.RemoveAudit(rule.AuditFlags, sid, rule.AccessMask, rule.InheritanceFlags, rule.PropagationFlags);
                            break;
                        case AccessControlModification.RemoveAll:
                            flag = _securityDescriptor.SystemAcl.RemoveAudit(AuditFlags.Success | AuditFlags.Failure, sid, -1, InheritanceFlags.ContainerInherit | InheritanceFlags.ObjectInherit, PropagationFlags.None);
                            if (!flag)
                            {
                                throw new InvalidProgramException();
                            }
                            break;
                        case AccessControlModification.RemoveSpecific:
                            _securityDescriptor.SystemAcl.RemoveAuditSpecific(rule.AuditFlags, sid, rule.AccessMask, rule.InheritanceFlags, rule.PropagationFlags);
                            break;
                        default:
                            throw new ArgumentOutOfRangeException("Modification value out of range.");
                    }
                modified = flag;
                AuditRulesModified |= modified;
                return flag;
            }
            finally
            {
                WriteUnlock();
            }
        }
        private bool ModifyAccess(AccessControlModification modification, ServiceAccessRule rule, out bool modified)
        {
            if (rule == null)
            {
                throw new ArgumentNullException("rule");
            }
            WriteLock();
            try
            {
                bool flag = true;
                if (_securityDescriptor.DiscretionaryAcl == null)
                {
                    if (modification == AccessControlModification.Remove || modification == AccessControlModification.RemoveAll || modification == AccessControlModification.RemoveSpecific)
                    {
                        modified = false;
                        return flag;
                    }
                    _securityDescriptor.DiscretionaryAcl = new DiscretionaryAcl(IsContainer, IsDS, GenericAcl.AclRevision, 1);
                    AddControlFlags(ControlFlags.DiscretionaryAclPresent);
                }

                SecurityIdentifier? sid = rule.IdentityReference.Translate(typeof(SecurityIdentifier)) as SecurityIdentifier;
                if (sid is not null)
                    if (rule.AccessControlType == AccessControlType.Allow)
                    {
                        switch (modification)
                        {
                            case AccessControlModification.Add:
                                _securityDescriptor.DiscretionaryAcl.AddAccess(AccessControlType.Allow, sid, rule.AccessMask, rule.InheritanceFlags, rule.PropagationFlags);
                                break;
                            case AccessControlModification.Set:
                                _securityDescriptor.DiscretionaryAcl.SetAccess(AccessControlType.Allow, sid, rule.AccessMask, rule.InheritanceFlags, rule.PropagationFlags);
                                break;
                            case AccessControlModification.Reset:
                                _securityDescriptor.DiscretionaryAcl.RemoveAccess(AccessControlType.Deny, sid, -1, InheritanceFlags.ContainerInherit | InheritanceFlags.ObjectInherit, PropagationFlags.None);
                                _securityDescriptor.DiscretionaryAcl.SetAccess(AccessControlType.Allow, sid, rule.AccessMask, rule.InheritanceFlags, rule.PropagationFlags);
                                break;
                            case AccessControlModification.Remove:
                                flag = _securityDescriptor.DiscretionaryAcl.RemoveAccess(AccessControlType.Allow, sid, rule.AccessMask, rule.InheritanceFlags, rule.PropagationFlags);
                                break;
                            case AccessControlModification.RemoveAll:
                                flag = _securityDescriptor.DiscretionaryAcl.RemoveAccess(AccessControlType.Allow, sid, -1, InheritanceFlags.ContainerInherit | InheritanceFlags.ObjectInherit, PropagationFlags.None);
                                if (!flag)
                                {
                                    throw new SystemException();
                                }
                                break;
                            case AccessControlModification.RemoveSpecific:
                                _securityDescriptor.DiscretionaryAcl.RemoveAccessSpecific(AccessControlType.Allow, sid, rule.AccessMask, rule.InheritanceFlags, rule.PropagationFlags);
                                break;
                            default:
                                throw new ArgumentOutOfRangeException("Modification value out of range.");
                        }
                    }
                    else
                    {
                        if (rule.AccessControlType != AccessControlType.Deny)
                        {
                            throw new ArgumentException("Illegal rule access control type.");
                        }
                        switch (modification)
                        {
                            case AccessControlModification.Add:
                                _securityDescriptor.DiscretionaryAcl.AddAccess(AccessControlType.Deny, sid, rule.AccessMask, rule.InheritanceFlags, rule.PropagationFlags);
                                break;
                            case AccessControlModification.Set:
                                _securityDescriptor.DiscretionaryAcl.SetAccess(AccessControlType.Deny, sid, rule.AccessMask, rule.InheritanceFlags, rule.PropagationFlags);
                                break;
                            case AccessControlModification.Reset:
                                _securityDescriptor.DiscretionaryAcl.RemoveAccess(AccessControlType.Allow, sid, -1, InheritanceFlags.ContainerInherit | InheritanceFlags.ObjectInherit, PropagationFlags.None);
                                _securityDescriptor.DiscretionaryAcl.SetAccess(AccessControlType.Deny, sid, rule.AccessMask, rule.InheritanceFlags, rule.PropagationFlags);
                                break;
                            case AccessControlModification.Remove:
                                flag = _securityDescriptor.DiscretionaryAcl.RemoveAccess(AccessControlType.Deny, sid, rule.AccessMask, rule.InheritanceFlags, rule.PropagationFlags);
                                break;
                            case AccessControlModification.RemoveAll:
                                flag = _securityDescriptor.DiscretionaryAcl.RemoveAccess(AccessControlType.Deny, sid, -1, InheritanceFlags.ContainerInherit | InheritanceFlags.ObjectInherit, PropagationFlags.None);
                                if (!flag)
                                {
                                    throw new SystemException();
                                }
                                break;
                            case AccessControlModification.RemoveSpecific:
                                _securityDescriptor.DiscretionaryAcl.RemoveAccessSpecific(AccessControlType.Deny, sid, rule.AccessMask, rule.InheritanceFlags, rule.PropagationFlags);
                                break;
                            default:
                                throw new ArgumentOutOfRangeException("Modification value out of range.");
                        }
                    }
                modified = flag;
                AccessRulesModified |= modified;
                return flag;
            }
            finally
            {
                WriteUnlock();
            }
        }

        private AuthorizationRuleCollection GetRules(bool access, bool includeExplicit, bool includeInherited, Type targetType)
        {
            ReadLock();
            try
            {
                AuthorizationRuleCollection authorizationRuleCollection = new();
                if (!IsValidTargetTypeStatic(targetType))
                {
                    throw new ArgumentException("Invalid target type.");
                }
                RawAcl? commonAcl = null;
                if (access)
                {
                    if ((_securityDescriptor.ControlFlags & ControlFlags.DiscretionaryAclPresent) != 0)
                    {
                        commonAcl = _rawSecDescriptor.DiscretionaryAcl;
                    }
                }
                else if ((_securityDescriptor.ControlFlags & ControlFlags.SystemAclPresent) != 0)
                {
                    commonAcl = _rawSecDescriptor.SystemAcl;
                }
                if (commonAcl == null)
                {
                    return authorizationRuleCollection;
                }
                IdentityReferenceCollection? identityReferenceCollection = null;
                if (targetType != typeof(SecurityIdentifier))
                {
                    IdentityReferenceCollection identityReferenceCollection2 = new IdentityReferenceCollection(commonAcl.Count);
                    for (int i = 0; i < commonAcl.Count; i++)
                    {
                        CommonAce? commonAce = commonAcl[i] as CommonAce;
                        if (commonAce is not null)
                            if (AceNeedsTranslation(commonAce, access, includeExplicit, includeInherited))
                            {
                                identityReferenceCollection2.Add(commonAce.SecurityIdentifier);
                            }
                    }
                    identityReferenceCollection = identityReferenceCollection2.Translate(targetType);
                }
                int num = 0;
                for (int j = 0; j < commonAcl.Count; j++)
                {
                    CommonAce? commonAce2 = commonAcl[j] as CommonAce;
                    if (commonAce2 is not null)
                        if (AceNeedsTranslation(commonAce2, access, includeExplicit, includeInherited))
                        {
                            IdentityReference? identityReference = (targetType == typeof(SecurityIdentifier)) ? commonAce2.SecurityIdentifier : identityReferenceCollection?[num++];
                            if (identityReference is not null)
                                if (access)
                                {
                                    authorizationRuleCollection.AddRule(AccessRuleFactory(type: (commonAce2.AceQualifier != 0) ? AccessControlType.Deny : AccessControlType.Allow, identityReference: identityReference, accessMask: commonAce2.AccessMask, isInherited: commonAce2.IsInherited, inheritanceFlags: commonAce2.InheritanceFlags, propagationFlags: commonAce2.PropagationFlags));
                                }
                                else
                                {
                                    authorizationRuleCollection.AddRule(AuditRuleFactory(identityReference, commonAce2.AccessMask, commonAce2.IsInherited, commonAce2.InheritanceFlags, commonAce2.PropagationFlags, commonAce2.AuditFlags));
                                }
                        }
                }
                return authorizationRuleCollection;
            }
            finally
            {
                ReadUnlock();
            }
        }

        internal void AddControlFlags(ControlFlags flags) => _rawSecDescriptor.SetFlags(_rawSecDescriptor.ControlFlags | flags);
        internal static bool IsValidTargetTypeStatic(Type targetType)
        {
            if (targetType == typeof(NTAccount))
            {
                return true;
            }
            if (targetType == typeof(SecurityIdentifier))
            {
                return true;
            }
            return false;
        }
        private bool AceNeedsTranslation(CommonAce ace, bool isAccessAce, bool includeExplicit, bool includeInherited)
        {
            if (ace == null)
            {
                return false;
            }
            if (isAccessAce)
            {
                if (ace.AceQualifier != 0 && ace.AceQualifier != AceQualifier.AccessDenied)
                {
                    return false;
                }
            }
            else if (ace.AceQualifier != AceQualifier.SystemAudit)
            {
                return false;
            }
            if ((includeExplicit && (ace.AceFlags & AceFlags.Inherited) == 0) || (includeInherited && (ace.AceFlags & AceFlags.Inherited) != 0))
            {
                return true;
            }
            return false;
        }
    
        internal static int AccessMaskFromRights(ServiceRights serviceRights)
        {
            if (serviceRights < 0 || serviceRights > ServiceRights.AllAccess)
                throw new ArgumentException("Invalid service rights.");

            return (int)serviceRights;
        }
    }

    public sealed class ServiceAccessRule : AccessRule
    {
        new internal int AccessMask => base.AccessMask;
        public ServiceRights ServiceRights  => (ServiceRights)base.AccessMask;

        public ServiceAccessRule(string identity, ServiceRights serviceRights, AccessControlType type)
            : this(new NTAccount(identity), ServiceSecurity.AccessMaskFromRights(serviceRights), isInherited: false, InheritanceFlags.None, PropagationFlags.None, type) { }
        public ServiceAccessRule(IdentityReference identity, ServiceRights serviceRights, AccessControlType type)
            : this(identity, ServiceSecurity.AccessMaskFromRights(serviceRights), isInherited: false, InheritanceFlags.None, PropagationFlags.None, type) { }
        public ServiceAccessRule(string identity, ServiceRights serviceRights, bool isInherited, InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AccessControlType type)
            : this(new NTAccount(identity), ServiceSecurity.AccessMaskFromRights(serviceRights), isInherited, inheritanceFlags, propagationFlags, type) { }
        public ServiceAccessRule(IdentityReference identity, ServiceRights serviceRights, bool isInherited, InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AccessControlType type)
            : this(identity, ServiceSecurity.AccessMaskFromRights(serviceRights), isInherited, inheritanceFlags, propagationFlags, type) { }
        internal ServiceAccessRule(IdentityReference identity, int accessMask, bool isInherited, InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AccessControlType type)
            : base(identity, accessMask, isInherited, inheritanceFlags, propagationFlags, type) { }
    }

    public sealed class ServiceAuditRule : AuditRule
    {
        new internal int AccessMask => base.AccessMask;
        public ServiceRights ServiceRights => (ServiceRights)base.AccessMask;

        public ServiceAuditRule(string identity, ServiceRights serviceRights, AuditFlags flags)
            : this(new NTAccount(identity), ServiceSecurity.AccessMaskFromRights(serviceRights), isInherited: false, InheritanceFlags.None, PropagationFlags.None, flags) { }
        public ServiceAuditRule(IdentityReference identity, ServiceRights serviceRights, AuditFlags flags)
            : this(identity, ServiceSecurity.AccessMaskFromRights(serviceRights), isInherited: false, InheritanceFlags.None, PropagationFlags.None, flags) { }
        public ServiceAuditRule(string identity, ServiceRights serviceRights, bool isInherited, InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AuditFlags flags)
            : this(new NTAccount(identity), ServiceSecurity.AccessMaskFromRights(serviceRights), isInherited, inheritanceFlags, propagationFlags, flags) { }
        public ServiceAuditRule(IdentityReference identity, ServiceRights serviceRights, bool isInherited, InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AuditFlags flags)
            : this(identity, ServiceSecurity.AccessMaskFromRights(serviceRights), isInherited, inheritanceFlags, propagationFlags, flags) { }
        internal ServiceAuditRule(IdentityReference identity, int accessMask, bool isInherited, InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AuditFlags flags)
            : base(identity, accessMask, isInherited, inheritanceFlags, propagationFlags, flags) { }
    }
}