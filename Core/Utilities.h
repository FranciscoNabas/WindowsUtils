#pragma once
#pragma unmanaged

#include "pch.h"
#include "NtUtilities.h"

#define LOCFREEWCHECK(mem) if (NULL != mem) { LocalFree(mem); }
#define ALLCHECK(ptr) if (NULL == ptr) { return ERROR_NOT_ENOUGH_MEMORY; }
#define DWERRORCHECKV(result) if (ERROR_SUCCESS != result) { return result; }
#define DWERRORCHECKF(result) if (ERROR_SUCCESS != result) { return GetLastError(); }

#define SharedVecPtr(T) std::shared_ptr<std::vector<T>>
#define MakeVecPtr(T) std::make_shared<std::vector<T>>()

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
		}WU_RESOURCE_MESSAGE_TABLE, * PWU_RESOURCE_MESSAGE_TABLE;

		// Get-ObjectHandle
		typedef enum _VERSION_INFO_PROPERTY
		{
			FileDescription = 1U
			, ProductName = 2U
			, FileVersion = 3U
			, CompanyName = 4U

		}VERSION_INFO_PROPERTY;

		typedef struct _WU_OBJECT_HANDLE
		{
			LPWSTR		InputObject;								// Input object path. Helps tracking which handle belongs to which object, when querying multiple objects.
			DWORD		ProcessId;									// ID from the process owning the handle.
			LPWSTR		Name;										// Process image name. File base name.
			LPWSTR		ImagePath;									// Process image path.
			std::map<VERSION_INFO_PROPERTY, LPCWSTR> VersionInfo;	// Image version information.

			_WU_OBJECT_HANDLE() { }
			_WU_OBJECT_HANDLE(LPWSTR inpobj, LPWSTR name, DWORD pid, LPWSTR imgpath, std::map<VERSION_INFO_PROPERTY, LPCWSTR> verinfo)
				: InputObject(inpobj), Name(name), ProcessId(pid), ImagePath(imgpath), VersionInfo(verinfo) { }
		}WU_OBJECT_HANDLE, * PWU_OBJECT_HANDLE;

		/*=========================================
		==		 Function identification		 ==
		===========================================*/

		// Get-ObjectHandle
		DWORD GetProcessObjectHandle(std::vector<WU_OBJECT_HANDLE>& ppvecfho, std::vector<LPCWSTR>& reslist, BOOL closeHandle);

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
	* Memory management class.
	* Ok. I'm not a good C++ programmer, yet, and the fact that I'm not sure if memory is being deallocated properly freaks me out.
	* I tried this in a couple of ways, but helper functions seems to be the easiest to implement without too much overhead.
	* The initial idea was to overload the 'new' operator, but then I couldn't create objects with new inside the class.
	* For now, I'm proud of myself. Until I learn a little more and cringeness assumes.
	*/

	class _WuMemoryManagement
	{
	public:
		_WuMemoryManagement();

		PVOID Allocate(size_t size);
		VOID Free(PVOID block);

	private:
		BOOL IsRegistered(PVOID block);
		SharedVecPtr(PVOID) MemoryList;

	};

	DWORD GetProccesVersionInfo(LPWSTR& imagepath, Utilities::VERSION_INFO_PROPERTY& propname, LPWSTR& value);
	DWORD CloseExtProcessHandle(HANDLE& hsourceproc, LPCWSTR& objname);
	BOOL EndsWith(PWSTR& inputstr, LPCWSTR& comparestr);
	VOID PrintBufferW(LPWSTR& lpbuffer, WCHAR const* const format, ...);
	BOOL IsNullOrWhiteSpace(LPWSTR& lpinputstr);
	DWORD GetEnvVariableW(LPCWSTR& rlpcvarname, LPWSTR& rlpvalue);
		
}