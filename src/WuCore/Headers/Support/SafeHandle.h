#pragma once
#pragma unmanaged

#include "String.h"

namespace WindowsUtils::Core
{
	/*
	*	~ SCM handles
	*/

	enum class ScmHandleType
	{
		ServiceControlManager,
		Service
	};

	class ScmHandle
	{
	private:
		SC_HANDLE m_h;
		bool m_isValid;

	public:
		ScmHandle();
		ScmHandle(const WWuString& computerName, DWORD desiredAccess, ScmHandleType type = ScmHandleType::ServiceControlManager);
		ScmHandle(const ScmHandle& scm, const WWuString& computerName, const WWuString& serviceName, DWORD desiredAccess, ScmHandleType type = ScmHandleType::Service);
		~ScmHandle();

		const SC_HANDLE get() const;
	};

	/*
	*	~ File handle
	*/

	class FileHandle
	{
	private:
		HANDLE m_hFile;
		bool m_isValid;

	public:
		FileHandle();
		FileHandle(const WWuString& fileName, DWORD desiredAccess, DWORD shareMode, LPSECURITY_ATTRIBUTES secAttr, DWORD disposition, DWORD flagsAndAttr, HANDLE hTemplate);
		~FileHandle();

		const HANDLE get() const;
	};

	/*
	*	~ Module handle
	*/

	class ModuleHandle
	{
	private:
		HMODULE m_hModule;
		bool m_isLoaded;
		bool m_isValid;
		int m_error;

	public:
		ModuleHandle();
		ModuleHandle(LPCWSTR name, bool forceLoad = false);
		~ModuleHandle();

		const HMODULE get() const;
		const bool IsValid() const;
		const int Error() const;
	};

	/*
	*	~ Process handle
	*/

	class ProcessHandle
	{
	private:
		HANDLE m_hProcess;
		bool m_isValid;
		int m_error;

	public:
		ProcessHandle();
		ProcessHandle(DWORD desiredAccess, BOOL inherit, DWORD processId);
		~ProcessHandle();

		const HANDLE get() const;
		const bool IsValid() const;
		const int Error() const;
	};
}