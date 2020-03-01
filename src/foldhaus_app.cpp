//
// File: foldhaus_app.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_APP_CPP

#include "foldhaus_platform.h"
#include "foldhaus_app.h"

internal v4
MouseToWorldRay(r32 MouseX, r32 MouseY, camera* Camera, rect WindowBounds)
{
    DEBUG_TRACK_SCOPE(MouseToWorldRay);
    r32 X = ((2.0f * MouseX) / Width(WindowBounds)) - 1;
    r32 Y = ((2.0f * MouseY) / Height(WindowBounds)) - 1;
    
    v4 ScreenPos = v4{X, Y, -1, 1};
    
    m44 InverseProjection = {};
    Inverse(GetCameraPerspectiveProjectionMatrix(*Camera), &InverseProjection);
    
    m44 InverseModelView = {};
    Inverse(GetCameraModelViewMatrix(*Camera), &InverseModelView);
    InverseModelView = Transpose(InverseModelView);
    
    v4 ClipSpacePos = InverseProjection * ScreenPos;
    v4 WorldPosition = InverseModelView * ClipSpacePos;
    return WorldPosition;
}

struct send_sacn_job_data
{
    
    platform_socket_handle SendSocket;
    platform_send_to* SendTo;
    dmx_buffer_list* DMXBuffers;
};

internal void
SACNSendDMXBufferListJob (s32 ThreadID, void* JobData)
{
    DEBUG_TRACK_FUNCTION;
    
    send_sacn_job_data* Data = (send_sacn_job_data*)JobData;
    platform_socket_handle SendSocket = Data->SendSocket;
    
    dmx_buffer_list* DMXBufferAt = Data->DMXBuffers;
    while (DMXBufferAt)
    {
        dmx_buffer Buffer = DMXBufferAt->Buffer;
        
        u32 V4SendAddress = SACNGetUniverseSendAddress(Buffer.Universe);
        
        Data->SendTo(SendSocket, V4SendAddress, DEFAULT_STREAMING_ACN_PORT, (const char*)Buffer.Base, Buffer.TotalSize, 0);
        
        DMXBufferAt = DMXBufferAt->Next;
    }
}

////////////////////////////////////////////////////////////////////////

RELOAD_STATIC_DATA(ReloadStaticData)
{
    app_state* State = (app_state*)Context.MemoryBase;
    
    GlobalDebugServices = DebugServices;
}

INITIALIZE_APPLICATION(InitializeApplication)
{
    app_state* State = (app_state*)Context.MemoryBase;
    State->Permanent = {};
    State->Permanent.Alloc = (gs_memory_alloc*)Context.PlatformAlloc;
    State->Permanent.Realloc = (gs_memory_realloc*)Context.PlatformRealloc;
    State->Transient = {};
    State->Transient.Alloc = (gs_memory_alloc*)Context.PlatformAlloc;
    State->Transient.Realloc = (gs_memory_realloc*)Context.PlatformRealloc;
    
    State->GlobalLog = PushStruct(&State->Transient, event_log);
    *State->GlobalLog = {0};
    
    s32 CommandQueueSize = 32;
    command_queue_entry* CommandQueueMemory = PushArray(&State->Permanent, 
                                                        command_queue_entry, 
                                                        CommandQueueSize);
    State->CommandQueue = InitializeCommandQueue(CommandQueueMemory, CommandQueueSize);
    
    State->ActiveTextEntry.Buffer = MakeString(PushArray(&State->Permanent, char, 256), 0, 256);
    
    // TODO(Peter): put in InitializeInterface?
    r32 FontSize = 14;
    {
        platform_memory_result FontFile = Context.PlatformReadEntireFile("Anonymous Pro.ttf");
        if (!FontFile.Error)
        {
            bitmap_font* Font = PushStruct(&State->Permanent, bitmap_font);
            
            Font->BitmapWidth = 512;
            Font->BitmapHeight = 512;
            Font->BitmapBytesPerPixel = 4;
            Font->BitmapMemory = PushArray(&State->Permanent, u8, Font->BitmapWidth * Font->BitmapHeight * Font->BitmapBytesPerPixel);
            Font->BitmapStride = Font->BitmapWidth * Font->BitmapBytesPerPixel;
            GSMemSet(Font->BitmapMemory, 0, Font->BitmapStride * Font->BitmapHeight);
            
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
            
            State->Interface.Font = Font;
            
            Font->BitmapTextureHandle = Context.PlatformGetGPUTextureHandle(Font->BitmapMemory, 
                                                                            Font->BitmapWidth, Font->BitmapHeight);
        } 
        else 
        {
            LogError(State->GlobalLog, "Unable to load font");
        }
    }
    
    State->Interface.FontSize = FontSize;
    State->Interface.PanelBGColors[0] = v4{.3f, .3f, .3f, 1};
    State->Interface.PanelBGColors[1] = v4{.4f, .4f, .4f, 1};
    State->Interface.PanelBGColors[2] = v4{.5f, .5f, .5f, 1};
    State->Interface.PanelBGColors[3] = v4{.6f, .6f, .6f, 1};
    State->Interface.ButtonColor_Inactive = BlackV4;
    State->Interface.ButtonColor_Active = v4{.1f, .1f, .1f, 1};
    State->Interface.ButtonColor_Selected = v4{.1f, .1f, .3f, 1};
    State->Interface.TextColor = WhiteV4;
    State->Interface.Margin = v2{5, 5};
    
    State->SACN = InitializeSACN(Context);
    State->NetworkProtocolHeaderSize = STREAM_HEADER_SIZE;
    
    State->Camera.FieldOfView = DegreesToRadians(45.0f);
    State->Camera.AspectRatio = AspectRatio(State->WindowBounds);
    State->Camera.Near = 1.0f;
    State->Camera.Far = 100.0f;
    State->Camera.Position = v3{0, 0, -250};
    State->Camera.LookAt = v3{0, 0, 0};
    
#if 1
    char Path[] = "blumen_lumen.fold";
    LoadAssembly(State, Context, Path);
#endif
    
    State->PixelsToWorldScale = .01f;
    
    GlobalDebugServices->Interface.RenderSculpture = true;
    
    ReloadStaticData(Context, GlobalDebugServices);
    
    // Setup Operation Modes
    State->Modes.ActiveModesCount = 0;
    State->Modes.Arena = {};
    State->Modes.Arena.Alloc = (gs_memory_alloc*)Context.PlatformAlloc;
    State->Modes.Arena.Realloc = (gs_memory_realloc*)Context.PlatformRealloc;
    State->Modes.Arena.FindAddressRule = FindAddress_InLastBufferOnly;
    
    { // Animation PLAYGROUND
        State->AnimationSystem = {};
        State->AnimationSystem.SecondsPerFrame = 1.f / 24.f;
        State->AnimationSystem.StartFrame = 0;
        State->AnimationSystem.EndFrame = SecondsToFrames(15, State->AnimationSystem);
    } // End Animation Playground
    
    
    InitializePanelSystem(&State->PanelSystem);
    panel* Panel = TakeNewPanel(&State->PanelSystem);
    SetPanelDefinition(Panel, 0, State);
}

internal void
HandleInput (app_state* State, rect WindowBounds, input_queue InputQueue, mouse_state Mouse)
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
        Entry->Command.Proc(State, Entry->Event, Mouse);
    }
    
    ClearCommandQueue(&State->CommandQueue);
}

internal dmx_buffer_list*
CreateDMXBuffers(assembly Assembly, s32 BufferHeaderSize, memory_arena* Arena)
{
    DEBUG_TRACK_FUNCTION;
    
    dmx_buffer_list* Result = 0;
    dmx_buffer_list* Head = 0;
    
    s32 BufferSize = BufferHeaderSize + 512;
    
    for (u32 Range = 0; Range < Assembly.LEDUniverseMapCount; Range++)
    {
        leds_in_universe_range LEDUniverseRange = Assembly.LEDUniverseMap[Range];
        
        dmx_buffer_list* NewBuffer = PushStruct(Arena, dmx_buffer_list);
        NewBuffer->Buffer.Universe = LEDUniverseRange.Universe;
        NewBuffer->Buffer.Base = PushArray(Arena, u8, BufferSize);
        NewBuffer->Buffer.TotalSize = BufferSize;
        NewBuffer->Buffer.HeaderSize = BufferHeaderSize;
        NewBuffer->Next = 0;
        
        // Append
        if (!Result) { 
            Result = NewBuffer; 
            Head = Result;
        }
        Head->Next = NewBuffer;
        Head = NewBuffer;
        
        u8* DestChannel = Head->Buffer.Base + BufferHeaderSize;
        for (s32 LEDIdx = LEDUniverseRange.RangeStart;
             LEDIdx < LEDUniverseRange.RangeOnePastLast;
             LEDIdx++)
        {
            led LED = Assembly.LEDs[LEDIdx];
            pixel Color = Assembly.Colors[LED.Index];
            
            
            DestChannel[0] = Color.R;
            DestChannel[1] = Color.G;
            DestChannel[2] = Color.B;
            DestChannel += 3;
        }
    }
    
    return Result;
}

UPDATE_AND_RENDER(UpdateAndRender)
{
    DEBUG_TRACK_FUNCTION;
    app_state* State = (app_state*)Context.MemoryBase;
    State->WindowBounds = Context.WindowBounds;
    
    // NOTE(Peter): We do this at the beginning because all the render commands are stored in Transient,
    // and need to persist beyond the end of the UpdateAndRender call. In the release version, we won't
    // zero the Transient arena when we clear it so it wouldn't be a problem, but it is technically 
    // incorrect to clear the arena, and then access the memory later.
    ClearArena(&State->Transient);
    
    HandleInput(State, State->WindowBounds, InputQueue, Mouse);
    
    if (State->AnimationSystem.TimelineShouldAdvance) { 
        // TODO(Peter): Revisit this. This implies that the framerate of the animation system
        // is tied to the framerate of the simulation. That seems correct to me, but I'm not sure
        State->AnimationSystem.CurrentFrame += 1; 
        // Loop back to the beginning
        if (State->AnimationSystem.CurrentFrame > State->AnimationSystem.EndFrame)
        {
            State->AnimationSystem.CurrentFrame = 0; 
        }
    }
    
    u32 CurrentFrame = State->AnimationSystem.CurrentFrame;
    if (CurrentFrame != State->AnimationSystem.LastUpdatedFrame)
    {
        State->AnimationSystem.LastUpdatedFrame = CurrentFrame;
        r32 FrameTime = CurrentFrame * State->AnimationSystem.SecondsPerFrame;
        
        for (u32 i = 0; i < State->AnimationSystem.Blocks.Used; i++)
        {
            gs_list_entry<animation_block>* BlockEntry = State->AnimationSystem.Blocks.GetEntryAtIndex(i);
            if (!EntryIsFree(BlockEntry))
            {
                animation_block Block = BlockEntry->Value;
                if (CurrentFrame >= Block.StartFrame && CurrentFrame <= Block.EndFrame)
                {
                    for (u32 j = 0; j < State->ActiveAssemblyIndecies.Used; j++)
                    {
                        gs_list_handle AssemblyHandle = *State->ActiveAssemblyIndecies.GetElementAtIndex(j);
                        assembly* Assembly = State->AssemblyList.GetElementWithHandle(AssemblyHandle);
                        
                        u32 FramesIntoBlock = CurrentFrame - Block.StartFrame;
                        r32 SecondsIntoBlock = FramesIntoBlock * State->AnimationSystem.SecondsPerFrame;
                        // TODO(Peter): Temporary
                        switch(Block.AnimationProcHandle)
                        {
                            case 1: 
                            {
                                TestPatternOne(Assembly, SecondsIntoBlock); 
                            }break;
                            
                            case 2:
                            {
                                TestPatternTwo(Assembly, SecondsIntoBlock);
                            }break;
                            
                            case 3:
                            {
                                TestPatternThree(Assembly, SecondsIntoBlock);
                            }break;
                            
                            // NOTE(Peter): Zero is invalid
                            InvalidDefaultCase;
                        }
                    }
                }
            }
        }
    }
    
    s32 HeaderSize = State->NetworkProtocolHeaderSize;
    dmx_buffer_list* DMXBuffers = 0;
    for (u32 i = 0; i < State->ActiveAssemblyIndecies.Used; i++)
    {
        gs_list_handle AssemblyHandle = *State->ActiveAssemblyIndecies.GetElementAtIndex(i);
        assembly* Assembly = State->AssemblyList.GetElementWithHandle(AssemblyHandle);
        dmx_buffer_list* NewDMXBuffers = CreateDMXBuffers(*Assembly, HeaderSize, &State->Transient);
        DMXBuffers = DMXBufferListAppend(DMXBuffers, NewDMXBuffers);
    }
    
    //DEBUG_IF(GlobalDebugServices->Interface.SendSACNData)
    {
        switch (State->NetworkProtocol)
        {
            case NetworkProtocol_SACN:
            {
                SACNUpdateSequence(&State->SACN);
                
                dmx_buffer_list* CurrentDMXBuffer = DMXBuffers;
                while (CurrentDMXBuffer)
                {
                    dmx_buffer Buffer = CurrentDMXBuffer->Buffer;
                    SACNPrepareBufferHeader(Buffer.Universe, Buffer.Base, Buffer.TotalSize, Buffer.HeaderSize, State->SACN);
                    CurrentDMXBuffer = CurrentDMXBuffer->Next;
                }
                
                send_sacn_job_data* Job = PushStruct(&State->Transient, send_sacn_job_data);
                Job->SendSocket = State->SACN.SendSocket;
                Job->SendTo = Context.PlatformSendTo;
                Job->DMXBuffers = DMXBuffers;
                
                Context.GeneralWorkQueue->PushWorkOnQueue(Context.GeneralWorkQueue, SACNSendDMXBufferListJob, Job, "SACN Send Data Job");
            }break;
            
            InvalidDefaultCase;
        }
    }
    
    PushRenderOrthographic(RenderBuffer, 0, 0, Width(State->WindowBounds), Height(State->WindowBounds));
    PushRenderClearScreen(RenderBuffer);
    
    panel_layout PanelsToRender = GetPanelLayout(&State->PanelSystem, State->WindowBounds, &State->Transient);
    DrawAllPanels(PanelsToRender, RenderBuffer, Mouse, State, Context);
    
    for (s32 m = 0; m < State->Modes.ActiveModesCount; m++)
    {
        operation_mode OperationMode = State->Modes.ActiveModes[m];
        if (OperationMode.Render != 0)
        {
            OperationMode.Render(State, RenderBuffer, OperationMode, Mouse);
        }
    }
    
    Context.GeneralWorkQueue->DoQueueWorkUntilDone(Context.GeneralWorkQueue, 0);
    Context.GeneralWorkQueue->ResetWorkQueue(Context.GeneralWorkQueue);
    
    // Checking for overflows
    {
        DEBUG_TRACK_SCOPE(OverflowChecks);
        AssertAllocationsNoOverflow(State->Permanent);
        for (u32 i = 0; i < State->ActiveAssemblyIndecies.Used; i++)
        {
            gs_list_handle AssemblyHandle = *State->ActiveAssemblyIndecies.GetElementAtIndex(i);
            assembly* Assembly = State->AssemblyList.GetElementWithHandle(AssemblyHandle);
            AssertAllocationsNoOverflow(Assembly->Arena);
        }
    }
}

CLEANUP_APPLICATION(CleanupApplication)
{
    app_state* State = (app_state*)Context.MemoryBase;
    SACNCleanup(&State->SACN, Context);
}

#define FOLDHAUS_APP_CPP
#endif // FOLDHAUS_APP_CPP