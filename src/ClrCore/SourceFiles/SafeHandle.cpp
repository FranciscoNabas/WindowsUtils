#include "..\pch.h"

#include "..\Headers\SafeHandle.h"

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

	ScmHandle::ScmHandle(const WWuString& computerName, DWORD desiredAccess, SCM_HANDLE_TYPE type)
	{
		if (type != SCM_HANDLE_TYPE::ServiceControlManager)
			throw WuStdException(L"This constructor can only be used for SCM handles.", __FILEW__, __LINE__);

		m_h = OpenSCManager(computerName.GetBuffer(), SERVICES_ACTIVE_DATABASE, desiredAccess);
		if (m_h == NULL)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		m_isValid = true;
	}

	ScmHandle::ScmHandle(const ScmHandle& scm, const WWuString& computerName, const WWuString& serviceName, DWORD desiredAccess, SCM_HANDLE_TYPE type)
	{
		if (type != SCM_HANDLE_TYPE::Service)
			throw WuStdException(L"This constructor can only be used for service handles.", __FILEW__, __LINE__);

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
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

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
}