using System.Security.Principal;
using System.Security.AccessControl;

namespace WindowsUtils.Engine.AccessControl;

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
    protected bool OwnerModified {
        get { return _ownerModified; }
        set { _ownerModified = value; }
    }
    protected bool GroupModified {
        get { return _groupModified; }
        set { _groupModified = value; }
    }
    protected bool AuditRulesModified {
        get { return _saclModified; }
        set { _saclModified = value; }
    }
    protected bool AccessRulesModified {
        get { return _daclModified; }
        set { _daclModified = value; }
    }
    protected bool IsDS => _securityDescriptor.IsDS;
    protected bool IsContainer => _securityDescriptor.IsContainer;

    public string? Owner => GetOwner(typeof(NTAccount))?.ToString();
    public string? Group => GetGroup(typeof(NTAccount))?.ToString();
    public string Sddl => _securityDescriptor.GetSddlForm(AccessControlSections.All);

    public bool AreAccessRulesProtected {
        get {
            ReadLock();
            try {
                return (_securityDescriptor.ControlFlags & ControlFlags.DiscretionaryAclProtected) != 0;
            }
            finally {
                ReadUnlock();
            }
        }
    }
    public bool AreAuditRulesProtected {
        get {
            ReadLock();
            try {
                return (_securityDescriptor.ControlFlags & ControlFlags.SystemAclProtected) != 0;
            }
            finally {
                ReadUnlock();
            }
        }
    }
    public bool AreAccessRulesCanonical {
        get {
            ReadLock();
            try {
                return _securityDescriptor.IsDiscretionaryAclCanonical;
            }
            finally {
                ReadUnlock();
            }
        }
    }
    public bool AreAuditRulesCanonical {
        get {
            ReadLock();
            try {
                return _securityDescriptor.IsSystemAclCanonical;
            }
            finally {
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

        if ((includeSections & AccessControlSections.Owner) != 0) {
            if (_rawSecDescriptor.Owner != newOne.Owner) {
                _ownerModified = true;
                _rawSecDescriptor.Owner = newOne.Owner;
            }
        }
        if ((includeSections & AccessControlSections.Group) != 0) {
            if (_rawSecDescriptor.Group != newOne.Group) {
                _groupModified = true;
                _rawSecDescriptor.Group = newOne.Group;
            }
        }
        if ((includeSections & AccessControlSections.Audit) != 0) {
            _saclModified = true;
            if (newOne.SystemAcl != null) {
                _rawSecDescriptor.SystemAcl = newOne.SystemAcl;
            }
            else {
                _rawSecDescriptor.SystemAcl = null;
            }

            _rawSecDescriptor.SetFlags(newOne.ControlFlags & (ControlFlags.SystemAclPresent | ControlFlags.SystemAclAutoInherited | ControlFlags.SystemAclProtected) | (_rawSecDescriptor.ControlFlags & ~(ControlFlags.SystemAclPresent | ControlFlags.SystemAclAutoInherited | ControlFlags.SystemAclProtected)));
        }
        if ((includeSections & AccessControlSections.Access) != 0) {
            _daclModified = true;
            if (newOne.DiscretionaryAcl != null) {
                _rawSecDescriptor.DiscretionaryAcl = newOne.DiscretionaryAcl;
            }
            else {
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
        try {
            if (_securityDescriptor.Owner == null) {
                return null;
            }
            return _securityDescriptor.Owner!.Translate(targetType);
        }
        finally {
            ReadUnlock();
        }
    }
    public void SetOwner(IdentityReference owner)
    {
        WriteLock();
        try {
            _securityDescriptor.Owner = owner.Translate(typeof(SecurityIdentifier)) as SecurityIdentifier;
            _ownerModified = true;
        }
        finally {
            WriteUnlock();
        }
    }
    public IdentityReference? GetGroup(Type targetType)
    {
        ReadLock();
        try {
            if (_securityDescriptor.Group == null) {
                return null;
            }
            return _securityDescriptor.Group!.Translate(targetType);
        }
        finally {
            ReadUnlock();
        }
    }
    public void SetGroup(IdentityReference identity)
    {
        WriteLock();
        try {
            _securityDescriptor.Group = identity.Translate(typeof(SecurityIdentifier)) as SecurityIdentifier;
            _groupModified = true;
        }
        finally {
            WriteUnlock();
        }
    }

    public virtual void PurgeAccessRules(IdentityReference identity)
    {
        WriteLock();
        try {
            _securityDescriptor.PurgeAccessControl((SecurityIdentifier)identity.Translate(typeof(SecurityIdentifier)));
            _daclModified = true;
        }
        finally {
            WriteUnlock();
        }
    }
    public virtual void PurgeAuditRules(IdentityReference identity)
    {
        WriteLock();
        try {
            _securityDescriptor.PurgeAudit((SecurityIdentifier)identity.Translate(typeof(SecurityIdentifier)));
            _saclModified = true;
        }
        finally {
            WriteUnlock();
        }
    }
    public void SetAccessRuleProtection(bool isProtected, bool preserveInheritance)
    {
        WriteLock();
        try {
            _securityDescriptor.SetDiscretionaryAclProtection(isProtected, preserveInheritance);
            _daclModified = true;
        }
        finally {
            WriteUnlock();
        }
    }
    public void SetAuditRuleProtection(bool isProtected, bool preserveInheritance)
    {
        WriteLock();
        try {
            _securityDescriptor.SetSystemAclProtection(isProtected, preserveInheritance);
            _saclModified = true;
        }
        finally {
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
        try {
            return _securityDescriptor.GetSddlForm(includeSections);
        }
        finally {
            ReadUnlock();
        }
    }
    public void SetSecurityDescriptorSddlForm(string sddlForm)
    {
        SetSecurityDescriptorSddlForm(sddlForm, AccessControlSections.All);
    }
    public void SetSecurityDescriptorSddlForm(string sddlForm, AccessControlSections includeSections)
    {
        if ((includeSections & AccessControlSections.All) == 0) {
            throw new ArgumentException("Input at least one access control section.");
        }
        WriteLock();
        try {
            UpdateWithNewSecurityDescriptor(new RawSecurityDescriptor(sddlForm), includeSections);
        }
        finally {
            WriteUnlock();
        }
    }
    public byte[] GetSecurityDescriptorBinaryForm()
    {
        ReadLock();
        try {
            byte[] array = new byte[_securityDescriptor.BinaryLength];
            _securityDescriptor.GetBinaryForm(array, 0);
            return array;
        }
        finally {
            ReadUnlock();
        }
    }
    public void SetSecurityDescriptorBinaryForm(byte[] binaryForm)
    {
        SetSecurityDescriptorBinaryForm(binaryForm, AccessControlSections.All);
    }
    public void SetSecurityDescriptorBinaryForm(byte[] binaryForm, AccessControlSections includeSections)
    {
        if ((includeSections & AccessControlSections.All) == 0) {
            throw new ArgumentException("Input at least one access control section.");
        }
        WriteLock();
        try {
            UpdateWithNewSecurityDescriptor(new RawSecurityDescriptor(binaryForm, 0), includeSections);
        }
        finally {
            WriteUnlock();
        }
    }
}