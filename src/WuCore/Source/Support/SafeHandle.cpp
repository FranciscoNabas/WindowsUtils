#include "../../pch.h"

#include "../../Headers/Support/SafeHandle.h"
#include "../../Headers/Support/WuStdException.h"

namespace WindowsUtils::Core
{
	/*
	*	~ ScmHandle ~
	*/

	ScmHandle::ScmHandle()
	{
		m_h = NULL;
		m_isValid = false;
	}

	ScmHandle::ScmHandle(const WWuString& computerName, DWORD desiredAccess, ScmHandleType type)
	{
		if (type != ScmHandleType::ServiceControlManager)
			throw WuStdException(-1, (LPWSTR)L"This constructor can only be used for SCM handles.", __FILEW__, __LINE__);

		m_h = OpenSCManager(computerName.GetBuffer(), SERVICES_ACTIVE_DATABASE, desiredAccess);
		if (m_h == NULL)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		m_isValid = true;
	}

	ScmHandle::ScmHandle(const ScmHandle& scm, const WWuString& computerName, const WWuString& serviceName, DWORD desiredAccess, ScmHandleType type)
	{
		if (type != ScmHandleType::Service)
			throw WuStdException(-1, L"This constructor can only be used for service handles.", __FILEW__, __LINE__);

		m_h = OpenService(scm.m_h, serviceName.GetBuffer(), desiredAccess);
		if (m_h == NULL)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		m_isValid = true;
	}

	ScmHandle::~ScmHandle()
	{
		if (m_h)
			CloseServiceHandle(m_h);

		m_h = NULL;
		m_isValid = false;
	}

	const SC_HANDLE ScmHandle::get() const { return m_h; }

	/*
	*	~ File handle
	*/

	FileHandle::FileHandle()
	{
		m_hFile = NULL;
		m_isValid = false;
	}

	FileHandle::FileHandle(const WWuString& fileName, DWORD desiredAccess, DWORD shareMode, LPSECURITY_ATTRIBUTES secAttr, DWORD disposition, DWORD flagsAndAttr, HANDLE hTemplate)
	{
		m_hFile = CreateFile(
			fileName.GetBuffer(),
			desiredAccess,
			shareMode,
			secAttr,
			disposition,
			flagsAndAttr,
			hTemplate
		);

		if (m_hFile == INVALID_HANDLE_VALUE)
			throw WuStdException(static_cast<int>(GetLastError()), __FILEW__, __LINE__);

		m_isValid = true;
	}

	FileHandle::~FileHandle()
	{
		if (m_hFile != NULL && m_hFile != INVALID_HANDLE_VALUE)
			CloseHandle(m_hFile);

		m_hFile = NULL;
		m_isValid = false;
	}

	const HANDLE FileHandle::get() const { return m_hFile; }

	/*
	*	~ Module handle
	*/

	ModuleHandle::ModuleHandle()
		: m_hModule(NULL), m_isLoaded(false), m_isValid(false), m_error(ERROR_SUCCESS)
	{ }

	ModuleHandle::ModuleHandle(const WWuString& name, bool forceLoad)
	{
		m_error = ERROR_SUCCESS;
		m_isValid = false;
		m_isLoaded = false;
		if (forceLoad) {
			m_hModule = LoadLibrary(name.GetBuffer());
			if (m_hModule == NULL) {
				m_error = GetLastError();
			}
			else {
				m_isValid = true;
				m_isLoaded = true;
			}
		}
		else {
			m_hModule = GetModuleHandle(name.GetBuffer());
			if (m_hModule == NULL) {
				m_hModule = LoadLibrary(name.GetBuffer());
				if (m_hModule == NULL) {
					m_error = GetLastError();
				}
				else {
					m_isValid = true;
					m_isLoaded = true;
				}
			}
			else {
				m_isValid = true;
			}
		}
	}

	ModuleHandle::~ModuleHandle()
	{
		if (m_hModule != NULL && m_isLoaded)
			FreeLibrary(m_hModule);
	}

	const HMODULE ModuleHandle::get() const { return m_hModule; }
	const bool ModuleHandle::IsValid() const { return m_isValid; }
	const int ModuleHandle::Error() const { return m_error; }

	/*
	*	~ Process handle
	*/

	ProcessHandle::ProcessHandle()
		: m_hProcess(NULL) { }

	ProcessHandle::ProcessHandle(DWORD desiredAccess, BOOL inherit, DWORD processId)
	{
		m_hProcess = OpenProcess(desiredAccess, inherit, processId);
		if (m_hProcess == NULL) {
			throw WuStdException { static_cast<int>(GetLastError()), __FILEW__, __LINE__ };
		}
	}

	ProcessHandle::~ProcessHandle()
	{
		if (m_hProcess != NULL)
			CloseHandle(m_hProcess);
	}

	const HANDLE ProcessHandle::get() const { return m_hProcess; }

	/*
	*	~ Object handle
	*/

	ObjectHandle::ObjectHandle() { m_hObject = 0; }
	ObjectHandle::ObjectHandle(HANDLE hObject) { m_hObject = hObject; }
	ObjectHandle::~ObjectHandle() { if (m_hObject) CloseHandle(m_hObject); }

	const HANDLE ObjectHandle::get() const { return m_hObject; }
}