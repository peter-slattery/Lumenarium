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
  GlobalDebugServices = DebugServices;
  GlobalLogBuffer = LogBuffer;
  if (AppReady)
  {
    app_state* State = (app_state*)Context.MemoryBase;
    State->PanelSystem.PanelDefs = GlobalPanelDefs;
    State->PanelSystem.PanelDefsCount = GlobalPanelDefsCount;
    
    gs_data UserData = State->UserSpaceDesc.UserData;
    State->UserSpaceDesc = BlumenLumen_UserSpaceCreate();
    if (UserData.Memory && !State->UserSpaceDesc.UserData.Memory)
    {
      State->UserSpaceDesc.UserData = UserData;
    }
    US_LoadPatterns(&State->UserSpaceDesc, State, Context);
  }
}

INITIALIZE_APPLICATION(InitializeApplication)
{
  Context->MemorySize = sizeof(app_state);
  Context->MemoryBase = Alloc(Context->ThreadContext.Allocator, Context->MemorySize, "Memory Base"); 
  app_state* State = (app_state*)Context->MemoryBase;
  *State = {};
  
  State->Permanent = MemoryArenaCreate(MB(4), Bytes(8), Context->ThreadContext.Allocator,0, 0, "Permanent");
  State->Transient = Context->ThreadContext.Transient;
  State->Assemblies = AssemblyArray_Create(8, &State->Permanent);
  
  State->CommandQueue = CommandQueue_Create(&State->Permanent, 32);
  
  animation_system_desc AnimSysDesc = {};
  AnimSysDesc.Storage = &State->Permanent;
  AnimSysDesc.AnimArrayCount = 32;
  AnimSysDesc.SecondsPerFrame = 1.0f / 24.0f;
  State->AnimationSystem = AnimationSystem_Init(AnimSysDesc);
  
  if (!Context->Headless)
  {
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
    State->Interface = ui_InterfaceCreate(*Context, IConfig, &State->Permanent);
    
    PanelSystem_Init(&State->PanelSystem, GlobalPanelDefs, GlobalPanelDefsCount, &State->Permanent);
    
  }
  
  State->SACN = SACN_Initialize(*Context);
  
  State->LedSystem = LedSystem_Create(Context->ThreadContext.Allocator, 128);
  State->AssemblyDebugState = AssemblyDebug_Create(&State->Permanent);
  State->AssemblyDebugState.Brightness = 255;
  State->AssemblyDebugState.Override = ADS_Override_None;
  
  State->Modes = OperationModeSystemInit(&State->Permanent, Context->ThreadContext);
  
  ReloadStaticData(*Context, GlobalDebugServices, GlobalLogBuffer, true);
  US_CustomInit(&State->UserSpaceDesc, State, *Context);
  
  if (!Context->Headless)
  {
    // NOTE(pjs): This just sets up the default panel layout
    panel* RootPanel = PanelSystem_PushPanel(&State->PanelSystem, PanelType_SculptureView, State, *Context);
    SplitPanel(RootPanel, .25f, PanelSplit_Horizontal, &State->PanelSystem, State, *Context);
    
    panel* AnimPanel = RootPanel->Bottom;
    Panel_SetType(AnimPanel, &State->PanelSystem, PanelType_AnimationTimeline, State, *Context);
    
    panel* TopPanel = RootPanel->Top;
    SplitPanel(TopPanel, .5f, PanelSplit_Vertical, &State->PanelSystem, State, *Context);
    
    panel* LeftPanel = TopPanel->Left;
    SplitPanel(LeftPanel, .5f, PanelSplit_Vertical, &State->PanelSystem, State, *Context);
    
    panel* Profiler = LeftPanel->Right;
    Panel_SetType(Profiler, &State->PanelSystem, PanelType_MessageLog, State, *Context);
    
    panel* Hierarchy = LeftPanel->Left;
    Panel_SetType(Hierarchy, &State->PanelSystem, PanelType_AssemblyDebug, State, *Context);
    
  }
  
  State->RunEditor = !Context->Headless;
}

internal void
BuildAssemblyData (app_state* State, context Context, addressed_data_buffer_list* OutputData)
{
  
#define SEND_DATA
#ifdef SEND_DATA
  // NOTE(pjs): Building data buffers to be sent out to the sculpture
  // This array is used on the platform side to actually send the information
  assembly_array SACNAssemblies = AssemblyArray_Filter(State->Assemblies, AssemblyFilter_OutputsViaSACN, State->Transient);
  assembly_array UARTAssemblies = AssemblyArray_Filter(State->Assemblies, AssemblyFilter_OutputsViaUART, State->Transient);
  SACN_BuildOutputData(&State->SACN, OutputData, SACNAssemblies, &State->LedSystem);
  UART_BuildOutputData(OutputData, UARTAssemblies, &State->LedSystem, State->Transient);
#endif
}

UPDATE_AND_RENDER(UpdateAndRender)
{
  DEBUG_TRACK_FUNCTION;
  app_state* State = (app_state*)Context->MemoryBase;
  
  // NOTE(Peter): We do this at the beginning because all the render commands are stored in Transient,
  // and need to persist beyond the end of the UpdateAndRender call. In the release version, we won't
  // zero the Transient arena when we clear it so it wouldn't be a problem, but it is technically
  // incorrect to clear the arena, and then access the memory later.
  MemoryArenaClear(State->Transient);
  Assert(State->UserSpaceDesc.UserData.Memory != 0);
  
  if (State->RunEditor)
  {
    Editor_Update(State, Context, InputQueue);
  }
  
  AnimationSystem_Update(&State->AnimationSystem, Context->DeltaTime);
  if (AnimationSystem_NeedsRender(State->AnimationSystem))
  {
    Assert(State->UserSpaceDesc.UserData.Memory != 0);
    AnimationSystem_RenderToLedBuffers(&State->AnimationSystem,
                                       State->Assemblies,
                                       &State->LedSystem,
                                       State->Patterns,
                                       State->Transient,
                                       *Context,
                                       State->UserSpaceDesc.UserData.Memory);
  }
  
  Assert(State->UserSpaceDesc.UserData.Memory != 0);
  US_CustomUpdate(&State->UserSpaceDesc, State, Context);
  Assert(State->UserSpaceDesc.UserData.Memory != 0);
  
  AssemblyDebug_OverrideOutput(State->AssemblyDebugState,
                               State->Assemblies,
                               State->LedSystem);
  
  if (State->RunEditor)
  {
    Editor_Render(State, Context, RenderBuffer);
  }
  ResetWorkQueue(Context->GeneralWorkQueue);
  
  Assert(State->UserSpaceDesc.UserData.Memory != 0);
  BuildAssemblyData(State, *Context, OutputData);
  
  // NOTE(PS): We introduced this in order to test some things on the 
  // blumen lumen circuit boards, to see if they were getting out
  // of sync
  if (State->SendEmptyPackets) {
    for (addressed_data_buffer* At = OutputData->Root;
         At != 0;
         At = At->Next)
    {
      ZeroMemoryBlock(At->Memory, At->MemorySize);
    }
  }
}

CLEANUP_APPLICATION(CleanupApplication)
{
  app_state* State = (app_state*)Context.MemoryBase;
  
  for (u32 i = 0; i < State->Assemblies.Count; i++)
  {
    assembly Assembly = State->Assemblies.Values[i];
    led_buffer LedBuffer = State->LedSystem.Buffers[Assembly.LedBufferIndex];
    AssemblyDebug_OverrideWithColor(Assembly, LedBuffer, pixel{0, 0, 0});
  }
  BuildAssemblyData(State, Context, OutputData);
  
  US_CustomCleanup(&State->UserSpaceDesc, State, Context);
  SACN_Cleanup(&State->SACN, Context);
}

#define FOLDHAUS_APP_CPP
#endif // FOLDHAUS_APP_CPP