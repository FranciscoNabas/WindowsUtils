#pragma once
#pragma unmanaged

#include "StubUtils.h"
#include "../Engine/Services.h"

namespace WindowsUtils
{
	enum class ServicesOperation
	{
		RemoveService,
		GetSecurity,
		SetSecurity,
	};
}

namespace WindowsUtils::Stubs
{
	class Services
	{
	public:
		template <ServicesOperation Opr, std::enable_if_t<Opr == ServicesOperation::RemoveService, int> = 0, class... TArgs>
		static void Dispatch(const Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Services::RemoveService(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <ServicesOperation Opr, std::enable_if_t<Opr == ServicesOperation::GetSecurity, int> = 0, class... TArgs>
		static void Dispatch(const Core::UnmanagedExceptionMarshaler marshal, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Services::GetServiceSecurity(std::forward<TArgs>(args)...);
			_WU_MARSHAL_CATCH_PTR(marshal)
		}

		template <ServicesOperation Opr, std::enable_if_t<Opr == ServicesOperation::SetSecurity, int> = 0, class... TArgs>
		static void Dispatch(const Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Services::SetServiceSecurity(std::forward<TArgs>(args)...);
			_WU_MARSHAL_CATCH(context)
		}
	};
}