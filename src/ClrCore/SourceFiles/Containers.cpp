#include "..\pch.h"

#include "..\Headers\Containers.h"

namespace WindowsUtils::Core {
	// WuCabinet

	WuCabinet::WuCabinet(const WWuString& filePath, WuNativeContext* context) {
		// Checking if the path is a valid file.
		if (!PathFileExists(filePath.GetBuffer())) {
			throw WuResult(ERROR_FILE_NOT_FOUND, __FILEW__, __LINE__);
		}

		// Getting directory name.
		WWuString wideDir = WWuString(filePath);
		HRESULT result = PathCchRemoveFileSpec(wideDir.GetBuffer(), wideDir.Length());
		if (result != S_OK) {
			if (result == S_FALSE) {
				throw WuResult(ERROR_FILE_NOT_FOUND, L"Path missing file name.", __FILEW__, __LINE__);
			}
			else {
				throw WuResult(result, __FILEW__, __LINE__);
			}
		}
		_directory = WWuStringToNarrow(wideDir);

		GetCabinetTotalUncompressedSize(filePath);

		_progressInfo.CabinetSetCount = static_cast<DWORD>(_processedCabinet.size());
		_progressInfo.TotalUncompressedSize = _totalUncompressedSize;
		_progressInfo.CompletedCabinetCount = 0;
		_progressInfo.CompletedSize = 0;
		_progressInfo.CurrentUncompressedSize = 0;

		_context = context;
		_isUnicode = false;
		_instance = this;
	}

	WuCabinet::~WuCabinet() { }

	WuCabinet* WuCabinet::_instance { nullptr };

	void* __cdecl WuCabinet::FdiAlloc(UINT cb) {
		return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
	}

	void __cdecl WuCabinet::FdiFree(void* pv) {
		HeapFree(GetProcessHeap(), NULL, pv);
	}

	INT_PTR __cdecl WuCabinet::FdiOpen(char* pszFile, int oflag, int pmode) {
		int fileHandle;

		if (_instance->IsCurrentUnicode()) {
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

	UINT __cdecl WuCabinet::FdiRead(INT_PTR hf, void* pv, UINT cb) {
		return _read(static_cast<int>(hf), pv, cb);
	}

	UINT __cdecl WuCabinet::FdiWrite(INT_PTR hf, void* pv, UINT cb) {
		return _write(static_cast<int>(hf), pv, cb);
	}

	int __cdecl WuCabinet::FdiClose(INT_PTR hf) {
		return _close(static_cast<int>(hf));
	}

	long __cdecl WuCabinet::FdiSeek(INT_PTR hf, long dist, int seektype) {
		return _lseek(static_cast<int>(hf), dist, seektype);
	}

	INT_PTR __cdecl WuCabinet::FdiNotify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin) {
		return _instance->NotifyCallback(fdint, pfdin);
	}

	const bool WuCabinet::IsCurrentUnicode() const {
		return _isUnicode;
	}

	const wuvector<CABINET_PROCESSING_INFO>& WuCabinet::GetCabinetInfo() {
		return _processedCabinet;
	}

	void WuCabinet::GetCabinetTotalUncompressedSize(const WWuString& filePath) {
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

				_processedCabinet.push_back(*buffer.get());
				currentCabInfo = buffer.get();
			}

			if (!WuString::IsNullOrEmpty(currentCabInfo->PreviousCabinet)) {
				hasPrevious = true;

				// Dir size + file name size + possible '\' + \0
				size_t pathBufSize = _directory.Length() + currentCabInfo->PreviousCabinet.Length() + 2;
				auto pathBuffer = make_wuunique<WCHAR[]>(pathBufSize);
				result = PathCchCombine(pathBuffer.get(), pathBufSize, WuStringToWide(_directory).GetBuffer(), WuStringToWide(currentCabInfo->PreviousCabinet).GetBuffer());
				if (result != S_OK) {
					throw WuResult(result, __FILEW__, __LINE__);
				}

				currentPath = pathBuffer.get();
			}
			else {
				hasPrevious = false;
				break;
			}

		} while (hasPrevious);

		// At this point this is the first cab.
		_filePath = currentCabInfo->Path;

		// Processing each cabinet until there is no next.
		bool hasNext;
		auto processedFiles = make_wusunique_vector<WuString>();
		do {
			try {
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

					_processedCabinet.push_back(*buffer.get());
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
					size_t pathBufSize = _directory.Length() + currentCabInfo->NextCabinet.Length() + 2;
					auto pathBuffer = make_wuunique<WCHAR[]>(pathBufSize);
					result = PathCchCombine(pathBuffer.get(), pathBufSize, WuStringToWide(_directory).GetBuffer(), WuStringToWide(currentCabInfo->NextCabinet).GetBuffer());
					if (result != S_OK) {
						throw WuResult(result, __FILEW__, __LINE__);
					}

					currentPath = pathBuffer.get();
				}
				else {
					hasNext = false;
					break;
				}
			}
			catch (const WuResult& ex) {
				throw WuResult(ex.Result, WWuString::Format(L"Failed getting size information for next cabinet. %ws", ex.Message), __FILEW__, __LINE__);
			}

		} while (hasNext);

		_totalUncompressedSize = totalSize;
	}

	void WuCabinet::GetCabinetInformation(const MemoryMappedFile& mappedFile, CABINET_PROCESSING_INFO* cabInfo) {
		__uint64 filePointer = 0;

		CHAR signature[5] = { 0 };
		RtlCopyMemory(signature, mappedFile.data(), 4);
		if (strcmp(signature, "MSCF") != 0)
			throw WuResult(ERROR_BAD_FORMAT, L"File is not a cabinet.", __FILEW__, __LINE__);

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

	_NODISCARD CABINET_PROCESSING_INFO* WuCabinet::GetInfoByPath(const WWuString& filePath) {
		for (CABINET_PROCESSING_INFO& cabInfo : _processedCabinet)
			if (cabInfo.Path == filePath)
				return &cabInfo;

		return NULL;
	}

	_NODISCARD CABINET_PROCESSING_INFO* WuCabinet::GetInfoByName(const WWuString& cabName) {
		for (CABINET_PROCESSING_INFO& cabInfo : _processedCabinet)
			if (cabInfo.Name == cabName)
				return &cabInfo;

		return NULL;
	}

	WuResult WuCabinet::ExpandCabinetFile(const WWuString& destination) {
		ERF erfError = { 0 };
		HFDI hContext;

		hContext = FDICreate(
			(PFNALLOC)FdiAlloc,
			(PFNFREE)FdiFree,
			(PFNOPEN)FdiOpen,
			(PFNREAD)FdiRead,
			(PFNWRITE)FdiWrite,
			(PFNCLOSE)FdiClose,
			(PFNSEEK)FdiSeek,
			cpuUNKNOWN,
			&erfError
		);

		if (hContext == NULL)
			return WuResult::GetResultFromFdiError((FDIERROR)erfError.erfOper, __FILEW__, __LINE__);

		if (!PathIsDirectory(destination.GetBuffer()))
			return WuResult(ERROR_FILE_NOT_FOUND, __FILEW__, __LINE__);

		// FDICopy does not manage trailing path separator. It just concatenates
		// file name and file path.
		_destination = destination;
		if (!_directory.EndsWith('\\')) {
			_directory += "\\";
		}

		WuString filePath = WWuStringToNarrow(_filePath);
		WuString fileName = filePath;
		PathStripPathA(fileName.GetBuffer());

		do {
			_nextCabinet.Clear();
			_progressInfo.CabinetName = WuStringToWide(fileName);
			
			if (!FDICopy(hContext, fileName.GetBuffer(), _directory.GetBuffer(), 0, (PFNFDINOTIFY)FdiNotify, NULL, NULL)) {
				if (hContext != NULL)
					FDIDestroy(hContext);

				return WuResult::GetResultFromFdiError((FDIERROR)erfError.erfOper, __FILEW__, __LINE__);
			}

			if (_nextCabinet.Length() == 0)
				break;

			CABINET_PROCESSING_INFO* nextCabInfo = GetInfoByName(_nextCabinet);
			if (nextCabInfo == NULL)
				return WuResult(ERROR_FILE_NOT_FOUND, __FILEW__, __LINE__);

			filePath = WWuStringToNarrow(nextCabInfo->Path);
			fileName = WWuStringToNarrow(nextCabInfo->Path);
			PathStripPathA(fileName.GetBuffer());

		} while (_nextCabinet.Length() != 0);
		
		if (hContext != NULL)
			FDIDestroy(hContext);

		return WuResult();
	}

	INT_PTR WuCabinet::NotifyCallback(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin) {
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
				_nextCabinet = WuStringToWide(pfdin->psz1);
				return 0;

			// Do nothing.
			default:
				break;
		}
		
		return 0;
	}

	INT_PTR WuCabinet::OnCabinetInfo() {
		_progressInfo.CompletedCabinetCount++;
		
#ifndef _DEBUG
		_progressInfo.Notify(_context);
#endif

		return 0;
	}

	INT_PTR WuCabinet::OnCopyFile(PFDINOTIFICATION cabInfo) {
		DWORD codePage = (cabInfo->attribs & _A_NAME_IS_UTF) ? CP_UTF8 : 1252;
		WWuString relativePath = WuStringToWide(cabInfo->psz1, codePage).Replace('/', '\\');

		if (codePage == CP_UTF8)
			_isUnicode = true;
		else
			_isUnicode = false;

		auto splitPath = relativePath.Split('\\');

		splitPath.insert(splitPath.begin(), _destination);
		
		wuvector<WWuString> splitDir(splitPath);
		splitDir.pop_back();

		WWuString targetDir;
		WWuString targetFullName;

		IO::CreatePath(splitDir, targetDir);
		IO::CreatePath(splitPath, targetFullName);

		IO::CreateFolderTree(targetDir);

		WuString narrowFullName = WWuStringToMultiByte(targetFullName.GetBuffer(), codePage);
		INT_PTR hFile = FdiOpen(narrowFullName.GetBuffer(), _O_TRUNC | _O_BINARY | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL, _S_IREAD | _S_IWRITE);
		if (hFile <= 0)
			throw WuResult::GetResultFromFdiError(cabInfo->fdie, __FILEW__, __LINE__);

		_progressInfo.CurrentFile = relativePath;
		_progressInfo.CurrentUncompressedSize = cabInfo->cb;
		_progressInfo.CompletedSize += cabInfo->cb;

#ifndef _DEBUG
		_progressInfo.Notify(_context);
#endif

		return hFile;
	}

	INT_PTR WuCabinet::OnCloseFileInfo(PFDINOTIFICATION cabInfo) {
		DWORD codePage = (cabInfo->attribs & _A_NAME_IS_UTF) ? CP_UTF8 : 1252;
		WWuString relativePath = WuStringToWide(cabInfo->psz1, codePage).Replace('/', '\\');
		
		wuvector<WWuString> splitPath;
		splitPath.push_back(_destination);
		splitPath.push_back(relativePath);

		WWuString targetFullName;
		IO::CreatePath(splitPath, targetFullName);

		FdiClose(cabInfo->hf);
		IO::SetFileAttributesAndDate(targetFullName, cabInfo->date, cabInfo->time, cabInfo->attribs);

		return TRUE;
	}
	
	// FDI Notification definition.

	_FDI_PROGRESS::_FDI_PROGRESS() { }

	_FDI_PROGRESS::~_FDI_PROGRESS() { }

	void _FDI_PROGRESS::Notify(WuNativeContext* context) {
		float floatPercent = (static_cast<float>(CompletedSize) / TotalUncompressedSize);
		floatPercent *= 100;
		long percentComplete = lround(floatPercent);
		WWuString status = WWuString::Format(L"Cabinet %d/%d: %ws. File: %ws. %ld/%ld", CompletedCabinetCount, CabinetSetCount, CabinetName.GetBuffer(), CurrentFile.GetBuffer(), CompletedSize, TotalUncompressedSize);
		
		Notification::MAPPED_PROGRESS_DATA progressData(
			L"Expanding cabinet...", 0, NULL, -1, static_cast<WORD>(percentComplete), Notification::PROGRESS_RECORD_TYPE::Processing, -1, status.GetBuffer()
		);

		context->NativeWriteProgress(&progressData);
	}

	template <class T>
	DWORD GetNumberDigitCount(T number) {
		DWORD digits = 0;
		while (number) {
			number /= 10;
			digits++;
		}

		return digits;
	}
}