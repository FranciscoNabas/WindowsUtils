#pragma once

#include "../Support/WuString.h"
#include "../Support/Expressions.h"
#include "../Support/Notification.h"
#include "../Support/WuException.h"
#include "../Support/WuList.h"

#include <Msi.h>
#include <map>
#include <memory>
#include <MsiQuery.h>

namespace WindowsUtils::Core
{
	enum class MsiPersistenceMode
	{
		CreateDirect,
		Create,
		Direct,
		ReadOnly,
		Transact,
		PatchFile,
	};

	typedef struct _WU_SUMMARY_PROPERTY
	{
		WWuString  Name;
		WORD       Index;

		_WU_SUMMARY_PROPERTY(const LPWSTR name, const WORD index);

	} WU_SUMMARY_PROPERTY, *PWU_SUMMARY_PROPERTY;

	typedef struct _WU_SUMMARY_INFO
	{
		WORD         Codepage;
		WWuString    Title;
		WWuString    Subject;
		WWuString    Author;
		WWuString    Keywords;
		WWuString    Comments;
		WWuString    Template;
		WWuString    LastSavedBy;
		WWuString    RevisionNumber;
		::FILETIME   LastPrinted;
		::FILETIME   CreateTimeDate;
		::FILETIME   LastSaveTimeDate;
		int			 PageCount;
		int			 WordCount;
		int			 CharacterCount;
		WWuString	 CreatingApplication;
		int			 Security;

	} WU_SUMMARY_INFO, *PWU_SUMMARY_INFO;

	class Installer
	{
	public:
		Installer(const WWuString& filePath, const MsiPersistenceMode mode, const Core::WuNativeContext* context);
		Installer(const WWuString& filePath, const MsiPersistenceMode mode, const WWuString& viewQuery, const Core::WuNativeContext* context);
		~Installer();

		// Views.
		bool ViewFetch(MSIHANDLE* hRecord);
		void ViewExecute(_In_opt_ MSIHANDLE hRecord);
		void OpenDatabaseView(const WWuString& query);

		// Read from a record.
		WuList<char> RecordReadStream(MSIHANDLE hRecord, int fieldIndex);
		WWuString RecordGetString(MSIHANDLE hRecord, int fieldIndex);

		// Modify a record.
		void RecordSetString(MSIHANDLE hRecord, UINT iField, const WWuString& szValue);
		void RecordSetInteger(MSIHANDLE hRecord, UINT iField, int iValue);
		void RecordSetStream(MSIHANDLE hRecord, UINT iField, const WWuString& szFilePath);

		// Commit to the database.
		void DatabaseCommit();

		// Utilities.
		void ProcessSummaryInfo(WU_SUMMARY_INFO& info);
		WuList<WWuString> GetMsiTableNames();
		bool TryFindTableName(const WuList<WWuString>& tableNames, _Out_ WWuString& tableName);
		WuList<WWuString> GetMsiTableKeys(const WWuString& tableName);
		void GetMsiColumnInfo(MSIHANDLE* hNames, MSIHANDLE* hTypes);
		void GetMsiColumnInfo(const WWuString& tableName, MSIHANDLE* hNames, MSIHANDLE* hTypes);
		void GetColumnPositionInfo(const WWuString& tableName, std::map<WWuString, int>& positionInfo);
		WWuString FormatRecord(MSIHANDLE hInstall, MSIHANDLE hRecord);

	private:
		MSIHANDLE m_hDatabase;
		MSIHANDLE m_hCurrentView;
		MSIHANDLE m_hSummaryInfo;
		WWuString m_databasePath;
		const Core::WuNativeContext* m_context;

		void GetSummaryInfo();
		void OpenDatabase(const WWuString& filePath, MsiPersistenceMode mode);
	};
}