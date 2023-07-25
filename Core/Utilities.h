#pragma once
#pragma unmanaged

#include "String.h"
#include "Expressions.h"

#define LOCFREEWCHECK(mem) if (NULL != mem) { LocalFree(mem); }
#define ALLCHECK(ptr) if (NULL == ptr) { return ERROR_NOT_ENOUGH_MEMORY; }

namespace WindowsUtils::Core
{
	extern "C" public class __declspec(dllexport) Utilities
	{
	public:
		/*========================================
		==		Unmanaged object definition		==
		==========================================*/

		// Get-ResourceMessageTable
		typedef struct _WU_RESOURCE_MESSAGE_TABLE
		{
			DWORD	 Id;			// Message ID.
			WWuString Message;		// Message text.

			_WU_RESOURCE_MESSAGE_TABLE() { }
			_WU_RESOURCE_MESSAGE_TABLE(DWORD id, LPWSTR message)
				: Id(id), Message(message)
			{ }

			~_WU_RESOURCE_MESSAGE_TABLE() { }

		} WU_RESOURCE_MESSAGE_TABLE, *PWU_RESOURCE_MESSAGE_TABLE;

		/*=========================================
		==		 Function identification		 ==
		===========================================*/

		//Get-ResourceMessageTable
		DWORD GetResourceMessageTable(wuvector<WU_RESOURCE_MESSAGE_TABLE>* messageTable, const WWuString& libName);

		// Get-LastWin32Error
		DWORD GetFormattedWin32Error(WWuString& errorMessage);
		
		// Get-FormattedError
		DWORD GetFormattedError(DWORD errorCode, WWuString& errorMessage);

		// Get-MsiProperties
		DWORD GetMsiProperties(wumap<WWuString, WWuString>* propertyMap, const WWuString& fileName);
		DWORD GetMsiExtendedError(WWuString& errorMessage);

		// Send-Click
		DWORD SendClick();
	};

	/*
	* This helper structure us used on cases where loading a module is not critical.
	*/
	typedef struct _LOAD_MODULE_ERROR_INFO
	{
		BOOL	IsLoaded;
		DWORD	ErrorCode;

		_LOAD_MODULE_ERROR_INFO() : IsLoaded(TRUE), ErrorCode(ERROR_SUCCESS) { }

	}LOAD_MODULE_ERROR_INFO, * PLOAD_MODULE_ERROR_INFO;

	DWORD GetEnvVariable(const WWuString& variableName, WWuString& value);
}