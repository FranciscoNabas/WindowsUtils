#include "../../pch.h"

#include "../../Headers/Engine/Containers.h"
#include "../../Headers/Support/WuStdException.h"

#include <PathCch.h>
#include <Shlwapi.h>
#include <io.h>
#include <fcntl.h>
#include <strsafe.h>

namespace WindowsUtils::Core
{
	// WuCabinet

	WuCabinet::WuCabinet(const WWuString& filePath, WuNativeContext* context)
	{
		// Checking if the path is a valid file.
		if (!PathFileExists(filePath.GetBuffer())) {
			throw WuStdException(ERROR_FILE_NOT_FOUND, __FILEW__, __LINE__);
		}

		// Getting directory name.
		WWuString wideDir { filePath };
		HRESULT result = PathCchRemoveFileSpec(wideDir.GetBuffer(), wideDir.Length());
		if (result != S_OK) {
			if (result == S_FALSE) {
				throw WuStdException(-1, L"Path missing file name.", __FILEW__, __LINE__);
			}
			else {
				throw WuStdException(result, __FILEW__, __LINE__);
			}
		}
		m_directory = WWuStringToNarrow(wideDir);

		GetCabinetTotalUncompressedSize(filePath);

		m_progressInfo.CabinetSetCount = static_cast<DWORD>(m_processedCabinet.size());
		m_progressInfo.TotalUncompressedSize = m_totalUncompressedSize;
		m_progressInfo.CompletedCabinetCount = 0;
		m_progressInfo.CompletedSize = 0;
		m_progressInfo.CurrentUncompressedSize = 0;

		m_context = context;
		m_isUnicode = false;
		m_instance = this;
	}

	WuCabinet::WuCabinet(const AbstractPathTree& apt, const WWuString& cabNameTemplate, USHORT compressionType, WuNativeContext* context)
	{
		m_compProgressInfo.TotalFileCount = apt.FileCount;
		m_compProgressInfo.TotalUncompressedSize = apt.TotalLength;
		m_compProgressInfo.CurrentUncompressedSize = 0;
		m_compProgressInfo.CompletedFileCount = 0;
		m_compProgressInfo.CompletedSize = 0;

		m_apt = apt;
		m_compressionType = compressionType;
		m_newCabNameTemplate = cabNameTemplate;
		m_context = context;
		m_isUnicode = false;
		m_instance = this;
	}

	WuCabinet::~WuCabinet() { }

	WuCabinet* WuCabinet::m_instance { nullptr };

	// General FDI/FCI functions.
	void* __cdecl WuCabinet::CabAlloc(UINT cb) { return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb); }
	void __cdecl WuCabinet::CabFree(void* pv) { HeapFree(GetProcessHeap(), NULL, pv); }
	UINT __cdecl WuCabinet::CabRead(INT_PTR hf, void* pv, UINT cb) { return _read(static_cast<int>(hf), pv, cb); }
	UINT __cdecl WuCabinet::CabWrite(INT_PTR hf, void* pv, UINT cb) { return _write(static_cast<int>(hf), pv, cb); }
	int __cdecl WuCabinet::CabClose(INT_PTR hf) { return _close(static_cast<int>(hf)); }
	long __cdecl WuCabinet::CabSeek(INT_PTR hf, long dist, int seektype) { return _lseek(static_cast<int>(hf), dist, seektype); }
	INT_PTR __cdecl WuCabinet::CabNotify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin) { return m_instance->NotifyCallback(fdint, pfdin); }

	INT_PTR __cdecl WuCabinet::CabOpen(char* pszFile, int oflag, int pmode)
	{
		int fileHandle;

		if (m_instance->IsCurrentUnicode()) {
			WWuString wideName = WuStringToWide(pszFile, CP_UTF8);

			if (oflag & _O_CREAT)
				SetFileAttributes(wideName.GetBuffer(), FILE_ATTRIBUTE_NORMAL);

			_wsopen_s(&fileHandle, wideName.GetBuffer(), oflag, _SH_DENYNO, pmode);
		}
		else {
			if (oflag & _O_CREAT)
				SetFileAttributesA(pszFile, FILE_ATTRIBUTE_NORMAL);

			_sopen_s(&fileHandle, pszFile, oflag, _SH_DENYNO, pmode);
		}

		return fileHandle;
	}

	// FCI functions.
	BOOL __cdecl WuCabinet::FciGetTempFile(char* tempName, int cbTemp, void* pv)
	{
		UNREFERENCED_PARAMETER(pv);

		// Attempting to get temp path.
		CHAR tempPathBuffer[MAX_PATH + 1];
		GetTempPathA(MAX_PATH + 1, tempPathBuffer);

		// Attempting to get a temporary file name. If 'uUnique' (third param) is zero,
		// the system will try to get a unique name using the system date time. If non
		// zero, the system doesn't guarantee the file already exists.
		CHAR tempFileNameBuffer[MAX_PATH];
		WuString tempFileName;
		if (GetTempFileNameA(tempPathBuffer, "WuCab", 0, tempFileNameBuffer) == 0) {

			// If it fails, we try to generate the path ourselves.
			__int64 index = 0;
			do {
				tempFileName = WuString::Format("WuCab-%d");

				// Check if the file already exists (-1 = Error reading file attributes).
				if (GetFileAttributesA(tempFileName.GetBuffer()) == -1)
					break;

			} while (true);
		}
		else {
			DeleteFileA(tempFileNameBuffer);
			tempFileName = tempFileNameBuffer;
		}

		strncpy_s(tempName, cbTemp, tempFileName.GetBuffer(), cbTemp);

		return TRUE;
	}

	int __cdecl WuCabinet::FciDelete(char* fileName, int* err, void* pv)
	{
		UNREFERENCED_PARAMETER(pv);

		int result = 0;
		if (!DeleteFileA(fileName)) {
			*err = GetLastError();
			result = -1;
		}

		return result;
	}

	INT_PTR __cdecl WuCabinet::FciGetOpenInfo(char* name, USHORT* date, USHORT* time, USHORT* attributes, int* err, void* pv)
	{
		WWuString fileName = WuStringToWide(name);

		HANDLE hFile = CreateFile(
			fileName.GetBuffer(),
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			NULL
		);
		if (hFile == INVALID_HANDLE_VALUE) {
			*err = GetLastError();
			return -1;
		}

		BY_HANDLE_FILE_INFORMATION fileInfo;
		if (!GetFileInformationByHandle(hFile, &fileInfo)) {
			*err = GetLastError();
			return -1;
		}

		FILETIME fileTime = { 0 };
		FileTimeToLocalFileTime(&fileTime, &fileInfo.ftCreationTime);
		FileTimeToDosDateTime(&fileTime, date, time);

		*attributes = static_cast<USHORT>(fileInfo.dwFileAttributes);
		*attributes &= (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE);

		return CabOpen(name, _O_RDONLY | _O_BINARY, _S_IREAD);
	}

	int __cdecl WuCabinet::FciFilePlaced(PCCAB pccab, char* fileName, long cbFile, BOOL isContinuation, void* pv)
	{
		return 0;
	}

	BOOL __cdecl WuCabinet::FciNextCabinet(PCCAB pccab, ULONG cbPrevCab, void* pv)
	{
		UNREFERENCED_PARAMETER(pv);
		UNREFERENCED_PARAMETER(cbPrevCab);

		HRESULT hr;

		WWuString fullCabTemplateName = m_instance->m_newCabNameTemplate + L"%02d.cab";
		hr = StringCchPrintfA(
			pccab->szCab,
			ARRAYSIZE(pccab->szCab),
			WWuStringToNarrowMultiByte(fullCabTemplateName, CP_UTF8).GetBuffer(),
			pccab->iCab
		);

		m_instance->m_compProgressInfo.CabinetName = WuStringToWide(pccab->szCab);
		m_instance->m_compProgressInfo.Notify(m_instance->m_context);

		return (SUCCEEDED(hr));
	}

	long __cdecl WuCabinet::FciStatus(UINT typeStatus, ULONG cb1, ULONG cb2, void* pv) { return 0; }

	// WuCabinet functions.
	const bool WuCabinet::IsCurrentUnicode() const
	{
		return m_isUnicode;
	}

	const wuvector<CABINET_PROCESSING_INFO>& WuCabinet::GetCabinetInfo()
	{
		return m_processedCabinet;
	}

	void WuCabinet::GetCabinetTotalUncompressedSize(const WWuString& filePath)
	{
		__uint64 totalSize = 0;
		HRESULT result = S_OK;

		// 'Moving' to the first cabinet.
		bool hasPrevious;
		WWuString currentPath = filePath;
		CABINET_PROCESSING_INFO* currentCabInfo;
		wuunique_ptr<CABINET_PROCESSING_INFO> buffer;
		do {
			currentCabInfo = NULL;
			currentCabInfo = GetInfoByPath(currentPath);
			if (currentCabInfo == NULL) {
				MemoryMappedFile mappedFile(currentPath);
				buffer = make_wuunique<CABINET_PROCESSING_INFO>();

				GetCabinetInformation(mappedFile, buffer.get());
				buffer->Path = currentPath;
				buffer->Name = currentPath;
				PathStripPath(buffer->Name.GetBuffer());

				m_processedCabinet.push_back(*buffer.get());
				currentCabInfo = buffer.get();
			}

			if (!WuString::IsNullOrEmpty(currentCabInfo->PreviousCabinet)) {
				hasPrevious = true;

				// Dir size + file name size + possible '\' + \0
				size_t pathBufSize = m_directory.Length() + currentCabInfo->PreviousCabinet.Length() + 2;
				auto pathBuffer = make_wuunique<WCHAR[]>(pathBufSize);
				result = PathCchCombine(pathBuffer.get(), pathBufSize, WuStringToWide(m_directory).GetBuffer(), WuStringToWide(currentCabInfo->PreviousCabinet).GetBuffer());
				if (result != S_OK) {
					throw WuStdException(result, __FILEW__, __LINE__);
				}

				currentPath = pathBuffer.get();
			}
			else {
				hasPrevious = false;
				break;
			}

		} while (hasPrevious);

		// At this point this is the first cab.
		m_filePath = currentCabInfo->Path;

		// Processing each cabinet until there is no next.
		bool hasNext;
		auto processedFiles = make_wusunique_vector<WuString>();
		do {
			__uint64 filePointer = 0;
			MemoryMappedFile mappedFile(currentPath);

			// Checking list because we might not been through all cabs.
			currentCabInfo = NULL;
			currentCabInfo = GetInfoByPath(currentPath);
			if (currentCabInfo == NULL) {
				buffer = make_wuunique<CABINET_PROCESSING_INFO>();

				GetCabinetInformation(mappedFile, buffer.get());
				buffer->Path = currentPath;
				buffer->Name = currentPath;
				PathStripPath(buffer->Name.GetBuffer());

				m_processedCabinet.push_back(*buffer.get());
				currentCabInfo = buffer.get();
			}

			// Advancing to the first CFFILE entry.
			filePointer += currentCabInfo->CFFileOffset;

			// Going through each CFFILE entry.
			for (WORD i = 0; i < currentCabInfo->FileCount; i++) {
				DWORD currentSize = *reinterpret_cast<LPDWORD>((BYTE*)mappedFile.data() + filePointer);
				filePointer += 16;

				// Reading file name.
				CHAR currentChar;
				wusunique_vector<CHAR> buffer = make_wusunique_vector<CHAR>();
				do {
					currentChar = *reinterpret_cast<LPSTR>((BYTE*)mappedFile.data() + filePointer);
					if (currentChar != '\0')
						buffer->push_back(currentChar);

					filePointer++;

				} while (currentChar != '\0');

				buffer->push_back('\0');
				if (std::find(processedFiles->begin(), processedFiles->end(), buffer->data()) == processedFiles->end()) {
					totalSize += currentSize;
					processedFiles->push_back(buffer->data());
				}
			}

			// Checking if there is a next.
			if (!WuString::IsNullOrEmpty(currentCabInfo->NextCabinet)) {
				hasNext = true;

				// Dir size + file name size + possible '\' + \0
				size_t pathBufSize = m_directory.Length() + currentCabInfo->NextCabinet.Length() + 2;
				auto pathBuffer = make_wuunique<WCHAR[]>(pathBufSize);
				result = PathCchCombine(pathBuffer.get(), pathBufSize, WuStringToWide(m_directory).GetBuffer(), WuStringToWide(currentCabInfo->NextCabinet).GetBuffer());
				if (result != S_OK) {
					throw WuStdException(result, __FILEW__, __LINE__);
				}

				currentPath = pathBuffer.get();
			}
			else {
				hasNext = false;
				break;
			}

		} while (hasNext);

		m_totalUncompressedSize = totalSize;
	}

	void WuCabinet::GetCabinetInformation(const MemoryMappedFile& mappedFile, CABINET_PROCESSING_INFO* cabInfo)
	{
		__uint64 filePointer = 0;

		CHAR signature[5] = { 0 };
		RtlCopyMemory(signature, mappedFile.data(), 4);
		if (strcmp(signature, "MSCF") != 0)
			throw WuStdException(ERROR_BAD_FORMAT, L"File is not a cabinet.", __FILEW__, __LINE__);

		filePointer += 16;
		cabInfo->CFFileOffset = *reinterpret_cast<LPDWORD>((BYTE*)mappedFile.data() + filePointer);

		filePointer += 12;
		cabInfo->FileCount = *reinterpret_cast<LPWORD>((BYTE*)mappedFile.data() + filePointer);

		filePointer += 2;
		WORD flags = *reinterpret_cast<LPWORD>((BYTE*)mappedFile.data() + filePointer);

		filePointer += 4;
		cabInfo->CabinetIndex = *reinterpret_cast<LPWORD>((BYTE*)mappedFile.data() + filePointer);

		filePointer += 2;
		if ((flags & cfhdrRESERVE_PRESENT) > 0) {
			WORD cbSize = *reinterpret_cast<LPWORD>((BYTE*)mappedFile.data() + filePointer);
			filePointer = filePointer + cbSize + 2;
		}

		wusunique_vector<CHAR> buffer;
		if ((flags & cfhdrPREV_CABINET) > 0) {
			buffer = make_wusunique_vector<CHAR>();
			CHAR currentChar;
			do {
				currentChar = *reinterpret_cast<LPSTR>((BYTE*)mappedFile.data() + filePointer);
				if (currentChar != '\0')
					buffer->push_back(currentChar);

				filePointer++;

			} while (currentChar != '\0');

			buffer->push_back('\0');
			cabInfo->PreviousCabinet = buffer->data();

			// Advancing the file pointer to the end of 'previous disk'.
			do {
				currentChar = *reinterpret_cast<LPSTR>((UCHAR*)mappedFile.data() + filePointer);
				filePointer++;

			} while (currentChar != '\0');
		}

		if ((flags & cfhdrNEXT_CABINET) > 0) {
			buffer = make_wusunique_vector<CHAR>();
			CHAR currentChar;
			do {
				currentChar = *reinterpret_cast<LPSTR>((UCHAR*)mappedFile.data() + filePointer);
				if (currentChar != '\0')
					buffer->push_back(currentChar);

				filePointer++;

			} while (currentChar != '\0');

			buffer->push_back('\0');
			cabInfo->NextCabinet = buffer->data();
		}
	}

	_NODISCARD CABINET_PROCESSING_INFO* WuCabinet::GetInfoByPath(const WWuString& filePath)
	{
		for (CABINET_PROCESSING_INFO& cabInfo : m_processedCabinet)
			if (cabInfo.Path == filePath)
				return &cabInfo;

		return NULL;
	}

	_NODISCARD CABINET_PROCESSING_INFO* WuCabinet::GetInfoByName(const WWuString& cabName)
	{
		for (CABINET_PROCESSING_INFO& cabInfo : m_processedCabinet)
			if (cabInfo.Name == cabName)
				return &cabInfo;

		return NULL;
	}

	void WuCabinet::ExpandCabinetFile(const WWuString& destination)
	{
		ERF erfError = { 0 };
		HFDI hContext;

		hContext = FDICreate(
			(PFNALLOC)CabAlloc,
			(PFNFREE)CabFree,
			(PFNOPEN)CabOpen,
			(PFNREAD)CabRead,
			(PFNWRITE)CabWrite,
			(PFNCLOSE)CabClose,
			(PFNSEEK)CabSeek,
			cpuUNKNOWN,
			&erfError
		);

		if (hContext == NULL)
			throw WuStdException((FDIERROR)erfError.erfOper, __FILEW__, __LINE__, CoreErrorType::FdiError);

		if (!PathIsDirectory(destination.GetBuffer()))
			throw WuStdException(ERROR_FILE_NOT_FOUND, __FILEW__, __LINE__);

		// FDICopy does not manage trailing path separator. It just concatenates
		// file name and file path.
		m_destination = destination;
		if (!m_directory.EndsWith('\\')) {
			m_directory += "\\";
		}

		WuString filePath = WWuStringToNarrow(m_filePath);
		WuString fileName = filePath;
		PathStripPathA(fileName.GetBuffer());

		do {
			m_nextCabinet.Clear();
			m_progressInfo.CabinetName = WuStringToWide(fileName);

			if (!FDICopy(hContext, fileName.GetBuffer(), m_directory.GetBuffer(), 0, (PFNFDINOTIFY)CabNotify, NULL, NULL)) {
				if (hContext != NULL)
					FDIDestroy(hContext);

				throw WuStdException((FDIERROR)erfError.erfOper, __FILEW__, __LINE__, CoreErrorType::FdiError);
			}

			if (m_nextCabinet.Length() == 0)
				break;

			CABINET_PROCESSING_INFO* nextCabInfo = GetInfoByName(m_nextCabinet);
			if (nextCabInfo == NULL)
				throw WuStdException(ERROR_FILE_NOT_FOUND, __FILEW__, __LINE__);

			filePath = WWuStringToNarrow(nextCabInfo->Path);
			fileName = WWuStringToNarrow(nextCabInfo->Path);
			PathStripPathA(fileName.GetBuffer());

		} while (m_nextCabinet.Length() != 0);

		if (hContext != NULL)
			FDIDestroy(hContext);
	}

	void WuCabinet::CompressCabinetFile(const WWuString& destination, ULONG splitSize)
	{
		ERF erfError = { 0 };
		CCAB cCab = { 0 };

		RtlZeroMemory(&erfError, sizeof(ERF));
		RtlZeroMemory(&cCab, sizeof(CCAB));

		IO::CreateFolderTree(destination);

		// Initializing.
		cCab.cb = splitSize;
		cCab.cbFolderThresh = 0x7FFFFFFF;
		cCab.iCab = 1;
		cCab.iDisk = 0;
		cCab.setID = 666;

		FciNextCabinet(&cCab, 0, NULL);

		// Creating context.
		HFCI hContext = FCICreate(
			&erfError,
			(PFNFCIFILEPLACED)FciFilePlaced,
			(PFNFCIALLOC)CabAlloc,
			(PFNFCIFREE)CabFree,
			(PFNFCIOPEN)CabOpen,
			(PFNFCIREAD)CabRead,
			(PFNFCIWRITE)CabWrite,
			(PFNFCICLOSE)CabClose,
			(PFNFCISEEK)CabSeek,
			(PFNFCIDELETE)FciDelete,
			(PFNFCIGETTEMPFILE)FciGetTempFile,
			&cCab,
			NULL
		);

		if (hContext == NULL)
			throw WuStdException(erfError.erfOper, __FILEW__, __LINE__, CoreErrorType::FciError);

		// Adding files to cabinet.
		for (AbstractPathTree::AptEntry& aptEntry : m_apt.GetApt()) {
			if (aptEntry.Type == FsObjectType::File) {

				m_compProgressInfo.CurrentFile = aptEntry.Name;

				WuString narrFullPath = WWuStringToNarrowMultiByte(aptEntry.FullPath, CP_UTF8);
				WuString narrRelPath = WWuStringToNarrow(aptEntry.RelativePath.GetBuffer());

				if (!FCIAddFile(
					hContext,
					narrFullPath.GetBuffer(),
					narrRelPath.GetBuffer(),
					FALSE,
					(PFNFCIGETNEXTCABINET)FciNextCabinet,
					(PFNFCISTATUS)FciStatus,
					(PFNFCIGETOPENINFO)FciGetOpenInfo,
					m_compressionType
				)) {
					FCIDestroy(hContext);
					throw WuStdException(erfError.erfOper, __FILEW__, __LINE__, CoreErrorType::FciError);
				}

				m_compProgressInfo.CompletedSize += aptEntry.Length;
				m_compProgressInfo.CompletedFileCount++;
				m_compProgressInfo.Notify(m_context);
			}
		}

		if (!FCIFlushCabinet(hContext, FALSE, (PFNFCIGETNEXTCABINET)FciNextCabinet, (PFNFCISTATUS)FciStatus)) {
			FCIDestroy(hContext);
			throw WuStdException(erfError.erfOper, __FILEW__, __LINE__, CoreErrorType::FciError);
		}

		FCIDestroy(hContext);

		// Moving files to destination.
		WCHAR currentDirBuff[MAX_PATH + 1];
		GetCurrentDirectory(MAX_PATH + 1, currentDirBuff);

		WWuString currentDir(currentDirBuff);
		WWuString searchPath(currentDirBuff);
		searchPath = WWuString::Format(L"%ws\\%ws*", searchPath.GetBuffer(), m_newCabNameTemplate.GetBuffer());

		WWuString finalDest;
		if (destination.EndsWith(L"\\"))
			finalDest = destination;
		else
			finalDest = destination + L"\\";

		WIN32_FIND_DATA findData;
		HANDLE hFind = FindFirstFileEx(searchPath.GetBuffer(), FindExInfoBasic, &findData, FindExSearchNameMatch, NULL, 0);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				WWuString fullName = WWuString::Format(L"%ws\\%ws", currentDir.GetBuffer(), findData.cFileName);
				WWuString destFullName = finalDest + findData.cFileName;
				DeleteFile(destFullName.GetBuffer());
				MoveFile(fullName.GetBuffer(), destFullName.GetBuffer());
			} while (FindNextFile(hFind, &findData));
		}
	}

	INT_PTR WuCabinet::NotifyCallback(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
	{
		switch (fdint) {
			// Called once for each cabinet opened by FDICopy, this includes
			// continuation cabinets opened due to files spanning cabinet boundaries.
			// Information:
			//	psz1: The name of the next cabinet, excluding path information.
			//	psz2: The name of the next disk.
			//	psz3: The cabinet path name.
			//	setID: The set ID of the current cabinet.
			//	iCabinet: The cabinet number within the cabinet set.
			case fdintCABINET_INFO:
				return OnCabinetInfo();

				// Called for each file that starts within the current cabinet.
				// Information:
				//	cb: The uncompressed file size.
				//	psz1: The name of the file in the cabinet.
				//	date: The 16-bit MS-DOS date
				//	time: The 16-bit MS-DOS time
				//	attribs: The 16-bit MS-DOS attributes.
				// Necessary actions:
				//	- Set progress information.
				//	- Create folder tree in the destination.
				//	- Call FDIOpen function.
				//	- Return the file handle where to write the file. it must be compatible
				//	  with the function defined for FDIClose.
			case fdintCOPY_FILE:
				return OnCopyFile(pfdin);

				// Called after all of the data has ben written to the targeting file.
				// Information:
				//	psz1: The name of the file in the cabinet.
				//	hf: The file handle originated from 'fdintCOPY_FILE'.
				//	date: The 16-bit MS-DOS date
				//	time: The 16-bit MS-DOS time
				//	attribs: The 16-bit MS-DOS attributes.
				//	cb: 0 or 1. 1 indicates the file should be executed after extract. Where not going to implement this.
				// Necessary actions:
				//	- Replace '/' with '\\'.
				//	- Close the file with 'FDIClose' provided fn. -FIRST-
				//	- Set attributes and datetime. -AFTER-
			case fdintCLOSE_FILE_INFO:
				return OnCloseFileInfo(pfdin);

				// Called only if 'fdintCOPY_FILE' is instructed to copy a file which
				// is continued from a subsequent cabinet.
				// Information:
				//	psz1: The name of the next cabinet on which the current file is continued.
				//	psz2: The file handle originated from 'fdintCOPY_FILE'.
				//	psz3: The cabinet path information.
				//	fdie: Error or success value.
				// Necessary actions:
				//	- Validate the next cabinet path.
				//	- Validate if the file exists and can be read.
				//	- Set setID and iCabinet to the correct next cabinet.
			case fdintNEXT_CABINET:
				m_nextCabinet = WuStringToWide(pfdin->psz1);
				return 0;

				// Do nothing.
			default:
				break;
		}

		return 0;
	}

	INT_PTR WuCabinet::OnCabinetInfo()
	{
		m_progressInfo.CompletedCabinetCount++;

#ifndef _DEBUG
		m_progressInfo.Notify(m_context);
#endif

		return 0;
	}

	INT_PTR WuCabinet::OnCopyFile(PFDINOTIFICATION cabInfo)
	{
		DWORD codePage = (cabInfo->attribs & _A_NAME_IS_UTF) ? CP_UTF8 : 1252;
		WWuString relativePath = WuStringToWide(cabInfo->psz1, codePage).Replace('/', '\\');

		if (codePage == CP_UTF8)
			m_isUnicode = true;
		else
			m_isUnicode = false;

		auto splitPath = relativePath.Split('\\');

		splitPath.insert(splitPath.begin(), m_destination);

		wuvector<WWuString> splitDir(splitPath);
		splitDir.pop_back();

		WWuString targetDir;
		WWuString targetFullName;

		IO::CreatePath(splitDir, targetDir);
		IO::CreatePath(splitPath, targetFullName);

		IO::CreateFolderTree(targetDir);

		WuString narrowFullName = WWuStringToMultiByte(targetFullName.GetBuffer(), codePage);
		INT_PTR hFile = CabOpen(narrowFullName.GetBuffer(), _O_TRUNC | _O_BINARY | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL, _S_IREAD | _S_IWRITE);
		if (hFile <= 0)
			throw WuStdException(cabInfo->fdie, __FILEW__, __LINE__, CoreErrorType::FdiError);

		m_progressInfo.CurrentFile = relativePath;
		m_progressInfo.CurrentUncompressedSize = cabInfo->cb;
		m_progressInfo.CompletedSize += cabInfo->cb;

#ifndef _DEBUG
		m_progressInfo.Notify(m_context);
#endif

		return hFile;
	}

	INT_PTR WuCabinet::OnCloseFileInfo(PFDINOTIFICATION cabInfo)
	{
		DWORD codePage = (cabInfo->attribs & _A_NAME_IS_UTF) ? CP_UTF8 : 1252;
		WWuString relativePath = WuStringToWide(cabInfo->psz1, codePage).Replace('/', '\\');

		wuvector<WWuString> splitPath;
		splitPath.push_back(m_destination);
		splitPath.push_back(relativePath);

		WWuString targetFullName;
		IO::CreatePath(splitPath, targetFullName);

		CabClose(cabInfo->hf);
		IO::SetFileAttributesAndDate(targetFullName, cabInfo->date, cabInfo->time, cabInfo->attribs);

		return TRUE;
	}

	// FDI Notification definition.

	_FDI_PROGRESS::_FDI_PROGRESS() { }
	_FDI_PROGRESS::~_FDI_PROGRESS() { }

	void _FDI_PROGRESS::Notify(WuNativeContext* context)
	{
		float floatPercent = (static_cast<float>(CompletedSize) / TotalUncompressedSize);
		floatPercent *= 100;
		long percentComplete = lround(floatPercent);
		WWuString status = WWuString::Format(L"Cabinet %d/%d: %ws. File: %ws. %ld/%ld", CompletedCabinetCount, CabinetSetCount, CabinetName.GetBuffer(), CurrentFile.GetBuffer(), CompletedSize, TotalUncompressedSize);

		MAPPED_PROGRESS_DATA progressData(
			(LPWSTR)L"Expanding cabinet...", 0, NULL, -1, static_cast<WORD>(percentComplete), ProgressRecordType::Processing, -1, status.GetBuffer()
		);

		context->NativeWriteProgress(&progressData);
	}

	// FCI Notification definition.

	_FCI_PROGRESS::_FCI_PROGRESS() { }
	_FCI_PROGRESS::~_FCI_PROGRESS() { }

	void _FCI_PROGRESS::Notify(WuNativeContext* context)
	{
		float floatPercent = (static_cast<float>(CompletedSize) / TotalUncompressedSize);
		floatPercent *= 100;
		long percentComplete = lround(floatPercent);

		WWuString status = WWuString::Format(
			L"File %d/%d: %ws. Cabinet: %ws. %lld/%lld",
			CompletedFileCount,
			TotalFileCount,
			CurrentFile.GetBuffer(),
			CabinetName.GetBuffer(),
			CompletedSize,
			TotalUncompressedSize
		);

		MAPPED_PROGRESS_DATA progressData(
			(LPWSTR)L"Compressing files...", 0, NULL, -1, static_cast<WORD>(percentComplete), ProgressRecordType::Processing, -1, status.GetBuffer()
		);

		context->NativeWriteProgress(&progressData);
	}

	// Utilities.

	template <class T>
	DWORD GetNumberDigitCount(T number)
	{
		DWORD digits = 0;
		while (number) {
			number /= 10;
			digits++;
		}

		return digits;
	}
}