#pragma once
#pragma unmanaged

#include "pch.h"
#include "NtUtilities.h"

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

		// Get-ObjectHandle
		typedef enum _VERSION_INFO_PROPERTY
		{
			FileDescription = 1U
			, ProductName = 2U
			, FileVersion = 3U
			, CompanyName = 4U

		} VERSION_INFO_PROPERTY;

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
		} WU_OBJECT_HANDLE, * PWU_OBJECT_HANDLE;

		// Expand-File
		typedef enum _ARCHIVE_FILE_TYPE
		{
			Cabinet

		} ARCHIVE_FILE_TYPE;

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

		// Expand-File
		DWORD ExpandArchiveFile(const LPSTR& lpszFileName, const LPSTR& lpszFilePath, const LPSTR& lpszDestination, ARCHIVE_FILE_TYPE fileType);
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

	DWORD GetProccesVersionInfo(LPWSTR& imagepath, Utilities::VERSION_INFO_PROPERTY& propname, LPWSTR& value);
	DWORD CloseExtProcessHandle(HANDLE& hsourceproc, LPCWSTR& objname);
	BOOL EndsWith(PWSTR& inputstr, LPCWSTR& comparestr);
	VOID PrintBufferW(LPWSTR& lpbuffer, WCHAR const* const format, ...);
	BOOL IsNullOrWhiteSpace(LPWSTR& lpinputstr);
	DWORD GetEnvVariableW(LPCWSTR& rlpcvarname, LPWSTR& rlpvalue);

	// Expand-File helper functions and objects.
	typedef struct _FDI_CABINET_INFO
	{
		LPSTR NextCabName;
		LPSTR NextDiskName;
		LPSTR Path;
		USHORT CurrentSetId;
		USHORT NextCabIndex;

	} FDI_CABINET_INFO, *PFDI_CABINET_INFO;

	typedef struct _FDI_PARTIAL_FILE
	{
		LPSTR FileName;
		LPSTR SourceCabName;
		LPSTR SourceDiskName;

	} FDI_PARTIAL_FILE, *PFDI_PARTIAL_FILE;

	typedef struct _FDI_COPY_FILE_INFO
	{
		DWORD Length;
		LPSTR FileName;
		USHORT Date;
		USHORT Time;
		USHORT Attributes;

	} FDI_COPY_FILE_INFO, *PFDI_COPY_FILE_INFO;

	typedef struct _FDI_CLOSE_FILE_INFO
	{
		LPSTR FileName;
		INT_PTR Handle;
		USHORT Date;
		USHORT Time;
		USHORT Attributes;
		BOOL Execute;

	} FDI_CLOSE_FILE_INFO, *PFDI_CLOSE_FILE_INFO;

	typedef struct _FDI_NEXT_CABINET_INFO
	{
		LPSTR NextCabName;

	} FDI_NEXT_CABINET_INFO, *PFDI_NEXT_CABINET_INFO;

	DWORD ExpandCabinetFile(const LPSTR& lpszFileName, const LPSTR& lpszFilePath, const LPSTR& lpszDestination);

	FNALLOC(FdiFnMemAloc);
	FNFREE(FdiFnMemFree);
	FNOPEN(FdiFnFileOpen);
	FNREAD(FdiFnFileRead);
	FNWRITE(FdiFnFileWrite);
	FNCLOSE(FdiFnFileClose);
	FNSEEK(FdiFnFileSeek);
	FNFDINOTIFY(FdiFnNotifyCallback);
}