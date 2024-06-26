#pragma once
#pragma unmanaged

#include "../Support/String.h"
#include "../Support/Expressions.h"

namespace WindowsUtils::Core
{
	/*
	*	~ Get-ResourceMessageTable
	*/

	typedef struct _WU_RESOURCE_MESSAGE_TABLE
	{
		DWORD	 Id;			// Message ID.
		WWuString Message;		// Message text.

		_WU_RESOURCE_MESSAGE_TABLE() { }
		_WU_RESOURCE_MESSAGE_TABLE(DWORD id, LPWSTR message)
			: Id(id), Message(message)
		{ }

		~_WU_RESOURCE_MESSAGE_TABLE() { }

	} WU_RESOURCE_MESSAGE_TABLE, * PWU_RESOURCE_MESSAGE_TABLE;

	/*
	*	~ Main class
	*/
	
	extern "C" public class __declspec(dllexport) Utilities
	{
	public:
		//Get-ResourceMessageTable
		void GetResourceMessageTable(wuvector<WU_RESOURCE_MESSAGE_TABLE>& messageTable, const WWuString& libName);

		// Get-LastWin32Error
		void GetFormattedWin32Error(WWuString& errorMessage);

		// Send-Click
		void SendClick();
	};

	/*
	*	~ Utility functions
	*/
	
	void GetEnvVariable(const WWuString& variableName, WWuString& value);
}