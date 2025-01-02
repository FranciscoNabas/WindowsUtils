#pragma once
#pragma unmanaged

#include "StubUtils.h"
#include "../Engine/Containers.h"

namespace WindowsUtils
{
	enum class ContainersOperation
	{
		Expand,
		Compress,
	};
}

namespace WindowsUtils::Stubs
{
	using namespace WindowsUtils::Core;

	class Containers
	{
	public:
		template <ContainersOperation Operation>
		static typename std::enable_if<Operation == ContainersOperation::Expand, void>::type Dispatch(const WWuString& path, const WWuString& destination, const WuNativeContext* context)
		{
			_WU_START_TRY
				Core::Containers::ExpandCabinetFile(path, destination, context);
			_WU_MARSHAL_CATCH(context)
		}

		template <ContainersOperation Operation>
		static typename std::enable_if<Operation == ContainersOperation::Compress, void>::type Dispatch(AbstractPathTree& apt, const WWuString& destination, const WWuString& namePrefix,
			const int maxCabSize, const Core::CabinetCompressionType compressionType, const WuNativeContext* context)
		{
			_WU_START_TRY
				Core::Containers::CreateCabinetFile(apt, destination, namePrefix, compressionType, maxCabSize, context);
			_WU_MARSHAL_CATCH(context)
		}
	};
}