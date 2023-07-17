#pragma once
#pragma unmanaged

#include "Notification.h"

namespace WindowsUtils::Core
{
	extern "C" public class __declspec(dllexport) Containers
	{
	public:
		/*========================================
		==		Unmanaged object definition		==
		==========================================*/

		// Expand-File
		typedef enum _ARCHIVE_FILE_TYPE
		{
			Cabinet

		} ARCHIVE_FILE_TYPE;

		/*=========================================
		==		 Function identification		 ==
		===========================================*/

		// Expand-File
		DWORD ExpandArchiveFile(const LPSTR& lpszFileName, const LPSTR& lpszFilePath, const LPSTR& lpszDestination, ARCHIVE_FILE_TYPE fileType);
	};

	// Expand-File helper functions and objects.
	typedef class _FDI_NOTIFICATION
	{
	public:
		// Information regarding the current file.
		LPWSTR Path;
		LPWSTR Name;
		DWORD Size;

		// Information regarding the current cabinet.
		DWORD CabIndex;
		LPWSTR NextCabinet;
		LPWSTR NextDisk;
		PFDICABINETINFO CabinetInfo;

		// Related with progress notification.
		Notification::PNATIVE_CONTEXT Context;
		void WriteProgress()
		{
			WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();
			auto progData = (Notification::PMAPPED_PROGRESS_DATA)MemoryManager.Allocate(sizeof(Notification::MAPPED_PROGRESS_DATA));

			size_t pathLen = wcslen(Path) + 1;
			size_t nameLen = wcslen(Name) + 1;

			
			progData->Activity = L"Expanding cabinet";

		}
		
		static _FDI_NOTIFICATION& GetNotifier()
		{
			static _FDI_NOTIFICATION instance;
			return instance;
		}

		~_FDI_NOTIFICATION()
		{
			MemoryManager.Free(_progressData);
			MemoryManager.Free(CabinetInfo);

			// If those were not freed outside we free them here.
			MemoryManager.Free(Path);
			MemoryManager.Free(Name);
			MemoryManager.Free(NextCabinet);
			MemoryManager.Free(NextDisk);
		}

	private:
		_FDI_NOTIFICATION()
			: Path(NULL), Name(NULL), CabIndex(0), NextCabinet(NULL), NextDisk(NULL), CabinetInfo(NULL)
		{
			_progressData = (Notification::PMAPPED_PROGRESS_DATA)MemoryManager.Allocate(sizeof(Notification::MAPPED_PROGRESS_DATA));
			CabinetInfo = (PFDICABINETINFO)MemoryManager.Allocate(sizeof(FDICABINETINFO));
		}

		Notification::PMAPPED_PROGRESS_DATA _progressData;
		WuMemoryManagement& MemoryManager = WuMemoryManagement::GetManager();

	public:
		_FDI_NOTIFICATION(const _FDI_NOTIFICATION&) = delete;
		void operator=(const _FDI_NOTIFICATION&) = delete;

	} FDI_NOTIFICATION, *PFDI_NOTIFICATION;

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