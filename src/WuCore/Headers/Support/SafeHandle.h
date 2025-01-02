#pragma once
#pragma unmanaged

#include "WuString.h"
#include "Assertion.h"
#include "WuException.h"
#include "Nt/NtFunctions.h"

namespace WindowsUtils::Core
{
	/*
	*	~ SCM handles ~
	*/

	enum class ScmHandleType
	{
		ServiceControlManager,
		Service,
	};

	class ScmHandle
	{
	public:
		ScmHandle();
		ScmHandle(const ScmHandle& other);

		ScmHandle(const WWuString& computerName, DWORD desiredAccess, ScmHandleType type = ScmHandleType::ServiceControlManager);
		ScmHandle(const ScmHandle& scm, const WWuString& computerName, const WWuString& serviceName, DWORD desiredAccess, ScmHandleType type = ScmHandleType::Service);
		~ScmHandle();

		const SC_HANDLE Get() const;

	private:
		bool m_ownsHandle;
		SC_HANDLE m_handle;
	};


	/*
	*	~ File handle ~
	*/

	class FileHandle
	{
	public:
		FileHandle();
		FileHandle(const FileHandle& other);

		FileHandle(const WWuString& fileName, DWORD desiredAccess, DWORD shareMode, LPSECURITY_ATTRIBUTES secAttr, DWORD disposition, DWORD flagsAndAttr, HANDLE hTemplate);
		~FileHandle();

		const HANDLE Get() const;

		PHANDLE operator &();

	private:
		bool m_ownsHandle;
		HANDLE m_handle;
	};


	/*
	*	~ Module handle ~
	* 
	*	This will attempt to get a handle to the module with 'GetModuleHandle' first. This does
	*	not increment the reference count for the module, and thus we won't call 'FreeLibrary'.
	*	If the call fails we attempt 'LoadLibrary', which if succeeds increments the reference count
	*	for the module, and 'FreeLibrary' needs to be called.
	*/

	class ModuleHandle
	{
	public:
		ModuleHandle();
		ModuleHandle(const ModuleHandle& other);

		ModuleHandle(const WWuString& name, bool forceLoad = false);
		~ModuleHandle();

		const HMODULE Get() const;
		const bool IsValid() const;
		const int Error() const;

	private:
		int m_error;
		bool m_isValid;
		bool m_ownsHandle;
		HMODULE m_handle;
	};


	/*
	*	~ Process handle ~
	*/

	class ProcessHandle
	{
	public:
		ProcessHandle();
		ProcessHandle(HANDLE handle);
		ProcessHandle(const ProcessHandle& other);

		ProcessHandle(DWORD processId, ACCESS_MASK desiredAccess, bool inherit);
		~ProcessHandle();

		const HANDLE Get() const;
		const bool IsValid() const;

		PHANDLE operator &();

	private:
		bool m_ownsHandle;
		HANDLE m_handle;

		static HANDLE WuOpenProcess(DWORD processId, ACCESS_MASK desiredAccess, bool inherit);
	};


	/*
	*	~ Object handle ~
	* 
	*	Generic object handle that is closed by 'CloseHandle'.
	*/

	class ObjectHandle
	{
	public:
		ObjectHandle();
		ObjectHandle(const ObjectHandle& other);

		ObjectHandle(HANDLE hObject, bool ownsHandle);
		~ObjectHandle();

		const HANDLE Get() const;

		PHANDLE operator &();

	private:
		bool m_ownsHandle;
		HANDLE m_handle;
	};


	/*
	*	~ MIB table pointer ~
	* 
	*	Some functions from the Management Information Base (MIB) API set allocate
	*	memory themselves. This memory needs to be freed using 'FreeMibTable'.
	*	It stores a pointer to the type specified in the template.
	*/

	template <typename T>
	class MibTablePointer
	{
	public:
		MibTablePointer() { m_tablePtr = nullptr; }
		~MibTablePointer() { if (m_tablePtr) { FreeMibTable(m_tablePtr); } }

		T* operator ->() { return m_tablePtr; }
		T** operator &() { if (m_tablePtr) FreeMibTable(m_tablePtr); return &m_tablePtr; }

	private:
		T* m_tablePtr;
	};

	/*
	*	~ Registry handle ~
	*/

	class RegistryHandle
	{
	public:
		RegistryHandle();
		RegistryHandle(const RegistryHandle& other);

		RegistryHandle(HKEY handle, bool ownsHandle);
		~RegistryHandle();

		const HKEY Get() const;

		PHKEY operator &();

	private:
		bool m_ownsHandle;
		HKEY m_handle;
	};
}