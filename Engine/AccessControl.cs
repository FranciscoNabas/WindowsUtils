using System.Security.Principal;
using System.Security.AccessControl;
using WindowsUtils.Engine;

namespace WindowsUtils.AccessControl
{
    public abstract class WindowsUtilsObjectSecurity
    {
        private readonly ReaderWriterLockSlim _lock = new(LockRecursionPolicy.SupportsRecursion);

        internal readonly RawSecurityDescriptor _rawSecDescriptor;
        internal readonly CommonSecurityDescriptor _securityDescriptor;
        private readonly string _sddl;
        private readonly string? _owner;
        private readonly string? _group;

        private bool _saclModified;
	    private bool _daclModified;

        protected bool IsDS => _securityDescriptor.IsDS;
        protected bool IsContainer => _securityDescriptor.IsContainer;

        public string? Owner { get { return _owner; } }
        public string? Group { get { return _group; } }
        public string Sddl { get { return _sddl; } }

        protected bool AuditRulesModified
        {
            get
            {
                if (!_lock.IsReadLockHeld && !_lock.IsWriteLockHeld)
                    throw new InvalidOperationException("Must lock for read or write.");
                
                return _saclModified;
            }
            set
            {
                if (!_lock.IsWriteLockHeld)
                    throw new InvalidOperationException("Must lock for write.");
                
                _saclModified = value;
            }
        }

        protected bool AccessRulesModified
	{
		get
		{
			if (!_lock.IsReadLockHeld && !_lock.IsWriteLockHeld)
                throw new InvalidOperationException("Must lock for read or write.");

			return _daclModified;
		}
		set
		{
			if (!_lock.IsWriteLockHeld)
                throw new InvalidOperationException("Must lock for write.");
            
			_daclModified = value;
		}
	}

        public virtual Type? AccessRightType { get; }
        public virtual Type? AccessRuleType { get; }
        public virtual Type? AuditRuleType { get; }

        internal WindowsUtilsObjectSecurity(string sddl)
        {
            _sddl = sddl;
            _rawSecDescriptor = new RawSecurityDescriptor(sddl);
            _securityDescriptor = new(false, false, _rawSecDescriptor);
            _owner = _securityDescriptor.Owner?.Translate(typeof(NTAccount)).ToString();
            _group = _securityDescriptor.Group?.Translate(typeof(NTAccount)).ToString();
        }

        internal WindowsUtilsObjectSecurity(RawSecurityDescriptor rawSecDescriptor)
        {
            _rawSecDescriptor = rawSecDescriptor;
            _sddl = rawSecDescriptor.GetSddlForm(AccessControlSections.All);
            _securityDescriptor = new(false, false, rawSecDescriptor);
        }

        public abstract AccessRule AccessRuleFactory(IdentityReference identityReference, int accessMask, bool isInherited, InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AccessControlType type);
        public abstract AuditRule AuditRuleFactory(IdentityReference identityReference, int accessMask, bool isInherited, InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AuditFlags flags);

        protected abstract bool ModifyAudit(AccessControlModification modification, AuditRule rule, out bool modified);
        protected abstract bool ModifyAccess(AccessControlModification modification, AccessRule rule, out bool modified);

        protected void ReadLock() => _lock.EnterReadLock();
        protected void ReadUnlock() => _lock.ExitReadLock();
        protected void WriteLock() => _lock.EnterWriteLock();
        protected void WriteUnlock() => _lock.ExitWriteLock();
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