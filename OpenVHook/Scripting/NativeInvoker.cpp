#include "NativeInvoker.h"
#include "ScriptEngine.h"

void NativeInvoke::Invoke( NativeContext * cxt, uint64_t oldHash ) {

	auto fn = ScriptEngine::GetNativeHandler( oldHash );

	if ( fn != 0 ) {
		fn( cxt );
	}
}

