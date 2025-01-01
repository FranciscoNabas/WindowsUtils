#pragma once
#pragma unmanaged

#include <bitset>

#include "../Support/IO.h"
#include "../Stubs/InstallerStub.h"

#pragma managed

#include "WrapperBase.h"
#include "NativeException.h"
#include "UtilitiesWrapper.h"
#include "Types/InstallerTypes.h"

namespace WindowsUtils::Wrappers
{
	using namespace System;
	using namespace System::Data;
	using namespace System::Collections::Generic;
	using namespace System::Runtime::InteropServices;
	using namespace WindowsUtils::Installer;

	public ref class InstallerWrapper : public WrapperBase
	{
	public:
		InstallerWrapper(Core::CmdletContextProxy^ context)
			: WrapperBase(context) { }

		// Get-MsiProperties
		Dictionary<String^, Object^>^ GetMsiProperties(String^ filepath);
		
		// Get-MsiSummaryInfo
		InstallerSummaryInfo^ GetMsiSummaryInfo(String^ filePath);
		
		// Get-MsiTableInfo
		List<InstallerTableInfoBase^>^ GetMsiTableInfo(String^ filePath, array<String^>^ tableNames);
		
		// Get-MsiTableData
		void GetMsiTableDump(String^ filePath, array<String^>^ tableNames);
		void GetMsiTableDump(String^ filePath, InstallerTableInfoBase^ tableInfo);

		// Invoke-MsiQuery
		void InvokeMsiQuery(String^ filePath, InstallerCommand^ command, List<InstallerCommandParameter^>^ parameters);
		
		// Utilities
		Boolean IsInstallerPackage(String^ filePath);

	private:
		List<InstallerTableInfoBase^>^ GetMsiTableInfo(Stubs::Installer& stub, const WuList<WWuString>& tableNames);
		array<InstallerColumnInfo^>^ GetMsiTableInfo(Stubs::Installer& stub);
		InstallerTableInfoBase^ GetMsiSingleTableInfo(Stubs::Installer& stub, const WWuString& tableName);

		void ExecuteMsiCommand(Stubs::Installer& stub, InstallerCommand^ command, List<InstallerCommandParameter^>^ parameters);
	};
}