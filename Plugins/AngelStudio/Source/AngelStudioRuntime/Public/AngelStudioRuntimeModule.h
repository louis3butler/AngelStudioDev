#pragma once

#include "Modules/ModuleManager.h"

class FAngelStudioRuntimeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
