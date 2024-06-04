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
	* 
	*	This will attempt to get a handle to the module with 'GetModuleHandle' first. This does
	*	not increment the reference count for the module, and thus we won't call 'FreeLibrary'.
	*	If the call fails we attempt 'LoadLibrary', which if succeeds increments the reference count
	*	for the module, and 'FreeLibrary' needs to be called.
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
		ModuleHandle(const WWuString& name, bool forceLoad = false);
		~ModuleHandle();

		const HMODULE get() const;
		const bool IsValid() const;
		const int Error() const;

		operator HMODULE() { return get(); }
	};

	/*
	*	~ Process handle
	*/

	class ProcessHandle
	{
	private:
		HANDLE m_hProcess;

	public:
		ProcessHandle();
		ProcessHandle(DWORD desiredAccess, BOOL inherit, DWORD processId);
		~ProcessHandle();

		const HANDLE get() const;

		void operator =(HANDLE other) { if (m_hProcess) CloseHandle(m_hProcess); m_hProcess = other; }
		operator HANDLE() { return get(); }
		operator HANDLE() const { return get(); }
		LPHANDLE operator &() { if (m_hProcess) CloseHandle(m_hProcess); m_hProcess = 0; return &m_hProcess; }
	};

	/*
	*	~ Object handle
	* 
	*	Generic object handle that is closed by 'CloseHandle'.
	*/

	class ObjectHandle
	{
	private:
		HANDLE m_hObject;

	public:
		ObjectHandle();
		ObjectHandle(HANDLE hObject);
		~ObjectHandle();

		const HANDLE get() const;

		void operator =(HANDLE hOther) { if (m_hObject) CloseHandle(m_hObject); m_hObject = hOther; }
		operator HANDLE() { return get(); }
		operator HANDLE() const { return get(); }
		LPHANDLE operator &() { if (m_hObject) CloseHandle(m_hObject); m_hObject = 0; return &m_hObject; }
	};
}