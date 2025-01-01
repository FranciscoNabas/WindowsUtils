#pragma once
#pragma unmanaged

#include "StubUtils.h"
#include "../Engine/Utilities.h"
#include "../Support/Notification.h"

namespace WindowsUtils
{
	enum class UtilitiesOperation
	{
		GetResMesTable,
		SendClick,
		GetEnvVar,
	};
}

namespace WindowsUtils::Stubs
{
	class Utilities
	{
	public:
		template <UtilitiesOperation Opr, std::enable_if_t<Opr == UtilitiesOperation::GetResMesTable, int> = 0, class... TArgs>
		static void Dispatch(const Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Utilities::GetResourceMessageTable(std::forward<TArgs>(args)...);
			_WU_MARSHAL_CATCH(context)
		}

		template <UtilitiesOperation Opr, std::enable_if_t<Opr == UtilitiesOperation::SendClick, int> = 0>
		static void Dispatch(const Core::WuNativeContext* context)
		{
			_WU_START_TRY
				Core::Utilities::SendClick();
			_WU_MARSHAL_CATCH(context)
		}

		template <UtilitiesOperation Opr, std::enable_if_t<Opr == UtilitiesOperation::GetEnvVar, int> = 0, class... TArgs>
		static void Dispatch(const Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::Utilities::GetEnvVariable(std::forward<TArgs>(args)...);
			_WU_MARSHAL_CATCH(context)
		}
	};
}