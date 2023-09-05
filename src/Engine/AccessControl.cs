using System.Security.Principal;
using System.Security.AccessControl;
using System.Runtime.InteropServices;
using WindowsUtils.Services;
using WindowsUtils.Attributes;

namespace WindowsUtils.Interop
{
    [Flags]
    internal enum AccessType : uint
    {
        DELETE = 0x00010000,
        READ_CONTROL = 0x00020000,
        WRITE_DAC = 0x00040000,
        WRITE_OWNER = 0x00080000,
        SYNCHRONIZE = 0x00100000,
        STANDARD_RIGHTS_REQUIRED = 0x000F0000,
        STANDARD_RIGHTS_READ = READ_CONTROL,
        STANDARD_RIGHTS_WRITE = READ_CONTROL,
        STANDARD_RIGHTS_EXECUTE = READ_CONTROL,
        STANDARD_RIGHTS_ALL = 0x001F0000,
        SPECIFIC_RIGHTS_ALL = 0x0000FFFF,
        ACCESS_SYSTEM_SECURITY = 0x01000000,
        MAXIMUM_ALLOWED = 0x02000000,
        GENERIC_READ = 0x80000000,
        GENERIC_WRITE = 0x40000000,
        GENERIC_EXECUTE = 0x20000000,
        GENERIC_ALL = 0x10000000
    }

    [Flags]
    internal enum TokenAccessRight : uint
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
        TOKEN_ALL_ACCESS_P = AccessType.STANDARD_RIGHTS_REQUIRED |
                             TOKEN_ASSIGN_PRIMARY |
                             TOKEN_DUPLICATE |
                             TOKEN_IMPERSONATE |
                             TOKEN_QUERY |
                             TOKEN_QUERY_SOURCE |
                             TOKEN_ADJUST_PRIVILEGES |
                             TOKEN_ADJUST_GROUPS |
                             TOKEN_ADJUST_DEFAULT,

        // #if ((defined(_WIN32_WINNT) && (_WIN32_WINNT > 0x0400)) || (!defined(_WIN32_WINNT)))
        TOKEN_ALL_ACCESS = TOKEN_ALL_ACCESS_P | TOKEN_ADJUST_SESSIONID,
        // #else
        // #define TOKEN_ALL_ACCESS TOKEN_ALL_ACCESS_P
        // #endif
        TOKEN_READ = AccessType.STANDARD_RIGHTS_READ | TOKEN_QUERY,
        TOKEN_WRITE = AccessType.STANDARD_RIGHTS_WRITE |
                      TOKEN_ADJUST_PRIVILEGES |
                      TOKEN_ADJUST_GROUPS |
                      TOKEN_ADJUST_DEFAULT,

        TOKEN_EXECUTE = AccessType.STANDARD_RIGHTS_EXECUTE,
        TOKEN_TRUST_CONSTRAINT_MASK = AccessType.STANDARD_RIGHTS_READ |
                                      TOKEN_QUERY |
                                      TOKEN_QUERY_SOURCE,

        TOKEN_TRUST_ALLOWED_MASK = TOKEN_TRUST_CONSTRAINT_MASK |
                                   TOKEN_DUPLICATE |
                                   TOKEN_IMPERSONATE,

        // #if (NTDDI_VERSION >= NTDDI_WIN8)
        TOKEN_ACCESS_PSEUDO_HANDLE_WIN8 = TOKEN_QUERY | TOKEN_QUERY_SOURCE,
        TOKEN_ACCESS_PSEUDO_HANDLE = TOKEN_ACCESS_PSEUDO_HANDLE_WIN8
        // #endif
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct SECURITY_ATTRIBUTES
    {
        internal uint nLength;
        internal IntPtr lpSecurityDescriptor;
        
        [MarshalAs(UnmanagedType.Bool)]
        internal bool bInheritHandle;
    }

    internal partial class NativeFunctions
    {
        [DllImport("Advapi32.dll", SetLastError = true)]
        internal static extern bool RevertToSelf();
    }
}

namespace WindowsUtils.AccessControl
{
    public abstract class WindowsUtilsObjectSecurity
    {
        private readonly ReaderWriterLockSlim _lock = new(LockRecursionPolicy.SupportsRecursion);

        protected RawSecurityDescriptor _rawSecDescriptor;
        protected CommonSecurityDescriptor _securityDescriptor;

        private bool _ownerModified;
        private bool _groupModified;
        private bool _saclModified;
        private bool _daclModified;

        protected CommonSecurityDescriptor SecurityDescriptor => _securityDescriptor;
        protected bool OwnerModified
        {
            get { return _ownerModified; }
            set { _ownerModified = value; }
        }
        protected bool GroupModified
        {
            get { return _groupModified; }
            set { _groupModified = value; }
        }
        protected bool AuditRulesModified
        {
            get { return _saclModified; }
            set { _saclModified = value; }
        }
        protected bool AccessRulesModified
        {
            get { return _daclModified; }
            set { _daclModified = value; }
        }
        protected bool IsDS => _securityDescriptor.IsDS;
        protected bool IsContainer => _securityDescriptor.IsContainer;

        public string? Owner => GetOwner(typeof(NTAccount))?.ToString();
        public string? Group => GetGroup(typeof(NTAccount))?.ToString();
        public string Sddl => _securityDescriptor.GetSddlForm(AccessControlSections.All);

        public bool AreAccessRulesProtected
        {
            get
            {
                ReadLock();
                try
                {
                    return (_securityDescriptor.ControlFlags & ControlFlags.DiscretionaryAclProtected) != 0;
                }
                finally
                {
                    ReadUnlock();
                }
            }
        }
        public bool AreAuditRulesProtected
        {
            get
            {
                ReadLock();
                try
                {
                    return (_securityDescriptor.ControlFlags & ControlFlags.SystemAclProtected) != 0;
                }
                finally
                {
                    ReadUnlock();
                }
            }
        }
        public bool AreAccessRulesCanonical
        {
            get
            {
                ReadLock();
                try
                {
                    return _securityDescriptor.IsDiscretionaryAclCanonical;
                }
                finally
                {
                    ReadUnlock();
                }
            }
        }
        public bool AreAuditRulesCanonical
        {
            get
            {
                ReadLock();
                try
                {
                    return _securityDescriptor.IsSystemAclCanonical;
                }
                finally
                {
                    ReadUnlock();
                }
            }
        }

        public virtual Type? AccessRightType { get; }
        public virtual Type? AccessRuleType { get; }
        public virtual Type? AuditRuleType { get; }

        internal WindowsUtilsObjectSecurity(string sddl)
        {
            _rawSecDescriptor = new RawSecurityDescriptor(sddl);
            _securityDescriptor = new(false, false, _rawSecDescriptor);
        }
        internal WindowsUtilsObjectSecurity(RawSecurityDescriptor rawSecDescriptor)
        {
            _rawSecDescriptor = rawSecDescriptor;
            _securityDescriptor = new(false, false, rawSecDescriptor);
        }

        private void UpdateWithNewSecurityDescriptor(RawSecurityDescriptor newOne, AccessControlSections includeSections)
        {
            byte[] sdBytes = new byte[_securityDescriptor.BinaryLength];
            _securityDescriptor.GetBinaryForm(sdBytes, 0);
            _rawSecDescriptor = new(sdBytes, 0);

            if ((includeSections & AccessControlSections.Owner) != 0)
            {
                if (_rawSecDescriptor.Owner != newOne.Owner)
                {
                    _ownerModified = true;
                    _rawSecDescriptor.Owner = newOne.Owner;
                }
            }
            if ((includeSections & AccessControlSections.Group) != 0)
            {
                if (_rawSecDescriptor.Group != newOne.Group)
                {
                    _groupModified = true;
                    _rawSecDescriptor.Group = newOne.Group;
                }
            }
            if ((includeSections & AccessControlSections.Audit) != 0)
            {
                _saclModified = true;
                if (newOne.SystemAcl != null)
                {
                    _rawSecDescriptor.SystemAcl = newOne.SystemAcl;
                }
                else
                {
                    _rawSecDescriptor.SystemAcl = null;
                }

                _rawSecDescriptor.SetFlags(newOne.ControlFlags & (ControlFlags.SystemAclPresent | ControlFlags.SystemAclAutoInherited | ControlFlags.SystemAclProtected) | (_rawSecDescriptor.ControlFlags & ~(ControlFlags.SystemAclPresent | ControlFlags.SystemAclAutoInherited | ControlFlags.SystemAclProtected)));
            }
            if ((includeSections & AccessControlSections.Access) != 0)
            {
                _daclModified = true;
                if (newOne.DiscretionaryAcl != null)
                {
                    _rawSecDescriptor.DiscretionaryAcl = newOne.DiscretionaryAcl;
                }
                else
                {
                    _rawSecDescriptor.DiscretionaryAcl = null;
                }
                ControlFlags newFlags = (newOne.ControlFlags | _securityDescriptor.ControlFlags & ControlFlags.DiscretionaryAclPresent) & (ControlFlags.DiscretionaryAclPresent | ControlFlags.DiscretionaryAclAutoInherited | ControlFlags.DiscretionaryAclProtected);
                ControlFlags flagsToUpdate = _rawSecDescriptor.ControlFlags & ~(ControlFlags.DiscretionaryAclPresent | ControlFlags.DiscretionaryAclAutoInherited | ControlFlags.DiscretionaryAclProtected);
                _rawSecDescriptor.SetFlags(newFlags | flagsToUpdate);
            }

            _securityDescriptor = new(false, false, _rawSecDescriptor);
        }

        public abstract AccessRule AccessRuleFactory(IdentityReference identityReference, int accessMask, bool isInherited, InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AccessControlType type);
        public abstract AuditRule AuditRuleFactory(IdentityReference identityReference, int accessMask, bool isInherited, InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AuditFlags flags);

        protected abstract bool ModifyAudit(AccessControlModification modification, AuditRule rule, out bool modified);
        protected abstract bool ModifyAccess(AccessControlModification modification, AccessRule rule, out bool modified);

        protected void ReadLock() => _lock.EnterReadLock();
        protected void ReadUnlock() => _lock.ExitReadLock();
        protected void WriteLock() => _lock.EnterWriteLock();
        protected void WriteUnlock() => _lock.ExitWriteLock();

        public IdentityReference? GetOwner(Type targetType)
        {
            ReadLock();
            try
            {
                if (_securityDescriptor.Owner == null)
                {
                    return null;
                }
                return _securityDescriptor.Owner!.Translate(targetType);
            }
            finally
            {
                ReadUnlock();
            }
        }
        public void SetOwner(IdentityReference owner)
        {
            WriteLock();
            try
            {
                _securityDescriptor.Owner = owner.Translate(typeof(SecurityIdentifier)) as SecurityIdentifier;
                _ownerModified = true;
            }
            finally
            {
                WriteUnlock();
            }
        }
        public IdentityReference? GetGroup(Type targetType)
        {
            ReadLock();
            try
            {
                if (_securityDescriptor.Group == null)
                {
                    return null;
                }
                return _securityDescriptor.Group!.Translate(targetType);
            }
            finally
            {
                ReadUnlock();
            }
        }
        public void SetGroup(IdentityReference identity)
        {
            WriteLock();
            try
            {
                _securityDescriptor.Group = identity.Translate(typeof(SecurityIdentifier)) as SecurityIdentifier;
                _groupModified = true;
            }
            finally
            {
                WriteUnlock();
            }
        }

        public virtual void PurgeAccessRules(IdentityReference identity)
        {
            WriteLock();
            try
            {
                _securityDescriptor.PurgeAccessControl((SecurityIdentifier)identity.Translate(typeof(SecurityIdentifier)));
                _daclModified = true;
            }
            finally
            {
                WriteUnlock();
            }
        }
        public virtual void PurgeAuditRules(IdentityReference identity)
        {
            WriteLock();
            try
            {
                _securityDescriptor.PurgeAudit((SecurityIdentifier)identity.Translate(typeof(SecurityIdentifier)));
                _saclModified = true;
            }
            finally
            {
                WriteUnlock();
            }
        }
        public void SetAccessRuleProtection(bool isProtected, bool preserveInheritance)
        {
            WriteLock();
            try
            {
                _securityDescriptor.SetDiscretionaryAclProtection(isProtected, preserveInheritance);
                _daclModified = true;
            }
            finally
            {
                WriteUnlock();
            }
        }
        public void SetAuditRuleProtection(bool isProtected, bool preserveInheritance)
        {
            WriteLock();
            try
            {
                _securityDescriptor.SetSystemAclProtection(isProtected, preserveInheritance);
                _saclModified = true;
            }
            finally
            {
                WriteUnlock();
            }
        }
        public static bool IsSddlConversionSupported()
        {
            return true;
        }
        public string GetSecurityDescriptorSddlForm(AccessControlSections includeSections)
        {
            ReadLock();
            try
            {
                return _securityDescriptor.GetSddlForm(includeSections);
            }
            finally
            {
                ReadUnlock();
            }
        }
        public void SetSecurityDescriptorSddlForm(string sddlForm)
        {
            SetSecurityDescriptorSddlForm(sddlForm, AccessControlSections.All);
        }
        public void SetSecurityDescriptorSddlForm(string sddlForm, AccessControlSections includeSections)
        {
            if ((includeSections & AccessControlSections.All) == 0)
            {
                throw new ArgumentException("Input at least one access control section.");
            }
            WriteLock();
            try
            {
                UpdateWithNewSecurityDescriptor(new RawSecurityDescriptor(sddlForm), includeSections);
            }
            finally
            {
                WriteUnlock();
            }
        }
        public byte[] GetSecurityDescriptorBinaryForm()
        {
            ReadLock();
            try
            {
                byte[] array = new byte[_securityDescriptor.BinaryLength];
                _securityDescriptor.GetBinaryForm(array, 0);
                return array;
            }
            finally
            {
                ReadUnlock();
            }
        }
        public void SetSecurityDescriptorBinaryForm(byte[] binaryForm)
        {
            SetSecurityDescriptorBinaryForm(binaryForm, AccessControlSections.All);
        }
        public void SetSecurityDescriptorBinaryForm(byte[] binaryForm, AccessControlSections includeSections)
        {
            if ((includeSections & AccessControlSections.All) == 0)
            {
                throw new ArgumentException("Input at least one access control section.");
            }
            WriteLock();
            try
            {
                UpdateWithNewSecurityDescriptor(new RawSecurityDescriptor(binaryForm, 0), includeSections);
            }
            finally
            {
                WriteUnlock();
            }
        }
    }

    public sealed class ServiceSecurity : WindowsUtilsObjectSecurity
    {
        private ServiceAccessRule[] _access;
        private ServiceAuditRule[] _audit;
        private readonly string _name;

        public string Name { get { return _name; } }
        public string AccessToString
        {
            get
            {
                bool first = true;
                string output = string.Empty;
                if (_access is null)
                    return string.Empty;

                foreach (ServiceAccessRule ace in _access)
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
                if (_audit is null)
                    return string.Empty;

                foreach (ServiceAuditRule ace in _audit)
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
        public ServiceAccessRule[] Access { get { return _access; } }
        public ServiceAuditRule[] Audit { get { return _audit; } }

        new internal bool OwnerModified => base.OwnerModified;

        public override Type AccessRightType => typeof(ServiceRights);
        public override Type AccessRuleType => typeof(ServiceAccessRule);
        public override Type AuditRuleType => typeof(ServiceAuditRule);

        internal ServiceSecurity(string sddl, string name)
            : base(sddl)
        {
            _name = name;

            AuthorizationRuleCollection _accTemp = GetAccessRules(true, true, typeof(NTAccount));
            _access = new ServiceAccessRule[_accTemp.Count];
            _accTemp.CopyTo(_access, 0);

            AuthorizationRuleCollection _audTemp = GetAuditRules(true, true, typeof(NTAccount));
            _audit = new ServiceAuditRule[_audTemp.Count];
            _audTemp.CopyTo(_audit, 0);
        }
        internal ServiceSecurity(RawSecurityDescriptor rawSecDescriptor, string name)
            : base(rawSecDescriptor)
        {
            _name = name;

            AuthorizationRuleCollection _accTemp = GetAccessRules(true, true, typeof(NTAccount));
            _access = new ServiceAccessRule[_accTemp.Count];
            _accTemp.CopyTo(_access, 0);

            AuthorizationRuleCollection _audTemp = GetAuditRules(true, true, typeof(NTAccount));
            _audit = new ServiceAuditRule[_audTemp.Count];
            _audTemp.CopyTo(_audit, 0);
        }

        public sealed override AccessRule AccessRuleFactory(IdentityReference identityReference, int accessMask, bool isInherited, InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AccessControlType type)
            => new ServiceAccessRule(identityReference, accessMask, isInherited, inheritanceFlags, propagationFlags, type);
        public sealed override AuditRule AuditRuleFactory(IdentityReference identityReference, int accessMask, bool isInherited, InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AuditFlags flags)
            => new ServiceAuditRule(identityReference, accessMask, isInherited, inheritanceFlags, propagationFlags, flags);

        public void AddAccessRule(ServiceAccessRule rule)
        {
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
            WriteLock();
            try
            {
                return ModifyAudit(AccessControlModification.Remove, rule, out bool modified);
            }
            finally
            {
                WriteUnlock();
            }
        }
        public void RemoveAuditRuleAll(ServiceAuditRule rule)
        {
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
                CommonAcl? commonAcl = null;
                if (access)
                {
                    if ((_securityDescriptor.ControlFlags & ControlFlags.DiscretionaryAclPresent) != 0)
                    {
                        commonAcl = _securityDescriptor.DiscretionaryAcl;
                    }
                }
                else if ((_securityDescriptor.ControlFlags & ControlFlags.SystemAclPresent) != 0)
                {
                    commonAcl = _securityDescriptor.SystemAcl;
                }
                if (commonAcl == null)
                {
                    return authorizationRuleCollection;
                }
                IdentityReferenceCollection? identityReferenceCollection = null;
                if (targetType != typeof(SecurityIdentifier))
                {
                    if (commonAcl is not null)
                    {
                        IdentityReferenceCollection identityReferenceCollection2 = new(commonAcl.Count);
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

                }
                int num = 0;
                if (commonAcl is not null)
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

        protected override bool ModifyAudit(AccessControlModification modification, AuditRule rule, out bool modified) => ModifyAudit(modification, (ServiceAuditRule)rule, out modified);
        protected override bool ModifyAccess(AccessControlModification modification, AccessRule rule, out bool modified) => ModifyAccess(modification, (ServiceAccessRule)rule, out modified);
        private bool ModifyAudit(AccessControlModification modification, ServiceAuditRule rule, out bool modified)
        {
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
                if (AuditRulesModified)
                {
                    AuthorizationRuleCollection _audTemp = GetAuditRules(true, true, typeof(NTAccount));
                    _audit = new ServiceAuditRule[_audTemp.Count];
                    _audTemp.CopyTo(_audit, 0);
                }
                return flag;
            }
            finally
            {
                WriteUnlock();
            }
        }
        private bool ModifyAccess(AccessControlModification modification, ServiceAccessRule rule, out bool modified)
        {
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
                if (AccessRulesModified)
                {
                    AuthorizationRuleCollection _accTemp = GetAccessRules(true, true, typeof(NTAccount));
                    _access = new ServiceAccessRule[_accTemp.Count];
                    _accTemp.CopyTo(_access, 0);
                }
                return flag;
            }
            finally
            {
                WriteUnlock();
            }
        }

        internal void AddControlFlags(ControlFlags flags)
        {
            /*
            * The CommonSecurityDescriptor.AddControlFlags() is internal.
            * Backing up our class on a RawSecurityDescriptor would be the right thing to do,
            * but that would involve changing the whole implementation.
            * So we change the CommonSecurityDescriptor raw sd by recreating everything.
            */
            byte[] binarySd = new byte[_securityDescriptor.BinaryLength];
            _securityDescriptor.GetBinaryForm(binarySd, 0);
            _rawSecDescriptor = new(binarySd, 0);

            // Setting the flags.
            _rawSecDescriptor.SetFlags(_rawSecDescriptor.ControlFlags | flags);

            // Recreating the common sd.
            _securityDescriptor = new(false, false, _rawSecDescriptor);
        }
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
        private bool AceNeedsTranslation([NotNullWhen(true)] CommonAce ace, bool isAccessAce, bool includeExplicit, bool includeInherited)
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

    public sealed class ServiceAccessRule : AccessRule, IEquatable<ServiceAccessRule>
    {
        new internal int AccessMask => base.AccessMask;
        public ServiceRights ServiceRights => (ServiceRights)base.AccessMask;

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

        public override bool Equals(object other) => Equals((ServiceAccessRule)other);
        public override int GetHashCode() => base.GetHashCode();
        public bool Equals(ServiceAccessRule other)
        {
            if (other.IdentityReference.ToString() == IdentityReference.ToString() &&
                other.ServiceRights == ServiceRights &&
                other.IsInherited == IsInherited &&
                other.InheritanceFlags == InheritanceFlags &&
                other.PropagationFlags == PropagationFlags &&
                other.AccessControlType == AccessControlType)
                return true;

            return false;
        }

        public static bool operator ==(ServiceAccessRule left, ServiceAccessRule right) => left.Equals(right);
        public static bool operator !=(ServiceAccessRule left, ServiceAccessRule right) => !left.Equals(right);
    }

    public sealed class ServiceAuditRule : AuditRule, IEquatable<ServiceAuditRule>
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

        public override bool Equals(object other) => Equals((ServiceAuditRule)other);
        public override int GetHashCode() => base.GetHashCode();
        public bool Equals(ServiceAuditRule other)
        {
            if (other.IdentityReference.ToString() == IdentityReference.ToString() &&
                other.ServiceRights == ServiceRights &&
                other.IsInherited == IsInherited &&
                other.InheritanceFlags == InheritanceFlags &&
                other.PropagationFlags == PropagationFlags &&
                other.AuditFlags == AuditFlags)
                return true;

            return false;
        }

        public static bool operator ==(ServiceAuditRule left, ServiceAuditRule right) => left.Equals(right);
        public static bool operator !=(ServiceAuditRule left, ServiceAuditRule right) => !left.Equals(right);
    }
}