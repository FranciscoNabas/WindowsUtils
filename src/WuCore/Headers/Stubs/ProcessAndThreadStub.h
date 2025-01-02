#pragma once
#pragma unmanaged

#include "StubUtils.h"
#include "../Support/WuList.h"
#include "../Engine/ProcessAndThread.h"

namespace WindowsUtils
{
	enum class PatOperation
	{
		GetHandle,
		RunAs,
		GetModuleInfo,
		ListHandleInfo,
		Suspend,
		Resume,
		ListProcess,
	};
}

namespace WindowsUtils::Stubs
{
	using namespace WindowsUtils::Core;

	class ProcessAndThread
	{
	public:
		template <PatOperation Opr, std::enable_if_t<Opr == PatOperation::GetHandle, int> = 0, class... TArgs>
		static void Dispatch(const WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::ProcessAndThread::GetProcessObjectHandle(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <PatOperation Opr, std::enable_if_t<Opr == PatOperation::RunAs, int> = 0, class... TArgs>
		static void Dispatch(const WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::ProcessAndThread::RunAs(std::forward<TArgs>(args)...);
			_WU_MARSHAL_CATCH(context)
		}

		template <PatOperation Opr, std::enable_if_t<Opr == PatOperation::GetModuleInfo, int> = 0, class... TArgs>
		static void Dispatch(const WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::ProcessAndThread::GetProcessLoadedModuleInformation(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <PatOperation Opr, std::enable_if_t<Opr == PatOperation::ListHandleInfo, int> = 0, class... TArgs>
		static void Dispatch(const WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::NtUtilities::ListProcessHandleInformation(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <PatOperation Opr, std::enable_if_t<Opr == PatOperation::Suspend, int> = 0, class... TArgs>
		static void Dispatch(const WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::ProcessAndThread::SuspendProcess(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <PatOperation Opr, std::enable_if_t<Opr == PatOperation::Resume, int> = 0, class... TArgs>
		static void Dispatch(const WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				Core::ProcessAndThread::ResumeProcess(std::forward<TArgs>(args)..., context);
			_WU_MARSHAL_CATCH(context)
		}

		template <PatOperation Opr, std::enable_if_t<Opr == PatOperation::ListProcess, int> = 0, class... TArgs>
		static void Dispatch(const WuNativeContext* context, TArgs&&... args)
		{
			_WU_START_TRY
				NtUtilities::ListRunningProcesses(std::forward<TArgs>(args)...);
			_WU_MARSHAL_CATCH(context)
		}
	};
}