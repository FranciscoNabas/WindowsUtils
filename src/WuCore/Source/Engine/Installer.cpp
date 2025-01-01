#include "../../pch.h"

#include "../../Headers/Engine/Installer.h"

namespace WindowsUtils::Core
{
	_WU_SUMMARY_PROPERTY::_WU_SUMMARY_PROPERTY(const LPWSTR name, const WORD index)
		: Name(name), Index(index) { }

	Installer::Installer(const WWuString& filePath, const MsiPersistenceMode mode, const Core::WuNativeContext* context)
	{
		m_context = context;
		m_databasePath = filePath;

		OpenDatabase(filePath, mode);
	}

	Installer::Installer(const WWuString& filePath, const MsiPersistenceMode mode, const WWuString& viewQuery, const Core::WuNativeContext* context)
	{
		m_context = context;
		m_databasePath = filePath;

		OpenDatabase(filePath, mode);
		OpenDatabaseView(viewQuery);
	}

	Installer::~Installer()
	{
		if (m_hDatabase) { MsiCloseHandle(m_hDatabase); }
		if (m_hCurrentView) { MsiCloseHandle(m_hCurrentView); }
		if (m_hSummaryInfo) { MsiCloseHandle(m_hSummaryInfo); }
	}

	// Views.
	void Installer::OpenDatabaseView(const WWuString& query)
	{
		if (!m_hDatabase)
			_WU_RAISE_NATIVE_EXCEPTION(ERROR_INVALID_HANDLE, L"OpenDatabaseView", WriteErrorCategory::InvalidData);

		if (m_hCurrentView)
			MsiCloseHandle(m_hCurrentView);

		DWORD result = MsiDatabaseOpenView(m_hDatabase, query.Raw(), &m_hCurrentView);
		if (result != ERROR_SUCCESS) {
			PMSIHANDLE hLastRecord = MsiGetLastErrorRecord();
			if (hLastRecord) {
				WWuString message = FormatRecord(NULL, hLastRecord);
				_WU_RAISE_NATIVE_EXCEPTION_WMESS(result, L"MsiDatabaseOpenView", WriteErrorCategory::OpenError, WWuString::Format(L"Error opening view: %ws", message.Raw()));
			}
			else {
				_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiDatabaseOpenView", WriteErrorCategory::OpenError);
			}
		}
	}

	void Installer::ViewExecute(_In_opt_ MSIHANDLE hRecord)
	{
		if (!m_hCurrentView) {
			_WU_RAISE_NATIVE_EXCEPTION(ERROR_INVALID_HANDLE, L"ViewExecute", WriteErrorCategory::InvalidData);
		}

		DWORD result = MsiViewExecute(m_hCurrentView, hRecord);
		if (result != ERROR_SUCCESS) {
			PMSIHANDLE hLastRecord = MsiGetLastErrorRecord();
			if (hLastRecord) {
				WWuString message = FormatRecord(NULL, hLastRecord);
				_WU_RAISE_NATIVE_EXCEPTION_WMESS(result, L"MsiViewExecute", WriteErrorCategory::InvalidResult, WWuString::Format(L"Error executing view: %ws", message.Raw()));
			}
			else {
				_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiViewExecute", WriteErrorCategory::InvalidResult);
			}
		}
	}

	bool Installer::ViewFetch(MSIHANDLE* hRecord)
	{
		if (!m_hCurrentView) {
			_WU_RAISE_NATIVE_EXCEPTION(ERROR_INVALID_HANDLE, L"ViewFetch", WriteErrorCategory::InvalidData);
		}

		DWORD result = MsiViewFetch(m_hCurrentView, hRecord);
		if (result != ERROR_SUCCESS) {
			if (result == ERROR_NO_MORE_ITEMS)
				return false;

			PMSIHANDLE hLastRecord = MsiGetLastErrorRecord();
			if (hLastRecord) {
				WWuString message = FormatRecord(NULL, hLastRecord);
				_WU_RAISE_NATIVE_EXCEPTION_WMESS(result, L"MsiViewFetch", WriteErrorCategory::InvalidResult, WWuString::Format(L"Error fetching view: %ws", message.Raw()));
			}
			else {
				_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiViewFetch", WriteErrorCategory::InvalidResult);
			}
		}

		if (hRecord == 0)
			return false;

		return true;
	}

	
	// Read from a record.
	WuList<char> Installer::RecordReadStream(MSIHANDLE hRecord, int fieldIndex)
	{
		DWORD result;

		WuList<char> output(100);
		do {
			char buffer[1024] { };
			DWORD buffSize = 1024;
			result = MsiRecordReadStream(hRecord, fieldIndex, buffer, &buffSize);
			if (result != ERROR_SUCCESS) {
				_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiRecordReadStream", WriteErrorCategory::ReadError);
			}

			if (buffSize > 0) {
				char* const start = &buffer[0];
				output.AddRange(start, start + buffSize);
			}

			if (buffSize < 1024)
				break;

		} while (true);

		return output;
	}

	WWuString Installer::RecordGetString(MSIHANDLE hRecord, int fieldIndex)
	{
		DWORD charsNeeded { };
		DWORD result = MsiRecordGetString(hRecord, fieldIndex, NULL, &charsNeeded);
		if (result != ERROR_SUCCESS && result != ERROR_MORE_DATA) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiRecordGetString", WriteErrorCategory::ReadError);
		}

		charsNeeded++;
		std::unique_ptr<WCHAR[]> buffer = std::make_unique<WCHAR[]>(charsNeeded);
		result = MsiRecordGetString(hRecord, fieldIndex, buffer.get(), &charsNeeded);
		if (result != ERROR_SUCCESS) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiRecordGetString", WriteErrorCategory::ReadError);
		}

		return WWuString(buffer.get());
	}


	// Modify a record.
	void Installer::RecordSetString(MSIHANDLE hRecord, UINT iField, const WWuString& szValue)
	{
		DWORD result = MsiRecordSetString(hRecord, iField, szValue.Raw());
		if (result != ERROR_SUCCESS) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiRecordSetString", WriteErrorCategory::WriteError);
		}
	}

	void Installer::RecordSetInteger(MSIHANDLE hRecord, UINT iField, int iValue)
	{
		DWORD result = MsiRecordSetInteger(hRecord, iField, iValue);
		if (result != ERROR_SUCCESS) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiRecordSetInteger", WriteErrorCategory::WriteError);
		}
	}

	void Installer::RecordSetStream(MSIHANDLE hRecord, UINT iField, const WWuString& szFilePath)
	{
		DWORD result = MsiRecordSetStream(hRecord, iField, szFilePath.Raw());
		if (result != ERROR_SUCCESS) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiRecordSetStream", WriteErrorCategory::WriteError);
		}
	}


	// Commit to the database.
	void Installer::DatabaseCommit()
	{
		DWORD result = MsiDatabaseCommit(m_hDatabase);
		if (result != ERROR_SUCCESS) {
			PMSIHANDLE hLastRecord = MsiGetLastErrorRecord();
			if (hLastRecord) {
				WWuString message = FormatRecord(NULL, hLastRecord);
				_WU_RAISE_NATIVE_EXCEPTION_WMESS(result, L"MsiDatabaseCommit", WriteErrorCategory::WriteError, WWuString::Format(L"Error committing to the database: %ws", message.Raw()));
			}
			else {
				_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiDatabaseCommit", WriteErrorCategory::WriteError);
			}
		}
	}


	// Utilities.
	WWuString Installer::FormatRecord(MSIHANDLE hInstall, MSIHANDLE hRecord)
	{
		DWORD charsNeeded { };
		DWORD result = MsiFormatRecord(hInstall, hRecord, NULL, &charsNeeded);
		if (result != ERROR_SUCCESS && result != ERROR_MORE_DATA) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiFormatRecord", WriteErrorCategory::InvalidResult);
		}

		charsNeeded++;
		std::unique_ptr<WCHAR[]> buffer = std::make_unique<WCHAR[]>(charsNeeded);

		result = MsiFormatRecord(hInstall, hRecord, buffer.get(), &charsNeeded);
		if (result != ERROR_SUCCESS) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiFormatRecord", WriteErrorCategory::InvalidResult);
		}

		return WWuString(buffer.get());
	}

	void Installer::ProcessSummaryInfo(WU_SUMMARY_INFO& info)
	{
		if (m_hSummaryInfo)
			MsiCloseHandle(m_hSummaryInfo);

		GetSummaryInfo();

		for (WORD pid : { 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16, 18, 19 }) {
			UINT dataType { };
			int intValue { };
			FILETIME ftValue { };
			DWORD charsNeeded { };
			std::unique_ptr<WCHAR[]> buffer;

			// Windows Installer functions that return data in a user provided memory location should not
			// be called with NULL as the value for the pointer, so we pass an empty string.
			DWORD result = MsiSummaryInfoGetProperty(m_hSummaryInfo, pid, &dataType, &intValue, &ftValue, L"", &charsNeeded);
			if (result != ERROR_SUCCESS) {
				if (result != ERROR_MORE_DATA) {
					_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiSummaryInfoGetProperty", WriteErrorCategory::InvalidResult);
				}

				charsNeeded++;
				buffer = std::make_unique<WCHAR[]>(charsNeeded);
				result = MsiSummaryInfoGetProperty(m_hSummaryInfo, pid, &dataType, &intValue, &ftValue, buffer.get(), &charsNeeded);
				if (result != ERROR_SUCCESS) {
					_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiSummaryInfoGetProperty", WriteErrorCategory::InvalidResult);
				}
			}

			// I know, not my brightest moment over here.
			switch (pid) {
				case 1:
					info.Codepage = static_cast<WORD>(intValue);
					break;
				case 2:
					info.Title = buffer.get();
					break;
				case 3:
					info.Subject = buffer.get();
					break;
				case 4:
					info.Author = buffer.get();
					break;
				case 5:
					info.Keywords = buffer.get();
					break;
				case 6:
					info.Comments = buffer.get();
					break;
				case 7:
					info.Template = buffer.get();
					break;
				case 8:
					info.LastSavedBy = buffer.get();
					break;
				case 9:
					info.RevisionNumber = buffer.get();
					break;
				case 11:
					info.LastPrinted = ftValue;
					break;
				case 12:
					info.CreateTimeDate = ftValue;
					break;
				case 13:
					info.LastSaveTimeDate = ftValue;
					break;
				case 14:
					info.PageCount = intValue;
					break;
				case 15:
					info.WordCount = intValue;
					break;
				case 16:
					info.CharacterCount = intValue;
					break;
				case 18:
					info.CreatingApplication = buffer.get();
					break;
				case 19:
					info.Security = intValue;
					break;
			}
		}
	}

	WuList<WWuString> Installer::GetMsiTableNames()
	{
		WuList<WWuString> output(60);
		OpenDatabaseView(L"Select * From _Tables");
		ViewExecute(NULL);

		PMSIHANDLE hRecord { };
		while (ViewFetch(&hRecord)) {
			output.Add(RecordGetString(hRecord, 1));
			MsiCloseHandle(hRecord);
		}

		return output;
	}

	WuList<WWuString> Installer::GetMsiTableKeys(const WWuString& tableName)
	{
		PMSIHANDLE hRecord { };
		
		UINT result = MsiDatabaseGetPrimaryKeys(m_hDatabase, tableName.Raw(), &hRecord);
		if (result != ERROR_SUCCESS) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiDatabaseGetPrimaryKeys", WriteErrorCategory::InvalidResult);
		}

		WuList<WWuString> output(2);
		for (UINT i = 1; i <= MsiRecordGetFieldCount(hRecord); i++)
			output.Add(RecordGetString(hRecord, i));

		return output;
	}

	void Installer::GetMsiColumnInfo(MSIHANDLE* hNames, MSIHANDLE* hTypes)
	{
		UINT result = MsiViewGetColumnInfo(m_hCurrentView, MSICOLINFO_NAMES, hNames);
		if (result != ERROR_SUCCESS) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiViewGetColumnInfo", WriteErrorCategory::InvalidResult);
		}

		result = MsiViewGetColumnInfo(m_hCurrentView, MSICOLINFO_TYPES, hTypes);
		if (result != ERROR_SUCCESS) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiViewGetColumnInfo", WriteErrorCategory::InvalidResult);
		}
	}
	
	void Installer::GetMsiColumnInfo(const WWuString& tableName, MSIHANDLE* hNames, MSIHANDLE* hTypes)
	{
		if (m_hCurrentView)
			MsiCloseHandle(m_hCurrentView);

		OpenDatabaseView(L"Select * From " + tableName);
		UINT result = MsiViewGetColumnInfo(m_hCurrentView, MSICOLINFO_NAMES, hNames);
		if (result != ERROR_SUCCESS) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiViewGetColumnInfo", WriteErrorCategory::InvalidResult);
		}

		result = MsiViewGetColumnInfo(m_hCurrentView, MSICOLINFO_TYPES, hTypes);
		if (result != ERROR_SUCCESS) {
			_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiViewGetColumnInfo", WriteErrorCategory::InvalidResult);
		}
	}

	void Installer::GetColumnPositionInfo(const WWuString& tableName, std::map<WWuString, int>& positionInfo)
	{
		this->OpenDatabaseView(L"Select * From _Columns");
		this->ViewExecute(NULL);
		do {
			PMSIHANDLE hRecord;
			if (!this->ViewFetch(&hRecord))
				break;

			if (this->RecordGetString(hRecord, 1) == tableName) {
				WWuString columnName = this->RecordGetString(hRecord, 3);
				int posish = MsiRecordGetInteger(hRecord, 2);
				positionInfo.emplace(columnName, posish);
			}

		} while (true);
	}

	bool Installer::TryFindTableName(const WuList<WWuString>& tableNames, WWuString& tableName)
	{
		for (const WWuString& currentTable : tableNames) {
			if (currentTable.CompareTo(tableName, true) == 0) {
				// Normalizing the input name with the real table name.
				tableName = currentTable;
				return true;
			}
		}

		return false;
	}


	// Private.
	void Installer::OpenDatabase(const WWuString& filePath, MsiPersistenceMode mode)
	{
		LPCWSTR modeStr { };

		switch (mode) {
			case WindowsUtils::Core::MsiPersistenceMode::CreateDirect:
				modeStr = MSIDBOPEN_CREATEDIRECT;
				break;
			case WindowsUtils::Core::MsiPersistenceMode::Create:
				modeStr = MSIDBOPEN_CREATE;
				break;
			case WindowsUtils::Core::MsiPersistenceMode::Direct:
				modeStr = MSIDBOPEN_DIRECT;
				break;
			case WindowsUtils::Core::MsiPersistenceMode::ReadOnly:
				modeStr = MSIDBOPEN_READONLY;
				break;
			case WindowsUtils::Core::MsiPersistenceMode::Transact:
				modeStr = MSIDBOPEN_TRANSACT;
				break;
			case WindowsUtils::Core::MsiPersistenceMode::PatchFile:
				modeStr = L"MSIDBOPEN_PATCHFILE";
				break;
			default:
				break;
		}

		DWORD result = MsiOpenDatabase(filePath.Raw(), modeStr, &m_hDatabase);
		if (result != ERROR_SUCCESS) {
			PMSIHANDLE hLastRecord = MsiGetLastErrorRecord();
			if (hLastRecord) {
				WWuString message = FormatRecord(NULL, hLastRecord);
				_WU_RAISE_NATIVE_EXCEPTION_WMESS(result, L"MsiOpenDatabase", WriteErrorCategory::OpenError, WWuString::Format(L"Error opening database: %ws", message.Raw()));
			}
			else {
				_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiOpenDatabase", WriteErrorCategory::OpenError);
			}
		}
	}

	void Installer::GetSummaryInfo()
	{
		DWORD result = MsiGetSummaryInformation(m_hDatabase, NULL, 0, &m_hSummaryInfo);
		if (result != ERROR_SUCCESS) {
			PMSIHANDLE hLastRecord = MsiGetLastErrorRecord();
			if (hLastRecord) {
				WWuString message = FormatRecord(NULL, hLastRecord);
				_WU_RAISE_NATIVE_EXCEPTION_WMESS(result, L"MsiGetSummaryInformation", WriteErrorCategory::InvalidResult, WWuString::Format(L"Error getting summary info: %ws", message.Raw()));
			}
			else {
				_WU_RAISE_NATIVE_EXCEPTION(result, L"MsiGetSummaryInformation", WriteErrorCategory::InvalidResult);
			}
		}
	}
}