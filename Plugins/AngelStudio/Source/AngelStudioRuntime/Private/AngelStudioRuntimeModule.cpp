#include "AngelStudioRuntimeModule.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FAngelStudioRuntimeModule, AngelStudioRuntime)

void FAngelStudioRuntimeModule::StartupModule()
{
	// Runtime init (if needed)
}

void FAngelStudioRuntimeModule::ShutdownModule()
{
	// Runtime shutdown
}
