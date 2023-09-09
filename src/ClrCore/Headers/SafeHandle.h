#pragma once
#pragma unmanaged

#include "String.h"
#include "Common.h"

///////////////////////////////////////////////////////////
//
//	~ Safe handles
//
//	These classes exists as wrappers to native OS handles
//	so they can be freed when they go out of scope.
//
///////////////////////////////////////////////////////////

namespace WindowsUtils::Core
{
	typedef enum _SCM_HANDLE_TYPE
	{
		ServiceControlManager,
		Service
	} SCM_HANDLE_TYPE;

	class ScmHandle
	{
	private:
		SC_HANDLE m_h;
		bool m_isValid;

	public:
		ScmHandle();
		ScmHandle(const WWuString& computerName, DWORD desiredAccess, SCM_HANDLE_TYPE type = SCM_HANDLE_TYPE::ServiceControlManager);
		ScmHandle(const ScmHandle& scm, const WWuString& computerName, const WWuString& serviceName, DWORD desiredAccess, SCM_HANDLE_TYPE type = SCM_HANDLE_TYPE::Service);
		~ScmHandle();

		const SC_HANDLE get() const;
	};
}