#pragma once
#pragma unmanaged

#include <memory>
#include <variant>

#include <fci.h>
#include <fdi.h>
#include <PathCch.h>
#include <Shlwapi.h>
#include <io.h>
#include <fcntl.h>
#include <strsafe.h>

#include "../Support/WuString.h"
#include "../Support/Expressions.h"
#include "../Support/Notification.h"
#include "../Support/IO.h"
#include "../Support/WuException.h"

constexpr WORD cfhdrPREV_CABINET     = 0x0001;
constexpr WORD cfhdrNEXT_CABINET     = 0x0002;
constexpr WORD cfhdrRESERVE_PRESENT  = 0x0004;

namespace WindowsUtils::Core
{
	enum class CabinetOperation
	{
		FDI,
		FCI,
	};

	enum class CabinetCompressionType : USHORT
	{
		None     = tcompTYPE_NONE,
		MSZip    = tcompTYPE_MSZIP,
		LZXLow   = tcompTYPE_LZX | tcompLZX_WINDOW_LO,
		LZXHigh  = tcompTYPE_LZX | tcompLZX_WINDOW_HI
	};

	struct FDIProgress
	{
		DWORD      CabinetSetCount;
		DWORD      CompletedCabinetCount;
		WWuString  CabinetName;
		WWuString  CurrentFile;
		DWORD      CurrentUncompressedSize;
		__uint64   CompletedSize;
		__uint64   TotalUncompressedSize;

		FDIProgress(const WuNativeContext* context, const DWORD cabSetCount, const __uint64 totalUncSize);
		~FDIProgress();

		void Notify();

	private:
		const WuNativeContext* m_context;
	};

	struct FCIProgress
	{
		WWuString  CabinetName;
		WWuString  CurrentFile;
		DWORD      CompletedFileCount;
		DWORD      TotalFileCount;
		__uint64   CurrentUncompressedSize;
		__uint64   CompletedSize;
		__uint64   TotalUncompressedSize;

		FCIProgress(const WuNativeContext* context, const DWORD totalFileCount, const __uint64 totalUncSize);
		~FCIProgress();

		void Notify();

	private:
		const WuNativeContext* m_context;
	};

	struct CabinetOperationInfo
	{
		struct FDI
		{
			FDIProgress* Progress;
			WWuString NextCabinet;
		};

		struct FCI
		{
			FCIProgress* Progress;
			const WWuString* NameTemplate;
		};

		const CabinetOperation Operation;
		const WWuString* Destination;
		std::variant<FDI, FCI> Info;

		CabinetOperationInfo(const WWuString* destination, FDIProgress* progress);
		CabinetOperationInfo(const WWuString* destination, FCIProgress* progress, const WWuString* nameTemplate);
		~CabinetOperationInfo();
	};

	struct CabinetProcessingInfo
	{
		WWuString  Name;
		WWuString  Path;
		WORD       CabinetIndex;
		WORD       FileCount;
		DWORD      CFFileOffset;
		WuString   PreviousCabinet;
		WuString   NextCabinet;
	};

	class Containers
	{
	public:
		static void ExpandCabinetFile(const WWuString& path, const WWuString& destination, const WuNativeContext* context);
		static void CreateCabinetFile(AbstractPathTree& apt, const WWuString& destination, const WWuString& nameTemplate,
			const CabinetCompressionType compressionType, ULONG splitSize, const WuNativeContext* context);

	private:
		static std::tuple<__uint64, WWuString> GetCabinetTotalUncompressedSize(const WWuString& path, const WWuString& directory, WuList<CabinetProcessingInfo>& cabInfoList);
		static CabinetProcessingInfo GetCabinetInformation(const MemoryMappedFile& mappedFile);
		static bool TryGetInfoByPath(const WWuString& path, const WuList<CabinetProcessingInfo>& list, CabinetProcessingInfo& info);
		static bool TryGetInfoByName(const WWuString& name, const WuList<CabinetProcessingInfo>& list, CabinetProcessingInfo& info);

		static INT_PTR OnCabinetInfo(PFDINOTIFICATION cabInfo);
		static INT_PTR OnCopyFile(PFDINOTIFICATION cabInfo);
		static INT_PTR OnCloseFileInfo(PFDINOTIFICATION cabInfo);

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