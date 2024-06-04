#pragma once

#include <Msi.h>
#include <unordered_map>

#include "../Support/String.h"
#include "../Support/Expressions.h"
#include "../Support/Notification.h"

namespace WindowsUtils::Core
{
	enum class MsiPersistenceMode
	{
		CreateDirect = 0,
		Create = 1,
		Direct = 2,
		ReadOnly = 3,
		Transact = 4,
		PatchFile = 5
	};

	typedef struct _WU_SUMMARY_PROPERTY
	{
		WWuString Name;
		WORD Index;

		_WU_SUMMARY_PROPERTY(const LPWSTR name, const WORD index)
			: Name(name), Index(index) { }

	} WU_SUMMARY_PROPERTY, *PWU_SUMMARY_PROPERTY;

	typedef struct _WU_SUMMARY_INFO
	{
		WORD Codepage;
		WWuString Title;
		WWuString Subject;
		WWuString Author;
		WWuString Keywords;
		WWuString Comments;
		WWuString Template;
		WWuString LastSavedBy;
		WWuString RevisionNumber;
		FILETIME LastPrinted;
		FILETIME CreateTimeDate;
		FILETIME LastSaveTimeDate;
		int PageCount;
		int WordCount;
		int CharacterCount;
		WWuString CreatingApplication;
		int Security;

	} WU_SUMMARY_INFO, *PWU_SUMMARY_INFO;

	class Installer
	{
	public:
		Installer(const WWuString& filePath, MsiPersistenceMode mode, Core::WuNativeContext* context);
		Installer(const WWuString& filePath, MsiPersistenceMode mode, const WWuString& viewQuery, Core::WuNativeContext* context);
		~Installer();

		// Views.
		bool ViewFetch(MSIHANDLE* hRecord);
		void ViewExecute(_In_opt_ MSIHANDLE hRecord);
		void OpenDatabaseView(const WWuString& query);

		// Read from a record.
		void RecordReadStream(MSIHANDLE hRecord, int fieldIndex, wuvector<char>& data);
		WWuString RecordGetString(MSIHANDLE hRecord, int fieldIndex);

		// Modify a record.
		void RecordSetString(MSIHANDLE hRecord, UINT iField, const WWuString& szValue);
		void RecordSetInteger(MSIHANDLE hRecord, UINT iField, int iValue);
		void RecordSetStream(MSIHANDLE hRecord, UINT iField, const WWuString& szFilePath);

		// Commit to the database.
		void DatabaseCommit();

		// Utilities.
		void ProcessSummaryInfo(WU_SUMMARY_INFO& info);
		void GetMsiTableNames(wuvector<WWuString>& tableNames);
		bool FindTableName(WWuString& table, const wuvector<WWuString>& tableNames);
		void GetMsiTableKeys(const WWuString& tableName, wuvector<WWuString>& tableNames);
		void GetMsiColumnInfo(MSIHANDLE* hNames, MSIHANDLE* hTypes);
		void GetMsiColumnInfo(const WWuString& tableName, MSIHANDLE* hNames, MSIHANDLE* hTypes);
		void GetColumnPositionInfo(const WWuString& tableName, wumap<WWuString, int>& positionInfo);
		WWuString FormatRecord(MSIHANDLE hInstall, MSIHANDLE hRecord);

	private:
		MSIHANDLE m_hDatabase;
		MSIHANDLE m_hCurrentView;
		MSIHANDLE m_hSummaryInfo;
		WWuString m_databasePath;
		Core::WuNativeContext* m_context;

		void GetSummaryInfo();
		void OpenDatabase(const WWuString& filePath, MsiPersistenceMode mode);
	};
}