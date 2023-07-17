#pragma once
#pragma unmanaged

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
			DWORD	Id;			// Message ID.
			LPWSTR	Message;	// Message text.

			_WU_RESOURCE_MESSAGE_TABLE() { }
			_WU_RESOURCE_MESSAGE_TABLE(DWORD id, LPWSTR message) : Id(id), Message(message) { }
		} WU_RESOURCE_MESSAGE_TABLE, * PWU_RESOURCE_MESSAGE_TABLE;

		/*=========================================
		==		 Function identification		 ==
		===========================================*/

		//Get-ResourceMessageTable
		DWORD GetResourceMessageTable(std::vector<WU_RESOURCE_MESSAGE_TABLE>& rvecresmestb, LPWSTR& lplibName);

		// Get-LastWin32Error
		DWORD GetFormattedWin32Error(LPWSTR& rlperrormess);
		// Get-FormattedError
		DWORD GetFormattedError(DWORD dwerrorcode, LPWSTR& rlperrormess);

		// Get-MsiProperties
		DWORD GetMsiProperties(std::map<LPWSTR, LPWSTR>& ppmapout, LPWSTR& fileName);
		DWORD GetMsiExtendedError(LPWSTR& lperrormessage);

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

	/*
	* Memory management class using the singleton design pattern.
	* Reference: https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
	*/

	class WuMemoryManagement
	{
	public:
		static WuMemoryManagement& GetManager();
		PVOID Allocate(size_t size);
		VOID Free(PVOID block);

	private:
		BOOL IsRegistered(PVOID block);
		std::vector<PVOID> MemoryList;

		WuMemoryManagement() { }
		~WuMemoryManagement();

	public:
		WuMemoryManagement(WuMemoryManagement const&) = delete;
		void operator=(WuMemoryManagement const&) = delete;
	};

	BOOL EndsWith(const LPWSTR& inputstr, const LPWSTR& comparestr);
	VOID PrintBufferW(LPWSTR& lpbuffer, const WCHAR* const format, ...);
	BOOL IsNullOrWhiteSpace(LPWSTR& lpinputstr);
	DWORD GetEnvVariable(LPCWSTR& rlpcvarname, LPWSTR& rlpvalue);
}