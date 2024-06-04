#include "../../pch.h"

#include "../../Headers/Support/IO.h"
#include "../../Headers/Support/WuStdException.h"
#include "../../Headers/Support/SafeHandle.h"

#include <PathCch.h>
#include <Shlwapi.h>

namespace WindowsUtils::Core
{
	/*
	*	~ IO functions ~
	*/

	void IO::CreateFolderTree(const WWuString& path)
	{
		if (CheckDirectoryExists(path))
			return;

		WWuString parent(path);
		PathCchRemoveFileSpec(parent.GetBuffer(), parent.Length());

		if (!parent.Contains('\\'))
			throw WuStdException(ERROR_INVALID_DRIVE, __FILEW__, __LINE__);

		CreateFolderTree(parent);

		if (!CreateDirectoryW(path.GetBuffer(), NULL)) {
			DWORD dwResult = GetLastError();
			if (dwResult != ERROR_ALREADY_EXISTS)
				throw WuStdException(dwResult, __FILEW__, __LINE__);
		}
	}

	BOOL IO::CheckDirectoryExists(const WWuString& path)
	{
		DWORD result = GetFileAttributes(path.GetBuffer());
		if (result == INVALID_FILE_ATTRIBUTES)
			return FALSE;

		return (result & FILE_ATTRIBUTE_DIRECTORY) > 0;
	}

	void IO::TrimEndingDirectorySeparator(WWuString& path)
	{
		if (path.EndsWith('\\'))
			path.Remove(path.Length() - 1, 1);
	}

	void IO::SplitPath(const WWuString& path, WWuString& directory, WWuString& fileName)
	{
		directory = WWuString(path);
		fileName = WWuString(path);

		PathStripPath(fileName.GetBuffer());
		PathCchRemoveFileSpec(directory.GetBuffer(), directory.Length());

		fileName.Length();
		directory.Length();
	}

	void IO::CreatePath(wuvector<WWuString> strVector, WWuString& path)
	{
		for (WWuString piece : strVector) {
			if (path.EndsWith('\\') && piece.StartsWith('\\')) {
				path += piece.Remove(0);
			}
			else if (!path.EndsWith('\\') && !piece.StartsWith('\\')) {
				if (path.Length() == 0) {
					path += WWuString::Format(L"%ws", piece.GetBuffer());
				}
				else
					path += WWuString::Format(L"\\%ws", piece.GetBuffer());
			}
			else {
				path += piece;
			}
		}
	}

	void IO::SetFileAttributesAndDate(const WWuString& filePath, USHORT date, USHORT time, USHORT attributes)
	{
		HANDLE hFile = CreateFile(
			filePath.GetBuffer(),
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		if (hFile == INVALID_HANDLE_VALUE)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		FILETIME dateTime;
		if (DosDateTimeToFileTime(date, time, &dateTime)) {
			if (!((attributes & FILE_ATTRIBUTE_UTC_TIME) > 0)) {
				LocalFileTimeToFileTime(&dateTime, &dateTime);
				SetFileTime(hFile, &dateTime, NULL, &dateTime);
			}
		}

		CloseHandle(hFile);

		attributes &= FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE;
		SetFileAttributes(filePath.GetBuffer(), attributes);
	}

	void IO::AppendTextToFile(const HANDLE hFile, const WWuString& text)
	{
		if (hFile == INVALID_HANDLE_VALUE)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		WuString mbText = WWuStringToMultiByte(text.GetBuffer(), CP_UTF8);
		DWORD bytesWritten;
		if (!WriteFile(hFile, mbText.GetBuffer(), static_cast<DWORD>(mbText.Length()), &bytesWritten, NULL))
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);
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

		if (!GetFileSizeEx(hFile.get(), &size))
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		return size.QuadPart;
	}

	wuvector<FS_INFO> IO::EnumerateFileSystemInfo(WWuString& path)
	{
		std::vector<FS_INFO> output;
		WIN32_FIND_DATA data;

		HANDLE hFind = FindFirstFile(path.GetBuffer(), &data);
		if (hFind != INVALID_HANDLE_VALUE && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			WWuString name = path;
			PathStripPath(name.GetBuffer());

			FS_INFO info = {
				FsObjectType::File,
				name,
				path,
				(static_cast<__uint64>(data.nFileSizeHigh) << 32) | data.nFileSizeLow
			};

			output.push_back(info);

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
			// It mapped the whole C: drive in 24 sec, in opose of 45 from 'FindFirstFile'.
			hFind = FindFirstFileEx(globbedPath.GetBuffer(), FindExInfoBasic, &data, FindExSearchNameMatch, NULL, 0);
			if (hFind == INVALID_HANDLE_VALUE)
				return output;

			do {
				WWuString name = WWuString(data.cFileName);
				if (name == L"." || name == L"..")
					continue;

				bool isDir = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;
				WWuString fullPath = path + name;
				if (isDir) {
					FS_INFO info = {
						FsObjectType::Directory,
						name,
						fullPath,
						0
					};

					output.push_back(info);

					auto recVec = EnumerateFileSystemInfo(fullPath);
					output.insert(output.end(), recVec.begin(), recVec.end());
				}
				else {
					FS_INFO info = {
						FsObjectType::File,
						name,
						fullPath,
						(static_cast<__uint64>(data.nFileSizeHigh) << 32) | data.nFileSizeLow
					};

					output.push_back(info);
				}

			} while (FindNextFile(hFind, &data));

			FindClose(hFind);
		}

		return output;
	}

	bool IO::FileExists(const WWuString& filePath)
	{
		DWORD attributes = GetFileAttributes(filePath.GetBuffer());
		return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0);
	}

	void IO::GetFileDosPathFromDevicePath(WWuString& devicePath)
	{
		constexpr DWORD drivesBuffSize = 512;
		constexpr DWORD deviceBuffSize = 256;
		WCHAR logicalDrives[drivesBuffSize] { };

		WWuString deviceName = WWuString::Format(L"\\Device\\%ws", devicePath.Split(L'\\')[1].GetBuffer());

		DWORD charCount = GetLogicalDriveStrings(drivesBuffSize, logicalDrives);
		if (!charCount)
			throw WuStdException { static_cast<int>(GetLastError()), __FILEW__, __LINE__ };

		for (WWuString& drive : SplitDriveStrings(logicalDrives, charCount)) {
			const WCHAR trimmedChar[3] { drive[0], L':', L'\0' };
			WCHAR currentDevice[deviceBuffSize] { };
			if (!QueryDosDevice(trimmedChar, currentDevice, deviceBuffSize))
				throw WuStdException { static_cast<int>(GetLastError()), __FILEW__, __LINE__ };

			if (deviceName == currentDevice) {
				WWuString relativePath = devicePath.Remove(0, deviceName.Length() + 1);
				devicePath = WWuString::Format(L"%ws\\%ws", trimmedChar, relativePath.GetBuffer());
			}
		}

		// We didn't found the drive, so we return the same string.
	}

	void IO::GetFileDevicePathFromDosPath(WWuString& dosPath)
	{
		WWuString drive = WWuString::Format(L"%c:", dosPath[0]);
		WCHAR device[256] { };
		if (!QueryDosDevice(drive.GetBuffer(), device, 256))
			throw WuStdException { static_cast<int>(GetLastError()), __FILEW__, __LINE__ };

		WWuString relativePath = dosPath.Remove(0, 3);
		dosPath = WWuString::Format(L"%ws\\%ws", device, relativePath.GetBuffer());
	}

	void IO::WriteByteArrayToTempFile(BYTE* data, DWORD length, _Out_ WWuString& filePath)
	{
		WCHAR tempPathBuffer[MAX_PATH] { };
		WCHAR tempFileBuffer[MAX_PATH] { };
		WCHAR tempFullPath[MAX_PATH] { };

		if (!GetTempPath(MAX_PATH, tempPathBuffer))
			throw WuStdException { static_cast<int>(GetLastError()), __FILEW__, __LINE__ };

		if (!GetTempFileName(tempPathBuffer, L"tmp", 0, tempFileBuffer))
			throw WuStdException { static_cast<int>(GetLastError()), __FILEW__, __LINE__ };

		filePath = tempFileBuffer;
		FileHandle hFile { filePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL };

		DWORD bytesWritten { };
		if (!WriteFile(hFile.get(), reinterpret_cast<LPCVOID>(data), length, &bytesWritten, NULL))
			throw WuStdException { static_cast<int>(GetLastError()), __FILEW__, __LINE__ };
	}

	wuvector<WWuString> IO::SplitDriveStrings(const LPWSTR drives, const DWORD charCount)
	{
		wuvector<WWuString> output;
		if (charCount <= 4) {
			output.push_back(WWuString(drives));
			return output;
		}

		for (DWORD i = 0; i < charCount; i += 4) {
			LPWSTR currentOffset = drives + i;
			WCHAR currentDrive[4] { };
			wcscpy_s(currentDrive, 4, currentOffset);
			output.push_back(currentDrive);
		}

		return output;
	}

	/*
	*	~ Memory mapped file ~
	*/

	MemoryMappedFile::MemoryMappedFile(const WWuString& filePath)
		: m_mappedFile(NULL), m_view(NULL), m_length(0)
	{
		m_hFile = CreateFile(filePath.GetBuffer(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (m_hFile == INVALID_HANDLE_VALUE)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		LARGE_INTEGER length;
		if (!GetFileSizeEx(m_hFile, &length))
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		m_mappedFile = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
		if (m_mappedFile == NULL)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

		m_view = MapViewOfFile(m_mappedFile, FILE_MAP_READ, 0, 0, length.QuadPart);
		if (m_view == NULL)
			throw WuStdException(GetLastError(), __FILEW__, __LINE__);

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
	wuvector<AbstractPathTree::AptEntry>& AbstractPathTree::GetApt() { return m_apt; }

	AbstractPathTree::_AptEntry::_AptEntry(const WWuString& name, const WWuString& filePath, const WWuString& rootPath, __uint64 length, FsObjectType type)
	{
		WWuString relPath;
		WWuString finalRoot;
		std::vector<WWuString>fpSplit;
		std::vector<WWuString>rpSplit;

		// It's a file.
		if (filePath == rootPath) {
			relPath = L"\\" + name;
			goto END;
		}

		if (!rootPath.EndsWith('\\'))
			finalRoot = WWuString::Format(L"%ws\\", rootPath.GetBuffer());
		else
			finalRoot = rootPath;

		fpSplit = filePath.Split('\\');
		rpSplit = finalRoot.Split('\\');

		for (size_t i = 0; i < fpSplit.size(); i++) {
			if (i >= rpSplit.size())
				relPath = WWuString::Format(L"%ws\\%ws", relPath.GetBuffer(), fpSplit[i].GetBuffer());
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