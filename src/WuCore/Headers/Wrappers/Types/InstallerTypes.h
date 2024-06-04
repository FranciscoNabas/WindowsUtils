#pragma once

#pragma managed

namespace WindowsUtils::Installer
{
    using namespace System;
	using namespace System::Data;

    public enum class InstallerFileNameSize
    {
        Long,
        Short
    };

    public enum class InstallerPackageSecurity
    {
        NoRestriction = 0,
        ReadOnlyRecommended = 2,
        ReadOnlyEnforced = 4
    };

    public enum class SqlActionType
    {
        Select,
        Delete,
        Update,
        Insert,
        Create,
        Drop,
        Alter
    };

	public enum class InstallerCommandParamType
	{
		String,
		Integer,
		ByteArray,
		File
	};

    public ref class InvalidCommandException : public Exception
    {
    public:
        property String^ Command { String^ get() { return m_command; } }

        InvalidCommandException(String^ command)
            : Exception("Invalid SQL command."), m_command(command) { }

        InvalidCommandException(String^ command, String^ message)
            : Exception(message), m_command(command) { }

    private:
        String^ m_command;
    };

    public ref class InstallerCommand sealed
    {
    public:
        property String^ Command { String^ get() { return m_command; } }
        property SqlActionType Type { SqlActionType get() { return m_type; } }
		property Int16 MarkerCount { Int16 get() { return m_markerCount; } }

		InstallerCommand(String^ command, SqlActionType type)
			: m_command(command), m_type(type)
		{
			for (int i = 0; i < command->Length; i++)
				if (command[i] == '?')
					m_markerCount++;
		}

    private:
        String^ m_command;
        SqlActionType m_type;
		Int16 m_markerCount;
    };

	public ref class InstallerCommandParameter sealed
	{
	public:
		property InstallerCommandParamType Type { InstallerCommandParamType get() { return m_type; } }
		property Object^ Data { Object^ get() { return m_data; } }

		InstallerCommandParameter(InstallerCommandParamType type, Object^ data)
			: m_type(type), m_data(data)
		{
			auto dataType = data->GetType();

			switch (type) {
				case WindowsUtils::Installer::InstallerCommandParamType::String:
				{
					if (dataType != String::typeid)
						throw gcnew ArgumentException("Input type is 'string' but object type is '" + dataType->Name + "'.");
				} break;

				case WindowsUtils::Installer::InstallerCommandParamType::Integer:
				{
					if (dataType != Int32::typeid && dataType != Int16::typeid)
						throw gcnew ArgumentException("Input type is 'integer' but object type is '" + dataType->Name + "'.");
				} break;

				case WindowsUtils::Installer::InstallerCommandParamType::ByteArray:
				{
					if (dataType != array<Byte>::typeid)
						throw gcnew ArgumentException("Input type is 'byte array' but object type is '" + dataType->Name + "'.");
				} break;

				case WindowsUtils::Installer::InstallerCommandParamType::File:
				{
					if (dataType != String::typeid)
						throw gcnew ArgumentException("Input type is 'file' but object type is '" + dataType->Name + "'.");

					if (!System::IO::File::Exists((String^)data))
						throw gcnew System::IO::FileNotFoundException("Cannot find path '" + (String^)data + "' because it does not exist.");
				} break;
			}
		}

	private:
		Object^ m_data;
		InstallerCommandParamType m_type;
	};

	public ref class InstallerSummaryInfo sealed
	{
	public:
		property Int32 Codepage { Int32 get() { return m_wrapper->Codepage; } }
		property String^ Title { String^ get() { return gcnew String(m_wrapper->Title.GetBuffer()); } }
		property String^ Subject { String^ get() { return gcnew String(m_wrapper->Subject.GetBuffer()); } }
		property String^ Author { String^ get() { return gcnew String(m_wrapper->Author.GetBuffer()); } }
		property String^ Keywords { String^ get() { return gcnew String(m_wrapper->Keywords.GetBuffer()); } }
		property String^ Comments { String^ get() { return gcnew String(m_wrapper->Comments.GetBuffer()); } }
		property String^ Platform { String^ get() { return m_platform; } }
		property String^ Languages { String^ get() { return m_languages; } }
		property String^ LastSavedBy { String^ get() { return gcnew String(m_wrapper->LastSavedBy.GetBuffer()); } }
		property String^ PackageCode { String^ get() { return gcnew String(m_wrapper->RevisionNumber.GetBuffer()); } }
		property DateTime^ LastPrinted { DateTime^ get() { return Wrappers::UtilitiesWrapper::GetDateTimeFromFileTime(m_wrapper->LastPrinted); } }
		property DateTime^ CreateTime { DateTime^ get() { return Wrappers::UtilitiesWrapper::GetDateTimeFromFileTime(m_wrapper->CreateTimeDate); } }
		property DateTime^ LastSaveTime { DateTime^ get() { return Wrappers::UtilitiesWrapper::GetDateTimeFromFileTime(m_wrapper->LastSaveTimeDate); } }
		property Int32 PageCount { Int32 get() { return m_wrapper->PageCount; } }
		property InstallerFileNameSize FileNameSize { InstallerFileNameSize get() { return m_fileNameSize; } }
		property Boolean Compressed { Boolean get() { return m_compressed; } }
		property Boolean Administrative { Boolean get() { return m_admin; } }
		property Boolean UacCompliant { Boolean get() { return m_uacCompliant; } }
		property Int32 CharacterCount { Int32 get() { return m_wrapper->CharacterCount; } }
		property String^ CreatingApplication { String^ get() { return gcnew String(m_wrapper->CreatingApplication.GetBuffer()); } }
		property InstallerPackageSecurity Security { InstallerPackageSecurity get() { return static_cast<InstallerPackageSecurity>(m_wrapper->Security); } }

		InstallerSummaryInfo(const Core::WU_SUMMARY_INFO& summaryInfo)
		{
			m_wrapper = new Core::WU_SUMMARY_INFO(summaryInfo);

			wuvector<WWuString> templateSplit = m_wrapper->Template.Split(';');
			if (templateSplit.size() > 0) {
				m_platform = gcnew String(templateSplit[0].GetBuffer());
				if (templateSplit.size() > 1) {
					m_languages = gcnew String(templateSplit[1].GetBuffer());
				}
			}

			std::bitset<4> wordCount(m_wrapper->WordCount);
			m_fileNameSize = wordCount[0] ? InstallerFileNameSize::Short : InstallerFileNameSize::Long;
			m_compressed = wordCount[1];
			m_admin = wordCount[2];
			m_uacCompliant = wordCount[3];
		}

		~InstallerSummaryInfo() { delete m_wrapper; }

	protected:
		!InstallerSummaryInfo() { delete m_wrapper; }

	private:
		String^ m_platform;
		String^ m_languages;
		Boolean m_compressed;
		Boolean m_admin;
		Boolean m_uacCompliant;
		InstallerFileNameSize m_fileNameSize;
		Core::PWU_SUMMARY_INFO m_wrapper;
	};

	public ref class InstallerColumnInfo sealed
	{
	public:
		property String^ Name { String^ get() { return m_name; } }
		property Type^ DataType { Type^ get() { return m_type; } }
		property Int32 Position { Int32 get() { return m_position; } }
		property Int16 MaxLength { Int16 get() { return m_maxLen; } }
		property Boolean IsNullable { Boolean get() { return m_nullable; } }
		property Boolean IsKey { Boolean get() { return m_key; } }
		property Boolean IsLocalizable { Boolean get() { return m_localizable; } }
		property Boolean IsVariableLength { Boolean get() { return m_variableLength; } }

		InstallerColumnInfo(String^ name, String^ type, Int32 position, Boolean key)
		{
			m_nullable = false;
			m_localizable = false;
			m_variableLength = false;
			m_maxLen = -1;

			switch (type->ToLower()[0]) {
				case 's':
				{
					if (type[1] == '0')
						m_variableLength = true;
					else
						m_maxLen = Convert::ToInt16(type->Substring(1));

					m_type = String::typeid;
				} break;

				case 'l':
				{
					if (type[1] == '0')
						m_variableLength = true;
					else
						m_maxLen = Convert::ToInt16(type->Substring(1));

					m_type = String::typeid;
					m_localizable = true;
				} break;

				case 'i':
				{
					if (type[1] == '2')
						m_type = Int16::typeid;
					else
						m_type = Int32::typeid;
				} break;

				case 'v':
					m_type = array<Byte>::typeid;
					break;

				default:
					throw gcnew ArgumentException("Unknown type '" + type + "'.");
					break;
			}

			if (Char::IsUpper(type[0]))
				m_nullable = true;

			m_key = key;
			m_name = name;
			m_position = position;
		}

		DataColumn^ ToDataColumn()
		{
			DataColumn^ output = gcnew DataColumn(this->m_name, this->DataType);
			output->AllowDBNull = this->IsNullable;
			if (this->m_maxLen != -1)
				output->MaxLength = this->m_maxLen;

			return output;
		}

	private:
		String^ m_name;
		Type^ m_type;
		Int32 m_position;
		Int16 m_maxLen;
		Boolean m_nullable;
		Boolean m_key;
		Boolean m_localizable;
		Boolean m_variableLength;
	};

	public ref class InstallerTableInfoBase
	{
	public:
		property String^ TableName { String^ get() { return m_tableName; } }
		property array<InstallerColumnInfo^>^ Columns { array<InstallerColumnInfo^>^ get() { return m_columns; } }

		InstallerTableInfoBase(String^ tableName, array<InstallerColumnInfo^>^ columns)
			: m_tableName(tableName), m_columns(columns)
		{ }

	private:
		String^ m_tableName;
		array<InstallerColumnInfo^>^ m_columns;
	};
}