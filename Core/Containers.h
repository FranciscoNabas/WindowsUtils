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
		DWORD ExpandArchiveFile(const WuString& lpszFileName, const WuString& lpszFilePath, const WuString& lpszDestination, ARCHIVE_FILE_TYPE fileType, Notification::PNATIVE_CONTEXT context);
	};

	// Expand-File helper functions and objects.
	typedef class _FDI_NOTIFICATION
	{
	public:
		// Information regarding the current file.
		WuString Name;

		// Information regarding the current cabinet.
		WuString CabinetName;

		// Related with progress notification.
		ULONGLONG TotalUncompressedSize;
		ULONGLONG ProcessedBytes;

		void WriteProgress(Notification::PNATIVE_CONTEXT context);
		static _FDI_NOTIFICATION& GetNotifier();

		~_FDI_NOTIFICATION();

	private:
		_FDI_NOTIFICATION();

		Notification::PMAPPED_PROGRESS_DATA _progressData;

	public:
		_FDI_NOTIFICATION(const _FDI_NOTIFICATION&) = delete;
		void operator=(const _FDI_NOTIFICATION&) = delete;

	} FDI_NOTIFICATION, *PFDI_NOTIFICATION;

	DWORD ExpandCabinetFile(const WuString& lpszFileName, const WuString& lpszFilePath, const WuString& lpszDestination, Notification::PNATIVE_CONTEXT context);
	
	
	template <class T>
	DWORD GetNumberDigitCount(T number);

	FNALLOC(FdiFnMemAloc);
	FNFREE(FdiFnMemFree);
	FNOPEN(FdiFnFileOpen);
	FNREAD(FdiFnFileRead);
	FNWRITE(FdiFnFileWrite);
	FNCLOSE(FdiFnFileClose);
	FNSEEK(FdiFnFileSeek);
	FNFDINOTIFY(FdiFnNotifyCallback);
}