#pragma once
#pragma managed

#include "Types/WuManagedCabinet.h"

namespace WindowsUtils::Wrappers
{
	public ref class ContainersWrapper
	{
	public:
		void ExpandArchiveFile(Object^ archiveObject, String^ destination, Core::ArchiveFileType fileType);
		void CompressArchiveFile(String^ path, String^ destination, String^ namePrefix, int maxCabSize, Core::CabinetCompressionType compressionType, Core::ArchiveFileType type, Core::CmdletContextProxy^ context);
	};
}