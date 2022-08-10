#pragma once

#include "Unmanaged.h"
#include <vcclr.h>

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;

namespace Wrapper {

	public ref class Managed
	{
	public:
		Unmanaged* ptr;

		int strucSize = sizeof(Unmanaged::SessionEnumOutput);

		ref class wSessionEnumOutput
		{
		public:
			String^ UserName;
			String^ SessionName;
			UINT SessionState;
		};

		List<wSessionEnumOutput^>^ GetEnumeratedSession(String^ computerName, bool onlyActive, bool excludeSystemSessions)
		{
			DWORD rCount;
			List<wSessionEnumOutput^>^ output = gcnew List<wSessionEnumOutput^>();
			Unmanaged::PSessionEnumOutput thisResult = ptr->GetEnumeratedSession(&rCount, (LPWSTR)&computerName, onlyActive, excludeSystemSessions);
			Unmanaged::PSessionEnumOutput* ppResult = &thisResult;
			for (DWORD i = 0; i < rCount; i++)
			{
				wSessionEnumOutput^ inner = gcnew wSessionEnumOutput();
				inner->UserName = Marshal::PtrToStringUni((IntPtr)ppResult[i]->UserName);
				inner->SessionName = Marshal::PtrToStringUni((IntPtr)ppResult[i]->SessionName);
				
				DWORD sState = (DWORD)ppResult[i]->SessionState;
				inner->SessionState = (UINT)sState;

				output->Add(inner);
			}

			
			return output;
		};
	};
}
