#include "../../pch.h"

#include "../../Headers/Engine/Containers.h"

namespace WindowsUtils::Core
{
	FDIProgress::FDIProgress(const WuNativeContext* context, const DWORD cabSetCount, const __uint64 totalUncSize)
		: m_context(context), CabinetSetCount(cabSetCount), CompletedCabinetCount(), CabinetName(), CurrentFile(),
		CurrentUncompressedSize(), CompletedSize(), TotalUncompressedSize(totalUncSize) { }

	FDIProgress::~FDIProgress() { }

	void FDIProgress::Notify()
	{
		float floatPercent = (static_cast<float>(CompletedSize) / TotalUncompressedSize);
		floatPercent *= 100;
		long percentComplete = lround(floatPercent);
		WWuString status = WWuString::Format(L"Cabinet %d/%d: %ws. File: %ws. %ld/%ld", CompletedCabinetCount, CabinetSetCount, CabinetName.Raw(), CurrentFile.Raw(), CompletedSize, TotalUncompressedSize);

		MAPPED_PROGRESS_DATA progressData(
			L"Expanding cabinet...", 0, nullptr, -1, static_cast<WORD>(percentComplete), ProgressRecordType::Processing, -1, status.Raw()
		);

		m_context->NativeWriteProgress(&progressData);
	}
		
	FCIProgress::FCIProgress(const WuNativeContext* context, const DWORD totalFileCount, const __uint64 totalUncSize)
		: m_context(context), CabinetName(), CurrentFile(), CompletedFileCount(), TotalFileCount(totalFileCount),
		CurrentUncompressedSize(), CompletedSize(), TotalUncompressedSize(totalUncSize) { }

	FCIProgress::~FCIProgress() { }

	void FCIProgress::Notify()
	{
		float floatPercent = (static_cast<float>(CompletedSize) / TotalUncompressedSize);
		floatPercent *= 100;
		long percentComplete = lround(floatPercent);

		WWuString status = WWuString::Format(
			L"File %d/%d: %ws. Cabinet: %ws. %lld/%lld",
			CompletedFileCount,
			TotalFileCount,
			CurrentFile.Raw(),
			CabinetName.Raw(),
			CompletedSize,
			TotalUncompressedSize
		);

		MAPPED_PROGRESS_DATA progressData(
			L"Compressing files...", 0, nullptr, -1, static_cast<WORD>(percentComplete), ProgressRecordType::Processing, -1, status.Raw()
		);

		m_context->NativeWriteProgress(&progressData);
	}

	CabinetOperationInfo::CabinetOperationInfo(const WWuString* destination, FDIProgress* progress)
		: Operation(CabinetOperation::FDI), Destination(destination), Info(FDI{ progress, nullptr }) { }

	CabinetOperationInfo::CabinetOperationInfo(const WWuString* destination, FCIProgress* progress, const WWuString* nameTemplate)
		: Operation(CabinetOperation::FCI), Destination(destination), Info(FCI{ progress, nameTemplate }) { }

	CabinetOperationInfo::~CabinetOperationInfo() { }

#pragma region Containers

	void Containers::ExpandCabinetFile(const WWuString& path, const WWuString& destination, const WuNativeContext* context)
	{
		if (!PathFileExists(path.Raw()))
			_WU_RAISE_NATIVE_EXCEPTION(ERROR_FILE_NOT_FOUND, L"PathFileExists", WriteErrorCategory::ObjectNotFound);

		// Getting directory name.
		WWuString wideDir = IO::RemoveFileSpec(path, true);
		
		WuString directory = wideDir.ToNarrow();
		WuList<CabinetProcessingInfo> cabInfoList(4);
		const auto sizeInfo = GetCabinetTotalUncompressedSize(path, wideDir, cabInfoList);

		FDIProgress progressInfo{ context, static_cast<DWORD>(cabInfoList.Count()), std::get<0>(sizeInfo) };

		const WWuString firstCab = std::get<1>(sizeInfo);

		CabinetOperationInfo operationInfo{ &destination, &progressInfo };

		HFDI hContext;
		ERF erfError{ };

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
			_WU_RAISE_NATIVE_FDI_EXCEPTION(erfError.erfOper, L"FDICreate", WriteErrorCategory::OpenError);

		if (!PathIsDirectory(destination.Raw()))
			_WU_RAISE_NATIVE_EXCEPTION(ERROR_FILE_NOT_FOUND, L"PathIsDirectory", WriteErrorCategory::ObjectNotFound);

		// FDICopy does not manage trailing path separator. It just concatenates
		// file name and file path.
		if (!directory.EndsWith('\\')) {
			directory += "\\";
		}

		WuString filePath = firstCab.ToNarrow();
		WuString fileName = IO::StripPath(firstCab).ToNarrow();
		WWuString& nextCabinet = std::get<CabinetOperationInfo::FDI>(operationInfo.Info).NextCabinet;
		do {
			progressInfo.CabinetName = fileName.ToWide();
			nextCabinet.Clear();

			if (!FDICopy(hContext, fileName.Raw(), directory.Raw(), 0, (PFNFDINOTIFY)CabNotify, nullptr, &operationInfo)) {
				if (hContext)
					FDIDestroy(hContext);

				_WU_RAISE_NATIVE_FDI_EXCEPTION(erfError.erfOper, L"FDICopy", WriteErrorCategory::OpenError);
			}

			if (nextCabinet.Length() == 0)
				break;

			CabinetProcessingInfo nextCabInfo{ };
			if (!TryGetInfoByName(nextCabinet, cabInfoList, nextCabInfo))
				_WU_RAISE_NATIVE_EXCEPTION(ERROR_FILE_NOT_FOUND, L"GetInfoByName", WriteErrorCategory::ObjectNotFound);

			filePath = nextCabInfo.Path.ToNarrow();
			fileName = IO::StripPath(nextCabInfo.Path).ToNarrow();

		} while (nextCabinet.Length() != 0);

		if (hContext)
			FDIDestroy(hContext);
	}

	void Containers::CreateCabinetFile(AbstractPathTree& apt, const WWuString& destination, const WWuString& nameTemplate,
		const CabinetCompressionType compressionType, ULONG splitSize, const WuNativeContext* context)
	{
		FCIProgress progressInfo{ context, apt.FileCount, apt.TotalLength };

		CCAB cCab{ };
		ERF erfError{ };

		IO::CreateFolderTree(destination);

		// Initializing.
		cCab.cb = splitSize;
		cCab.cbFolderThresh = 0x7FFFFFFF;
		cCab.iCab = 1;
		cCab.iDisk = 0;
		cCab.setID = 666;

		CabinetOperationInfo operationInfo{ &destination, &progressInfo, &nameTemplate };

		FciNextCabinet(&cCab, 0, &operationInfo);

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
			&operationInfo
		);

		if (hContext == NULL)
			_WU_RAISE_NATIVE_FCI_EXCEPTION(erfError.erfOper, L"FCICreate", WriteErrorCategory::OpenError);

		// Adding files to cabinet.
		for (AbstractPathTree::AptEntry& aptEntry : apt.GetApt()) {
			if (aptEntry.Type == FsObjectType::File) {

				progressInfo.CurrentFile = aptEntry.Name;

				WuString narrFullPath = aptEntry.FullPath.ToMb(CP_UTF8);
				WuString narrRelPath = aptEntry.RelativePath.ToNarrow();

				if (!FCIAddFile(
					hContext,
					narrFullPath.Raw(),
					narrRelPath.Raw(),
					FALSE,
					(PFNFCIGETNEXTCABINET)FciNextCabinet,
					(PFNFCISTATUS)FciStatus,
					(PFNFCIGETOPENINFO)FciGetOpenInfo,
					static_cast<USHORT>(compressionType)
				)) {
					FCIDestroy(hContext);
					_WU_RAISE_NATIVE_FCI_EXCEPTION(erfError.erfOper, L"FCIAddFile", WriteErrorCategory::WriteError);
				}

				progressInfo.CompletedSize += aptEntry.Length;
				progressInfo.CompletedFileCount++;
				progressInfo.Notify();
			}
		}

		if (!FCIFlushCabinet(hContext, FALSE, (PFNFCIGETNEXTCABINET)FciNextCabinet, (PFNFCISTATUS)FciStatus)) {
			FCIDestroy(hContext);
			_WU_RAISE_NATIVE_FCI_EXCEPTION(erfError.erfOper, L"FCIFlushCabinet", WriteErrorCategory::WriteError);
		}

		FCIDestroy(hContext);

		// Moving files to destination.
		WCHAR currentDirBuff[MAX_PATH + 1];
		GetCurrentDirectory(MAX_PATH + 1, currentDirBuff);

		WWuString currentDir(currentDirBuff);
		WWuString searchPath(currentDirBuff);
		searchPath = WWuString::Format(L"%ws\\%ws*", searchPath.Raw(), nameTemplate.Raw());

		WWuString finalDest;
		if (destination.EndsWith(L"\\"))
			finalDest = destination;
		else
			finalDest = destination + L"\\";

		WIN32_FIND_DATA findData;
		HANDLE hFind = FindFirstFileEx(searchPath.Raw(), FindExInfoBasic, &findData, FindExSearchNameMatch, NULL, 0);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				WWuString fullName = WWuString::Format(L"%ws\\%ws", currentDir.Raw(), findData.cFileName);
				WWuString destFullName = finalDest + findData.cFileName;
				DeleteFile(destFullName.Raw());
				MoveFile(fullName.Raw(), destFullName.Raw());
			} while (FindNextFile(hFind, &findData));
		}
	}

	std::tuple<__uint64, WWuString> Containers::GetCabinetTotalUncompressedSize(const WWuString& path, const WWuString& directory, WuList<CabinetProcessingInfo>& cabInfoList)
	{
		__uint64 totalSize = 0;
		HRESULT result = S_OK;

		// 'Moving' to the first cabinet.
		bool hasPrevious;
		UINT fileCount = 0;
		WWuString currentPath = path;
		CabinetProcessingInfo currentCabInfo{ };
		do {
			if (!TryGetInfoByPath(currentPath, cabInfoList, currentCabInfo)) {
				MemoryMappedFile mappedFile(currentPath);

				currentCabInfo = GetCabinetInformation(mappedFile);
				currentCabInfo.Path = currentPath;
				currentCabInfo.Name = IO::StripPath(currentPath);

				fileCount += currentCabInfo.FileCount;
				cabInfoList.Add(currentCabInfo);
			}

			if (!WuString::IsNullOrEmpty(currentCabInfo.PreviousCabinet)) {
				hasPrevious = true;

				// Dir size + file name size + possible '\' + \0
				size_t pathBufSize = directory.Length() + currentCabInfo.PreviousCabinet.Length() + 2;
				auto pathBuffer = std::make_unique<WCHAR[]>(pathBufSize);
				result = PathCchCombine(pathBuffer.get(), pathBufSize, directory.Raw(), currentCabInfo.PreviousCabinet.ToWide().Raw());
				if (result != S_OK)
					_WU_RAISE_NATIVE_EXCEPTION(result, L"PathCchCombine", WriteErrorCategory::InvalidResult);

				currentPath = pathBuffer.get();
			}
			else {
				hasPrevious = false;
				break;
			}

		} while (hasPrevious);

		// At this point this is the first cab.
		WWuString filePath = currentCabInfo.Path;

		// Processing each cabinet until there is no next.
		bool hasNext;
		WuList<WuString> processedFiles(fileCount);
		do {
			__uint64 filePointer = 0;
			MemoryMappedFile mappedFile(currentPath);

			// Checking list because we might not been through all cabs.
			if (!TryGetInfoByPath(currentPath, cabInfoList, currentCabInfo)) {
				MemoryMappedFile mappedFile(currentPath);

				currentCabInfo = GetCabinetInformation(mappedFile);
				currentCabInfo.Path = currentPath;
				currentCabInfo.Name = IO::StripPath(currentPath);

				cabInfoList.Add(currentCabInfo);
			}

			// Advancing to the first CFFILE entry.
			filePointer += currentCabInfo.CFFileOffset;

			// Going through each CFFILE entry.
			for (WORD i = 0; i < currentCabInfo.FileCount; i++) {
				DWORD currentSize = *reinterpret_cast<LPDWORD>((BYTE*)mappedFile.data() + filePointer);
				filePointer += 16;

				// Reading file name.
				CHAR currentChar;
				std::vector<CHAR> fileNameBuffer;
				do {
					currentChar = *reinterpret_cast<LPSTR>((BYTE*)mappedFile.data() + filePointer);
					if (currentChar != '\0')
						fileNameBuffer.push_back(currentChar);

					filePointer++;

				} while (currentChar != '\0');

				fileNameBuffer.push_back('\0');
				if (std::find(processedFiles.begin(), processedFiles.end(), fileNameBuffer.data()) == processedFiles.end()) {
					totalSize += currentSize;
					processedFiles.Add(fileNameBuffer.data());
				}
			}

			// Checking if there is a next.
			if (!WuString::IsNullOrEmpty(currentCabInfo.NextCabinet)) {
				hasNext = true;

				// Dir size + file name size + possible '\' + \0
				size_t pathBufSize = directory.Length() + currentCabInfo.NextCabinet.Length() + 2;
				auto pathBuffer = std::make_unique<WCHAR[]>(pathBufSize);
				result = PathCchCombine(pathBuffer.get(), pathBufSize, directory.Raw(), currentCabInfo.NextCabinet.ToWide().Raw());
				if (result != S_OK)
					_WU_RAISE_NATIVE_EXCEPTION(result, L"PathCchCombine", WriteErrorCategory::InvalidResult);

				currentPath = pathBuffer.get();
			}
			else {
				hasNext = false;
				break;
			}

		} while (hasNext);

		return { totalSize, filePath };
	}

	CabinetProcessingInfo Containers::GetCabinetInformation(const MemoryMappedFile& mappedFile)
	{
		__uint64 filePointer = 0;

		CHAR signature[5] = { 0 };
		RtlCopyMemory(signature, mappedFile.data(), 4);
		if (strcmp(signature, "MSCF") != 0)
			_WU_RAISE_NATIVE_EXCEPTION_WMESS(ERROR_BAD_FORMAT, L"GetCabinetInformation", WriteErrorCategory::InvalidData, L"File is not a cabinet.");

		CabinetProcessingInfo cabInfo{ };

		filePointer += 16;
		cabInfo.CFFileOffset = *reinterpret_cast<LPDWORD>((BYTE*)mappedFile.data() + filePointer);

		filePointer += 12;
		cabInfo.FileCount = *reinterpret_cast<LPWORD>((BYTE*)mappedFile.data() + filePointer);

		filePointer += 2;
		WORD flags = *reinterpret_cast<LPWORD>((BYTE*)mappedFile.data() + filePointer);

		filePointer += 4;
		cabInfo.CabinetIndex = *reinterpret_cast<LPWORD>((BYTE*)mappedFile.data() + filePointer);

		filePointer += 2;
		if ((flags & cfhdrRESERVE_PRESENT) > 0) {
			WORD cbSize = *reinterpret_cast<LPWORD>((BYTE*)mappedFile.data() + filePointer);
			filePointer = filePointer + cbSize + 2;
		}

		if ((flags & cfhdrPREV_CABINET) > 0) {
			std::vector<CHAR> buffer;
			CHAR currentChar;
			do {
				currentChar = *reinterpret_cast<LPSTR>((BYTE*)mappedFile.data() + filePointer);
				if (currentChar != '\0')
					buffer.push_back(currentChar);

				filePointer++;

			} while (currentChar != '\0');

			buffer.push_back('\0');
			cabInfo.PreviousCabinet = buffer.data();

			// Advancing the file pointer to the end of 'previous disk'.
			do {
				currentChar = *reinterpret_cast<LPSTR>((UCHAR*)mappedFile.data() + filePointer);
				filePointer++;

			} while (currentChar != '\0');
		}

		if ((flags & cfhdrNEXT_CABINET) > 0) {
			std::vector<CHAR> buffer;
			CHAR currentChar;
			do {
				currentChar = *reinterpret_cast<LPSTR>((UCHAR*)mappedFile.data() + filePointer);
				if (currentChar != '\0')
					buffer.push_back(currentChar);

				filePointer++;

			} while (currentChar != '\0');

			buffer.push_back('\0');
			cabInfo.NextCabinet = buffer.data();
		}

		return cabInfo;
	}

	bool Containers::TryGetInfoByPath(const WWuString& path, const WuList<CabinetProcessingInfo>& list, CabinetProcessingInfo& info)
	{
		const auto end = list.end();
		const auto iterator = std::find_if(list.begin(), end, [&](const CabinetProcessingInfo& item) { return item.Path == path; });
		if (iterator != end) {
			info = *iterator;
			return true;
		}

		return false;
	}

	bool Containers::TryGetInfoByName(const WWuString& name, const WuList<CabinetProcessingInfo>& list, CabinetProcessingInfo& info)
	{
		const auto end = list.end();
		const auto iterator = std::find_if(list.begin(), end, [&](const CabinetProcessingInfo& item) { return item.Name == name; });
		if (iterator != end) {
			info = *iterator;
			return true;
		}

		return false;
	}

#pragma region Callbacks

	INT_PTR Containers::OnCabinetInfo(PFDINOTIFICATION cabInfo)
	{
		auto operationInfo = reinterpret_cast<CabinetOperationInfo*>(cabInfo->pv);
		auto progressInfo = std::get<CabinetOperationInfo::FDI>(operationInfo->Info).Progress;

		progressInfo->CompletedCabinetCount++;
		progressInfo->Notify();

		return 0;
	}

	INT_PTR Containers::OnCopyFile(PFDINOTIFICATION cabInfo)
	{
		DWORD codePage = (cabInfo->attribs & _A_NAME_IS_UTF) ? CP_UTF8 : 1252;
		WWuString relativePath = WuString::ToWide(cabInfo->psz1, codePage).Replace('/', '\\');

		auto operationInfo = reinterpret_cast<CabinetOperationInfo*>(cabInfo->pv);

		auto splitPath = relativePath.Split('\\');

		const auto& fileName = splitPath.Back();
		if (IO::ContainsInvalidFileNameChars(fileName))
			_WU_RAISE_COR_EXCEPTION_WMESS(COR_E_ARGUMENT, L"ContainsInvalidFileNameChars", WriteErrorCategory::InvalidArgument, L"Cabinet file name contains invalid characters.");

		splitPath.Insert(0,*operationInfo->Destination);

		WuList<WWuString> splitDir(splitPath);
		splitDir.RemoveBack();

		WWuString targetDir;
		WWuString targetFullName;

		IO::CreatePath(splitDir, targetDir);
		if (IO::ContainsInvalidPathNameChars(targetDir))
			_WU_RAISE_COR_EXCEPTION_WMESS(COR_E_ARGUMENT, L"ContainsInvalidPathNameChars", WriteErrorCategory::InvalidArgument, L"Cabinet destination directory name contains invalid characters.");

		IO::CreateFolderTree(targetDir);

		IO::CreatePath(splitPath, targetFullName);
		WuString narrowFullName = targetFullName.ToMb(codePage);
		INT_PTR hFile = CabOpen(narrowFullName.Raw(), _O_TRUNC | _O_BINARY | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL, _S_IREAD | _S_IWRITE);
		if (hFile <= 0)
			_WU_RAISE_NATIVE_FDI_EXCEPTION(cabInfo->fdie, L"CabOpen", WriteErrorCategory::OpenError);

		auto progressData = std::get<CabinetOperationInfo::FDI>(operationInfo->Info).Progress;

		progressData->CurrentFile = relativePath;
		progressData->CurrentUncompressedSize = cabInfo->cb;
		progressData->CompletedSize += cabInfo->cb;
		progressData->Notify();

		return hFile;
	}

	INT_PTR Containers::OnCloseFileInfo(PFDINOTIFICATION cabInfo)
	{
		DWORD codePage = (cabInfo->attribs & _A_NAME_IS_UTF) ? CP_UTF8 : 1252;
		WWuString relativePath = WuString::ToWide(cabInfo->psz1, codePage).Replace('/', '\\');

		auto operationInfo = reinterpret_cast<CabinetOperationInfo*>(cabInfo->pv);

		WuList<WWuString> splitPath;
		splitPath.Add(*operationInfo->Destination);
		splitPath.Add(relativePath);

		WWuString targetFullName;
		IO::CreatePath(splitPath, targetFullName);

		CabClose(cabInfo->hf);
		IO::SetFileAttributesAndDate(targetFullName, cabInfo->date, cabInfo->time, cabInfo->attribs);

		return TRUE;
	}

	void* __cdecl Containers::CabAlloc(UINT cb)
	{
		return HeapAlloc(GetProcessHeap(), 0, cb);
	}

	void __cdecl Containers::CabFree(void* pv)
	{
		HeapFree(GetProcessHeap(), 0, pv);
	}

	INT_PTR __cdecl Containers::CabOpen(char* pszFile, int oflag, int pmode)
	{
		HANDLE hFile{ };
		DWORD desiredAccess = 0;
		DWORD creationDisposition = 0;

		UNREFERENCED_PARAMETER(pmode);

		if (oflag & _O_RDWR) {
			desiredAccess = GENERIC_READ | GENERIC_WRITE;
		}
		else if (oflag & _O_WRONLY) {
			desiredAccess = GENERIC_WRITE;
		}
		else {
			desiredAccess = GENERIC_READ;
		}

		if (oflag & _O_CREAT) {
			creationDisposition = CREATE_ALWAYS;
		}
		else {
			creationDisposition = OPEN_EXISTING;
		}

		hFile = CreateFileA(pszFile,
			desiredAccess,
			FILE_SHARE_READ,
			nullptr,
			creationDisposition,
			FILE_ATTRIBUTE_NORMAL,
			nullptr);

		return reinterpret_cast<INT_PTR>(hFile);
	}

	UINT __cdecl Containers::CabRead(INT_PTR hf, void* pv, UINT cb)
	{
		DWORD bytesRead = 0;

		if (!ReadFile(reinterpret_cast<HANDLE>(hf), pv, cb, &bytesRead, NULL))
			bytesRead = static_cast<DWORD>(-1L);

		return bytesRead;
	}

	UINT __cdecl Containers::CabWrite(INT_PTR hf, void* pv, UINT cb)
	{
		DWORD bytesWritten = 0;

		if (!WriteFile((HANDLE)hf, pv, cb, &bytesWritten, NULL))
			bytesWritten = static_cast<DWORD>(-1L);

		return bytesWritten;
	}

	int __cdecl Containers::CabClose(INT_PTR hf)
	{
		return (CloseHandle((HANDLE)hf) == TRUE) ? 0 : -1;
	}

	long __cdecl Containers::CabSeek(INT_PTR hf, long dist, int seektype)
	{
		return SetFilePointer(reinterpret_cast<HANDLE>(hf), dist, NULL, seektype);
	}

	INT_PTR __cdecl Containers::CabNotify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
	{
		auto operationInfo = reinterpret_cast<CabinetOperationInfo*>(pfdin->pv);

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
			return OnCabinetInfo(pfdin);

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
			std::get<CabinetOperationInfo::FDI>(operationInfo->Info).NextCabinet = WuString::ToWide(pfdin->psz1);
			return 0;

			// Do nothing.
		default:
			break;
		}

		return 0;
	}

	BOOL __cdecl Containers::FciGetTempFile(char* tempName, int cbTemp, void* pv)
	{
		UNREFERENCED_PARAMETER(pv);

		// Attempting to get temp path.
		CHAR tempPathBuffer[MAX_PATH + 1];
		GetTempPathA(MAX_PATH + 1, tempPathBuffer);

		// Attempting to get a temporary file name.
		__int64 index = 0;
		WuString tempFileName;
		do {
			__int64 currentIndex = index;
			tempFileName = WuString::Format("%sWuCab-%d.tmp", tempPathBuffer, index++);

			// Check if the file already exists.
			if (GetFileAttributesA(tempFileName.Raw()) == INVALID_FILE_ATTRIBUTES)
				break;

			// Overflowed. We're not going to find a non-existent file name.
			if (index < currentIndex)
				_WU_RAISE_COR_EXCEPTION_WMESS(COR_E_OVERFLOW, L"GetTempFile", WriteErrorCategory::LimitsExceeded, L"Could not find a non-existent temporary file name. Clean your %TEMP% location.");

		} while (true);

		strncpy_s(tempName, cbTemp, tempFileName.Raw(), cbTemp);

		return TRUE;
	}

	int __cdecl Containers::FciFilePlaced(PCCAB pccab, char* fileName, long cbFile, BOOL isContinuation, void* pv)
	{
		return 0;
	}

	int __cdecl Containers::FciDelete(char* fileName, int* err, void* pv)
	{
		UNREFERENCED_PARAMETER(pv);

		int result = 0;
		if (!DeleteFileA(fileName)) {
			*err = GetLastError();
			result = -1;
		}

		return result;
	}

	BOOL __cdecl Containers::FciNextCabinet(PCCAB pccab, ULONG cbPrevCab, void* pv)
	{
		UNREFERENCED_PARAMETER(pv);
		UNREFERENCED_PARAMETER(cbPrevCab);

		HRESULT hr;

		auto operationInfo = reinterpret_cast<CabinetOperationInfo*>(pv);
		auto& fciInfo = std::get<CabinetOperationInfo::FCI>(operationInfo->Info);

		WWuString fullCabTemplateName = *fciInfo.NameTemplate + L"%02d.cab";
		hr = StringCchPrintfA(
			pccab->szCab,
			ARRAYSIZE(pccab->szCab),
			fullCabTemplateName.ToMb(CP_UTF8).Raw(),
			pccab->iCab
		);

		fciInfo.Progress->CabinetName = WuString::ToWide(pccab->szCab);
		fciInfo.Progress->Notify();

		return (SUCCEEDED(hr));
	}

	long __cdecl Containers::FciStatus(UINT typeStatus, ULONG cb1, ULONG cb2, void* pv)
	{
		return 0;
	}

	INT_PTR __cdecl Containers::FciGetOpenInfo(char* name, USHORT* date, USHORT* time, USHORT* attributes, int* err, void* pv)
	{
		INT_PTR hFile;
		FILETIME fileTime;
		BY_HANDLE_FILE_INFORMATION fileInfo;

		hFile = CabOpen(name, _O_RDONLY | _O_BINARY, _S_IREAD);
		if (hFile != -1) {
			if (GetFileInformationByHandle(reinterpret_cast<HANDLE>(hFile), &fileInfo)
				&& FileTimeToLocalFileTime(&fileInfo.ftCreationTime, &fileTime)
				&& FileTimeToDosDateTime(&fileTime, date, time)) {
				*attributes = static_cast<USHORT>(fileInfo.dwFileAttributes);
				*attributes &= (_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH);
			}
			else {
				CabClose(hFile);
				hFile = -1;
			}
		}

		return hFile;
	}

#pragma endregion

#pragma endregion

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