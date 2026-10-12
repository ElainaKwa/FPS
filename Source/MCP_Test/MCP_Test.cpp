// Copyright Epic Games, Inc. All Rights Reserved.

#include "MCP_Test.h"
#include "Modules/ModuleManager.h"
#include "FPSWeaponAuthoring.h"

class FMCPTestModule : public FDefaultGameModuleImpl
{
    virtual void StartupModule() override
    {
        FDefaultGameModuleImpl::StartupModule();
#if WITH_EDITOR
        FPSWeaponAuthoring::Register();
#endif
    }
    virtual void ShutdownModule() override
    {
#if WITH_EDITOR
        FPSWeaponAuthoring::Unregister();
#endif
        FDefaultGameModuleImpl::ShutdownModule();
    }
};
IMPLEMENT_PRIMARY_GAME_MODULE(FMCPTestModule, MCP_Test, "MCP_Test");
