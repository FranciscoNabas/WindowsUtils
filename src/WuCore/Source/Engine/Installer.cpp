#include "../../pch.h"

#include <MsiQuery.h>

#include "../../Headers/Engine/Installer.h"
#include "../../Headers/Support/WuStdException.h"

namespace WindowsUtils::Core
{
	Installer::Installer(const WWuString& filePath, MsiPersistenceMode mode, Core::WuNativeContext* context)
	{
		m_context = context;
		m_databasePath = filePath;

		OpenDatabase(filePath, mode);
	}

	Installer::Installer(const WWuString& filePath, MsiPersistenceMode mode, const WWuString& viewQuery, Core::WuNativeContext* context)
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
		if (!m_hDatabase) {
			WuStdException ex(ERROR_INVALID_HANDLE, __FILEW__, __LINE__);
			ex.Cry(L"InstallerInvalidDatabase", Core::WriteErrorCategory::InvalidArgument, m_databasePath, m_context);

			throw ex;
		}

		if (m_hCurrentView)
			MsiCloseHandle(m_hCurrentView);

		DWORD result = MsiDatabaseOpenView(m_hDatabase, query.GetBuffer(), &m_hCurrentView);
		if (result != ERROR_SUCCESS) {
			PMSIHANDLE hLastRecord = MsiGetLastErrorRecord();
			if (hLastRecord) {
				WWuString message = FormatRecord(NULL, hLastRecord);
				Core::WuStdException ex(result, WWuString::Format(L"Error opening view: %ws", message.GetBuffer()), __FILEW__, __LINE__);
				ex.Cry(L"InstallerOpenDatabaseView", Core::WriteErrorCategory::OpenError, m_databasePath, m_context);

				throw ex;
			}
			else {
				Core::WuStdException ex(result, __FILEW__, __LINE__);
				ex.Cry(L"InstallerOpenDatabaseView", Core::WriteErrorCategory::OpenError, m_databasePath, m_context);

				throw ex;
			}
		}
	}

	void Installer::ViewExecute(_In_opt_ MSIHANDLE hRecord)
	{
		if (!m_hCurrentView) {
			WuStdException ex(ERROR_INVALID_HANDLE, __FILEW__, __LINE__);
			ex.Cry(L"InstallerInvalidView", Core::WriteErrorCategory::InvalidArgument, m_databasePath, m_context);

			throw ex;
		}

		DWORD result = MsiViewExecute(m_hCurrentView, hRecord);
		if (result != ERROR_SUCCESS) {
			PMSIHANDLE hLastRecord = MsiGetLastErrorRecord();
			if (hLastRecord) {
				WWuString message = FormatRecord(NULL, hLastRecord);
				Core::WuStdException ex(result, WWuString::Format(L"Error executing view: %ws", message.GetBuffer()), __FILEW__, __LINE__);
				ex.Cry(L"InstallerExecuteView", Core::WriteErrorCategory::InvalidResult, m_databasePath, m_context);

				throw ex;
			}
			else {
				Core::WuStdException ex(result, __FILEW__, __LINE__);
				ex.Cry(L"InstallerExecuteView", Core::WriteErrorCategory::InvalidResult, m_databasePath, m_context);

				throw ex;
			}
		}
	}

	bool Installer::ViewFetch(MSIHANDLE* hRecord)
	{
		if (!m_hCurrentView) {
			WuStdException ex(ERROR_INVALID_HANDLE, __FILEW__, __LINE__);
			ex.Cry(L"InstallerInvalidView", Core::WriteErrorCategory::InvalidArgument, m_databasePath, m_context);

			throw ex;
		}

		DWORD result = MsiViewFetch(m_hCurrentView, hRecord);
		if (result != ERROR_SUCCESS) {
			if (result == ERROR_NO_MORE_ITEMS)
				return false;

			PMSIHANDLE hLastRecord = MsiGetLastErrorRecord();
			if (hLastRecord) {
				WWuString message = FormatRecord(NULL, hLastRecord);
				Core::WuStdException ex(result, WWuString::Format(L"Error fetching view: %ws", message.GetBuffer()), __FILEW__, __LINE__);
				ex.Cry(L"InstallerFetchView", Core::WriteErrorCategory::InvalidResult, m_databasePath, m_context);

				throw ex;
			}
			else {
				Core::WuStdException ex(result, __FILEW__, __LINE__);
				ex.Cry(L"InstallerFetchView", Core::WriteErrorCategory::InvalidResult, m_databasePath, m_context);

				throw ex;
			}
		}

		if (hRecord == 0)
			return false;

		return true;
	}

	
	// Read from a record.
	void Installer::RecordReadStream(MSIHANDLE hRecord, int fieldIndex, wuvector<char>& data)
	{
		DWORD result;

		do {
			char buffer[1024] { };
			DWORD buffSize = 1024;
			result = MsiRecordReadStream(hRecord, fieldIndex, buffer, &buffSize);
			if (result != ERROR_SUCCESS) {
				Core::WuStdException ex(result, __FILEW__, __LINE__);
				ex.Cry(L"InstallerRecordReadStream", Core::WriteErrorCategory::ReadError, m_databasePath, m_context);

				throw ex;
			}

			if (buffSize > 0)
				data.insert(data.end(), buffer, buffer + buffSize);

			if (buffSize < 1024)
				break;

		} while (true);
	}

	WWuString Installer::RecordGetString(MSIHANDLE hRecord, int fieldIndex)
	{
		DWORD charsNeeded { };
		DWORD result = MsiRecordGetString(hRecord, fieldIndex, NULL, &charsNeeded);
		if (result != ERROR_SUCCESS && result != ERROR_MORE_DATA) {
			Core::WuStdException ex(result, __FILEW__, __LINE__);
			ex.Cry(L"InstallerRecordGetString", Core::WriteErrorCategory::ReadError, m_databasePath, m_context);

			throw ex;
		}

		charsNeeded++;
		DWORD bytesNeeded = charsNeeded * 2;
		wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(bytesNeeded);

		result = MsiRecordGetString(hRecord, fieldIndex, buffer.get(), &charsNeeded);
		if (result != ERROR_SUCCESS) {
			Core::WuStdException ex(result, __FILEW__, __LINE__);
			ex.Cry(L"InstallerRecordGetString", Core::WriteErrorCategory::ReadError, m_databasePath, m_context);

			throw ex;
		}

		return WWuString(buffer.get());
	}


	// Modify a record.
	void Installer::RecordSetString(MSIHANDLE hRecord, UINT iField, const WWuString& szValue)
	{
		DWORD result = MsiRecordSetString(hRecord, iField, szValue.GetBuffer());
		if (result != ERROR_SUCCESS) {
			Core::WuStdException ex(result, __FILEW__, __LINE__);
			ex.Cry(L"InstallerRecordSetString", Core::WriteErrorCategory::OpenError, m_databasePath, m_context);

			throw ex;
		}
	}

	void Installer::RecordSetInteger(MSIHANDLE hRecord, UINT iField, int iValue)
	{
		DWORD result = MsiRecordSetInteger(hRecord, iField, iValue);
		if (result != ERROR_SUCCESS) {
			Core::WuStdException ex(result, __FILEW__, __LINE__);
			ex.Cry(L"InstallerRecordSetString", Core::WriteErrorCategory::OpenError, m_databasePath, m_context);

			throw ex;
		}
	}

	void Installer::RecordSetStream(MSIHANDLE hRecord, UINT iField, const WWuString& szFilePath)
	{
		DWORD result = MsiRecordSetStream(hRecord, iField, szFilePath.GetBuffer());
		if (result != ERROR_SUCCESS) {
			Core::WuStdException ex(result, __FILEW__, __LINE__);
			ex.Cry(L"InstallerRecordSetString", Core::WriteErrorCategory::OpenError, m_databasePath, m_context);

			throw ex;
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
				Core::WuStdException ex(result, WWuString::Format(L"Error committing to the database: %ws", message.GetBuffer()), __FILEW__, __LINE__);
				ex.Cry(L"InstallerCommitDatabase", Core::WriteErrorCategory::OpenError, m_databasePath, m_context);

				throw ex;
			}
			else {
				Core::WuStdException ex(result, __FILEW__, __LINE__);
				ex.Cry(L"InstallerCommitDatabase", Core::WriteErrorCategory::OpenError, m_databasePath, m_context);

				throw ex;
			}
		}
	}


	// Utilities.
	WWuString Installer::FormatRecord(MSIHANDLE hInstall, MSIHANDLE hRecord)
	{
		DWORD charsNeeded { };
		DWORD result = MsiFormatRecord(hInstall, hRecord, NULL, &charsNeeded);
		if (result != ERROR_SUCCESS && result != ERROR_MORE_DATA) {
			Core::WuStdException ex(result, __FILEW__, __LINE__);
			ex.Cry(L"InstallerFormatRecord", Core::WriteErrorCategory::ReadError, m_databasePath, m_context);

			throw ex;
		}

		charsNeeded++;
		wuunique_ha_ptr<WCHAR> buffer = make_wuunique_ha<WCHAR>(static_cast<size_t>(charsNeeded) * 2);

		result = MsiFormatRecord(hInstall, hRecord, buffer.get(), &charsNeeded);
		if (result != ERROR_SUCCESS) {
			Core::WuStdException ex(result, __FILEW__, __LINE__);
			ex.Cry(L"InstallerFormatRecord", Core::WriteErrorCategory::ReadError, m_databasePath, m_context);

			throw ex;
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
			wuunique_ha_ptr<WCHAR> buffer;

			// Windows Installer functions that return data in a user provided memory location should not
			// be called with NULL as the value for the pointer, so we pass an empty string.
			DWORD result = MsiSummaryInfoGetProperty(m_hSummaryInfo, pid, &dataType, &intValue, &ftValue, L"", &charsNeeded);
			if (result != ERROR_SUCCESS) {
				if (result != ERROR_MORE_DATA) {
					Core::WuStdException ex(result, __FILEW__, __LINE__);
					ex.Cry(L"InstallerGetSummaryInfoProperty", Core::WriteErrorCategory::ReadError, m_databasePath, m_context);

					throw ex;
				}

				charsNeeded++;
				buffer = make_wuunique_ha<WCHAR>(static_cast<size_t>(charsNeeded) * 2);
				result = MsiSummaryInfoGetProperty(m_hSummaryInfo, pid, &dataType, &intValue, &ftValue, buffer.get(), &charsNeeded);
				if (result != ERROR_SUCCESS) {
					Core::WuStdException ex(result, __FILEW__, __LINE__);
					ex.Cry(L"InstallerGetSummaryInfoProperty", Core::WriteErrorCategory::ReadError, m_databasePath, m_context);

					throw ex;
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

	void Installer::GetMsiTableNames(wuvector<WWuString>& tableNames)
	{
		OpenDatabaseView(L"Select * From _Tables");
		ViewExecute(NULL);

		PMSIHANDLE hRecord { };
		while (ViewFetch(&hRecord)) {
			tableNames.push_back(RecordGetString(hRecord, 1));
			MsiCloseHandle(hRecord);
		}
	}

	void Installer::GetMsiTableKeys(const WWuString& tableName, wuvector<WWuString>& tableNames)
	{
		PMSIHANDLE hRecord { };

		UINT result = MsiDatabaseGetPrimaryKeys(m_hDatabase, tableName.GetBuffer(), &hRecord);
		if (result != ERROR_SUCCESS) {
			Core::WuStdException ex(result, __FILEW__, __LINE__);
			ex.Cry(L"InstallerDatabaseGetPrimaryKeys", Core::WriteErrorCategory::InvalidResult, m_databasePath, m_context);

			throw ex;
		}

		for (UINT i = 1; i <= MsiRecordGetFieldCount(hRecord); i++)
			tableNames.push_back(RecordGetString(hRecord, i));
	}

	void Installer::GetMsiColumnInfo(MSIHANDLE* hNames, MSIHANDLE* hTypes)
	{
		UINT result = MsiViewGetColumnInfo(m_hCurrentView, MSICOLINFO_NAMES, hNames);
		if (result != ERROR_SUCCESS) {
			Core::WuStdException ex(result, __FILEW__, __LINE__);
			ex.Cry(L"InstallerViewGetColumnInfo", Core::WriteErrorCategory::InvalidResult, m_databasePath, m_context);

			throw ex;
		}

		result = MsiViewGetColumnInfo(m_hCurrentView, MSICOLINFO_TYPES, hTypes);
		if (result != ERROR_SUCCESS) {
			Core::WuStdException ex(result, __FILEW__, __LINE__);
			ex.Cry(L"InstallerViewGetColumnInfo", Core::WriteErrorCategory::InvalidResult, m_databasePath, m_context);

			throw ex;
		}
	}
	
	void Installer::GetMsiColumnInfo(const WWuString& tableName, MSIHANDLE* hNames, MSIHANDLE* hTypes)
	{
		if (m_hCurrentView)
			MsiCloseHandle(m_hCurrentView);

		OpenDatabaseView(L"Select * From " + tableName);
		UINT result = MsiViewGetColumnInfo(m_hCurrentView, MSICOLINFO_NAMES, hNames);
		if (result != ERROR_SUCCESS) {
			Core::WuStdException ex(result, __FILEW__, __LINE__);
			ex.Cry(L"InstallerViewGetColumnInfo", Core::WriteErrorCategory::InvalidResult, m_databasePath, m_context);

			throw ex;
		}

		result = MsiViewGetColumnInfo(m_hCurrentView, MSICOLINFO_TYPES, hTypes);
		if (result != ERROR_SUCCESS) {
			Core::WuStdException ex(result, __FILEW__, __LINE__);
			ex.Cry(L"InstallerViewGetColumnInfo", Core::WriteErrorCategory::InvalidResult, m_databasePath, m_context);

			throw ex;
		}
	}

	void Installer::GetColumnPositionInfo(const WWuString& tableName, wumap<WWuString, int>& positionInfo)
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

	bool Installer::FindTableName(WWuString& table, const wuvector<WWuString>& tableNames)
	{
		for (const WWuString& currentTable : tableNames) {
			if (currentTable.CompareTo(table, true) == 0) {
				// Normalizing the input name with the real table name.
				table = currentTable;
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

		DWORD result = MsiOpenDatabase(filePath.GetBuffer(), modeStr, &m_hDatabase);
		if (result != ERROR_SUCCESS) {
			PMSIHANDLE hLastRecord = MsiGetLastErrorRecord();
			if (hLastRecord) {
				WWuString message = FormatRecord(NULL, hLastRecord);
				Core::WuStdException ex(result, WWuString::Format(L"Error opening database: %ws", message.GetBuffer()), __FILEW__, __LINE__);
				ex.Cry(L"InstallerOpenDatabase", Core::WriteErrorCategory::OpenError, m_databasePath, m_context);

				throw ex;
			}
			else {
				Core::WuStdException ex(result, __FILEW__, __LINE__);
				ex.Cry(L"InstallerOpenDatabase", Core::WriteErrorCategory::OpenError, m_databasePath, m_context);

				throw ex;
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
				Core::WuStdException ex(result, WWuString::Format(L"Error getting summary info: %ws", message.GetBuffer()), __FILEW__, __LINE__);
				ex.Cry(L"InstallerSummaryInformation", Core::WriteErrorCategory::InvalidResult, m_databasePath, m_context);

				throw ex;
			}
			else {
				Core::WuStdException ex(result, __FILEW__, __LINE__);
				ex.Cry(L"InstallerSummaryInformation", Core::WriteErrorCategory::InvalidResult, m_databasePath, m_context);

				throw ex;
			}
		}
	}
}