#include "../../pch.h"

#include "../../Headers/Support/SafeHandle.h"

namespace WindowsUtils::Core
{
	/*
	*	~ ScmHandle ~
	*/

	ScmHandle::ScmHandle()
		: m_handle{ nullptr }, m_ownsHandle{ false } { }

	ScmHandle::ScmHandle(const ScmHandle& other)
		: m_handle{ other.m_handle }, m_ownsHandle{ other.m_ownsHandle } { }

	ScmHandle::ScmHandle(const WWuString& computerName, DWORD desiredAccess, ScmHandleType type)
	{
		_WU_ASSERT(type == ScmHandleType::ServiceControlManager, L"This constructor can only be used for SCM handles!");

		m_handle = OpenSCManager(computerName.Raw(), SERVICES_ACTIVE_DATABASE, desiredAccess);
		if (!m_handle)
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"OpenSCManager", WriteErrorCategory::OpenError);

		m_ownsHandle = true;
	}

	ScmHandle::ScmHandle(const ScmHandle& scm, const WWuString& computerName, const WWuString& serviceName, DWORD desiredAccess, ScmHandleType type)
	{
		_WU_ASSERT(type == ScmHandleType::Service, L"This constructor can only be used for service handles!");

		m_handle = OpenService(scm.m_handle, serviceName.Raw(), desiredAccess);
		if (!m_handle)
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"OpenService", WriteErrorCategory::OpenError);

		m_ownsHandle = true;
	}

	ScmHandle::~ScmHandle()
	{
		if (m_handle && m_ownsHandle)
			CloseServiceHandle(m_handle);

		m_handle = nullptr;
		m_ownsHandle = false;
	}

	const SC_HANDLE ScmHandle::Get() const { return m_handle; }


	/*
	*	~ File handle ~
	*/

	FileHandle::FileHandle()
		: m_handle{ nullptr }, m_ownsHandle{ false } { }

	FileHandle::FileHandle(const FileHandle& other)
		: m_handle{ other.m_handle }, m_ownsHandle{ other.m_ownsHandle } { }

	FileHandle::FileHandle(const WWuString& fileName, DWORD desiredAccess, DWORD shareMode, LPSECURITY_ATTRIBUTES secAttr, DWORD disposition, DWORD flagsAndAttr, HANDLE hTemplate)
	{
		m_handle = CreateFile(
			fileName.Raw(),
			desiredAccess,
			shareMode,
			secAttr,
			disposition,
			flagsAndAttr,
			hTemplate
		);

		if (m_handle == INVALID_HANDLE_VALUE)
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"CreateFile", WriteErrorCategory::OpenError);

		m_ownsHandle = true;
	}

	FileHandle::~FileHandle()
	{
		if (m_handle && m_handle != INVALID_HANDLE_VALUE && m_ownsHandle)
			NtClose(m_handle);

		m_handle = nullptr;
		m_ownsHandle = false;
	}

	const HANDLE FileHandle::Get() const { return m_handle; }

	PHANDLE FileHandle::operator &()
	{
		if (m_handle && m_ownsHandle) {
			NtClose(m_handle);
			m_handle = nullptr;
		}

		m_ownsHandle = true;
		
		return &m_handle;
	}


	/*
	*	~ Module handle ~
	*/

	ModuleHandle::ModuleHandle()
		: m_handle { nullptr }, m_isValid{ false }, m_ownsHandle{ false }, m_error{ } { }

	ModuleHandle::ModuleHandle(const ModuleHandle& other)
	{
		m_handle = other.m_handle;
		m_isValid = other.m_isValid;
		m_ownsHandle = other.m_ownsHandle;
		m_error = other.m_error;
	}

	ModuleHandle::ModuleHandle(const WWuString& name, bool forceLoad)
		: m_error{ }, m_isValid{ false }, m_ownsHandle{ false }
	{
		if (forceLoad) {
			m_handle = LoadLibrary(name.Raw());
			if (!m_handle) {
				m_error = GetLastError();
			}
			else {
				m_isValid = true;
				m_ownsHandle = true;
			}
		}
		else {
			m_handle = GetModuleHandle(name.Raw());
			if (!m_handle) {
				m_handle = LoadLibrary(name.Raw());
				if (!m_handle) {
					m_error = GetLastError();
				}
				else {
					m_isValid = true;
					m_ownsHandle = true;
				}
			}
			else {
				m_isValid = true;
			}
		}
	}

	ModuleHandle::~ModuleHandle()
	{
		if (m_handle && m_isValid && m_ownsHandle)
			FreeLibrary(m_handle);
	}

	const int ModuleHandle::Error() const { return m_error; }
	const HMODULE ModuleHandle::Get() const { return m_handle; }
	const bool ModuleHandle::IsValid() const { return m_isValid; }


	/*
	*	~ Process handle ~
	*/

	ProcessHandle::ProcessHandle()
		: m_handle{ nullptr }, m_ownsHandle{ false } { }

	ProcessHandle::ProcessHandle(HANDLE handle)
		: m_handle{ handle }, m_ownsHandle{ true } { }

	ProcessHandle::ProcessHandle(const ProcessHandle& other)
		: m_handle{ other.m_handle }, m_ownsHandle{ other.m_ownsHandle } { }

	ProcessHandle::ProcessHandle(DWORD processId, ACCESS_MASK desiredAccess, bool inherit)
	{
		m_ownsHandle = false;
		m_handle = WuOpenProcess(processId, desiredAccess, inherit);
		if (!m_handle) {
			return;
		}

		m_ownsHandle = true;
	}

	ProcessHandle::~ProcessHandle()
	{
		if (m_handle && m_ownsHandle)
			NtClose(m_handle);
	}

	const HANDLE ProcessHandle::Get() const { return m_handle; }
	const bool ProcessHandle::IsValid() const { return m_handle != 0 && m_handle != INVALID_HANDLE_VALUE; }
	
	PHANDLE ProcessHandle::operator &()
	{
		if (m_handle && m_ownsHandle) {
			NtClose(m_handle);
			m_handle = nullptr;
		}

		m_ownsHandle = true;

		return &m_handle;
	}

	HANDLE ProcessHandle::WuOpenProcess(DWORD processId, ACCESS_MASK desiredAccess, bool inherit)
	{
		HANDLE handle;
		OBJECT_ATTRIBUTES objAttr{
			sizeof(OBJECT_ATTRIBUTES),
			nullptr,
			nullptr,
			inherit ? OBJ_INHERIT : 0,
			nullptr,
			nullptr
		};

		CLIENT_ID clientId{
			reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(processId)),
			nullptr
		};

		NTSTATUS status = NtOpenProcess(&handle, desiredAccess, &objAttr, &clientId);
		if (status) {
			ULONG error = RtlNtStatusToDosError(status);
			SetLastError(error);

			return nullptr;
		}

		return handle;
	}


	/*
	*	~ Object handle ~
	*/

	SafeObjectHandle::SafeObjectHandle()
		: m_handle{ nullptr }, m_ownsHandle{ false } { }

	SafeObjectHandle::SafeObjectHandle(const SafeObjectHandle& other)
		: m_handle{ other.m_handle }, m_ownsHandle{ other.m_ownsHandle } { }

	SafeObjectHandle::SafeObjectHandle(HANDLE hObject, bool ownsHandle)
		: m_handle{ hObject }, m_ownsHandle{ ownsHandle } { }

	SafeObjectHandle::~SafeObjectHandle()
	{
		if (m_handle && m_ownsHandle)
			NtClose(m_handle);
	}

	const HANDLE SafeObjectHandle::Get() const { return m_handle; }

	PHANDLE SafeObjectHandle::operator &()
	{
		if (m_handle && m_ownsHandle) {
			NtClose(m_handle);
			m_handle = nullptr;
		}

		m_ownsHandle = true;

		return &m_handle;
	}

	/*
	*	~ Registry handle ~
	*/

	RegistryHandle::RegistryHandle()
		: m_handle{ nullptr }, m_ownsHandle{ false } { }

	RegistryHandle::RegistryHandle(const RegistryHandle& other)
		: m_handle{ other.m_handle }, m_ownsHandle{ other.m_ownsHandle } { }

	RegistryHandle::RegistryHandle(HKEY handle, bool ownsHandle)
		: m_handle{ handle }, m_ownsHandle{ ownsHandle } { }

	RegistryHandle::~RegistryHandle()
	{
		if (m_handle && m_ownsHandle)
			RegCloseKey(m_handle);
	}

	const HKEY RegistryHandle::Get() const { return m_handle; }

	PHKEY RegistryHandle::operator &()
	{
		if (m_handle && m_ownsHandle) {
			RegCloseKey(m_handle);
			m_handle = nullptr;
		}

		m_ownsHandle = true;

		return &m_handle;
	}
}