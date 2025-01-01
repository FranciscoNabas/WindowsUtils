#pragma once
#pragma unmanaged

#include "StubUtils.h"
#include "../Engine/Registry.h"
#include "../Support/Notification.h"
#include "../Support/ScopedBuffer.h"

namespace WindowsUtils
{
	enum class RegistryOperation
	{
		GetValue,
		GetValueList,
		GetSubkeys,
		GetValueNames,
		GetPathFromNtPath,
	};
}

namespace WindowsUtils::Stubs
{
	class Registry
	{
	public:
		template <RegistryOperation Opr, std::enable_if_t<Opr == RegistryOperation::GetValue, int> = 0, class... TArgs>
		static Core::ScopedBuffer Dispatch(const Core::UnmanagedExceptionMarshaler marshal, TArgs&&... args)
		{
			Core::ScopedBuffer res;
			_WU_START_TRY
				res = Core::Registry::GetRegistryKeyValue(std::forward<TArgs>(args)...);
			_WU_MARSHAL_CATCH_PTR(marshal)
		
			return res;
		}

		template <RegistryOperation Opr, std::enable_if_t<Opr == RegistryOperation::GetValueList, int> = 0, class... TArgs>
		static Core::ScopedBuffer Dispatch(const Core::UnmanagedExceptionMarshaler marshal, TArgs&&... args)
		{
			Core::ScopedBuffer res;
			_WU_START_TRY
				res = Core::Registry::GetRegistryKeyValueList(std::forward<TArgs>(args)...);
			_WU_MARSHAL_CATCH_PTR(marshal)
		
			return res;
		}

		template <RegistryOperation Opr, std::enable_if_t<Opr == RegistryOperation::GetSubkeys, int> = 0, class... TArgs>
		static void Dispatch(const Core::UnmanagedExceptionMarshaler marshal, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Registry::GetRegistrySubkeyNames(std::forward<TArgs>(args)...);
			_WU_MARSHAL_CATCH_PTR(marshal)
		}

		template <RegistryOperation Opr, std::enable_if_t<Opr == RegistryOperation::GetValueNames, int> = 0, class... TArgs>
		static void Dispatch(const Core::UnmanagedExceptionMarshaler marshal, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Registry::GetRegistryKeyValueNames(std::forward<TArgs>(args)...);
			_WU_MARSHAL_CATCH_PTR(marshal)
		}

		template <RegistryOperation Opr, std::enable_if_t<Opr == RegistryOperation::GetPathFromNtPath, int> = 0, class... TArgs>
		static void Dispatch(const Core::UnmanagedExceptionMarshaler marshal, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Registry::GetRegistryPathFromNtPath(std::forward<TArgs>(args)...);
			_WU_MARSHAL_CATCH_PTR(marshal)
		}
	};
}