//
// File: foldhaus_app.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_APP_CPP

#include "foldhaus_platform.h"
#include "foldhaus_app.h"

////////////////////////////////////////////////////////////////////////

internal void
ClearAndPushPatterns(animation_pattern_array* Patterns)
{
    if (Patterns->CountMax == 0) { return; }
    
    Patterns->Count = 0;
    Patterns_PushPattern(Patterns, TestPatternOne);
    Patterns_PushPattern(Patterns, TestPatternTwo);
    Patterns_PushPattern(Patterns, TestPatternThree);
    Patterns_PushPattern(Patterns, Pattern_AllGreen);
    Patterns_PushPattern(Patterns, Pattern_HueShift);
    Patterns_PushPattern(Patterns, Pattern_HueFade);
    Patterns_PushPattern(Patterns, Pattern_Spots);
    Patterns_PushPattern(Patterns, Pattern_LighthouseRainbow);
    Patterns_PushPattern(Patterns, Pattern_SmoothGrowRainbow);
    Patterns_PushPattern(Patterns, Pattern_GrowAndFade);
}

RELOAD_STATIC_DATA(ReloadStaticData)
{
    app_state* State = (app_state*)Context.MemoryBase;
    
    GlobalDebugServices = DebugServices;
    State->PanelSystem.PanelDefs = GlobalPanelDefs;
    State->PanelSystem.PanelDefsCount = GlobalPanelDefsCount;
    
    ClearAndPushPatterns(&State->Patterns);
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
    
    State->Patterns = Patterns_Create(&State->Permanent, 10);
    ClearAndPushPatterns(&State->Patterns);
    
    interface_config IConfig = {0};
    IConfig.FontSize = 14;
    IConfig.PanelBGColors[0] = v4{.3f, .3f, .3f, 1};
    IConfig.PanelBGColors[1] = v4{.4f, .4f, .4f, 1};
    IConfig.PanelBGColors[2] = v4{.5f, .5f, .5f, 1};
    IConfig.PanelBGColors[3] = v4{.6f, .6f, .6f, 1};
    IConfig.ButtonColor_Inactive = BlackV4;
    IConfig.ButtonColor_Active = v4{.1f, .1f, .1f, 1};
    IConfig.ButtonColor_Selected = v4{.3f, .3f, .3f, 1};
    IConfig.TextColor = WhiteV4;
    IConfig.ListBGColors[0] = v4{ .16f, .16f, .16f, 1.f };
    IConfig.ListBGColors[1] = v4{ .18f, .18f, .18f, 1.f };
    IConfig.ListBGHover = v4{ .22f, .22f, .22f, 1.f };
    IConfig.ListBGSelected = v4{.44f, .44f, .44f, 1.f };
    IConfig.Margin = v2{5, 5};
    State->Interface = ui_InterfaceCreate(Context, IConfig, &State->Permanent);
    
    State->SACN = SACN_Initialize(Context);
    
    State->LedSystem = LedSystem_Create(Context.ThreadContext.Allocator, 128);
    
    GlobalDebugServices->Interface.RenderSculpture = true;
    
    PanelSystem_Init(&State->PanelSystem, GlobalPanelDefs, GlobalPanelDefsCount, &State->Permanent);
    PanelSystem_PushPanel(&State->PanelSystem, PanelType_SculptureView, State, Context);
    State->Modes = OperationModeSystemInit(&State->Permanent, Context.ThreadContext);
    
    ReloadStaticData(Context, GlobalDebugServices);
    
#if 1
    gs_const_string SculpturePath = ConstString("data/test_blumen.fold");
    LoadAssembly(&State->Assemblies, &State->LedSystem, State->Transient, Context, SculpturePath, State->GlobalLog);
#endif
    
    { // Animation PLAYGROUND
        State->AnimationSystem = {};
        State->AnimationSystem.Storage = &State->Permanent;
        State->AnimationSystem.Animations = AnimationArray_Create(State->AnimationSystem.Storage, 32);
        
        State->AnimationSystem.SecondsPerFrame = 1.f / 24.f;
        
        animation Anim = {0};
        Anim.Name = PushStringF(&State->Permanent, 256, "test_anim_one");
        Anim.Layers = AnimLayerArray_Create(State->AnimationSystem.Storage, 8);
        Anim.Blocks_ = AnimBlockArray_Create(State->AnimationSystem.Storage, 8);
        Anim.PlayableRange.Min = 0;
        Anim.PlayableRange.Max = SecondsToFrames(15, State->AnimationSystem);
        Animation_AddLayer(&Anim, MakeString("Base Layer"), BlendMode_Overwrite, &State->AnimationSystem);
        Animation_AddLayer(&Anim, MakeString("Color Layer"), BlendMode_Multiply, &State->AnimationSystem);
        Animation_AddLayer(&Anim, MakeString("Sparkles"), BlendMode_Add, &State->AnimationSystem);
        
        Animation_AddBlock(&Anim, 0, Anim.PlayableRange.Max, Patterns_IndexToHandle(5), 0);
        
        AnimationArray_Push(&State->AnimationSystem.Animations, Anim);
        
        State->AnimationSystem.TimelineShouldAdvance = true;
    } // End Animation Playground
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
                                           State->UserData.Memory);
    }
    
    AssemblyDebug_OverrideOutput(State->AssemblyDebugState,
                                 State->Assemblies,
                                 State->LedSystem);
    
    // NOTE(pjs): Building data buffers to be sent out to the sculpture
    // This array is used on the platform side to actually send the information
    assembly_array SACNAssemblies = AssemblyArray_Filter(State->Assemblies, AssemblyFilter_OutputsViaSACN, State->Transient);
    assembly_array UARTAssemblies = AssemblyArray_Filter(State->Assemblies, AssemblyFilter_OutputsViaUART, State->Transient);
    SACN_BuildOutputData(&State->SACN, OutputData, SACNAssemblies, &State->LedSystem);
    UART_BuildOutputData(OutputData, UARTAssemblies, &State->LedSystem, State->Transient);
    
    Editor_Render(State, Context, RenderBuffer);
}

CLEANUP_APPLICATION(CleanupApplication)
{
    app_state* State = (app_state*)Context.MemoryBase;
    SACN_Cleanup(&State->SACN, Context);
}

#define FOLDHAUS_APP_CPP
#endif // FOLDHAUS_APP_CPP