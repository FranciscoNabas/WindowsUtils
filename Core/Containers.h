#pragma once
#pragma unmanaged

#include "Notification.h"
#include "IO.h"

#define cfhdrPREV_CABINET       0x0001
#define cfhdrNEXT_CABINET       0x0002
#define cfhdrRESERVE_PRESENT    0x0004

namespace WindowsUtils::Core {
	class CABINET_PROCESSING_INFO {
	public:
		WWuString Name;
		WWuString Path;
		WORD CabinetIndex;
		WORD FileCount;
		DWORD CFFileOffset;
		WuString PreviousCabinet;
		WuString NextCabinet;
	};

	extern "C" public class __declspec(dllexport) Containers {
	public:
		/*========================================
		==		Unmanaged object definition		==
		==========================================*/

		// Expand-File
		typedef enum _ARCHIVE_FILE_TYPE {
			Cabinet

		} ARCHIVE_FILE_TYPE;

		/*=========================================
		==		 Function identification		 ==
		===========================================*/

		// Expand-File
		DWORD ExpandArchiveFile(WuString& lpszFileName, WuString& lpszFilePath, const WuString& lpszDestination, ARCHIVE_FILE_TYPE fileType, Notification::PNATIVE_CONTEXT context);
	};

	// Expand-File helper functions and objects.
	class WuCabinet {
	public:
		static WuCabinet* GetWuCabinet(const WWuString& filePath, Notification::PNATIVE_CONTEXT context);
		static WuCabinet* GetWuCabinet();
		
		DWORD ExpandCabinetFile(const WuString& destination, Notification::PNATIVE_CONTEXT context);
		INT_PTR NotifyCallback(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin);

		~WuCabinet() { }

		WuCabinet(WuCabinet const&) = delete;
		void operator=(WuCabinet const&) = delete;

	private:
		WuCabinet(const WWuString& filePath, Notification::PNATIVE_CONTEXT context);
		static WuCabinet* _instance;

		WuString _filePath;
		WuString _directory;
		WuString _destination;
		FDI_PROGRESS _progressInfo;
		__uint64 _totalUncompressedSize;
		Notification::PNATIVE_CONTEXT _context;
		wuvector<CABINET_PROCESSING_INFO> _processedCabinet;

		void GetCabinetTotalUncompressedSize(const WWuString& filePath);
		void GetCabinetInformation(const MemoryMappedFile& mappedFile, CABINET_PROCESSING_INFO* cabInfo);
		_NODISCARD CABINET_PROCESSING_INFO* GetInfoByPath(const WWuString& filePath);

		void OnCabinetInfo(const WuString& cabName);
		void OnCopyFile(PFDINOTIFICATION cabInfo);
		void OnCloseFileInfo(PFDINOTIFICATION cabInfo);
	};

	typedef struct _FDI_PROGRESS {
		DWORD CabinetSetCount;
		DWORD CompletedCabinetCount;
		WuString CabinetName;
		WuString CurrentFile;
		DWORD CurrentUncompressedSize;
		__uint64 CompletedSize;
		__uint64 TotalUncompressedSize;

		_FDI_PROGRESS();
		~_FDI_PROGRESS();

		void Notify(Notification::PNATIVE_CONTEXT context);

	} FDI_PROGRESS, *PFDI_PROGRESS;

	static void* __cdecl FdiAlloc(UINT cb) {
		return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
	}

	static void __cdecl FdiFree(void* pv) {
		HeapFree(GetProcessHeap(), NULL, pv);
	}

	static INT_PTR __cdecl FdiOpen(char* pszFile, int oflag, int pmode) {
		int fileHandle;

		if (oflag & _O_CREAT)
			SetFileAttributesA(pszFile, FILE_ATTRIBUTE_NORMAL);

		_sopen_s(&fileHandle, pszFile, oflag, _SH_DENYNO, pmode);

		return fileHandle;
	}

	static UINT __cdecl FdiRead(INT_PTR hf, void* pv, UINT cb) {
		return _read(static_cast<int>(hf), pv, cb);
	}

	static UINT __cdecl FdiWrite(INT_PTR hf, void* pv, UINT cb) {
		return _write(static_cast<int>(hf), pv, cb);
	}

	static int __cdecl FdiClose(INT_PTR hf) {
		return _close(static_cast<int>(hf));
	}

	static long __cdecl FdiSeek(INT_PTR hf, long dist, int seektype) {
		return _lseek(static_cast<int>(hf), dist, seektype);
	}

	static INT_PTR __cdecl FdiNotify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin) {
		WuCabinet* cabInfo = WuCabinet::GetWuCabinet();
		cabInfo->NotifyCallback(fdint, pfdin);
	}

	template <class T>
	DWORD GetNumberDigitCount(T number);
}