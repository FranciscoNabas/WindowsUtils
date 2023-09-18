#pragma once


#include "Common.h"
#include "Wrapper.h"
#include "Notification.h"

/////////////////////////////////////////////////////////
//
//	~ Lab
//
// This header file will contain function definitions
// So we can test certain features easily without
// having to have an entire different PowerShell mo-
// dule. The Cmdlets at the end won't be included in
// the module manifest, but will be accessible through
// the DLL.
//
/////////////////////////////////////////////////////////

namespace WindowsUtils::Core
{
#pragma unmanaged
	// Here we define our native functions.
	extern "C" public class __declspec(dllexport) NativeLab
	{
		typedef struct _LAB_STRUCT
		{
			DWORD Id;
			WWuString String;
		} LAB_STRUCT;

	public:
		void WriteWWuStringToPowerShellOutput(WuNativeContext* context);
		void WriteStructToPowerShellOutput(WuNativeContext* context);
		void WriteVectorToPowerShellOutput(WuNativeContext* context);
	};

#pragma managed
	// Here we define our wrappers.
	using namespace System;

	public ref class LabWrapper
	{
	public:
		void WriteWWuStringAsObject(CmdletContextBase^ context);
		void WriteStructAsObject(CmdletContextBase^ context);
		void WriteVectorAsObject(CmdletContextBase^ context);

	private:
		NativeLab* labPtr;
	};
}
