//
// File: foldhaus_app.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_APP_CPP

#include "foldhaus_platform.h"
#include "foldhaus_app.h"

////////////////////////////////////////////////////////////////////////

RELOAD_STATIC_DATA(ReloadStaticData)
{
    app_state* State = (app_state*)Context.MemoryBase;
    
    GlobalDebugServices = DebugServices;
    State->PanelSystem.PanelDefs = GlobalPanelDefs;
    State->PanelSystem.PanelDefsCount = GlobalPanelDefsCount;
}

INITIALIZE_APPLICATION(InitializeApplication)
{
    app_state* State = (app_state*)Context.MemoryBase;
    *State = {};
    
    State->Permanent = CreateMemoryArena(Context.ThreadContext.Allocator);
    State->Transient = Context.ThreadContext.Transient;
    
    State->Assemblies = AssemblyArray_Create(8, &State->Permanent);
    
    State->GlobalLog = PushStruct(State->Transient, event_log);
    *State->GlobalLog = {0};
    
    State->CommandQueue = CommandQueue_Create(&State->Permanent, 32);
    
    // TODO(Peter): put in InitializeInterface?
    r32 FontSize = 14;
    {
        gs_file FontFile = ReadEntireFile(Context.ThreadContext.FileHandler, ConstString("data/Anonymous Pro.ttf"));
        if (FileNoError(FontFile))
        {
            bitmap_font* Font = PushStruct(&State->Permanent, bitmap_font);
            
            Font->BitmapWidth = 512;
            Font->BitmapHeight = 512;
            Font->BitmapBytesPerPixel = 4;
            Font->BitmapMemory = PushArray(&State->Permanent, u8, Font->BitmapWidth * Font->BitmapHeight * Font->BitmapBytesPerPixel);
            Font->BitmapStride = Font->BitmapWidth * Font->BitmapBytesPerPixel;
            ZeroMemoryBlock(Font->BitmapMemory, Font->BitmapStride * Font->BitmapHeight);
            
            platform_font_info FontInfo = Context.PlatformGetFontInfo("Anonymous Pro", FontSize, FontWeight_Normal, false, false, false);
            Font->PixelHeight = FontInfo.PixelHeight;
            Font->Ascent = FontInfo.Ascent;
            Font->Descent = FontInfo.Descent;
            Font->Leading = FontInfo.Leading;
            Font->MaxCharWidth = FontInfo.MaxCharWidth;
            
            Font->CodepointDictionarySize = (FontInfo.CodepointOnePastLast - FontInfo.CodepointStart);
            Font->CodepointDictionaryCount = 0;
            Font->CodepointKeys = PushArray(&State->Permanent, char, Font->CodepointDictionarySize);
            Font->CodepointValues = PushArray(&State->Permanent, codepoint_bitmap, Font->CodepointDictionarySize);
            
            for (s32 Codepoint = FontInfo.CodepointStart;
                 Codepoint < FontInfo.CodepointOnePastLast;
                 Codepoint++)
            {
                
                u32 CodepointX, CodepointY;
                GetNextCodepointOffset(Font, &CodepointX, &CodepointY);
                
                u32 CodepointW, CodepointH;
                Context.PlatformDrawFontCodepoint(
                                                  Font->BitmapMemory,
                                                  Font->BitmapWidth,
                                                  Font->BitmapHeight,
                                                  CodepointX, CodepointY,
                                                  Codepoint, FontInfo,
                                                  &CodepointW, &CodepointH);
                
                AddCodepointToFont(Font, Codepoint, 0, 0, CodepointW, CodepointH, CodepointX, CodepointY);
            }
            
            State->Interface.Style.Font = Font;
            
            Font->BitmapTextureHandle = Context.PlatformGetGPUTextureHandle(Font->BitmapMemory,
                                                                            Font->BitmapWidth, Font->BitmapHeight);
        }
        else
        {
            LogError(State->GlobalLog, "Unable to load font");
        }
    }
    
    State->Interface.Style.FontSize = FontSize;
    State->Interface.Style.PanelBGColors[0] = v4{.3f, .3f, .3f, 1};
    State->Interface.Style.PanelBGColors[1] = v4{.4f, .4f, .4f, 1};
    State->Interface.Style.PanelBGColors[2] = v4{.5f, .5f, .5f, 1};
    State->Interface.Style.PanelBGColors[3] = v4{.6f, .6f, .6f, 1};
    State->Interface.Style.ButtonColor_Inactive = BlackV4;
    State->Interface.Style.ButtonColor_Active = v4{.1f, .1f, .1f, 1};
    State->Interface.Style.ButtonColor_Selected = v4{.3f, .3f, .3f, 1};
    State->Interface.Style.TextColor = WhiteV4;
    State->Interface.Style.ListBGColors[0] = v4{ .16f, .16f, .16f, 1.f };
    State->Interface.Style.ListBGColors[1] = v4{ .18f, .18f, .18f, 1.f };
    State->Interface.Style.ListBGHover = v4{ .22f, .22f, .22f, 1.f };
    State->Interface.Style.ListBGSelected = v4{.44f, .44f, .44f, 1.f };
    State->Interface.Style.Margin = v2{5, 5};
    State->Interface.Style.RowHeight = ui_GetTextLineHeight(State->Interface) + (2 * State->Interface.Style.Margin.y);
    
    State->Interface.WidgetsCountMax = 4096;
    State->Interface.Widgets = PushArray(&State->Permanent, ui_widget, State->Interface.WidgetsCountMax);
    State->Interface.PerFrameMemory = PushStruct(&State->Permanent, gs_memory_arena);
    *State->Interface.PerFrameMemory = CreateMemoryArena(Context.ThreadContext.Allocator);
    
    State->SACN = SACN_Initialize(Context);
    
    State->LedSystem = LedSystemInitialize(Context.ThreadContext.Allocator, 128);
    
#if 1
    gs_const_string SculpturePath = ConstString("data/blumen_lumen_silver_spring.fold");
    LoadAssembly(&State->Assemblies, &State->LedSystem, State->Transient, Context, SculpturePath, State->GlobalLog);
#endif
    
    State->PixelsToWorldScale = .01f;
    
    GlobalDebugServices->Interface.RenderSculpture = true;
    
    ReloadStaticData(Context, GlobalDebugServices);
    
    State->Modes = OperationModeSystemInit(&State->Permanent, Context.ThreadContext);
    
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
        
        Animation_AddBlock(&Anim, 0, Anim.PlayableRange.Max, 4, 0);
        
        AnimationArray_Push(&State->AnimationSystem.Animations, Anim);
        
        State->AnimationSystem.TimelineShouldAdvance = true;
    } // End Animation Playground
    
    PanelSystem_Init(&State->PanelSystem, GlobalPanelDefs, GlobalPanelDefsCount, &State->Permanent);
    PanelSystem_PushPanel(&State->PanelSystem, PanelType_SculptureView, State, Context);
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
        State->AnimationSystem.LastUpdatedFrame = State->AnimationSystem.CurrentFrame;
        AnimationSystem_RenderToLedBuffers(&State->AnimationSystem,
                                           State->Assemblies,
                                           &State->LedSystem,
                                           GlobalAnimationPatterns,
                                           State->Transient);
    }
    
    {
        // NOTE(pjs): Building data buffers to be sent out to the sculpture
        // This array is used on the platform side to actually send the information
        assembly_array SACNAssemblies = AssemblyArray_Filter(State->Assemblies, AssemblyFilter_OutputsViaSACN, State->Transient);
        assembly_array UARTAssemblies = AssemblyArray_Filter(State->Assemblies, AssemblyFilter_OutputsViaUART, State->Transient);
        SACN_BuildOutputData(&State->SACN, OutputData, SACNAssemblies, &State->LedSystem);
        UART_BuildOutputData(OutputData, UARTAssemblies, &State->LedSystem);
    }
    
    Editor_Render(State, Context, RenderBuffer);
    
    // Checking for overflows
#if 0
    {
        DEBUG_TRACK_SCOPE(OverflowChecks);
        AssertAllocationsNoOverflow(State->Permanent);
        for (u32 i = 0; i < State->Assemblies.Count; i++)
        {
            assembly* Assembly = &State->Assemblies.Values[i];
            AssertAllocationsNoOverflow(Assembly->Arena);
        }
    }
#endif
}

CLEANUP_APPLICATION(CleanupApplication)
{
    app_state* State = (app_state*)Context.MemoryBase;
    SACN_Cleanup(&State->SACN, Context);
}

#define FOLDHAUS_APP_CPP
#endif // FOLDHAUS_APP_CPP