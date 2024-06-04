#pragma once
#pragma unmanaged

#include <bitset>

#include "../Support/IO.h"

#pragma managed

#include "UtilitiesWrapper.h"
#include "CmdletContextProxy.h"
#include "Types/InstallerTypes.h"

namespace WindowsUtils::Wrappers
{
	using namespace System;
	using namespace System::Data;
	using namespace System::Collections::Generic;
	using namespace System::Runtime::InteropServices;
	using namespace WindowsUtils::Installer;

	public ref class InstallerWrapper
	{
	public:
		// Get-MsiProperties
		Dictionary<String^, Object^>^ GetMsiProperties(String^ filepath, Core::CmdletContextProxy^ context);
		
		// Get-MsiSummaryInfo
		InstallerSummaryInfo^ GetMsiSummaryInfo(String^ filePath, Core::CmdletContextProxy^ context);
		
		// Get-MsiTableInfo
		List<InstallerTableInfoBase^>^ GetMsiTableInfo(String^ filePath, array<String^>^ tableNames, Core::CmdletContextProxy^ context);
		
		// Get-MsiTableData
		void GetMsiTableDump(String^ filePath, array<String^>^ tableNames, Core::CmdletContextProxy^ context);
		void GetMsiTableDump(String^ filePath, InstallerTableInfoBase^ tableInfo, Core::CmdletContextProxy^ context);

		// Invoke-MsiQuery
		void InvokeMsiQuery(String^ filePath, InstallerCommand^ command, List<InstallerCommandParameter^>^ parameters, Core::CmdletContextProxy^ context);
		
		// Utilities
		static Boolean IsInstallerPackage(String^ filePath, Core::CmdletContextProxy^ context);

	private:
		List<InstallerTableInfoBase^>^ GetMsiTableInfo(Core::Installer& installer, const wuvector<WWuString>& tableNames);
		array<InstallerColumnInfo^>^ GetMsiTableInfo(Core::Installer& installer);
		InstallerTableInfoBase^ GetMsiSingleTableInfo(Core::Installer& installer, const WWuString& tableName);

		void ExecuteMsiCommand(Core::Installer& installer, InstallerCommand^ command, List<InstallerCommandParameter^>^ parameters, Core::CmdletContextProxy^ context);
	};
}