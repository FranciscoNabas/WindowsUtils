#pragma unmanaged

#include "../../Headers/Support/IO.h"
#include "../../Headers/Engine/Installer.h"
#include "../../Headers/Support/WuException.h"

#include <MsiQuery.h>

#pragma managed

#include "../../Headers/Wrappers/InstallerWrapper.h"
#include "../../Headers/Wrappers/UtilitiesWrapper.h"

namespace WindowsUtils::Wrappers
{
	using namespace System::Management::Automation;
	using namespace WindowsUtils::Installer;

	// Get-MsiProperties
	Dictionary<String^, Object^>^ InstallerWrapper::GetMsiProperties(String^ filePath)
	{
		WWuString wuFileName = UtilitiesWrapper::GetWideStringFromSystemString(filePath);
		Dictionary<String^, Object^>^ output = gcnew Dictionary<String^, Object^>(0);

		_WU_START_TRY
			Stubs::Installer stub{ wuFileName, Core::MsiPersistenceMode::ReadOnly, L"Select Property, Value From Property", Context->GetUnderlyingContext() };
			stub.Execute(NULL);

			do {
				PMSIHANDLE hRecord;
				if (!stub.Fetch(&hRecord))
					break;

				WWuString property = stub.GetString(hRecord, 1);
				WWuString value = stub.GetString(hRecord, 2);

				output->Add(gcnew String(property.Raw()), gcnew String(value.Raw()));

			} while (true);
		_WU_MANAGED_CATCH
		
		return output;
	}

	// Get-MsiSummaryInfo
	InstallerSummaryInfo^ InstallerWrapper::GetMsiSummaryInfo(String^ filePath)
	{
		InstallerSummaryInfo^ output;
		Core::WU_SUMMARY_INFO result;
		WWuString wuFileName = UtilitiesWrapper::GetWideStringFromSystemString(filePath);

		_WU_START_TRY
			Stubs::Installer stub{ wuFileName, Core::MsiPersistenceMode::ReadOnly, Context->GetUnderlyingContext() };
			output = gcnew InstallerSummaryInfo(stub.GetSummary());
		_WU_MANAGED_CATCH
		
		return output;
	}

	// Get-MsiTableInfo
	List<InstallerTableInfoBase^>^ InstallerWrapper::GetMsiTableInfo(String^ filePath, array<String^>^ tableNames)
	{
		std::vector<WWuString> allTables;
		WWuString wFilePath = UtilitiesWrapper::GetWideStringFromSystemString(filePath);

		_WU_START_TRY
			Stubs::Installer stub{ wFilePath, Core::MsiPersistenceMode::ReadOnly, Context->GetUnderlyingContext() };
			WuList<WWuString> allTables = stub.GetTables();

			// List all tables.
			if (tableNames == nullptr || tableNames->Length == 0) {
				return GetMsiTableInfo(stub, allTables);
			}
			else {
				WuList<WWuString> tablesToQuery;
				for each (String ^ table in tableNames) {
					WWuString wTable = UtilitiesWrapper::GetWideStringFromSystemString(table);

					// Checking if table exists and normalizing the name.
					if (!stub.TryGetTableName(allTables, wTable)) {
						ArgumentException^ ex{ gcnew ArgumentException("Invalid or unknown table identified: " + table + ".") };
						Context->WriteError(gcnew ErrorRecord(
							ex,
							"InstallerDatabaseTableNotFound",
							ErrorCategory::ObjectNotFound,
							table
						));

						continue;
					}

					tablesToQuery.Add(wTable);
				}

				return GetMsiTableInfo(stub, tablesToQuery);
			}
		_WU_MANAGED_CATCH
	}
	List<InstallerTableInfoBase^>^ InstallerWrapper::GetMsiTableInfo(Stubs::Installer& stub, const WuList<WWuString>& tableNames)
	{
		List<InstallerTableInfoBase^>^ output = gcnew List<InstallerTableInfoBase^>(0);

		_WU_START_TRY
			for (const WWuString& table : tableNames) {
				PMSIHANDLE hNamesRecord;
				PMSIHANDLE hTypesRecord;

				WuList<WWuString> keys = stub.GetTableKeys(table);
				std::map<WWuString, int> positionInfo = stub.GetColumnsPosition(table);
				stub.GetColumnInfo(table, &hNamesRecord, &hTypesRecord);

				UINT recordCount = MsiRecordGetFieldCount(hNamesRecord);
				array<InstallerColumnInfo^>^ columnInfo = gcnew array<InstallerColumnInfo^>(recordCount);
				for (UINT i = 1; i <= recordCount; i++) {
					WWuString name = stub.GetString(hNamesRecord, i);
					WWuString type = stub.GetString(hTypesRecord, i);

					bool isKey = false;
					if (std::find(keys.begin(), keys.end(), name) != keys.end())
						isKey = true;

					columnInfo[i - 1] = gcnew InstallerColumnInfo(gcnew String(name.Raw()), gcnew String(type.Raw()), positionInfo[name], isKey);
				}

				output->Add(gcnew InstallerTableInfoBase(gcnew String(table.Raw()), columnInfo));
			}
		_WU_MANAGED_CATCH

		return output;
	}
	
	// Get-MsiTableDump
	void InstallerWrapper::GetMsiTableDump(String^ filePath, array<String^>^ tableNames)
	{
		// The intent is never call this function without the table names from the module, but since
		// this is a public API better safe than sorry.
		if (tableNames == nullptr || tableNames->Length == 0) {
			ArgumentNullException^ ex { gcnew ArgumentNullException("'tableNames' cannot be null or empty.") };
			Context->WriteError(gcnew ErrorRecord(
				ex,
				"GetMsiTableDumpNullOrEmptyTableNames",
				ErrorCategory::InvalidArgument,
				tableNames
			));

			throw ex;
		}

		WWuString wFilePath = UtilitiesWrapper::GetWideStringFromSystemString(filePath);
		_WU_START_TRY
			// Creating the installer and listing all table names.
			Stubs::Installer stub{ wFilePath, Core::MsiPersistenceMode::ReadOnly, Context->GetUnderlyingContext() };
			WuList<WWuString> allTables = stub.GetTables();
			
			// Iterating through each name.
			for each (String ^ table in tableNames) {
				WWuString wTable = UtilitiesWrapper::GetWideStringFromSystemString(table);

				// Checking if table exists and normalizing the name.
				if (!stub.TryGetTableName(allTables, wTable)) {
					ArgumentException^ ex { gcnew ArgumentException("Invalid or unknown table identified: " + table + ".") };
					Context->WriteError(gcnew ErrorRecord(
						ex,
						"InstallerDatabaseTableNotFound",
						ErrorCategory::ObjectNotFound,
						table
					));

					continue;
				}

				// At this point the table exists, and we have the correct name.
				// Here we create the data table, get the table information, and add the columns to the data table.
				DataTable^ currentOutput = gcnew DataTable();
				InstallerTableInfoBase^ tableInfo = GetMsiSingleTableInfo(stub, wTable);
				for each (InstallerColumnInfo ^ column in tableInfo->Columns)
					currentOutput->Columns->Add(column->ToDataColumn());

				// Querying the table.
				stub.OpenView(L"Select * From " + wTable);
				stub.Execute(NULL);
				do {
					// If 'ViewFetch' returns false there are no more records.
					PMSIHANDLE hRecord;
					if (!stub.Fetch(&hRecord))
						break;

					// Here we iterate through each column info, and depending on the data type we read
					// from the record accordingly and set the row values for each column.
					// The only managed types supported are string, short, int, or byte array.
					DataRow^ row = currentOutput->NewRow();
					for each (InstallerColumnInfo ^ column in tableInfo->Columns) {
						Object^ currentValue = nullptr;
						
						// String.
						if (column->DataType == String::typeid)
							row[column->Name] = gcnew String(stub.GetString(hRecord, column->Position).Raw());
						
						// Short or int.
						else if (column->DataType == Int16::typeid || column->DataType == Int32::typeid) {
							int interInt = MsiRecordGetInteger(hRecord, column->Position);
							if (interInt != MSI_NULL_INTEGER)
								row[column->Name] = interInt;
							else
								row[column->Name] = DBNull::Value;
						}

						// Byte array.
						else if (column->DataType == array<Byte>::typeid) {
							WuList<char> data = stub.ReadStream(hRecord, column->Position);
							int dataSize = static_cast<int>(data.Count());
							if (dataSize > 0) {
								array<Byte>^ dataArray = gcnew array<Byte>(dataSize);
								for (int i = 0; i < dataSize; i++)
									dataArray[i] = data[i];

								row[column->Name] = dataArray;
							}
							else
								row[column->Name] = DBNull::Value;
						}
					}

					// Adding the row to the data table.
					currentOutput->Rows->Add(row);

				} while (true);

				// Writing the object to the stream.
				Context->WriteObject(currentOutput);
			}
		_WU_MANAGED_CATCH
	}
	void InstallerWrapper::GetMsiTableDump(String^ filePath, InstallerTableInfoBase^ tableInfo)
	{
		// The intent is never call this function without the table information from the module, but since
		// this is a public API better safe than sorry.
		if (tableInfo == nullptr) {
			ArgumentNullException^ ex { gcnew ArgumentNullException("'tableInfo' cannot be null.") };
			Context->WriteError(gcnew ErrorRecord(
				ex,
				"GetMsiTableDumpNullTableInfo",
				ErrorCategory::InvalidArgument,
				tableInfo
			));

			throw ex;
		}

		_WU_START_TRY
			// Creating the installer.
			Stubs::Installer stub{ UtilitiesWrapper::GetWideStringFromSystemString(filePath), Core::MsiPersistenceMode::ReadOnly, Context->GetUnderlyingContext() };

			// We will not check for table existence here because once the 'InstallerTableInfoBase'
			// object is created (from a real table) you can change the name. And although the constructor
			// is public, if you go to the trouble of creating an instance with the wrong table name
			// you're asking for something bad to happen right?
			// Also performance.
			DataTable^ currentOutput = gcnew DataTable();
			for each (InstallerColumnInfo ^ column in tableInfo->Columns)
				currentOutput->Columns->Add(column->ToDataColumn());

			// Querying the table.
			stub.OpenView(L"Select * From " + UtilitiesWrapper::GetWideStringFromSystemString(tableInfo->TableName));
			stub.Execute(NULL);
			do {
				// If 'ViewFetch' returns false there are no more records.
				PMSIHANDLE hRecord;
				if (!stub.Fetch(&hRecord))
					break;

				// Here we iterate through each column info, and depending on the data type we read
				// from the record accordingly and set the row values for each column.
				// The only managed types supported are string, short, int, or byte array.
				DataRow^ row = currentOutput->NewRow();
				for each (InstallerColumnInfo ^ column in tableInfo->Columns) {
					Object^ currentValue = nullptr;

					// String.
					if (column->DataType == String::typeid)
						row[column->Name] = gcnew String(stub.GetString(hRecord, column->Position).Raw());

					// Short or int.
					else if (column->DataType == Int16::typeid || column->DataType == Int32::typeid) {
						int interInt = MsiRecordGetInteger(hRecord, column->Position);
						if (interInt != MSI_NULL_INTEGER)
							row[column->Name] = interInt;
						else
							row[column->Name] = DBNull::Value;
					}

					// Byte array.
					else if (column->DataType == array<Byte>::typeid) {
						WuList<char> data = stub.ReadStream(hRecord, column->Position);
						int dataSize = static_cast<int>(data.Count());
						if (dataSize > 0) {
							array<Byte>^ dataArray = gcnew array<Byte>(dataSize);
							for (int i = 0; i < dataSize; i++)
								dataArray[i] = data[i];

							row[column->Name] = dataArray;
						}
						else
							row[column->Name] = DBNull::Value;
					}
				}

				// Adding the row to the data table.
				currentOutput->Rows->Add(row);

			} while (true);

			// Writing the object to the stream.
			Context->WriteObject(currentOutput);
		_WU_MANAGED_CATCH
	}

	// Invoke-MsiQuery
	void InstallerWrapper::InvokeMsiQuery(String^ filePath, InstallerCommand^ command, List<InstallerCommandParameter^>^ parameters)
	{
		if (command->MarkerCount != parameters->Count) {
			ArgumentException^ ex { gcnew ArgumentException("The number of parameters is different than the number of markers in the query.") };
			Context->WriteError(gcnew ErrorRecord(
				ex,
				"InvokeMsiQueryInvalidParameterCount",
				ErrorCategory::InvalidArgument,
				parameters
			));

			throw ex;
		}
		
		_WU_START_TRY
			Core::MsiPersistenceMode databaseMode;
			if (command->Type == SqlActionType::Select)
				databaseMode = Core::MsiPersistenceMode::ReadOnly;
			else
				databaseMode = Core::MsiPersistenceMode::Transact;

			WWuString wrappedFilePath = UtilitiesWrapper::GetWideStringFromSystemString(filePath);
			Stubs::Installer stub{ wrappedFilePath, databaseMode, Context->GetUnderlyingContext() };

			ExecuteMsiCommand(stub, command, parameters);
		_WU_MANAGED_CATCH
	}

	// Utilities
	Boolean InstallerWrapper::IsInstallerPackage(String^ filePath)
	{
		WWuString wFilePath = UtilitiesWrapper::GetWideStringFromSystemString(filePath);
		if (!Core::IO::FileExists(wFilePath))
			throw gcnew System::IO::FileNotFoundException("Cannot find path '" + filePath + "' because it does not exist.");

		_WU_START_TRY
			return Stubs::Installer::IsInstaller(wFilePath, Context->GetUnderlyingContext());
		_WU_MANAGED_CATCH
	}

	InstallerTableInfoBase^ InstallerWrapper::GetMsiSingleTableInfo(Stubs::Installer& stub, const WWuString& tableName)
	{
		_WU_START_TRY
			PMSIHANDLE hNamesRecord;
			PMSIHANDLE hTypesRecord;

			WuList<WWuString> keys = stub.GetTableKeys(tableName);
			std::map<WWuString, int> positionInfo = stub.GetColumnsPosition(tableName);
			stub.GetColumnInfo(tableName, &hNamesRecord, &hTypesRecord);

			UINT recordCount = MsiRecordGetFieldCount(hNamesRecord);
			array<InstallerColumnInfo^>^ columnInfo = gcnew array<InstallerColumnInfo^>(recordCount);
			for (UINT i = 1; i <= recordCount; i++) {
				WWuString name = stub.GetString(hNamesRecord, i);
				WWuString type = stub.GetString(hTypesRecord, i);

				bool isKey = false;
				if (std::find(keys.begin(), keys.end(), name) != keys.end())
					isKey = true;

				columnInfo[i - 1] = gcnew InstallerColumnInfo(gcnew String(name.Raw()), gcnew String(type.Raw()), positionInfo[name], isKey);
			}

			return gcnew InstallerTableInfoBase(gcnew String(tableName.Raw()), columnInfo);
		_WU_MANAGED_CATCH
	}

	array<InstallerColumnInfo^>^ InstallerWrapper::GetMsiTableInfo(Stubs::Installer& stub)
	{
		PMSIHANDLE hNamesRecord;
		PMSIHANDLE hTypesRecord;

		stub.GetColumnInfo(&hNamesRecord, &hTypesRecord);

		UINT recordCount = MsiRecordGetFieldCount(hNamesRecord);
		array<InstallerColumnInfo^>^ columnInfo = gcnew array<InstallerColumnInfo^>(recordCount);
		for (UINT i = 1; i <= recordCount; i++) {
			WWuString name = stub.GetString(hNamesRecord, i);
			WWuString type = stub.GetString(hTypesRecord, i);

			columnInfo[i - 1] = gcnew InstallerColumnInfo(gcnew String(name.Raw()), gcnew String(type.Raw()), i, false);
		}

		return columnInfo;
	}

	void InstallerWrapper::ExecuteMsiCommand(Stubs::Installer& stub, InstallerCommand^ command, List<InstallerCommandParameter^>^ parameters)
	{
		PMSIHANDLE hParams { };

		// Open a database view.
		WWuString sqlCommand = UtilitiesWrapper::GetWideStringFromSystemString(command->Command);
		stub.OpenView(sqlCommand);

		// Checking if there are parameters to be passed to the view query. If so, we
		// create a record and write all the parameters in that record.
		WWuString tempFilePath;
		bool deleteFile = false;
		if (command->MarkerCount > 0) {
			hParams = MsiCreateRecord(static_cast<UINT>(command->MarkerCount));
			UINT recordIndex = 1;
			for each (InstallerCommandParameter^ param in parameters) {
				switch (param->Type) {
					case InstallerCommandParamType::String:
						stub.SetString(hParams, recordIndex, UtilitiesWrapper::GetWideStringFromSystemString((String^)param->Data));
						break;
					case InstallerCommandParamType::Integer:
						stub.SetInteger(hParams, recordIndex, (int)param->Data);
						break;
					case InstallerCommandParamType::File:
						stub.SetStream(hParams, recordIndex, UtilitiesWrapper::GetWideStringFromSystemString((String^)param->Data));
						break;
					case InstallerCommandParamType::ByteArray:
					{
						// The installer API doesn't support writing a buffer to a record stream, so we
						// create a temporary file, save the data, write it to the stream, and delete the file.
						int dataLength = ((array<Byte>^)param->Data)->Length;
						IntPtr buffer = Marshal::AllocHGlobal(dataLength);
						Marshal::Copy((array<Byte>^)param->Data, 0, buffer, dataLength);
						
						Core::IO::WriteByteArrayToTempFile(reinterpret_cast<BYTE*>((void*)buffer), static_cast<DWORD>(dataLength), tempFilePath);

						// When we call 'RecordSetStream' the API opens the file and uses it up until the database commit.
						// Since we need to cleanup the temporary file, we mark it to delete at the end.
						stub.SetStream(hParams, recordIndex, tempFilePath);
						deleteFile = true;
					} break;
				}

				recordIndex++;
			}
		}

		// Executing the view.
		stub.Execute(hParams);

		// We only check for results if the command was a 'SELECT'.
		if (command->Type == SqlActionType::Select) {
			
			// Creating the data table and adding the columns.
			DataTable^ output = gcnew DataTable();
			auto columnInfo = GetMsiTableInfo(stub);
			for each (InstallerColumnInfo^ column in columnInfo)
				output->Columns->Add(column->ToDataColumn());

			do {
				// If 'ViewFetch' returns false there are no more records.
				PMSIHANDLE hRecord;
				if (!stub.Fetch(&hRecord))
					break;

				// Here we iterate through each column info, and depending on the data type we read
				// from the record accordingly and set the row values for each column.
				// The only managed types supported are string, short, int, or byte array.
				DataRow^ row = output->NewRow();
				for each (InstallerColumnInfo ^ column in columnInfo) {
					Object^ currentValue = nullptr;

					// String.
					if (column->DataType == String::typeid)
						row[column->Name] = gcnew String(stub.GetString(hRecord, column->Position).Raw());

					// Short or int.
					else if (column->DataType == Int16::typeid || column->DataType == Int32::typeid) {
						int interInt = MsiRecordGetInteger(hRecord, column->Position);
						if (interInt != MSI_NULL_INTEGER)
							row[column->Name] = interInt;
						else
							row[column->Name] = DBNull::Value;
					}

					// Byte array.
					else if (column->DataType == array<Byte>::typeid) {
						WuList<char> data = stub.ReadStream(hRecord, column->Position);
						int dataSize = static_cast<int>(data.Count());
						if (dataSize > 0) {
							array<Byte>^ dataArray = gcnew array<Byte>(dataSize);
							for (int i = 0; i < dataSize; i++)
								dataArray[i] = data[i];

							row[column->Name] = dataArray;
						}
						else
							row[column->Name] = DBNull::Value;
					}
				}

				// Adding the row to the data table.
				output->Rows->Add(row);

			} while (true);

			Context->WriteObject(output);
		}

		// Committing the database, if necessary.
		if (command->Type != SqlActionType::Select) {
			stub.Commit();
			if (deleteFile)
				DeleteFile(tempFilePath.Raw());
		}
	}
}