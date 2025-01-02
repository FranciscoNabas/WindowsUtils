#include "../../pch.h"

#include "../../Headers/Support/IO.h"

namespace WindowsUtils::Core
{
	/*
	*	~ IO functions ~
	*/

	_NODISCARD WWuString IO::StripPath(const WWuString& path)
	{
		wchar_t buffer[MAX_PATH]{ };
		size_t len = path.Length() + 1;
		wcscpy_s(buffer, len, path.Raw());
		PathStripPath(buffer);

		return WWuString(buffer);
	}

	_NODISCARD WWuString IO::RemoveFileSpec(const WWuString& path, const bool throwOnFail)
	{
		wchar_t buffer[MAX_PATH]{ };
		size_t len = path.Length() + 1;
		wcscpy_s(buffer, len, path.Raw());
		HRESULT res = PathCchRemoveFileSpec(buffer, len);
		if (FAILED(res)) {
			if (res == S_FALSE) {
				if (throwOnFail)
					_WU_RAISE_NATIVE_EXCEPTION(res, L"PathCchRemoveFileSpec", WriteErrorCategory::InvalidResult);

				return WWuString(path);
			}

			_WU_RAISE_NATIVE_EXCEPTION(res, L"PathCchRemoveFileSpec", WriteErrorCategory::InvalidResult);
		}

		return WWuString(buffer);
	}

	void IO::CreateFolderTree(const WWuString& path)
	{
		if (CheckDirectoryExists(path))
			return;

		WWuString parent(path);
		PathCchRemoveFileSpec(parent.Raw(), parent.Length());

		if (!parent.Contains('\\'))
			_WU_RAISE_NATIVE_EXCEPTION(ERROR_INVALID_DRIVE, L"PathCchRemoveFileSpec", WriteErrorCategory::InvalidArgument);

		CreateFolderTree(parent);

		if (!CreateDirectoryW(path.Raw(), NULL)) {
			DWORD dwResult = GetLastError();
			if (dwResult != ERROR_ALREADY_EXISTS)
				_WU_RAISE_NATIVE_EXCEPTION(dwResult, L"CreateDirectory", WriteErrorCategory::InvalidResult);
		}
	}

	BOOL IO::CheckDirectoryExists(const WWuString& path)
	{
		DWORD result = GetFileAttributes(path.Raw());
		if (result == INVALID_FILE_ATTRIBUTES)
			return FALSE;

		return (result & FILE_ATTRIBUTE_DIRECTORY) > 0;
	}

	void IO::TrimEndingDirectorySeparator(WWuString& path)
	{
		if (path.EndsWith('\\'))
			path = path.Remove(path.Length() - 1, 1);
	}

	void IO::SplitPath(const WWuString& path, WWuString& directory, WWuString& fileName)
	{
		directory = WWuString(path);
		fileName = WWuString(path);

		PathStripPath(fileName.Raw());
		PathCchRemoveFileSpec(directory.Raw(), directory.Length());

		fileName.Length();
		directory.Length();
	}

	void IO::CreatePath(WuList<WWuString> strList, WWuString& path)
	{
		for (WWuString& piece : strList) {
			if (path.EndsWith('\\') && piece.StartsWith('\\')) {
				path += piece.Remove(0);
			}
			else if (!path.EndsWith('\\') && !piece.StartsWith('\\')) {
				if (path.Length() == 0) {
					path += WWuString::Format(L"%ws", piece.Raw());
				}
				else
					path += WWuString::Format(L"\\%ws", piece.Raw());
			}
			else {
				path += piece;
			}
		}
	}

	void IO::SetFileAttributesAndDate(const WWuString& filePath, USHORT date, USHORT time, USHORT attributes)
	{
		HANDLE hFile = CreateFile(
			filePath.Raw(),
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		if (hFile == INVALID_HANDLE_VALUE)
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"CreateFile", WriteErrorCategory::OpenError);

		FILETIME dateTime;
		if (DosDateTimeToFileTime(date, time, &dateTime)) {
			if (!((attributes & FILE_ATTRIBUTE_UTC_TIME) > 0)) {
				LocalFileTimeToFileTime(&dateTime, &dateTime);
				SetFileTime(hFile, &dateTime, NULL, &dateTime);
			}
		}

		CloseHandle(hFile);

		attributes &= FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE;
		SetFileAttributes(filePath.Raw(), attributes);
	}

	void IO::AppendTextToFile(const HANDLE hFile, const WWuString& text)
	{
		if (hFile == INVALID_HANDLE_VALUE)
			_WU_RAISE_NATIVE_EXCEPTION(ERROR_INVALID_HANDLE, L"AppendTextToFile", WriteErrorCategory::InvalidData);

		WuString mbText = text.ToMb(CP_UTF8);
		DWORD bytesWritten;
		if (!WriteFile(hFile, mbText.Raw(), static_cast<DWORD>(mbText.Length()), &bytesWritten, NULL))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"WriteFile", WriteErrorCategory::WriteError);
	}

	__uint64 IO::GetFileSize(const WWuString& filePath)
	{
		LARGE_INTEGER size;

		FileHandle hFile(
			filePath,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (!GetFileSizeEx(hFile.Get(), &size))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"GetFileSizeEx", WriteErrorCategory::InvalidResult);

		return size.QuadPart;
	}

	WuList<FS_INFO> IO::EnumerateFileSystemInfo(WWuString& path)
	{
		WuList<FS_INFO> output(20);
		WIN32_FIND_DATA data;

		HANDLE hFind = FindFirstFile(path.Raw(), &data);
		if (hFind != INVALID_HANDLE_VALUE && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			WWuString name = path;
			PathStripPath(name.Raw());

			output.Add(FsObjectType::File,
				name,
				path,
				(static_cast<__uint64>(data.nFileSizeHigh) << 32) | data.nFileSizeLow
			);

			FindClose(hFind);
		}
		else {
			WWuString globbedPath;
			if (path.EndsWith(L"\\"))
				globbedPath = path + L"*";
			else {
				globbedPath = path + L"\\*";
				path += L"\\";
			}

			// 'FindFirstFileEx' have a slight better performance than 'FindFirstFile' when used with 'FindExInfoBasic'.
			// It mapped the whole C: drive in 24 sec, in oppose of 45 from 'FindFirstFile'.
			hFind = FindFirstFileEx(globbedPath.Raw(), FindExInfoBasic, &data, FindExSearchNameMatch, NULL, 0);
			if (hFind == INVALID_HANDLE_VALUE)
				return output;

			do {
				WWuString name = WWuString(data.cFileName);
				if (name == L"." || name == L"..")
					continue;

				bool isDir = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;
				WWuString fullPath = path + name;
				if (isDir) {
					output.Add(FsObjectType::Directory,
						name,
						fullPath,
						0
					);

					auto recVec = EnumerateFileSystemInfo(fullPath);
					output.InsertRange(output.end(), recVec.begin(), recVec.end());
				}
				else {
					output.Add(FsObjectType::File,
						name,
						fullPath,
						(static_cast<__uint64>(data.nFileSizeHigh) << 32) | data.nFileSizeLow
					);
				}

			} while (FindNextFile(hFind, &data));

			FindClose(hFind);
		}

		return output;
	}

	bool IO::FileExists(const WWuString& filePath)
	{
		DWORD attributes = GetFileAttributes(filePath.Raw());
		return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0);
	}

	void IO::GetFileDosPathFromDevicePath(WWuString& devicePath)
	{
		constexpr DWORD drivesBuffSize = 512;
		constexpr DWORD deviceBuffSize = 256;
		WCHAR logicalDrives[drivesBuffSize] { };

		WWuString deviceName = WWuString::Format(L"\\Device\\%ws", devicePath.Split(L'\\')[1].Raw());

		DWORD charCount = GetLogicalDriveStrings(drivesBuffSize, logicalDrives);
		if (!charCount)
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"GetLogicalDriveStrings", WriteErrorCategory::InvalidResult);

		for (WWuString& drive : SplitDriveStrings(logicalDrives, charCount)) {
			const WCHAR trimmedChar[3] { drive[0], L':', L'\0' };
			WCHAR currentDevice[deviceBuffSize] { };
			if (!QueryDosDevice(trimmedChar, currentDevice, deviceBuffSize))
				_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"QueryDosDevice", WriteErrorCategory::InvalidResult);

			if (deviceName == currentDevice) {
				WWuString relativePath = devicePath.Remove(0, deviceName.Length() + 1);
				devicePath = WWuString::Format(L"%ws\\%ws", trimmedChar, relativePath.Raw());
			}
		}

		// We didn't found the drive, so we return the same string.
	}

	void IO::GetFileDevicePathFromDosPath(WWuString& dosPath)
	{
		WWuString drive = WWuString::Format(L"%c:", dosPath[0]);
		WCHAR device[256] { };
		if (!QueryDosDevice(drive.Raw(), device, 256))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"QueryDosDevice", WriteErrorCategory::InvalidResult);

		WWuString relativePath = dosPath.Remove(0, 3);
		dosPath = WWuString::Format(L"%ws\\%ws", device, relativePath.Raw());
	}

	void IO::WriteByteArrayToTempFile(BYTE* data, DWORD length, _Out_ WWuString& filePath)
	{
		WCHAR tempPathBuffer[MAX_PATH] { };
		WCHAR tempFileBuffer[MAX_PATH] { };
		WCHAR tempFullPath[MAX_PATH] { };

		if (!GetTempPath(MAX_PATH, tempPathBuffer))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"GetTempPath", WriteErrorCategory::InvalidResult);

		if (!GetTempFileName(tempPathBuffer, L"tmp", 0, tempFileBuffer))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"GetTempFileName", WriteErrorCategory::InvalidResult);

		filePath = tempFileBuffer;
		FileHandle hFile { filePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL };

		DWORD bytesWritten { };
		if (!WriteFile(hFile.Get(), reinterpret_cast<LPCVOID>(data), length, &bytesWritten, NULL))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"WriteFile", WriteErrorCategory::WriteError);
	}

	WuList<WWuString> IO::SplitDriveStrings(const LPWSTR drives, const DWORD charCount)
	{
		WuList<WWuString> output(4);
		if (charCount <= 4) {
			output.Add(drives);
			return output;
		}

		for (DWORD i = 0; i < charCount; i += 4) {
			LPWSTR currentOffset = drives + i;
			WCHAR currentDrive[4] { };
			wcscpy_s(currentDrive, 4, currentOffset);
			output.Add(currentDrive);
		}

		return output;
	}

	/*
	*	~ Memory mapped file ~
	*/

	MemoryMappedFile::MemoryMappedFile(const WWuString& filePath)
		: m_mappedFile(NULL), m_view(NULL), m_length(0)
	{
		m_hFile = CreateFile(filePath.Raw(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (m_hFile == INVALID_HANDLE_VALUE)
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"CreateFile", WriteErrorCategory::OpenError);

		LARGE_INTEGER length;
		if (!GetFileSizeEx(m_hFile, &length))
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"GetFileSizeEx", WriteErrorCategory::InvalidResult);

		m_mappedFile = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
		if (m_mappedFile == NULL)
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"CreateFileMapping", WriteErrorCategory::InvalidResult);

		m_view = MapViewOfFile(m_mappedFile, FILE_MAP_READ, 0, 0, length.QuadPart);
		if (m_view == NULL)
			_WU_RAISE_NATIVE_EXCEPTION(GetLastError(), L"MapViewOfFile", WriteErrorCategory::InvalidResult);

		m_length = length.QuadPart;
	}

	MemoryMappedFile::~MemoryMappedFile()
	{
		UnmapViewOfFile(m_view);
		CloseHandle(m_mappedFile);
		CloseHandle(m_hFile);
	}

	const PVOID MemoryMappedFile::data() const
	{
		return m_view;
	}

	const __uint64 MemoryMappedFile::size() const
	{
		return m_length;
	}

	/*
	*	~ Native Abstract Path Tree
	*/

	AbstractPathTree::AbstractPathTree(const AbstractPathTree& other)
	{
		for (AptEntry entry : other.m_apt) {
			if (entry.Type == FsObjectType::Directory)
				DirectoryCount++;
			else
				FileCount++;

			TotalLength += entry.Length;
			m_apt.push_back(entry);
		}
	}

	AbstractPathTree& AbstractPathTree::operator=(const AbstractPathTree& other)
	{
		for (AptEntry entry : other.m_apt) {
			if (entry.Type == FsObjectType::Directory)
				DirectoryCount++;
			else
				FileCount++;

			TotalLength += entry.Length;
			m_apt.push_back(entry);
		}

		return *this;
	}

	void AbstractPathTree::PushEntry(const AbstractPathTree::AptEntry& entry)
	{
		if (entry.Type == FsObjectType::Directory)
			DirectoryCount++;
		else
			FileCount++;

		TotalLength += entry.Length;
		m_apt.push_back(entry);
	}

	AbstractPathTree::AbstractPathTree()
		: FileCount(0), DirectoryCount(0), TotalLength(0)
	{ }

	AbstractPathTree::~AbstractPathTree() { }
	std::vector<AbstractPathTree::AptEntry>& AbstractPathTree::GetApt() { return m_apt; }

	AbstractPathTree::_AptEntry::_AptEntry(const WWuString& name, const WWuString& filePath, const WWuString& rootPath, __uint64 length, FsObjectType type)
	{
		WWuString relPath;
		WWuString finalRoot;
		WuList<WWuString> fpSplit;
		WuList<WWuString> rpSplit;

		// It's a file.
		if (filePath == rootPath) {
			relPath = L"\\" + name;
			goto END;
		}

		if (!rootPath.EndsWith('\\'))
			finalRoot = WWuString::Format(L"%ws\\", rootPath.Raw());
		else
			finalRoot = rootPath;

		fpSplit = filePath.Split('\\');
		rpSplit = finalRoot.Split('\\');

		for (size_t i = 0; i < fpSplit.Count(); i++) {
			if (i >= rpSplit.Count())
				relPath = WWuString::Format(L"%ws\\%ws", relPath.Raw(), fpSplit[i].Raw());
		}

	END:

		Name = name;
		FullPath = filePath;
		RelativePath = relPath.Remove(0);
		Type = type;
		Length = length;
	}

	AbstractPathTree::_AptEntry::~_AptEntry() { }
}