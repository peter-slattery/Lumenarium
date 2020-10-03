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
    
    s32 CommandQueueSize = 32;
    command_queue_entry* CommandQueueMemory = PushArray(&State->Permanent,
                                                        command_queue_entry,
                                                        CommandQueueSize);
    State->CommandQueue = InitializeCommandQueue(CommandQueueMemory, CommandQueueSize);
    
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
    State->Interface.Style.ButtonColor_Selected = v4{.1f, .1f, .3f, 1};
    State->Interface.Style.TextColor = WhiteV4;
    State->Interface.Style.ListBGColors[0] = v4{ .16f, .16f, .16f, 1.f };
    State->Interface.Style.ListBGColors[1] = v4{ .18f, .18f, .18f, 1.f };
    State->Interface.Style.ListBGHover = v4{ .22f, .22f, .22f, 1.f };
    State->Interface.Style.ListBGSelected = v4{.44f, .44f, .44f, 1.f };
    State->Interface.Style.Margin = v2{5, 5};
    State->Interface.Style.RowHeight = ui_GetTextLineHeight(State->Interface);
    
    State->SACN = SACN_Initialize(Context);
    
    State->Camera.FieldOfView = 45.0f;
    State->Camera.AspectRatio = RectAspectRatio(State->WindowBounds);
    State->Camera.Near = .1f;
    State->Camera.Far = 800.0f;
    State->Camera.Position = v3{0, 0, 400};
    State->Camera.LookAt = v3{0, 0, 0
    };
    
    State->LedSystem = LedSystemInitialize(Context.ThreadContext.Allocator, 128);
    
#if 1
    gs_const_string SculpturePath = ConstString("data/blumen_lumen_v2.fold");
    LoadAssembly(&State->Assemblies, &State->LedSystem, State->Transient, Context, SculpturePath, State->GlobalLog);
#endif
    
    State->PixelsToWorldScale = .01f;
    
    GlobalDebugServices->Interface.RenderSculpture = true;
    
    ReloadStaticData(Context, GlobalDebugServices);
    
    State->Modes = OperationModeSystemInit(&State->Permanent, Context.ThreadContext);
    
    { // Animation PLAYGROUND
        State->AnimationSystem = {};
        State->AnimationSystem.Storage = &State->Permanent;
        State->AnimationSystem.SecondsPerFrame = 1.f / 24.f;
        State->AnimationSystem.PlayableRange.Min = 0;
        State->AnimationSystem.PlayableRange.Max = SecondsToFrames(15, State->AnimationSystem);
        State->AnimationSystem.LayersMax = 32;
        State->AnimationSystem.Layers = PushArray(&State->Permanent, anim_layer, State->AnimationSystem.LayersMax);
        AddLayer(MakeString("Base Layer"), &State->AnimationSystem, BlendMode_Overwrite);
        AddLayer(MakeString("Color Layer"), &State->AnimationSystem, BlendMode_Multiply);
        AddLayer(MakeString("Sparkles"), &State->AnimationSystem, BlendMode_Add);
    } // End Animation Playground
    
    
    InitializePanelSystem(&State->PanelSystem);
    panel* Panel = TakeNewPanel(&State->PanelSystem);
    SetPanelDefinition(Panel, PanelType_SculptureView, State, Context);
}

internal void
HandleInput (app_state* State, rect2 WindowBounds, input_queue InputQueue, mouse_state Mouse, context Context)
{
    DEBUG_TRACK_FUNCTION;
    
    b32 PanelSystemHandledInput = HandleMousePanelInteraction(&State->PanelSystem, State->WindowBounds, Mouse, State);
    if (!PanelSystemHandledInput)
    {
        input_command_registry ActiveCommands = {};
        if (State->Modes.ActiveModesCount > 0)
        {
            ActiveCommands = State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1].Commands;
        }
        else
        {
            panel_and_bounds PanelWithMouseOverIt = GetPanelContainingPoint(Mouse.Pos, &State->PanelSystem, WindowBounds);
            if (!PanelWithMouseOverIt.Panel) { return; }
            State->HotPanel = PanelWithMouseOverIt.Panel;
            
            panel_definition PanelDefinition = GlobalPanelDefs[PanelWithMouseOverIt.Panel->PanelDefinitionIndex];
            if (!PanelDefinition.InputCommands) { return; }
            
            ActiveCommands.Commands = PanelDefinition.InputCommands;
            ActiveCommands.Size = sizeof(*PanelDefinition.InputCommands) / sizeof(PanelDefinition.InputCommands[0]);
            ActiveCommands.Used = ActiveCommands.Size;
        }
        
        for (s32 EventIdx = 0; EventIdx < InputQueue.QueueUsed; EventIdx++)
        {
            input_entry Event = InputQueue.Entries[EventIdx];
            
            // NOTE(Peter): These are in the order Down, Up, Held because we want to privalege
            // Down and Up over Held. In other words, we don't want to call a Held command on the
            // frame when the button was released, even if the command is registered to both events
            if (KeyTransitionedDown(Event))
            {
                FindAndPushExistingCommand(ActiveCommands, Event, Command_Began, &State->CommandQueue);
            }
            else if (KeyTransitionedUp(Event))
            {
                FindAndPushExistingCommand(ActiveCommands, Event, Command_Ended, &State->CommandQueue);
            }
            else if (KeyHeldDown(Event))
            {
                FindAndPushExistingCommand(ActiveCommands, Event, Command_Held, &State->CommandQueue);
            }
        }
    }
    
    // Execute all commands in CommandQueue
    for (s32 CommandIdx = State->CommandQueue.Used - 1; CommandIdx >= 0; CommandIdx--)
    {
        command_queue_entry* Entry = &State->CommandQueue.Commands[CommandIdx];
        Entry->Command.Proc(State, Entry->Event, Mouse, Context);
    }
    
    ClearCommandQueue(&State->CommandQueue);
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
    Context->Mouse.CursorType = CursorType_Arrow;
    
    PushRenderClearScreen(RenderBuffer);
    State->Camera.AspectRatio = RectAspectRatio(Context->WindowBounds);
    
    HandleInput(State, State->WindowBounds, InputQueue, Context->Mouse, *Context);
    
    if (State->AnimationSystem.TimelineShouldAdvance) {
        // TODO(Peter): Revisit this. This implies that the framerate of the animation system
        // is tied to the framerate of the simulation. That seems correct to me, but I'm not sure
        State->AnimationSystem.CurrentFrame += 1;
        // Loop back to the beginning
        if (State->AnimationSystem.CurrentFrame > State->AnimationSystem.PlayableRange.Max)
        {
            State->AnimationSystem.CurrentFrame = 0;
        }
    }
    
    s32 CurrentFrame = State->AnimationSystem.CurrentFrame;
    if (CurrentFrame != State->AnimationSystem.LastUpdatedFrame)
    {
        State->AnimationSystem.LastUpdatedFrame = CurrentFrame;
        r32 FrameTime = CurrentFrame * State->AnimationSystem.SecondsPerFrame;
        
        u32 CurrentBlocksMax = State->AnimationSystem.LayersCount;
        b8* CurrentBlocksFilled = PushArray(State->Transient, b8, CurrentBlocksMax);
        ZeroArray(CurrentBlocksFilled, b8, CurrentBlocksMax);
        animation_block* CurrentBlocks = PushArray(State->Transient, animation_block, CurrentBlocksMax);
        
        for (u32 i = 0; i < State->AnimationSystem.Blocks.Used; i++)
        {
            gs_list_entry<animation_block>* BlockEntry = State->AnimationSystem.Blocks.GetEntryAtIndex(i);
            if (EntryIsFree(BlockEntry)) { continue; }
            animation_block Block = BlockEntry->Value;
            if (CurrentFrame < Block.Range.Min || CurrentFrame > Block.Range.Max) { continue; }
            CurrentBlocksFilled[Block.Layer] = true;
            CurrentBlocks[Block.Layer] = Block;
        }
        
        led_buffer* LayerLEDBuffers = PushArray(State->Transient, led_buffer, CurrentBlocksMax);
        for (u32 AssemblyIndex = 0; AssemblyIndex < State->Assemblies.Count; AssemblyIndex++)
        {
            assembly* Assembly = &State->Assemblies.Values[AssemblyIndex];
            led_buffer* AssemblyLedBuffer = LedSystemGetBuffer(&State->LedSystem, Assembly->LedBufferIndex);
            
            for (u32 Layer = 0; Layer < CurrentBlocksMax; Layer++)
            {
                if (!CurrentBlocksFilled[Layer]) { continue; }
                animation_block Block = CurrentBlocks[Layer];
                
                // Prep Temp Buffer
                LayerLEDBuffers[Layer] = *AssemblyLedBuffer;
                LayerLEDBuffers[Layer].Colors = PushArray(State->Transient, pixel, AssemblyLedBuffer->LedCount);
                
                u32 FramesIntoBlock = CurrentFrame - Block.Range.Min;
                r32 SecondsIntoBlock = FramesIntoBlock * State->AnimationSystem.SecondsPerFrame;
                
                u32 AnimationProcIndex = Block.AnimationProcHandle - 1;
                animation_proc* AnimationProc = GlobalAnimationClips[AnimationProcIndex].Proc;
                AnimationProc(&LayerLEDBuffers[Layer], *Assembly, SecondsIntoBlock, State->Transient);
            }
            
            // Consolidate Temp Buffers
            // We do this in reverse order so that they go from top to bottom
            for (u32 Layer = 0; Layer < CurrentBlocksMax; Layer++)
            {
                if (!CurrentBlocksFilled[Layer]) { continue; }
                
                switch (State->AnimationSystem.Layers[Layer].BlendMode)
                {
                    case BlendMode_Overwrite:
                    {
                        for (u32 LED = 0; LED < AssemblyLedBuffer->LedCount; LED++)
                        {
                            AssemblyLedBuffer->Colors[LED] = LayerLEDBuffers[Layer].Colors[LED];
                        }
                    }break;
                    
                    case BlendMode_Add:
                    {
                        for (u32 LED = 0; LED < AssemblyLedBuffer->LedCount; LED++)
                        {
                            u32 R = (u32)AssemblyLedBuffer->Colors[LED].R + (u32)LayerLEDBuffers[Layer].Colors[LED].R;
                            u32 G = (u32)AssemblyLedBuffer->Colors[LED].G + (u32)LayerLEDBuffers[Layer].Colors[LED].G;
                            u32 B = (u32)AssemblyLedBuffer->Colors[LED].B + (u32)LayerLEDBuffers[Layer].Colors[LED].B;
                            
                            AssemblyLedBuffer->Colors[LED].R = (u8)Min(R, (u32)255);
                            AssemblyLedBuffer->Colors[LED].G = (u8)Min(G, (u32)255);
                            AssemblyLedBuffer->Colors[LED].B = (u8)Min(B, (u32)255);
                        }
                    }break;
                    
                    case BlendMode_Multiply:
                    {
                        for (u32 LED = 0; LED < AssemblyLedBuffer->LedCount; LED++)
                        {
                            r32 DR = (r32)AssemblyLedBuffer->Colors[LED].R / 255.f;
                            r32 DG = (r32)AssemblyLedBuffer->Colors[LED].G / 255.f;
                            r32 DB = (r32)AssemblyLedBuffer->Colors[LED].B / 255.f;
                            
                            r32 SR = (r32)LayerLEDBuffers[Layer].Colors[LED].R / 255.f;
                            r32 SG = (r32)LayerLEDBuffers[Layer].Colors[LED].G / 255.f;
                            r32 SB = (r32)LayerLEDBuffers[Layer].Colors[LED].B / 255.f;
                            
                            AssemblyLedBuffer->Colors[LED].R = (u8)((DR * SR) * 255.f);
                            AssemblyLedBuffer->Colors[LED].G = (u8)((DG * SG) * 255.f);
                            AssemblyLedBuffer->Colors[LED].B = (u8)((DB * SB) * 255.f);
                        }
                    }break;
                }
            }
        }
    }
    
    {
        // NOTE(pjs): Building data buffers to be sent out to the sculpture
        // This array is used on the platform side to actually send the information
        assembly_array SACNAssemblies = AssemblyArray_Filter(State->Assemblies, AssemblyFilter_OutputsViaSACN, State->Transient);
        assembly_array UARTAssemblies = AssemblyArray_Filter(State->Assemblies, AssemblyFilter_OutputsViaUART, State->Transient);
        SACN_BuildOutputData(&State->SACN, OutputData, SACNAssemblies, &State->LedSystem);
        UART_BuildOutputData(OutputData, UARTAssemblies, &State->LedSystem);
    }
    
    PushRenderOrthographic(RenderBuffer, State->WindowBounds);
    PushRenderClearScreen(RenderBuffer);
    
    State->WindowBounds = Context->WindowBounds;
    State->Interface.RenderBuffer = RenderBuffer;
    State->Interface.Mouse = Context->Mouse;
    
    panel_layout PanelsToRender = GetPanelLayout(&State->PanelSystem, State->WindowBounds, State->Transient);
    DrawAllPanels(PanelsToRender, RenderBuffer, &Context->Mouse, State, *Context);
    
    for (s32 m = 0; m < State->Modes.ActiveModesCount; m++)
    {
        operation_mode OperationMode = State->Modes.ActiveModes[m];
        if (OperationMode.Render != 0)
        {
            OperationMode.Render(State, RenderBuffer, OperationMode, Context->Mouse, *Context);
        }
    }
    
    Context->GeneralWorkQueue->CompleteQueueWork(Context->GeneralWorkQueue, Context->ThreadContext);
    Context->GeneralWorkQueue->ResetWorkQueue(Context->GeneralWorkQueue);
    
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