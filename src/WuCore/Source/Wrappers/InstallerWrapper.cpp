#pragma unmanaged

#include "../../Headers/Support/IO.h"
#include "../../Headers/Engine/Installer.h"
#include "../../Headers/Support/WuStdException.h"

#include <MsiQuery.h>

#pragma managed

#include "../../Headers/Wrappers/InstallerWrapper.h"
#include "../../Headers/Wrappers/UtilitiesWrapper.h"

namespace WindowsUtils::Wrappers
{
	using namespace System::Management::Automation;
	using namespace WindowsUtils::Installer;

	// Get-MsiProperties
	Dictionary<String^, Object^>^ InstallerWrapper::GetMsiProperties(String^ filePath, Core::CmdletContextProxy^ context)
	{
		WWuString wuFileName = UtilitiesWrapper::GetWideStringFromSystemString(filePath);
		Dictionary<String^, Object^>^ output = gcnew Dictionary<String^, Object^>(0);

		try {
			Core::Installer installer { wuFileName, Core::MsiPersistenceMode::ReadOnly, L"Select Property, Value From Property", context->GetUnderlyingContext() };
			installer.ViewExecute(NULL);

			do {
				PMSIHANDLE hRecord;
				if (!installer.ViewFetch(&hRecord))
					break;

				WWuString property = installer.RecordGetString(hRecord, 1);
				WWuString value = installer.RecordGetString(hRecord, 2);

				output->Add(gcnew String(property.GetBuffer()), gcnew String(value.GetBuffer()));

			} while (true);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}

		return output;
	}

	// Get-MsiSummaryInfo
	InstallerSummaryInfo^ InstallerWrapper::GetMsiSummaryInfo(String^ filePath, Core::CmdletContextProxy^ context)
	{
		InstallerSummaryInfo^ output;
		Core::WU_SUMMARY_INFO result;
		WWuString wuFileName = UtilitiesWrapper::GetWideStringFromSystemString(filePath);

		try {
			Core::Installer installer { wuFileName, Core::MsiPersistenceMode::ReadOnly, context->GetUnderlyingContext() };
			installer.ProcessSummaryInfo(result);
			output = gcnew InstallerSummaryInfo(result);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}

		return output;
	}

	// Get-MsiTableInfo
	List<InstallerTableInfoBase^>^ InstallerWrapper::GetMsiTableInfo(String^ filePath, array<String^>^ tableNames, Core::CmdletContextProxy^ context)
	{
		wuvector<WWuString> allTables;
		WWuString wFilePath = UtilitiesWrapper::GetWideStringFromSystemString(filePath);

		try {
			Core::Installer installer { wFilePath, Core::MsiPersistenceMode::ReadOnly, context->GetUnderlyingContext() };
			installer.GetMsiTableNames(allTables);

			// List all tables.
			if (tableNames == nullptr || tableNames->Length == 0) {
				return GetMsiTableInfo(installer, allTables);
			}
			else {
				wuvector<WWuString> tablesToQuery;
				for each (String ^ table in tableNames) {
					WWuString wTable = UtilitiesWrapper::GetWideStringFromSystemString(table);
					
					// Checking if table exists and normalizing the name.
					if (!installer.FindTableName(wTable, allTables)) {
						ArgumentException^ ex { gcnew ArgumentException("Invalid or unknown table identified: " + table + ".") };
						context->WriteError(gcnew ErrorRecord(
							ex,
							"InstallerDatabaseTableNotFound",
							ErrorCategory::ObjectNotFound,
							table
						));

						continue;
					}

					tablesToQuery.push_back(wTable);
				}

				return GetMsiTableInfo(installer, tablesToQuery);
			}
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}
	}
	List<InstallerTableInfoBase^>^ InstallerWrapper::GetMsiTableInfo(Core::Installer& installer, const wuvector<WWuString>& tableNames)
	{
		List<InstallerTableInfoBase^>^ output = gcnew List<InstallerTableInfoBase^>(0);

		try {
			for (const WWuString& table : tableNames) {
				PMSIHANDLE hNamesRecord;
				PMSIHANDLE hTypesRecord;
				wuvector<WWuString> keys;
				wumap<WWuString, int> positionInfo;

				installer.GetMsiTableKeys(table, keys);
				installer.GetColumnPositionInfo(table, positionInfo);
				installer.GetMsiColumnInfo(table, &hNamesRecord, &hTypesRecord);

				UINT recordCount = MsiRecordGetFieldCount(hNamesRecord);
				array<InstallerColumnInfo^>^ columnInfo = gcnew array<InstallerColumnInfo^>(recordCount);
				for (UINT i = 1; i <= recordCount; i++) {
					WWuString name = installer.RecordGetString(hNamesRecord, i);
					WWuString type = installer.RecordGetString(hTypesRecord, i);

					bool isKey = false;
					if (std::find(keys.begin(), keys.end(), name) != keys.end())
						isKey = true;

					columnInfo[i - 1] = gcnew InstallerColumnInfo(gcnew String(name.GetBuffer()), gcnew String(type.GetBuffer()), positionInfo[name], isKey);
				}

				output->Add(gcnew InstallerTableInfoBase(gcnew String(table.GetBuffer()), columnInfo));
			}
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}

		return output;
	}
	
	// Get-MsiTableDump
	void InstallerWrapper::GetMsiTableDump(String^ filePath, array<String^>^ tableNames, Core::CmdletContextProxy^ context)
	{
		// The intent is never call this function without the table names from the module, but since
		// this is a public API better safe than sorry.
		if (tableNames == nullptr || tableNames->Length == 0) {
			ArgumentNullException^ ex { gcnew ArgumentNullException("'tableNames' cannot be null or empty.") };
			context->WriteError(gcnew ErrorRecord(
				ex,
				"GetMsiTableDumpNullOrEmptyTableNames",
				ErrorCategory::InvalidArgument,
				tableNames
			));

			throw ex;
		}

		WWuString wFilePath = UtilitiesWrapper::GetWideStringFromSystemString(filePath);
		try {
			// Creating the installer and listing all table names.
			wuvector<WWuString> allTables;
			Core::Installer installer { wFilePath, Core::MsiPersistenceMode::ReadOnly, context->GetUnderlyingContext() };
			installer.GetMsiTableNames(allTables);
			
			// Iterating through each name.
			for each (String ^ table in tableNames) {
				WWuString wTable = UtilitiesWrapper::GetWideStringFromSystemString(table);

				// Checking if table exists and normalizing the name.
				if (!installer.FindTableName(wTable, allTables)) {
					ArgumentException^ ex { gcnew ArgumentException("Invalid or unknown table identified: " + table + ".") };
					context->WriteError(gcnew ErrorRecord(
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
				InstallerTableInfoBase^ tableInfo = GetMsiSingleTableInfo(installer, wTable);
				for each (InstallerColumnInfo ^ column in tableInfo->Columns)
					currentOutput->Columns->Add(column->ToDataColumn());

				// Querying the table.
				installer.OpenDatabaseView(L"Select * From " + wTable);
				installer.ViewExecute(NULL);
				do {
					// If 'ViewFetch' returns false there are no more records.
					PMSIHANDLE hRecord;
					if (!installer.ViewFetch(&hRecord))
						break;

					// Here we iterate through each column info, and depending on the data type we read
					// from the record accordingly and set the row values for each column.
					// The only managed types supported are string, short, int, or byte array.
					DataRow^ row = currentOutput->NewRow();
					for each (InstallerColumnInfo ^ column in tableInfo->Columns) {
						Object^ currentValue = nullptr;
						
						// String.
						if (column->DataType == String::typeid)
							row[column->Name] = gcnew String(installer.RecordGetString(hRecord, column->Position).GetBuffer());
						
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
							wuvector<char> dataVec;
							installer.RecordReadStream(hRecord, column->Position, dataVec);
							int vecSize = static_cast<int>(dataVec.size());
							if (dataVec.size() > 0) {
								array<Byte>^ dataArray = gcnew array<Byte>(vecSize);
								for (int i = 0; i < vecSize; i++)
									dataArray[i] = dataVec[i];

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
				context->WriteObject(currentOutput);
			}
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}
	}
	void InstallerWrapper::GetMsiTableDump(String^ filePath, InstallerTableInfoBase^ tableInfo, Core::CmdletContextProxy^ context)
	{
		// The intent is never call this function without the table information from the module, but since
		// this is a public API better safe than sorry.
		if (tableInfo == nullptr) {
			ArgumentNullException^ ex { gcnew ArgumentNullException("'tableInfo' cannot be null.") };
			context->WriteError(gcnew ErrorRecord(
				ex,
				"GetMsiTableDumpNullTableInfo",
				ErrorCategory::InvalidArgument,
				tableInfo
			));

			throw ex;
		}

		try {
			// Creating the installer.
			Core::Installer installer { UtilitiesWrapper::GetWideStringFromSystemString(filePath), Core::MsiPersistenceMode::ReadOnly, context->GetUnderlyingContext() };

			// We will not check for table existence here because once the 'InstallerTableInfoBase'
			// object is created (from a real table) you can change the name. And although the constructor
			// is public, if you go to the trouble of creating an instance with the wrong table name
			// you're asking for something bad to happen right?
			// Also performance.
			DataTable^ currentOutput = gcnew DataTable();
			for each (InstallerColumnInfo ^ column in tableInfo->Columns)
				currentOutput->Columns->Add(column->ToDataColumn());

			// Querying the table.
			installer.OpenDatabaseView(L"Select * From " + UtilitiesWrapper::GetWideStringFromSystemString(tableInfo->TableName));
			installer.ViewExecute(NULL);
			do {
				// If 'ViewFetch' returns false there are no more records.
				PMSIHANDLE hRecord;
				if (!installer.ViewFetch(&hRecord))
					break;

				// Here we iterate through each column info, and depending on the data type we read
				// from the record accordingly and set the row values for each column.
				// The only managed types supported are string, short, int, or byte array.
				DataRow^ row = currentOutput->NewRow();
				for each (InstallerColumnInfo ^ column in tableInfo->Columns) {
					Object^ currentValue = nullptr;

					// String.
					if (column->DataType == String::typeid)
						row[column->Name] = gcnew String(installer.RecordGetString(hRecord, column->Position).GetBuffer());

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
						wuvector<char> dataVec;
						installer.RecordReadStream(hRecord, column->Position, dataVec);
						int vecSize = static_cast<int>(dataVec.size());
						if (dataVec.size() > 0) {
							array<Byte>^ dataArray = gcnew array<Byte>(vecSize);
							for (int i = 0; i < vecSize; i++)
								dataArray[i] = dataVec[i];

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
			context->WriteObject(currentOutput);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}
	}

	// Invoke-MsiQuery
	void InstallerWrapper::InvokeMsiQuery(String^ filePath, InstallerCommand^ command, List<InstallerCommandParameter^>^ parameters, Core::CmdletContextProxy^ context)
	{
		if (command->MarkerCount != parameters->Count) {
			ArgumentException^ ex { gcnew ArgumentException("The number of parameters is different than the number of markers in the query.") };
			context->WriteError(gcnew ErrorRecord(
				ex,
				"InvokeMsiQueryInvalidParameterCount",
				ErrorCategory::InvalidArgument,
				parameters
			));

			throw ex;
		}
		
		try {
			Core::MsiPersistenceMode databaseMode;
			if (command->Type == SqlActionType::Select)
				databaseMode = Core::MsiPersistenceMode::ReadOnly;
			else
				databaseMode = Core::MsiPersistenceMode::Transact;

			WWuString wrappedFilePath = UtilitiesWrapper::GetWideStringFromSystemString(filePath);
			Core::Installer installer { wrappedFilePath, databaseMode, context->GetUnderlyingContext() };

			ExecuteMsiCommand(installer, command, parameters, context);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException { ex };
		}
	}

	// Utilities
	Boolean InstallerWrapper::IsInstallerPackage(String^ filePath, Core::CmdletContextProxy^ context)
	{
		WWuString wFilePath = UtilitiesWrapper::GetWideStringFromSystemString(filePath);
		if (!Core::IO::FileExists(wFilePath))
			throw gcnew System::IO::FileNotFoundException("Cannot find path '" + filePath + "' because it does not exist.");

		try {
			Core::MemoryMappedFile mappedFile { wFilePath };
			if (*reinterpret_cast<__uint64*>(mappedFile.data()) == 0xE11AB1A1E011CFD0 &&
				*reinterpret_cast<__uint64*>((BYTE*)mappedFile.data() + 8) == 0)
				return true;
		}
		catch (const Core::WuStdException& ex) {
			NativeException^ wrappedEx = gcnew NativeException(ex);
			context->WriteError(gcnew ErrorRecord(
				wrappedEx,
				"OpenFileError",
				ErrorCategory::OpenError,
				filePath
			));

			throw wrappedEx;
		}

		return false;
	}

	InstallerTableInfoBase^ InstallerWrapper::GetMsiSingleTableInfo(Core::Installer& installer, const WWuString& tableName)
	{
		try {
			PMSIHANDLE hNamesRecord;
			PMSIHANDLE hTypesRecord;
			wuvector<WWuString> keys;
			wumap<WWuString, int> positionInfo;

			installer.GetMsiTableKeys(tableName, keys);
			installer.GetColumnPositionInfo(tableName, positionInfo);
			installer.GetMsiColumnInfo(tableName, &hNamesRecord, &hTypesRecord);

			UINT recordCount = MsiRecordGetFieldCount(hNamesRecord);
			array<InstallerColumnInfo^>^ columnInfo = gcnew array<InstallerColumnInfo^>(recordCount);
			for (UINT i = 1; i <= recordCount; i++) {
				WWuString name = installer.RecordGetString(hNamesRecord, i);
				WWuString type = installer.RecordGetString(hTypesRecord, i);

				bool isKey = false;
				if (std::find(keys.begin(), keys.end(), name) != keys.end())
					isKey = true;

				columnInfo[i - 1] = gcnew InstallerColumnInfo(gcnew String(name.GetBuffer()), gcnew String(type.GetBuffer()), positionInfo[name], isKey);
			}

			return gcnew InstallerTableInfoBase(gcnew String(tableName.GetBuffer()), columnInfo);
		}
		catch (const Core::WuStdException& ex) {
			throw gcnew NativeException(ex);
		}
	}

	array<InstallerColumnInfo^>^ InstallerWrapper::GetMsiTableInfo(Core::Installer& installer)
	{
		PMSIHANDLE hNamesRecord;
		PMSIHANDLE hTypesRecord;

		installer.GetMsiColumnInfo(&hNamesRecord, &hTypesRecord);

		UINT recordCount = MsiRecordGetFieldCount(hNamesRecord);
		array<InstallerColumnInfo^>^ columnInfo = gcnew array<InstallerColumnInfo^>(recordCount);
		for (UINT i = 1; i <= recordCount; i++) {
			WWuString name = installer.RecordGetString(hNamesRecord, i);
			WWuString type = installer.RecordGetString(hTypesRecord, i);

			columnInfo[i - 1] = gcnew InstallerColumnInfo(gcnew String(name.GetBuffer()), gcnew String(type.GetBuffer()), i, false);
		}

		return columnInfo;
	}

	void InstallerWrapper::ExecuteMsiCommand(Core::Installer& installer, InstallerCommand^ command, List<InstallerCommandParameter^>^ parameters, Core::CmdletContextProxy^ context)
	{
		PMSIHANDLE hParams { };

		// Open a database view.
		WWuString sqlCommand = UtilitiesWrapper::GetWideStringFromSystemString(command->Command);
		installer.OpenDatabaseView(sqlCommand);

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
						installer.RecordSetString(hParams, recordIndex, UtilitiesWrapper::GetWideStringFromSystemString((String^)param->Data));
						break;
					case InstallerCommandParamType::Integer:
						installer.RecordSetInteger(hParams, recordIndex, (int)param->Data);
						break;
					case InstallerCommandParamType::File:
						installer.RecordSetStream(hParams, recordIndex, UtilitiesWrapper::GetWideStringFromSystemString((String^)param->Data));
						break;
					case InstallerCommandParamType::ByteArray:
					{
						// The installer API doesn't support writing a buffer to a record stream, so we
						// create a temporary file, save the data, write it to the stream, and delete the file.
						int dataLength = ((array<Byte>^)param->Data)->Length;
						IntPtr buffer = Marshal::AllocHGlobal(dataLength);
						Marshal::Copy((array<Byte>^)param->Data, 0, buffer, dataLength);
						
						try {
							Core::IO::WriteByteArrayToTempFile(reinterpret_cast<BYTE*>((void*)buffer), static_cast<DWORD>(dataLength), tempFilePath);
						}
						catch (const Core::WuStdException& ex) {
							ex.Cry(L"ErrorWriteBytesToTempFile", Core::WriteErrorCategory::InvalidResult, L"ParameterData", context->GetUnderlyingContext());
							throw ex;
						}

						// When we call 'RecordSetStream' the API opens the file and uses it up until the database commit.
						// Since we need to cleanup the temporary file, we mark it to delete at the end.
						installer.RecordSetStream(hParams, recordIndex, tempFilePath);
						deleteFile = true;
					} break;
				}

				recordIndex++;
			}
		}

		// Executing the view.
		installer.ViewExecute(hParams);

		// We only check for results if the command was a 'SELECT'.
		if (command->Type == SqlActionType::Select) {
			
			// Creating the data table and adding the columns.
			DataTable^ output = gcnew DataTable();
			auto columnInfo = GetMsiTableInfo(installer);
			for each (InstallerColumnInfo^ column in columnInfo)
				output->Columns->Add(column->ToDataColumn());

			do {
				// If 'ViewFetch' returns false there are no more records.
				PMSIHANDLE hRecord;
				if (!installer.ViewFetch(&hRecord))
					break;

				// Here we iterate through each column info, and depending on the data type we read
				// from the record accordingly and set the row values for each column.
				// The only managed types supported are string, short, int, or byte array.
				DataRow^ row = output->NewRow();
				for each (InstallerColumnInfo ^ column in columnInfo) {
					Object^ currentValue = nullptr;

					// String.
					if (column->DataType == String::typeid)
						row[column->Name] = gcnew String(installer.RecordGetString(hRecord, column->Position).GetBuffer());

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
						wuvector<char> dataVec;
						installer.RecordReadStream(hRecord, column->Position, dataVec);
						int vecSize = static_cast<int>(dataVec.size());
						if (dataVec.size() > 0) {
							array<Byte>^ dataArray = gcnew array<Byte>(vecSize);
							for (int i = 0; i < vecSize; i++)
								dataArray[i] = dataVec[i];

							row[column->Name] = dataArray;
						}
						else
							row[column->Name] = DBNull::Value;
					}
				}

				// Adding the row to the data table.
				output->Rows->Add(row);

			} while (true);

			context->WriteObject(output);
		}

		// Committing the database, if necessary.
		if (command->Type != SqlActionType::Select) {
			installer.DatabaseCommit();
			if (deleteFile)
				DeleteFile(tempFilePath.GetBuffer());
		}
	}
}