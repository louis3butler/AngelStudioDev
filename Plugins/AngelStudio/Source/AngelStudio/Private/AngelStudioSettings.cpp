#include "AngelStudioSettings.h"

UAngelStudioSettings::UAngelStudioSettings()
{
	DefaultOutputFolder.Path = TEXT("/Game/AngelStudio/Generated");
	LandmarkDebugLifetime = 10.f;
	LandmarkDebugPointSize = 10.f;
}
