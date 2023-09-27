#pragma once
#pragma unmanaged

#include "../Support/String.h"
#include "../Support/Expressions.h"
#include "../Support/Notification.h"
#include "../Support/IO.h"

#include <fci.h>
#include <fdi.h>

constexpr WORD cfhdrPREV_CABINET = 0x0001;
constexpr WORD cfhdrNEXT_CABINET = 0x0002;
constexpr WORD cfhdrRESERVE_PRESENT = 0x0004;

namespace WindowsUtils::Core
{
	typedef struct _FDI_PROGRESS
	{
		DWORD CabinetSetCount;
		DWORD CompletedCabinetCount;
		WWuString CabinetName;
		WWuString CurrentFile;
		DWORD CurrentUncompressedSize;
		__uint64 CompletedSize;
		__uint64 TotalUncompressedSize;

		_FDI_PROGRESS();
		~_FDI_PROGRESS();

		void Notify(WuNativeContext* context);

	} FDI_PROGRESS, * PFDI_PROGRESS;

	typedef struct _FCI_PROGRESS
	{
		WWuString CabinetName;
		WWuString CurrentFile;
		DWORD CompletedFileCount;
		DWORD TotalFileCount;
		__uint64 CurrentUncompressedSize;
		__uint64 CompletedSize;
		__uint64 TotalUncompressedSize;

		_FCI_PROGRESS();
		~_FCI_PROGRESS();

		void Notify(WuNativeContext* context);

	} FCI_PROGRESS, * PFCI_PROGRESS;

	class CABINET_PROCESSING_INFO
	{
	public:
		WWuString Name;
		WWuString Path;
		WORD CabinetIndex;
		WORD FileCount;
		DWORD CFFileOffset;
		WuString PreviousCabinet;
		WuString NextCabinet;
	};

	class WuCabinet
	{
	public:
		const bool IsCurrentUnicode() const;
		void ExpandCabinetFile(const WWuString& destination);
		void CompressCabinetFile(const WWuString& destination, ULONG splitSize);
		const wuvector<CABINET_PROCESSING_INFO>& GetCabinetInfo();
		INT_PTR NotifyCallback(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin);

		WuCabinet(const WWuString& filePath, WuNativeContext* context);
		WuCabinet(const AbstractPathTree& apt, const WWuString& cabNameTemplate, USHORT compressionType, WuNativeContext* context);
		~WuCabinet();

		WuCabinet(WuCabinet const&) = delete;
		void operator=(WuCabinet const&) = delete;

	private:
		static WuCabinet* m_instance;

		bool m_isUnicode;
		WWuString m_filePath;
		WuString m_directory;
		WWuString m_destination;
		WWuString m_nextCabinet;
		USHORT m_compressionType;
		WuNativeContext* m_context;
		FDI_PROGRESS m_progressInfo;
		AbstractPathTree m_apt;
		WWuString m_newCabNameTemplate;
		FCI_PROGRESS m_compProgressInfo;
		__uint64 m_totalUncompressedSize;
		wuvector<CABINET_PROCESSING_INFO> m_processedCabinet;

		void GetCabinetTotalUncompressedSize(const WWuString& filePath);
		void GetCabinetInformation(const MemoryMappedFile& mappedFile, CABINET_PROCESSING_INFO* cabInfo);
		_NODISCARD CABINET_PROCESSING_INFO* GetInfoByPath(const WWuString& filePath);
		_NODISCARD CABINET_PROCESSING_INFO* GetInfoByName(const WWuString& cabName);

		INT_PTR OnCabinetInfo();
		INT_PTR OnCopyFile(PFDINOTIFICATION cabInfo);
		INT_PTR OnCloseFileInfo(PFDINOTIFICATION cabInfo);

		static void* __cdecl CabAlloc(UINT cb);
		static void __cdecl CabFree(void* pv);
		static INT_PTR __cdecl CabOpen(char* pszFile, int oflag, int pmode);
		static UINT __cdecl CabRead(INT_PTR hf, void* pv, UINT cb);
		static UINT __cdecl CabWrite(INT_PTR hf, void* pv, UINT cb);
		static int __cdecl CabClose(INT_PTR hf);
		static long __cdecl CabSeek(INT_PTR hf, long dist, int seektype);
		static INT_PTR __cdecl CabNotify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin);

		static BOOL __cdecl FciGetTempFile(char* tempName, int cbTemp, void* pv);
		static int __cdecl FciFilePlaced(PCCAB pccab, char* fileName, long cbFile, BOOL isContinuation, void* pv);
		static int __cdecl FciDelete(char* fileName, int* err, void* pv);
		static BOOL __cdecl FciNextCabinet(PCCAB pccab, ULONG cbPrevCab, void* pv);
		static long __cdecl FciStatus(UINT typeStatus, ULONG cb1, ULONG cb2, void* pv);
		static INT_PTR __cdecl FciGetOpenInfo(char* name, USHORT* date, USHORT* time, USHORT* attributes, int* err, void* pv);
	};
}