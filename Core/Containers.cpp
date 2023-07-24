#include "pch.h"

#include "Containers.h"

namespace WindowsUtils::Core
{
	DWORD Containers::ExpandArchiveFile(const WuString& fileName, const WuString& filePath, const WuString& destination, ARCHIVE_FILE_TYPE fileType, Notification::PNATIVE_CONTEXT context)
	{
		DWORD result = ERROR_SUCCESS;
		switch (fileType)
		{
		case ARCHIVE_FILE_TYPE::Cabinet:
			result = ExpandCabinetFile(fileName, filePath, destination, context);
			break;
		}

		return result;
	}

	DWORD ExpandCabinetFile(const WuString& fileName, const WuString& filePath, const WuString& destination, Notification::PNATIVE_CONTEXT context)
	{
		DWORD result = ERROR_SUCCESS;
		ERF erfError;
		HFDI hContext;

		hContext = FDICreate(
			FdiFnMemAloc,
			FdiFnMemFree,
			FdiFnFileOpen,
			FdiFnFileRead,
			FdiFnFileWrite,
			FdiFnFileClose,
			FdiFnFileSeek,
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

		if (!FDICopy(hContext, fileName.GetAnsiBuffer(), filePath.GetAnsiBuffer(), 0, FdiFnNotifyCallback, NULL, NULL))
		{
			if (hContext != NULL)
				FDIDestroy(hContext);

			return (DWORD)erfError.erfOper;
		}

		if (hContext != NULL)
			FDIDestroy(hContext);

		return result;
	}

	// FDI Notification definition.

	_FDI_NOTIFICATION::_FDI_NOTIFICATION()
		: Name(), CabinetName(), _progressData()
	{ }

	_FDI_NOTIFICATION::~_FDI_NOTIFICATION()	{ }

	_FDI_NOTIFICATION& _FDI_NOTIFICATION::GetNotifier()
	{
		static _FDI_NOTIFICATION instance;
		return instance;
	}

	void _FDI_NOTIFICATION::WriteProgress(Notification::PNATIVE_CONTEXT context)
	{
		
	}

	// FDI macro functions.

	FNALLOC(FdiFnMemAloc)
	{
		return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
	}
	FNFREE(FdiFnMemFree)
	{
		HeapFree(GetProcessHeap(), NULL, pv);
	}
	FNOPEN(FdiFnFileOpen)
	{
		int fileHandle;

		if (oflag & _O_CREAT)
			SetFileAttributesA(pszFile, FILE_ATTRIBUTE_NORMAL);

		_sopen_s(&fileHandle, pszFile, oflag, _SH_DENYNO, pmode);

		return fileHandle;
	}
	FNREAD(FdiFnFileRead)
	{
		return _read(static_cast<int>(hf), pv, cb);
	}
	FNWRITE(FdiFnFileWrite)
	{
		return _write(static_cast<int>(hf), pv, cb);
	}
	FNCLOSE(FdiFnFileClose)
	{
		return _close(static_cast<int>(hf));
	}
	FNSEEK(FdiFnFileSeek)
	{
		return _lseek(static_cast<int>(hf), dist, seektype);
	}

	FNFDINOTIFY(FdiFnNotifyCallback)
	{
		switch (fdint)
		{
		// Called once for each cabinet opened by FDICopy, this includes
		// continuation cabinets opened due to files spanning cabinet boundaries.
		// Information:
		//	psz1: The name of the next cabinet, excluding path information.
		//	psz2: The name of the next disk.
		//	psz3: The cabinet path name.
		//	setID: The set ID of the current cabinet.
		//	iCabinet: The cabinet number within the cabinet set.
		case fdintCABINET_INFO:
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
			break;

		// Do nothing.
		default:
			break;
		}

		return 0;
	}

	template <class T>
	DWORD GetNumberDigitCount(T number)
	{
		DWORD digits = 0;
		while (number)
		{
			number /= 10;
			digits++;
		}

		return digits;
	}
}