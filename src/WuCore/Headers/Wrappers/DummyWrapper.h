#pragma once
#pragma unmanaged

#include "NativeException.h"
#include "../Engine/AccessControl.h"
#include "../Stubs/ProcessAndThreadStub.h"

#pragma managed

#include "WrapperBase.h"
#include "UtilitiesWrapper.h"

using namespace System;
using namespace System::Collections::Generic;

namespace WindowsUtils::Wrappers
{
#if defined(_DEBUG)
	/// <summary>
	/// The DummyWrapper class was designed to quickly test unmanaged functions
	/// calling from PowerShell. It is associated with the 'Start-DummyWork' Cmdlet.
	/// It contains a single method called 'DoWork()' that should be changed to call
	/// the function to be tested.
	/// </summary>
	public ref class DummyWrapper : public WrapperBase
	{
	public:
		DummyWrapper(Core::CmdletContextProxy^ context)
			: WrapperBase(context) { }

		void DoWork();
	};
#endif
}