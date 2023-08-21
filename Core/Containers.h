#pragma once
#pragma unmanaged

#include "Notification.h"
#include "IO.h"

#define cfhdrPREV_CABINET       0x0001
#define cfhdrNEXT_CABINET       0x0002
#define cfhdrRESERVE_PRESENT    0x0004

namespace WindowsUtils::Core {
	typedef struct _FDI_PROGRESS {
		DWORD CabinetSetCount;
		DWORD CompletedCabinetCount;
		WWuString CabinetName;
		WWuString CurrentFile;
		DWORD CurrentUncompressedSize;
		__uint64 CompletedSize;
		__uint64 TotalUncompressedSize;

		_FDI_PROGRESS();
		~_FDI_PROGRESS();

		void Notify(Notification::PNATIVE_CONTEXT context);

	} FDI_PROGRESS, * PFDI_PROGRESS;

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
		WuResult ExpandArchiveFile(const WWuString& filePath, const WWuString& destination, ARCHIVE_FILE_TYPE fileType, Notification::PNATIVE_CONTEXT context);
	};

	// The original ideia was using this object as a singleton, but due
	// the nature of the PowerShell process, this object would transcend
	// across cmdlet calls.
	// In this implementation we create it at function call, and pass it
	// around.
	// This way the object has a well defined lifetime.
	
	class WuCabinet {
	public:
		const bool IsCurrentUnicode() const;
		WuResult ExpandCabinetFile(const WWuString& destination);
		INT_PTR NotifyCallback(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin);

		WuCabinet(const WWuString& filePath, Notification::PNATIVE_CONTEXT context);
		~WuCabinet();

		WuCabinet(WuCabinet const&) = delete;
		void operator=(WuCabinet const&) = delete;

	private:
		static WuCabinet* _instance;

		bool _isUnicode;
		WWuString _filePath;
		WuString _directory;
		WWuString _destination;
		WWuString _nextCabinet;
		FDI_PROGRESS _progressInfo;
		__uint64 _totalUncompressedSize;
		Notification::PNATIVE_CONTEXT _context;
		wuvector<CABINET_PROCESSING_INFO> _processedCabinet;

		void GetCabinetTotalUncompressedSize(const WWuString& filePath);
		void GetCabinetInformation(const MemoryMappedFile& mappedFile, CABINET_PROCESSING_INFO* cabInfo);
		_NODISCARD CABINET_PROCESSING_INFO* GetInfoByPath(const WWuString& filePath);
		_NODISCARD CABINET_PROCESSING_INFO* GetInfoByName(const WWuString& cabName);

		INT_PTR OnCabinetInfo();
		INT_PTR OnCopyFile(PFDINOTIFICATION cabInfo);
		INT_PTR OnCloseFileInfo(PFDINOTIFICATION cabInfo);

		static void* __cdecl FdiAlloc(UINT cb);
		static void __cdecl FdiFree(void* pv);
		static INT_PTR __cdecl FdiOpen(char* pszFile, int oflag, int pmode);
		static UINT __cdecl FdiRead(INT_PTR hf, void* pv, UINT cb);
		static UINT __cdecl FdiWrite(INT_PTR hf, void* pv, UINT cb);
		static int __cdecl FdiClose(INT_PTR hf);
		static long __cdecl FdiSeek(INT_PTR hf, long dist, int seektype);
		static INT_PTR __cdecl FdiNotify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin);
	};

	template <class T>
	DWORD GetNumberDigitCount(T number);
}