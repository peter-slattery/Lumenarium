#include "foldhaus_platform.h"
#include "foldhaus_app.h"

internal v4
MouseToWorldRay(r32 MouseX, r32 MouseY, camera* Camera, r32 WindowWidth, r32 WindowHeight)
{
    DEBUG_TRACK_SCOPE(MouseToWorldRay);
    r32 X = ((2.0f * MouseX) / WindowWidth) - 1;
    r32 Y = ((2.0f * MouseY) / WindowHeight) - 1;
    
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

internal void 
PushLEDBufferOnList (led_buffer* List, led_buffer* Entry)
{
    if (List->Next)
    {
        PushLEDBufferOnList(List->Next, Entry);
    }
    else
    {
        List->Next = Entry;
    }
}

internal led_buffer* 
RemoveLEDBufferFromList (led_buffer* List, led_buffer* Entry)
{
    led_buffer* ListHead = 0;
    if (List != Entry && List->Next)
    {
        ListHead = RemoveLEDBufferFromList(List->Next, Entry);
    }
    else if (List == Entry)
    {
        ListHead = Entry->Next;
    }
    else
    {
        // NOTE(Peter): Trying to remove an entry from a list that doesn't contain it
        InvalidCodePath;
    }
    return ListHead;
}

internal void
ConstructAssemblyFromDefinition (assembly_definition Definition,
                                 string AssemblyName,
                                 v3 RootPosition,
                                 r32 Scale,
                                 context Context,
                                 app_state* State)
{
    Assert(State->AssembliesUsed < ASSEMBLY_LIST_LENGTH);
    
    assembly* Assembly = State->AssemblyList + State->AssembliesUsed++;
    
    // 1. Find # of LEDs, # of Universes
    s32 UniversesUsedByLEDs[2048]; // TODO(Peter): find the max universe number and size these accordingly
    s32 ChannelsInUniverse[2048];
    GSZeroMemory(UniversesUsedByLEDs, sizeof(s32) * 2048);
    GSZeroMemory(ChannelsInUniverse, sizeof(s32) * 2048);
    s32 UniverseCount = 0;
    s32 LEDCount = 0;
    
    for (s32 StripIdx = 0; StripIdx < Definition.LEDStripCount; StripIdx++)
    {
        led_strip_definition StripDef = Definition.LEDStrips[StripIdx];
        
        s32 ChannelsPerStrip = StripDef.LEDsPerStrip * 3;
        s32 UniversesPerStrip = IntegerDivideRoundUp(ChannelsPerStrip, 512);
        s32 ChannelsAllocated = 0;
        for (s32 UniverseIdx = 0; UniverseIdx < UniversesPerStrip; UniverseIdx++)
        {
            s32 UniverseID = StripDef.StartUniverse + UniverseIdx;
            s32 UniverseIndex = -1;
            
            for (s32 RegisteredUniverse = 0; RegisteredUniverse < UniverseCount; RegisteredUniverse++)
            {
                if (UniversesUsedByLEDs[RegisteredUniverse] == UniverseID)
                {
                    UniverseIndex = RegisteredUniverse;
                    break;
                }
            }
            if (UniverseIndex < 0)
            {
                UniverseIndex = UniverseCount++;
            }
            
            s32 ChannelsRequested = GSMin(STREAM_BODY_SIZE, ChannelsPerStrip - ChannelsAllocated);
            ChannelsAllocated += ChannelsRequested;
            ChannelsInUniverse[UniverseIndex] += ChannelsRequested;
            Assert(ChannelsInUniverse[UniverseIndex] <= 512);
            
            UniversesUsedByLEDs[UniverseIndex++] = UniverseID;
        }
        
        LEDCount += StripDef.LEDsPerStrip;
    }
    
    sacn_add_universes_result AddedUniverses = SACNAddUniverses(UniversesUsedByLEDs, UniverseCount, &State->SACN, Context);
    
    Assembly->MemorySize = CalculateMemorySizeForAssembly(LEDCount, AssemblyName.Length);
    memory_arena TemporaryAssemblyArena = AllocateNonGrowableArenaWithSpace(Context.PlatformAlloc, Assembly->MemorySize);
    Assembly->MemoryBase = TemporaryAssemblyArena.CurrentRegion->Base;
    
    Assembly->Universes = AddedUniverses.NewUniverseBuffer;
    Assembly->SendBuffer = AddedUniverses.NewSendBuffer;
    
    Assembly->Name = MakeString(PushArray(&TemporaryAssemblyArena, char, AssemblyName.Length), AssemblyName.Length);
    CopyStringTo(AssemblyName, &Assembly->Name);
    
    led_buffer* LEDBuffer = PushStruct(&TemporaryAssemblyArena, led_buffer);
    LEDBuffer->Next = 0;
    LEDBuffer->Count = 0;
    LEDBuffer->Max = LEDCount;
    LEDBuffer->LEDs = PushArray(&TemporaryAssemblyArena, led, LEDCount);
    LEDBuffer->Colors = PushArray(&TemporaryAssemblyArena, sacn_pixel, LEDCount);
    
    Assembly->LEDBuffer = LEDBuffer;
    
    if (State->LEDBufferList)
    {
        PushLEDBufferOnList(State->LEDBufferList, LEDBuffer);
    }
    else
    {
        State->LEDBufferList = LEDBuffer;
    }
    State->TotalLEDsCount += LEDCount;
    
    // Add LEDs
    for (s32 StripIdx = 0; StripIdx < Definition.LEDStripCount; StripIdx++)
    {
        led_strip_definition StripDef = Definition.LEDStrips[StripIdx];
        
        v3 WS_StripStart = {};
        v3 WS_StripEnd = {};
        s32 LEDsInStripCount = 0;
        
        switch(StripDef.InterpolationType)
        {
            case StripInterpolate_Points:
            {
                WS_StripStart= StripDef.InterpolatePositionStart * Scale;
                WS_StripEnd= StripDef.InterpolatePositionEnd * Scale;
                LEDsInStripCount = StripDef.LEDsPerStrip;
                
            }break;
            
            default:
            {
                InvalidCodePath;
            }break;
        }
        
        sacn_universe* CurrentUniverse = SACNGetUniverse(StripDef.StartUniverse, &State->SACN);
        s32 ChannelsUsed = 0;
        CurrentUniverse->BeginPixelCopyFromOffset = LEDBuffer->Count * sizeof(sacn_pixel);
        
        r32 Percent = 0;
        r32 PercentStep = 1 / (r32)LEDsInStripCount;
        for (s32 Step = 0; Step < LEDsInStripCount; Step++)
        {
            v3 LEDPosition = Lerp(WS_StripStart, WS_StripEnd, Percent);
            s32 LEDIndex = LEDBuffer->Count++;
            Assert(LEDIndex < LEDCount);
            
            led* LED = LEDBuffer->LEDs + LEDIndex;
            sacn_pixel* LEDColor = LEDBuffer->Colors + LEDIndex;
            
            LED->Position = LEDPosition;
            LED->PositionMatrix = GetPositionM44(V4(LED->Position, 1));
            LED->Index = LEDIndex; 
            
            Percent += PercentStep;
            
            ChannelsUsed += 3;
            if (ChannelsUsed > STREAM_BODY_SIZE)
            {
                ChannelsUsed -= STREAM_BODY_SIZE;
                CurrentUniverse = SACNGetUniverse(CurrentUniverse->Universe + 1, &State->SACN);
                CurrentUniverse->BeginPixelCopyFromOffset = (LEDBuffer->Count + sizeof(sacn_pixel)) - ChannelsUsed;
            }
        }
    }
}

struct draw_leds_job_data
{
    led* LEDs;
    sacn_pixel* Colors;
    s32 StartIndex;
    s32 OnePastLastIndex;
    
    render_quad_batch_constructor* Batch;
    
    m44 FaceCameraMatrix;
    m44 ModelViewMatrix;
    r32 LEDHalfWidth;
};

internal void
DrawLEDsInBufferRangeJob (s32 ThreadID, void* JobData)
{
    DEBUG_TRACK_FUNCTION;
    
    draw_leds_job_data* Data = (draw_leds_job_data*)JobData;
    
    s32 DrawCommandsCount = Data->OnePastLastIndex - Data->StartIndex;
    
    r32 HalfWidth = Data->LEDHalfWidth;
    
    v4 P0_In = v4{-HalfWidth, -HalfWidth, 0, 1};
    v4 P1_In = v4{HalfWidth, -HalfWidth, 0, 1};
    v4 P2_In = v4{HalfWidth, HalfWidth, 0, 1};
    v4 P3_In = v4{-HalfWidth, HalfWidth, 0, 1};
    
    v2 UV0 = v2{0, 0};
    v2 UV1 = v2{1, 0};
    v2 UV2 = v2{1, 1};
    v2 UV3 = v2{0, 1};
    
    led* LED = Data->LEDs + Data->StartIndex;
    for (s32 LEDIdx = 0;
         LEDIdx < DrawCommandsCount;
         LEDIdx++)
    {
        sacn_pixel SACNColor = Data->Colors[LED->Index];
        v4 Color = v4{SACNColor.R / 255.f, SACNColor.G / 255.f, SACNColor.B / 255.f, 1.0f};
        
        m44 ModelMatrix = Data->FaceCameraMatrix * LED->PositionMatrix;// * Data->FaceCameraMatrix;
        
        v4 P0 = ModelMatrix * P0_In;
        v4 P1 = ModelMatrix * P1_In;
        v4 P2 = ModelMatrix * P2_In;
        v4 P3 = ModelMatrix * P3_In;
        
        PushTri3DOnBatch(Data->Batch, P0, P1, P2, UV0, UV1, UV2, Color, Color, Color);
        PushTri3DOnBatch(Data->Batch, P0, P2, P3, UV0, UV2, UV3, Color, Color, Color);
        
        LED++;
    }
}

struct send_sacn_job_data
{
    streaming_acn SACN;
    sacn_universe_buffer UniverseList;
    s32 StartUniverse;
    s32 OnePastLastUniverse;
    platform_send_to* PlatformSendTo;
};

internal void
SendSACNBufferData (s32 ThreadID, void* JobData)
{
    DEBUG_TRACK_FUNCTION;
    
    send_sacn_job_data* Data = (send_sacn_job_data*)JobData;
    
    sacn_universe* SendUniverse = Data->UniverseList.Universes + Data->StartUniverse;
    for (s32 UniverseIdx = Data->StartUniverse; UniverseIdx < Data->OnePastLastUniverse; UniverseIdx++)
    {
        SACNSendDataToUniverse(&Data->SACN, SendUniverse, Data->PlatformSendTo);
        SendUniverse++;
    }
}

internal void
LoadAssembly (app_state* State, context Context, char* Path)
{
    assembly_definition AssemblyDefinition = {};
    
    arena_snapshot TempMemorySnapshot = TakeSnapshotOfArena(*State->Transient);
    
    platform_memory_result TestAssemblyFile = Context.PlatformReadEntireFile(Path);
    Assert(TestAssemblyFile.Size > 0);
    {
        tokenizer AssemblyFileTokenizer = {};
        AssemblyFileTokenizer.At = (char*)TestAssemblyFile.Base;
        AssemblyFileTokenizer.Memory = (char*)TestAssemblyFile.Base;
        AssemblyFileTokenizer.MemoryLength = TestAssemblyFile.Size;
        
        ParseAssemblyFileHeader(&AssemblyDefinition, &AssemblyFileTokenizer);
        
        AssemblyDefinition.LEDStrips = PushArray(State->Transient, led_strip_definition, AssemblyDefinition.LEDStripSize);
        
        ParseAssemblyFileBody(&AssemblyDefinition, &AssemblyFileTokenizer);
    }
    Context.PlatformFree(TestAssemblyFile.Base, TestAssemblyFile.Size);
    
    string PathString = MakeStringLiteral(Path);
    s32 IndexOfLastSlash = FastLastIndexOfCharInCharArray(PathString.Memory, PathString.Length, '\\');
    string FileName = Substring(PathString, IndexOfLastSlash + 1);
    
    r32 Scale = 100;
    ConstructAssemblyFromDefinition(AssemblyDefinition, FileName, v3{0, 0, 0}, Scale, Context, State);
    
    ClearArenaToSnapshot(State->Transient, TempMemorySnapshot);
}

internal void
UnloadAssembly (s32 AssemblyIndex, app_state* State, context Context)
{
    assembly Assembly = State->AssemblyList[AssemblyIndex];
    SACNRemoveUniverseAndSendBuffer(&State->SACN, Assembly.Universes, Assembly.SendBuffer);
    
    State->LEDBufferList = RemoveLEDBufferFromList(State->LEDBufferList, Assembly.LEDBuffer);
    
    s32 LEDsInAssembly = Assembly.LEDBuffer->Count;
    s32 MemoryRequiredForAssembly = CalculateMemorySizeForAssembly(LEDsInAssembly, Assembly.Name.Length);
    Context.PlatformFree((u8*)Assembly.LEDBuffer, MemoryRequiredForAssembly);
    
    State->TotalLEDsCount -= LEDsInAssembly;
    
    if (AssemblyIndex != (State->AssembliesUsed - 1))
    {
        State->AssemblyList[AssemblyIndex] = State->AssemblyList[State->AssembliesUsed - 1];
    }
    State->AssembliesUsed -= 1;
}

////////////////////////////////////////////////////////////////////////

internal void
RegisterTextEntryCommands (input_command_registry* CommandRegistry)
{
    
}

RELOAD_STATIC_DATA(ReloadStaticData)
{
    app_state* State = (app_state*)Context.MemoryBase;
    
    GlobalDebugServices = DebugServices;
    
    if (State->InputCommandRegistry.Size > 0)
    {
        RegisterKeyPressCommand(&State->InputCommandRegistry, KeyCode_MouseLeftButton, true, KeyCode_Invalid,
                                CameraMouseControl);
        RegisterKeyPressCommand(&State->InputCommandRegistry, KeyCode_U, false, KeyCode_Invalid, OpenUniverseView);
        RegisterKeyPressCommand(&State->InputCommandRegistry, KeyCode_A, false, KeyCode_Invalid, OpenNodeLister);
        RegisterKeyPressCommand(&State->InputCommandRegistry, KeyCode_Tab, false, KeyCode_Invalid, ToggleNodeDisplay);
        
        
        // Node Lister
        RegisterKeyPressCommand(&State->NodeListerCommandRegistry, KeyCode_DownArrow, false, KeyCode_Invalid, NodeListerNextItem);
        RegisterKeyPressCommand(&State->NodeListerCommandRegistry, KeyCode_UpArrow, false, KeyCode_Invalid, NodeListerPrevItem);
        RegisterKeyPressCommand(&State->NodeListerCommandRegistry, KeyCode_Enter, false, KeyCode_Invalid, SelectAndCloseNodeLister);
        RegisterKeyPressCommand(&State->NodeListerCommandRegistry, KeyCode_MouseLeftButton, false, KeyCode_Invalid, CloseNodeLister);
        RegisterKeyPressCommand(&State->NodeListerCommandRegistry, KeyCode_Esc, false, KeyCode_Invalid, CloseNodeLister);
        InitializeTextInputCommands(&State->NodeListerCommandRegistry, State->Permanent);
    }
}

INITIALIZE_APPLICATION(InitializeApplication)
{
    app_state* State = (app_state*)Context.MemoryBase;
    u8* MemoryCursor = Context.MemoryBase + sizeof(app_state);
    s32 PermanentStorageSize = Megabytes(32);
    s32 TransientStorageSize = Context.MemorySize - PermanentStorageSize;
    State->Permanent = BootstrapArenaIntoMemory(MemoryCursor, PermanentStorageSize);
    State->Transient = BootstrapArenaIntoMemory(MemoryCursor + PermanentStorageSize, TransientStorageSize);
    
    InitMemoryArena(&State->SACNMemory, 0, 0, Context.PlatformAlloc);
    
    InitializeInputCommandRegistry(&State->InputCommandRegistry, 32, State->Permanent);
    InitializeInputCommandRegistry(&State->NodeListerCommandRegistry, 128, State->Permanent);
    State->ActiveCommands = &State->InputCommandRegistry;
    
    s32 CommandQueueSize = 32;
    command_queue_entry* CommandQueueMemory = PushArray(State->Permanent, 
                                                        command_queue_entry, 
                                                        CommandQueueSize);
    State->CommandQueue = InitializeCommandQueue(CommandQueueMemory, CommandQueueSize);
    
    State->ActiveTextEntry.Buffer = MakeString(PushArray(State->Permanent, char, 256), 0, 256);
    
    // TODO(Peter): put in InitializeInterface?
    r32 FontSize = 14;
    {
        platform_memory_result FontFile = Context.PlatformReadEntireFile("Anonymous Pro.ttf");
        if (FontFile.Size)
        {
            bitmap_font* Font = PushStruct(State->Permanent, bitmap_font);
            
            Font->BitmapWidth = 512;
            Font->BitmapHeight = 512;
            Font->BitmapBytesPerPixel = 4;
            Font->BitmapMemory = PushArray(State->Permanent, u8, Font->BitmapWidth * Font->BitmapHeight * Font->BitmapBytesPerPixel);
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
            Font->CodepointKeys = PushArray(State->Permanent, char, Font->CodepointDictionarySize);
            Font->CodepointValues = PushArray(State->Permanent, codepoint_bitmap, Font->CodepointDictionarySize);
            
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
            State->Font = Font;
            
            Font->BitmapTextureHandle = Context.PlatformGetGPUTextureHandle(Font->BitmapMemory, 
                                                                            Font->BitmapWidth, Font->BitmapHeight);
            
        } else {}
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
    
    State->SACN = InitializeSACN(Context.PlatformAlloc, Context);
    
    State->Camera.FieldOfView = DegreesToRadians(45.0f);
    State->Camera.AspectRatio = (r32)Context.WindowWidth / (r32)Context.WindowHeight;
    State->Camera.Near = 1.0f;
    State->Camera.Far = 100.0f;
    State->Camera.Position = v3{0, 0, -250};
    State->Camera.LookAt = v3{0, 0, 0};
    State->Camera_StartDragPos = V4(State->Camera.Position, 1);
    
#if 1
    char Path[] = "radialumia.fold";
    LoadAssembly(State, Context, Path);
#endif
    
    State->PixelsToWorldScale = .01f;
    
    GlobalDebugServices->Interface.RenderSculpture = true;
    
    State->NodeList = AllocateNodeList(State->Permanent, Kilobytes(64));
    
    State->NodeInteraction = NewEmptyNodeInteraction(); 
    State->NodeRenderSettings.PortDim = v2{20, 15};
    State->NodeRenderSettings.PortStep = State->NodeRenderSettings.PortDim.y + 10;
    State->NodeRenderSettings.PortColors[MemberType_r32] = RedV4;
    State->NodeRenderSettings.PortColors[MemberType_s32] = GreenV4;
    State->NodeRenderSettings.PortColors[MemberType_v4] = BlueV4;
    State->NodeRenderSettings.Font = State->Font;
    
    State->OutputNode = PushOutputNodeOnList(State->NodeList, v2{500, 250}, State->Permanent);
    
    InitializeEmptyString(&State->GeneralPurposeSearchString, PushArray(State->Permanent, char, 256), 256);
    
    ReloadStaticData(Context, GlobalDebugServices);
    
    { // MODES PLAYGROUND
        State->Modes.ActiveModesCount = 0;
        
        s32 ModesMemorySize = Kilobytes(32);
        u8* ModesMemory = PushSize(State->Permanent, ModesMemorySize);
        InitMemoryArena(&State->Modes.Arena, ModesMemory, ModesMemorySize, 0);
    }
}

UPDATE_AND_RENDER(UpdateAndRender)
{
    app_state* State = (app_state*)Context.MemoryBase;
    
    // NOTE(Peter): We do this at the beginning because all the render commands are stored in Transient,
    // and need to persist beyond the end of the UpdateAndRender call. In the release version, we won't
    // zero the Transient arena when we clear it so it wouldn't be a problem, but it is technically 
    // incorrect to clear the arena, and then access the memory later.
    ClearArena(State->Transient);
    
    // NOTE(Peter): Input Handling
    gui_mouse GuiMouse = {};
    GuiMouse.Pos = Mouse.Pos;
    GuiMouse.OldPos = Mouse.OldPos;
    GuiMouse.DeltaPos = Mouse.DeltaPos;
    GuiMouse.DownPos = Mouse.DownPos;
    
    {
        input_command_registry* ActiveCommands = State->ActiveCommands;
        if (State->Modes.ActiveModesCount > 0)
        {
            ActiveCommands = &State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1].Commands;
        }
        
        ActivateQueuedCommandRegistry(State);
        
        // CommandQueue holds the list of commands, generated from the current InputCommandRegistry
        // Every command entry in the queue should be executed.
        // For every Input Event, attempt to add an entry to the CommandQueue if an appropriate command
        // exists in ActiveCommands
        RemoveNonPersistantCommandsFromQueueAndUpdatePersistentEvents(&State->CommandQueue);
        
        for (s32 EventIdx = 0; EventIdx < InputQueue.QueueUsed; EventIdx++)
        {
            input_entry Event = InputQueue.Entries[EventIdx];
            
            input_command* Command = FindExistingCommand(ActiveCommands, Event.Key, (key_code)0);
            if (Command)
            {
                if (KeyTransitionedDown(Event))
                {
                    PushCommandOnQueue(&State->CommandQueue, *Command, Event);
                }
                else if (Command->PersistsUntilReleased && KeyTransitionedUp(Event))
                {
                    RemoveCommandFromQueue(&State->CommandQueue, *Command, Event);
                }
            }
            
            if (Event.Key == KeyCode_MouseLeftButton)
            {
                GuiMouse.LeftButtonTransitionedDown = KeyTransitionedDown(Event);
                GuiMouse.LeftButtonTransitionedUp = KeyTransitionedUp(Event);
            }
            else if (Event.Key == KeyCode_MouseMiddleButton)
            {
                GuiMouse.MiddleButtonTransitionedDown = KeyTransitionedDown(Event);
                GuiMouse.MiddleButtonTransitionedUp = KeyTransitionedUp(Event);
            }
            else if (Event.Key == KeyCode_MouseRightButton)
            {
                GuiMouse.RightButtonTransitionedDown = KeyTransitionedDown(Event);
                GuiMouse.RightButtonTransitionedUp = KeyTransitionedUp(Event);
            }
        }
        
        // Execute all commands in CommandQueue
        for (s32 CommandIdx = 0; CommandIdx < State->CommandQueue.Used; CommandIdx++)
        {
            command_queue_entry Entry = State->CommandQueue.Commands[CommandIdx];
            Entry.Command.Proc(State, Entry.Event, Mouse);
        }
    }
    
    if (State->LEDBufferList)
    {
        UpdateOutputNodeCalculations(State->OutputNode, State->NodeList, 
                                     State->Permanent, State->Transient, 
                                     State->LEDBufferList->LEDs,
                                     State->LEDBufferList->Colors, 
                                     State->LEDBufferList->Count, 
                                     Context.DeltaTime);
    }
    ClearTransientNodeColorBuffers(State->NodeList);
    
    {
        // NOTE(Peter): We know that these two lists should be maintained together. Each element in the list is one sculpture's worth of
        // information, and should always be evaluated in pairs.
        sacn_universe_buffer* UniverseList = State->SACN.UniverseBuffer;
        led_buffer* LEDBuffer = State->LEDBufferList;
        while (UniverseList && LEDBuffer)
        {
            for (s32 U = 0; U < UniverseList->Used; U++)
            {
                sacn_universe* UniverseOne = UniverseList->Universes + U;
                Assert(UniverseOne->BeginPixelCopyFromOffset >= 0);
                
                u8* LEDColorBuffer = (u8*)LEDBuffer->Colors + UniverseOne->BeginPixelCopyFromOffset;
                u8* SACNSendBuffer = UniverseOne->StartPositionInSendBuffer + STREAM_HEADER_SIZE;
                
                GSMemCopy(LEDColorBuffer, SACNSendBuffer, STREAM_BODY_SIZE);
            }
            UniverseList = UniverseList->Next;
            LEDBuffer = LEDBuffer->Next;
        }
        Assert(!LEDBuffer && !UniverseList);
    }
    
    DEBUG_IF(GlobalDebugServices->Interface.SendSACNData)
    {
        if (++State->SACN.SequenceIterator == 0) // Never use 0 after the first one
        {
            ++State->SACN.SequenceIterator;
        }
        
        
        sacn_universe_buffer* UniverseList = State->SACN.UniverseBuffer;
        while (UniverseList)
        {
            s32 JobCount = 2;
            s32 UniversesPerJob = UniverseList->Used / JobCount;
            send_sacn_job_data* SACNData = PushArray(State->Transient, send_sacn_job_data, JobCount);
            for (s32 i = 0; i < JobCount; i++)
            {
                SACNData[i].SACN = State->SACN;
                SACNData[i].UniverseList = *UniverseList;
                SACNData[i].StartUniverse = i * UniversesPerJob;
                SACNData[i].OnePastLastUniverse = (i * UniversesPerJob) + UniversesPerJob;
                if (SACNData[i].OnePastLastUniverse > UniverseList->Used)
                {
                    SACNData[i].OnePastLastUniverse = UniverseList->Used;
                }
                Context.GeneralWorkQueue->PushWorkOnQueue(
                    Context.GeneralWorkQueue,
                    SendSACNBufferData,
                    SACNData + i);
            }
            UniverseList = UniverseList->Next;
        }
    }
    
    ////////////////////////////////
    //   Render Assembly
    ///////////////////////////////
    if (Context.WindowIsVisible)
    {
        State->Camera.AspectRatio = (r32)Context.WindowWidth / (r32)Context.WindowHeight;
        
        m44 ModelViewMatrix = GetCameraModelViewMatrix(State->Camera);
        m44 ProjectionMatrix = GetCameraPerspectiveProjectionMatrix(State->Camera);
        
        r32 LEDHalfWidth = .5f;
        
        PushRenderPerspective(RenderBuffer, 0, 0, Context.WindowWidth, Context.WindowHeight, State->Camera);
        PushRenderClearScreen(RenderBuffer);
        
        DEBUG_IF(GlobalDebugServices->Interface.RenderSculpture) // DebugServices RenderSculpture Toggle
        {
            s32 JobsNeeded = IntegerDivideRoundUp(State->TotalLEDsCount, LED_BUFFER_SIZE);
            
            draw_leds_job_data* JobDataBank = PushArray(State->Transient, draw_leds_job_data, JobsNeeded);
            s32 JobDataBankUsed = 0;
            
            // TODO(Peter): Pretty sure this isn't working right now
            m44 FaceCameraMatrix = GetLookAtMatrix(v4{0, 0, 0, 1}, V4(State->Camera.Position, 1));
            FaceCameraMatrix = FaceCameraMatrix;
            
            render_quad_batch_constructor BatchConstructor = PushRenderQuad3DBatch(RenderBuffer, State->TotalLEDsCount);
            
            led_buffer* LEDBuffer = State->LEDBufferList;
            s32 LEDBufferLEDsAssignedToJobs = 0;
            
            for (s32 Job = 0; Job < JobsNeeded; Job++)
            {
                draw_leds_job_data* JobData = JobDataBank + JobDataBankUsed++;
                JobData->LEDs = LEDBuffer->LEDs;
                JobData->Colors = LEDBuffer->Colors;
                JobData->StartIndex = LEDBufferLEDsAssignedToJobs;
                JobData->OnePastLastIndex = GSMin(JobData->StartIndex + LED_BUFFER_SIZE, LEDBuffer->Count);
                
                LEDBufferLEDsAssignedToJobs += JobData->OnePastLastIndex - JobData->StartIndex;
                
                // New
                JobData->Batch = &BatchConstructor;
                
                JobData->FaceCameraMatrix = FaceCameraMatrix;
                JobData->ModelViewMatrix = ModelViewMatrix;
                JobData->LEDHalfWidth = LEDHalfWidth;
                
                Context.GeneralWorkQueue->PushWorkOnQueue(
                    Context.GeneralWorkQueue,
                    DrawLEDsInBufferRangeJob,
                    JobData);
                
                Assert(LEDBufferLEDsAssignedToJobs <= LEDBuffer->Count); // We should never go OVER the number of leds in the buffer
                if (LEDBufferLEDsAssignedToJobs == LEDBuffer->Count)
                {
                    LEDBuffer = LEDBuffer->Next;
                    LEDBufferLEDsAssignedToJobs = 0;
                }
            }
            
            Context.GeneralWorkQueue->DoQueueWorkUntilDone(Context.GeneralWorkQueue, 0);
            Context.GeneralWorkQueue->ResetWorkQueue(Context.GeneralWorkQueue);
        }
        
        ///////////////////////////////////////
        //           Interface
        //////////////////////////////////////
        
        DEBUG_TRACK_SCOPE(DrawInterface);
        
        PushRenderOrthographic(RenderBuffer, 0, 0, Context.WindowWidth, Context.WindowHeight);
        
        ///////////////////////////////////////
        //     Menu Bar
        //////////////////////////////////////
        
        r32 TopBarHeight = 40;
        {
            panel_result TopBarPanel = EvaluatePanel(RenderBuffer, 
                                                     v2{0, Context.WindowHeight - TopBarHeight},
                                                     v2{Context.WindowWidth, Context.WindowHeight},
                                                     0, State->Interface);
            
            v2 ButtonDim = v2{200, (r32)NewLineYOffset(*State->Interface.Font) + 10};
            v2 ButtonPos = v2{State->Interface.Margin.x, Context.WindowHeight - (ButtonDim.y + 10)};
            button_result LoadAssemblyBtn = EvaluateButton(RenderBuffer, ButtonPos, ButtonPos + ButtonDim, 
                                                           MakeStringLiteral("Load Assembly"), 
                                                           State->Interface, GuiMouse);
            
            string InterfaceString = MakeString(PushArray(State->Transient, char, 256), 256);
            for (int i = 0; i < State->AssembliesUsed; i++)
            {
                PrintF(&InterfaceString, "Unload %.*s", State->AssemblyList[i].Name.Length, State->AssemblyList[i].Name.Memory);
                
                ButtonPos.x += ButtonDim.x + 10;
                button_result UnloadAssemblyBtn = EvaluateButton(RenderBuffer, ButtonPos, ButtonPos + ButtonDim,
                                                                 InterfaceString, State->Interface, GuiMouse);
                
                if (UnloadAssemblyBtn.Pressed)
                {
                    UnloadAssembly(i, State, Context);
                }
            }
            
            if (LoadAssemblyBtn.Pressed)
            {
                char FilePath[256];
                b32 Success = Context.PlatformGetFilePath(FilePath, 256, "Foldhaus Files\0*.fold\0\0");
                if (Success)
                {
                    LoadAssembly(State, Context, FilePath);
                }
            }
        }
        
        ///////////////////////////////////////
        //    Figuring Out Nodes
        //////////////////////////////////////
        
        if (GuiMouse.LeftButtonTransitionedDown)
        {
            node_offset Node = GetNodeUnderPoint(State->NodeList, GuiMouse.Pos, State->NodeRenderSettings);
            if (Node.Node)
            {
                State->NodeInteraction = GetNodeInteractionType(Node.Node, Node.Offset, GuiMouse.Pos, State->NodeRenderSettings);
            }
        }
        else if (GuiMouse.LeftButtonTransitionedUp)
        {
            if (IsDraggingNodePort(State->NodeInteraction))
            {
                TryConnectNodes(State->NodeInteraction, GuiMouse.Pos, State->NodeList, State->NodeRenderSettings);
                State->NodeInteraction = NewEmptyNodeInteraction();
            }
            else if(IsDraggingNodeValue(State->NodeInteraction))
            {
                // This is just a click
                if (Mag(Mouse.DeltaPos) < 10)
                {
                    node_interaction Interaction = State->NodeInteraction;
                    interface_node* Node = GetNodeAtOffset(State->NodeList, Interaction.NodeOffset);
                    node_connection* Connection = Node->Connections + Interaction.InputValue;
                    struct_member_type InputType = Connection->Type;
                    if (InputType == MemberType_r32)
                    {
                        SetTextInputDestinationToFloat(&State->ActiveTextEntry, &Connection->R32Value);
                    }
                    State->NodeInteraction = NewEmptyNodeInteraction();
                    
                    // TODO(Peter): This is wrong, should be something to do with capturing text input
                    State->ActiveCommands = &State->NodeListerCommandRegistry;
                }
                else // This is the case where you dragged the value
                {
                    State->NodeInteraction = NewEmptyNodeInteraction();
                }
            }
            else
            {
                State->NodeInteraction = NewEmptyNodeInteraction();
            }
            
        }
        
        UpdateDraggingNode(Mouse.Pos, State->NodeInteraction, State->NodeList, 
                           State->NodeRenderSettings);
        UpdateDraggingNodePort(Mouse.Pos, State->NodeInteraction, State->NodeList, 
                               State->NodeRenderSettings, RenderBuffer);
        UpdateDraggingNodeValue(Mouse.Pos, Mouse.OldPos, State->NodeInteraction, State->NodeList, State->NodeRenderSettings, State);
        
        ResetNodesUpdateState(State->NodeList);
        
        if (State->NodeRenderSettings.Display)
        {
            RenderNodeList(State->NodeList, State->NodeRenderSettings, RenderBuffer);
        }
        
        if (State->ColorPickerEditValue != 0)
        {
            b32 ShouldClose = EvaluateColorPicker(RenderBuffer, State->ColorPickerEditValue, 
                                                  v2{200, 200}, State->Interface, GuiMouse);
            
            if (ShouldClose)
            {
                State->ColorPickerEditValue = 0;
            }
        }
        
        for (s32 m = 0; m < State->Modes.ActiveModesCount; m++)
        {
            operation_mode OperationMode = State->Modes.ActiveModes[m];
            OperationMode.Render(State, RenderBuffer, OperationMode, GuiMouse);
        }
        
        DrawDebugInterface(RenderBuffer, 25,
                           State->Interface, Context.WindowWidth, Context.WindowHeight - TopBarHeight,
                           Context.DeltaTime, State, State->Camera, GuiMouse, State->Transient);
    }
    
    EndDebugFrame(GlobalDebugServices);
}

CLEANUP_APPLICATION(CleanupApplication)
{
    app_state* State = (app_state*)Context.MemoryBase;
    SACNCleanup(&State->SACN, Context);
}