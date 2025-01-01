#pragma once

#define _WU_START_TRY try {

#define _WU_MARSHAL_CATCH(context)						 \
	}													 \
	catch (const WindowsUtils::Core::WuException& ex) {  \
		context->GetExceptionMarshaler()(ex);			 \
	}

#define _WU_MARSHAL_CATCH_PTR(marshal)                   \
	}                                                    \
	catch (const WindowsUtils::Core::WuException& ex) {  \
		marshal(ex);                                     \
	}

#define _WU_MANAGED_CATCH								 \
	}													 \
	catch (WindowsUtils::NativeException^ ex) {			 \
		Context->WriteError(ex->Record);				 \
		throw;											 \
	}