//
// File: foldhaus_app.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_APP_CPP

#include "foldhaus_platform.h"
#include "foldhaus_app.h"

RELOAD_STATIC_DATA(ReloadStaticData)
{
    app_state* State = (app_state*)Context.MemoryBase;
    
    GlobalDebugServices = DebugServices;
    State->PanelSystem.PanelDefs = GlobalPanelDefs;
    State->PanelSystem.PanelDefsCount = GlobalPanelDefsCount;
    
    US_LoadPatterns(&State->UserSpaceDesc, State, Context);
}

INITIALIZE_APPLICATION(InitializeApplication)
{
    app_state* State = (app_state*)Context.MemoryBase;
    *State = {};
    
    State->Permanent = CreateMemoryArena(Context.ThreadContext.Allocator);
    State->Transient = Context.ThreadContext.Transient;
    State->Assemblies = AssemblyArray_Create(8, &State->Permanent);
    
    State->GlobalLog = PushStruct(&State->Permanent, event_log);
    
    State->CommandQueue = CommandQueue_Create(&State->Permanent, 32);
    
    animation_system_desc AnimSysDesc = {};
    AnimSysDesc.Storage = &State->Permanent;
    AnimSysDesc.AnimArrayCount = 32;
    AnimSysDesc.SecondsPerFrame = 1.0f / 24.0f;
    State->AnimationSystem = AnimationSystem_Init(AnimSysDesc);
    
    interface_config IConfig = {0};
    IConfig.FontSize = 14;
    IConfig.PanelBG              = v4{ .3f,  .3f,  .3f, 1.f };
    IConfig.ButtonColor_Inactive = BlackV4;
    IConfig.ButtonColor_Active   = v4{ .1f,  .1f,  .1f, 1.f };
    IConfig.ButtonColor_Selected = v4{ .3f,  .3f,  .3f, 1.f };
    IConfig.TextColor            = WhiteV4;
    IConfig.ListBGColors[0]      = v4{ .16f, .16f, .16f, 1.f };
    IConfig.ListBGColors[1]      = v4{ .18f, .18f, .18f, 1.f };
    IConfig.ListBGHover          = v4{ .22f, .22f, .22f, 1.f };
    IConfig.ListBGSelected       = v4{ .44f, .44f, .44f, 1.f };
    IConfig.Margin = v2{5, 5};
    State->Interface = ui_InterfaceCreate(Context, IConfig, &State->Permanent);
    
    State->SACN = SACN_Initialize(Context);
    
    State->LedSystem = LedSystem_Create(Context.ThreadContext.Allocator, 128);
    State->AssemblyDebugState = AssemblyDebug_Create(&State->Permanent);
    State->AssemblyDebugState.Override = ADS_Override_AllRed;
    
    GlobalDebugServices->Interface.RenderSculpture = true;
    
    PanelSystem_Init(&State->PanelSystem, GlobalPanelDefs, GlobalPanelDefsCount, &State->Permanent);
    PanelSystem_PushPanel(&State->PanelSystem, PanelType_SculptureView, State, Context);
    State->Modes = OperationModeSystemInit(&State->Permanent, Context.ThreadContext);
    
    State->UserSpaceDesc = BlumenLumen_UserSpaceCreate();
    
    ReloadStaticData(Context, GlobalDebugServices);
    US_CustomInit(&State->UserSpaceDesc, State, Context);
}

UPDATE_AND_RENDER(UpdateAndRender)
{
    DEBUG_TRACK_FUNCTION;
    app_state* State = (app_state*)Context->MemoryBase;
    
    // NOTE(Peter): We do this at the beginning because all the render commands are stored in Transient,
    // and need to persist beyond the end of the UpdateAndRender call. In the release version, we won't
    // zero the Transient arena when we clear it so it wouldn't be a problem, but it is technically
    // incorrect to clear the arena, and then access the memory later.
    ClearArena(State->Transient);
    
    Editor_Update(State, Context, InputQueue);
    
    AnimationSystem_Update(&State->AnimationSystem);
    if (AnimationSystem_NeedsRender(State->AnimationSystem))
    {
        AnimationSystem_RenderToLedBuffers(&State->AnimationSystem,
                                           State->Assemblies,
                                           &State->LedSystem,
                                           State->Patterns,
                                           State->Transient,
                                           State->UserSpaceDesc.UserData.Memory);
    }
    
    US_CustomUpdate(&State->UserSpaceDesc, State, Context);
    
    AssemblyDebug_OverrideOutput(State->AssemblyDebugState,
                                 State->Assemblies,
                                 State->LedSystem);
    
    Editor_Render(State, Context, RenderBuffer);
    
    // NOTE(pjs): Building data buffers to be sent out to the sculpture
    // This array is used on the platform side to actually send the information
    assembly_array SACNAssemblies = AssemblyArray_Filter(State->Assemblies, AssemblyFilter_OutputsViaSACN, State->Transient);
    assembly_array UARTAssemblies = AssemblyArray_Filter(State->Assemblies, AssemblyFilter_OutputsViaUART, State->Transient);
    SACN_BuildOutputData(&State->SACN, OutputData, SACNAssemblies, &State->LedSystem);
    UART_BuildOutputData(OutputData, UARTAssemblies, &State->LedSystem, State->Transient);
}

CLEANUP_APPLICATION(CleanupApplication)
{
    app_state* State = (app_state*)Context.MemoryBase;
    SACN_Cleanup(&State->SACN, Context);
}

#define FOLDHAUS_APP_CPP
#endif // FOLDHAUS_APP_CPP