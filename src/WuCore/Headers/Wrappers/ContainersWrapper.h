#pragma once
#pragma unmanaged

#include "../Stubs/ContainersStub.h"

#pragma managed

#include "WrapperBase.h"
#include "NativeException.h"
#include "Types/WuManagedCabinet.h"

namespace WindowsUtils::Wrappers
{
	public ref class ContainersWrapper : public WrapperBase
	{
	public:
		ContainersWrapper(Core::CmdletContextProxy^ context)
			: WrapperBase(context) { }
		
		void ExpandArchiveFile(String^ path, String^ destination, ArchiveFileType type);
		void CompressArchiveFile(String^ path, String^ destination, String^ namePrefix, int maxCabSize, CabinetCompressionType compressionType, ArchiveFileType type);
	};
}