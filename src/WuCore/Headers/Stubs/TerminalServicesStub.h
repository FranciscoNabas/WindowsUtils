#pragma once
#pragma unmanaged

#include "StubUtils.h"
#include "../Engine/TerminalServices.h"
#include "../Support/Notification.h"

namespace WindowsUtils
{
	enum class WtsOperation
	{
		EnumSessions,
		SendMessage,
		Disconnect,
	};
}

namespace WindowsUtils::Stubs
{
	class TerminalServices
	{
	public:
		template <WtsOperation Ops, std::enable_if_t<Ops == WtsOperation::EnumSessions, int> = 0, class... TArgs>
		static void Dispatch(const Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::TerminalServices::GetEnumeratedSession(std::forward<TArgs>(args)...);
			_WU_MARSHAL_CATCH(context)
		}

		template <WtsOperation Ops, std::enable_if_t<Ops == WtsOperation::SendMessage, int> = 0, class... TArgs>
		static void Dispatch(const Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::TerminalServices::SendMessage(std::forward<TArgs>(args)...);
			_WU_MARSHAL_CATCH(context)
		}

		template <WtsOperation Ops, std::enable_if_t<Ops == WtsOperation::Disconnect, int> = 0, class... TArgs>
		static void Dispatch(const Core::WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::TerminalServices::DisconnectSession(std::forward<TArgs>(args)...);
			_WU_MARSHAL_CATCH(context)
		}
	};
}