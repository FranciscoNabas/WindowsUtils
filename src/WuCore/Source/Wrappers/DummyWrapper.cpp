#pragma unmanaged

#include "../../pch.h"

#pragma managed

#include "../../Headers/Wrappers/DummyWrapper.h"

namespace WindowsUtils::Wrappers
{
#if defined(_DEBUG)
	void DummyWrapper::DoWork() { }
#endif
}