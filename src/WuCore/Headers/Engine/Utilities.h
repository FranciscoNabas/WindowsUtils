#pragma once
#pragma unmanaged

#include "../Support/WuString.h"
#include "../Support/Expressions.h"
#include "../Support/WuException.h"

#include <Msi.h>
#include <MsiQuery.h>
#include <memory>

namespace WindowsUtils::Core
{
	/*
	*	~ Get-ResourceMessageTable
	*/

	typedef struct _WU_RESOURCE_MESSAGE_TABLE
	{
		DWORD	   Id;		 // Message ID.
		WWuString  Message;	 // Message text.

		_WU_RESOURCE_MESSAGE_TABLE();
		_WU_RESOURCE_MESSAGE_TABLE(DWORD id, LPWSTR message);

	} WU_RESOURCE_MESSAGE_TABLE, * PWU_RESOURCE_MESSAGE_TABLE;

	/*
	*	~ Main class
	*/
	
	class Utilities
	{
	public:
		//Get-ResourceMessageTable
		static void GetResourceMessageTable(std::vector<WU_RESOURCE_MESSAGE_TABLE>& messageTable, const WWuString& libName);

		// Send-Click
		static void SendClick();

		// Utilities
		static void GetEnvVariable(const WWuString& variableName, WWuString& value);
	};
}