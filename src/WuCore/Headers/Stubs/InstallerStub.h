#pragma once
#pragma unmanaged

#include "StubUtils.h"
#include "../Engine/Installer.h"
#include "../Support/WuException.h"

namespace WindowsUtils::Stubs
{
	class Installer
	{
	public:
		Installer(const WWuString& filePath, const Core::MsiPersistenceMode mode, const Core::WuNativeContext* context)
			: m_installer(filePath, mode, context), m_context(context) { }

		Installer(const WWuString& filePath, const Core::MsiPersistenceMode mode, const WWuString& viewQuery, const Core::WuNativeContext* context)
			: m_installer(filePath, mode, viewQuery, context), m_context(context) { }

		void Execute(_In_opt_ MSIHANDLE record)
		{
			_WU_START_TRY
				m_installer.ViewExecute(record);
			_WU_MARSHAL_CATCH(m_context)
		}

		bool Fetch(MSIHANDLE* record)
		{
			bool fetched = false;
			_WU_START_TRY
				fetched = m_installer.ViewFetch(record);
			_WU_MARSHAL_CATCH(m_context)

			return fetched;
		}

		Core::WU_SUMMARY_INFO GetSummary()
		{
			Core::WU_SUMMARY_INFO info{ };
			_WU_START_TRY
				m_installer.ProcessSummaryInfo(info);
			_WU_MARSHAL_CATCH(m_context)

			return info;
		}

		WuList<WWuString> GetTables()
		{
			WuList<WWuString> res;
			_WU_START_TRY
				res = m_installer.GetMsiTableNames();
			_WU_MARSHAL_CATCH(m_context)

			return res;
		}

		bool TryGetTableName(const WuList<WWuString>& tableNames, _Out_ WWuString& tableName)
		{
			bool got = false;
			_WU_START_TRY
				got = m_installer.TryFindTableName(tableNames, tableName);
			_WU_MARSHAL_CATCH(m_context)

			return got;
		}

		WuList<WWuString> GetTableKeys(const WWuString& table)
		{
			WuList<WWuString> res;
			_WU_START_TRY
				res = m_installer.GetMsiTableKeys(table);
			_WU_MARSHAL_CATCH(m_context)

			return res;
		}

		std::map<WWuString, int> GetColumnsPosition(const WWuString& table)
		{
			std::map<WWuString, int> res;
			_WU_START_TRY
				m_installer.GetColumnPositionInfo(table, res);
			_WU_MARSHAL_CATCH(m_context)

			return res;
		}

		void GetColumnInfo(MSIHANDLE* names, MSIHANDLE* types)
		{
			_WU_START_TRY
				m_installer.GetMsiColumnInfo(names, types);
			_WU_MARSHAL_CATCH(m_context)
		}

		void GetColumnInfo(const WWuString& table, MSIHANDLE* names, MSIHANDLE* types)
		{
			_WU_START_TRY
				m_installer.GetMsiColumnInfo(table, names, types);
			_WU_MARSHAL_CATCH(m_context)
		}

		void OpenView(const WWuString& query)
		{
			_WU_START_TRY
				m_installer.OpenDatabaseView(query);
			_WU_MARSHAL_CATCH(m_context)
		}

		WWuString GetString(const MSIHANDLE record, const int index)
		{
			WWuString res;
			_WU_START_TRY
				res = m_installer.RecordGetString(record, index);
			_WU_MARSHAL_CATCH(m_context)

			return res;
		}

		WuList<char> ReadStream(const MSIHANDLE record, const int index)
		{
			WuList<char> res;
			_WU_START_TRY
				res = m_installer.RecordReadStream(record, index);
			_WU_MARSHAL_CATCH(m_context)

			return res;
		}

		void SetString(MSIHANDLE record, const UINT index, const WWuString& value)
		{
			_WU_START_TRY
				m_installer.RecordSetString(record, index, value);
			_WU_MARSHAL_CATCH(m_context)
		}

		void SetInteger(MSIHANDLE record, const UINT index, const int value)
		{
			_WU_START_TRY
				m_installer.RecordSetInteger(record, index, value);
			_WU_MARSHAL_CATCH(m_context)
		}

		void SetStream(MSIHANDLE record, const UINT index, const WWuString& path)
		{
			_WU_START_TRY
				m_installer.RecordSetStream(record, index, path);
			_WU_MARSHAL_CATCH(m_context)
		}

		void Commit()
		{
			_WU_START_TRY
				m_installer.DatabaseCommit();
			_WU_MARSHAL_CATCH(m_context)
		}

		static bool IsInstaller(const WWuString& path, const Core::WuNativeContext* context)
		{
			_WU_START_TRY
				Core::MemoryMappedFile mappedFile{ path };
				if (*reinterpret_cast<__uint64*>(mappedFile.data()) == 0xE11AB1A1E011CFD0 &&
					*reinterpret_cast<__uint64*>((BYTE*)mappedFile.data() + 8) == 0)
					return true;
			_WU_MARSHAL_CATCH(context)

			return false;
		}

	private:
		Core::Installer m_installer;
		const Core::WuNativeContext* m_context;
	};
}