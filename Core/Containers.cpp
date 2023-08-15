#include "pch.h"

#include "Containers.h"

namespace WindowsUtils::Core {
	DWORD Containers::ExpandArchiveFile(WuString& fileName, WuString& filePath, const WuString& destination, ARCHIVE_FILE_TYPE fileType, Notification::PNATIVE_CONTEXT context) {
		DWORD result = ERROR_SUCCESS;
		switch (fileType) {
			case ARCHIVE_FILE_TYPE::Cabinet:
				// result = ExpandCabinetFile(fileName, filePath, destination, context);
				break;
		}

		return result;
	}

	DWORD WuCabinet::ExpandCabinetFile(const WuString& destination, Notification::PNATIVE_CONTEXT context) {
		DWORD result = ERROR_SUCCESS;
		ERF erfError;
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
			return (DWORD)erfError.erfOper;

		/*
		*	TODO:
		*
		*	Set up progress notification thread first!
		*
		*	Call 'FDIIsCabinet' to get cabinet information to set up the progress.
		*	Values like cabinet name, and path will be filled by the notification callback.
		*
		*	Use the 'hasnext' property from 'FDICABINETINFO' to determine how many cabinets are there.
		*	Call 'FDIIsCabinet' on each one to determine the complete number of files to extract, thus
		*	having an accurate progress bar.
		*/

		WuString filePath = _filePath;
		WuString fileName = filePath;
		PathStripPathA(fileName.GetBuffer());

		if (!FDICopy(hContext, fileName.GetBuffer(), filePath.GetBuffer(), 0, FdiNotify, NULL, NULL)) {
			if (hContext != NULL)
				FDIDestroy(hContext);

			return (DWORD)erfError.erfOper;
		}

		if (hContext != NULL)
			FDIDestroy(hContext);

		return result;
	}

	// WuCabinet

	WuCabinet::WuCabinet(const WWuString& filePath, Notification::PNATIVE_CONTEXT context) {
		// Checking if the path is a valid file.
		if (!PathFileExists(filePath.GetBuffer())) {
			throw WuOsException(ERROR_FILE_NOT_FOUND);
		}

		// Getting directory name.
		_directory = WWuStringToNarrow(filePath);
		HRESULT result = PathCchRemoveFileSpec((PWSTR)_directory.GetBuffer(), _directory.Length());
		if (result != S_OK) {
			if (result == S_FALSE) {
				throw WuOsException(ERROR_FILE_NOT_FOUND, L"Path missing file name.");
			}
			else {
				throw WuOsException(result);
			}
		}

		GetCabinetTotalUncompressedSize(filePath);

		_progressInfo.CabinetSetCount = static_cast<DWORD>(_processedCabinet.size());
		_progressInfo.TotalUncompressedSize = _totalUncompressedSize;
		_progressInfo.CompletedCabinetCount = 0;
		_progressInfo.CompletedSize = 0;
		_progressInfo.CurrentUncompressedSize = 0;
	}

	WuCabinet* WuCabinet::GetWuCabinet(const WWuString& filePath, Notification::PNATIVE_CONTEXT context) {
		if (_instance == nullptr) {
			_instance = new WuCabinet(filePath, context);
		}
		
		return _instance;
	}

	WuCabinet* WuCabinet::GetWuCabinet() {
		if (_instance == nullptr) {
			throw WuOsException(0x80004003, L"WuCabinet instance was not initialized.");
		}

		return _instance;
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
				OnCabinetInfo(WuString(pfdin->psz3));
				break;

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
				break;

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
				OnCopyFile(pfdin);
				break;

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
				OnCloseFileInfo(pfdin);
				break;

			// Do nothing.
			default:
				break;
		}
		
		return 0;
	}

	void WuCabinet::OnCabinetInfo(const WuString& cabName) {
		_progressInfo.CabinetName = cabName;
		_progressInfo.CompletedCabinetCount++;
		_progressInfo.Notify(_context);
	}

	void WuCabinet::OnCopyFile(PFDINOTIFICATION cabInfo) {

	}

	void WuCabinet::OnCloseFileInfo(PFDINOTIFICATION cabInfo) {

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
					throw WuOsException(result);
				}

				currentPath = pathBuffer.get();
			}
			else {
				hasPrevious = false;
				break;
			}

		} while (hasPrevious);

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
						throw WuOsException(result);
					}

					currentPath = pathBuffer.get();
				}
				else {
					hasNext = false;
					break;
				}
			}
			catch (const WuOsException& ex) {
				throw WuOsException(ex.ErrorCode, WWuString::Format(L"Failed getting size information for next cabinet. %ws", ex.Message));
			}

		} while (hasNext);

		_totalUncompressedSize = totalSize;
	}

	void WuCabinet::GetCabinetInformation(const MemoryMappedFile& mappedFile, CABINET_PROCESSING_INFO* cabInfo) {
		__uint64 filePointer = 0;
		
		CHAR signature[5] = { 0 };
		RtlCopyMemory(signature, mappedFile.data(), 4);
		if (strcmp(signature, "MSCF") != 0)
			throw WuOsException(ERROR_BAD_FORMAT, L"File is not a cabinet.");

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
	
	// FDI Notification definition.

	FDI_PROGRESS::_FDI_PROGRESS() { }

	FDI_PROGRESS::~_FDI_PROGRESS() { }

	void FDI_PROGRESS::Notify(Notification::PNATIVE_CONTEXT context) {
		WORD percentComplete = (CompletedSize / TotalUncompressedSize) * 100;
		WWuString status = WWuString::Format(L"Cabinet %d/%d: %ws. File: %ws. %ld/%ld", CompletedCabinetCount, CabinetSetCount, CabinetName, CurrentFile, CompletedSize, TotalUncompressedSize);
		
		Notification::MAPPED_PROGRESS_DATA progressData(
			L"Expanding cabinet", 0, NULL, -1, percentComplete, Notification::PROGRESS_RECORD_TYPE::Processing, -1, status
		);

		NativeWriteProgress(context, &progressData);
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