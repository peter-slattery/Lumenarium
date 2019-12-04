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

struct draw_leds_job_data
{
    led* LEDs;
    pixel* Colors;
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
    
    s32 LEDCount = Data->OnePastLastIndex - Data->StartIndex;
    
    quad_batch_constructor_reserved_range BatchReservedRange = ThreadSafeReserveRangeInQuadConstructor(Data->Batch, LEDCount * 2);
    s32 TrisUsed = 0;
    
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
         LEDIdx < LEDCount;
         LEDIdx++)
    {
        pixel PixelColor = Data->Colors[LED->Index];
        v4 Color = v4{PixelColor.R / 255.f, PixelColor.G / 255.f, PixelColor.B / 255.f, 1.0f};
        
        v4 V4Position = LED->Position;
        V4Position.w = 0;
        v4 P0 = P0_In + V4Position;
        v4 P1 = P1_In + V4Position;
        v4 P2 = P2_In + V4Position;
        v4 P3 = P3_In + V4Position;
        
        SetTri3DInBatch(Data->Batch, BatchReservedRange.Start + TrisUsed++,
                        P0, P1, P2, UV0, UV1, UV2, Color, Color, Color);
        SetTri3DInBatch(Data->Batch, BatchReservedRange.Start + TrisUsed++,
                        P0, P2, P3, UV0, UV2, UV3, Color, Color, Color);
        
        LED++;
    }
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
    platform_send_to* SendTo = Data->SendTo;
    
    dmx_buffer_list* DMXBufferAt = Data->DMXBuffers;
    while (DMXBufferAt)
    {
        dmx_buffer Buffer = DMXBufferAt->Buffer;
        
        u_long V4SendAddress = SACNGetUniverseSendAddress(Buffer.Universe);
        
        platform_network_address SendAddress = {};
        SendAddress.Family = AF_INET;
        SendAddress.Port = DEFAULT_STREAMING_ACN_PORT;
        SendAddress.Address = V4SendAddress;
        
        SendTo(SendSocket, SendAddress, (const char*)Buffer.Base, Buffer.TotalSize, 0);
        
        DMXBufferAt = DMXBufferAt->Next;
    }
}

internal void
LoadAssembly (app_state* State, context Context, char* Path)
{
    arena_snapshot TempMemorySnapshot = TakeSnapshotOfArena(*State->Transient);
    
    platform_memory_result TestAssemblyFile = Context.PlatformReadEntireFile(Path);
    Assert(TestAssemblyFile.Size > 0);
    
    assembly_definition AssemblyDefinition = ParseAssemblyFile(TestAssemblyFile.Base, TestAssemblyFile.Size, State->Transient);
    
    Context.PlatformFree(TestAssemblyFile.Base, TestAssemblyFile.Size);
    
    string PathString = MakeStringLiteral(Path);
    s32 IndexOfLastSlash = FastLastIndexOfCharInCharArray(PathString.Memory, PathString.Length, '\\');
    string FileName = Substring(PathString, IndexOfLastSlash + 1);
    
    r32 Scale = 100;
    s32 AssemblyMemorySize = GetAssemblyMemorySizeFromDefinition(AssemblyDefinition, FileName);
    u8* AssemblyMemory = Context.PlatformAlloc(AssemblyMemorySize);
    
    assembly NewAssembly = ConstructAssemblyFromDefinition(AssemblyDefinition, 
                                                           FileName, 
                                                           v3{0, 0, 0}, 
                                                           Scale, 
                                                           AssemblyMemory,
                                                           AssemblyMemorySize);
    array_entry_handle NewAssemblyHandle = PushElement(NewAssembly, &State->AssemblyList);
    PushElement(NewAssemblyHandle, &State->ActiveAssemblyIndecies);
    
    State->TotalLEDsCount += NewAssembly.LEDCount;
    
    ClearArenaToSnapshot(State->Transient, TempMemorySnapshot);
}

internal void
UnloadAssembly (s32 AssemblyIndex, app_state* State, context Context)
{
    assembly* Assembly = GetElementAtIndex(AssemblyIndex, State->AssemblyList);
    State->TotalLEDsCount -= Assembly->LEDCount;
    Context.PlatformFree(Assembly->Arena.Base, Assembly->Arena.Size);
    
    RemoveElementAtIndex(AssemblyIndex, &State->AssemblyList);
    for (s32 i = 0; i < State->ActiveAssemblyIndecies.Used; i++)
    {
        array_entry_handle Handle = *GetElementAtIndex(i, State->ActiveAssemblyIndecies);
        if (Handle.Index == AssemblyIndex)
        {
            RemoveElementAtIndex(i, &State->ActiveAssemblyIndecies);
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////

RELOAD_STATIC_DATA(ReloadStaticData)
{
    app_state* State = (app_state*)Context.MemoryBase;
    
    GlobalDebugServices = DebugServices;
    GSAlloc = Alloc;
    GSFree = Free;
    
    if (State->DefaultInputCommandRegistry.Size > 0)
    {
        RegisterKeyPressCommand(&State->DefaultInputCommandRegistry, KeyCode_MouseLeftButton, Command_Began, KeyCode_Invalid,
                                Begin3DViewMouseRotate);
        RegisterKeyPressCommand(&State->DefaultInputCommandRegistry, KeyCode_U, Command_Began, KeyCode_Invalid, OpenUniverseView);
    }
}

INITIALIZE_APPLICATION(InitializeApplication)
{
    app_state* State = (app_state*)Context.MemoryBase;
    u8* MemoryCursor = Context.MemoryBase + sizeof(app_state);
    s32 PermanentStorageSize = Context.MemorySize; //Megabytes(32);
    //s32 TransientStorageSize = Context.MemorySize - PermanentStorageSize;
    State->Permanent = BootstrapArenaIntoMemory(MemoryCursor, PermanentStorageSize);
    //State->Transient = BootstrapArenaIntoMemory(MemoryCursor + PermanentStorageSize, TransientStorageSize);
    
    u8* TransientMemory = Context.PlatformAlloc(Megabytes(32));
    InitMemoryArena(&State->TransientMemory, TransientMemory, Megabytes(32), Context.PlatformAlloc);
    State->Transient = &State->TransientMemory;
    
    InitMemoryArena(&State->SACNMemory, 0, 0, Context.PlatformAlloc);
    
    InitializeInputCommandRegistry(&State->DefaultInputCommandRegistry, 32, State->Permanent);
    
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
    
    State->SACN = InitializeSACN(Context);
    State->NetworkProtocolHeaderSize = STREAM_HEADER_SIZE;
    
    State->Camera.FieldOfView = DegreesToRadians(45.0f);
    State->Camera.AspectRatio = (r32)Context.WindowWidth / (r32)Context.WindowHeight;
    State->Camera.Near = 1.0f;
    State->Camera.Far = 100.0f;
    State->Camera.Position = v3{0, 0, -250};
    State->Camera.LookAt = v3{0, 0, 0};
    
    State->AssemblyList.BucketSize = 32;
    State->AssemblyList.FreeList.Next = &State->AssemblyList.FreeList;
    State->ActiveAssemblyIndecies.BucketSize = 32;
#if 1
    char Path[] = "blumen_lumen.fold";
    LoadAssembly(State, Context, Path);
#endif
    
    State->PixelsToWorldScale = .01f;
    
    GlobalDebugServices->Interface.RenderSculpture = true;
    
    ReloadStaticData(Context, GlobalDebugServices, Alloc, Free);
    
    { // MODES PLAYGROUND
        State->Modes.ActiveModesCount = 0;
        
        s32 ModesMemorySize = Kilobytes(32);
        u8* ModesMemory = PushSize(State->Permanent, ModesMemorySize);
        InitMemoryArena(&State->Modes.Arena, ModesMemory, ModesMemorySize, 0);
    }
}

internal void
HandleInput (app_state* State, input_queue InputQueue, mouse_state Mouse)
{
    DEBUG_TRACK_FUNCTION;
    
    input_command_registry ActiveCommands = State->DefaultInputCommandRegistry;
    if (State->Modes.ActiveModesCount > 0)
    {
        ActiveCommands = State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1].Commands;
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
    
    for (s32 Range = 0; Range < Assembly.LEDUniverseMapCount; Range++)
    {
        leds_in_universe_range LEDUniverseRange = Assembly.LEDUniverseMap[Range];
        
        dmx_buffer_list* NewBuffer = PushStruct(Arena, dmx_buffer_list);
        NewBuffer->Buffer.Universe = LEDUniverseRange.Universe;
        NewBuffer->Buffer.Base = PushArray(Arena, u8, BufferSize);
        NewBuffer->Buffer.TotalSize = BufferSize;
        NewBuffer->Buffer.HeaderSize = BufferHeaderSize;
        
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
    
    // NOTE(Peter): We do this at the beginning because all the render commands are stored in Transient,
    // and need to persist beyond the end of the UpdateAndRender call. In the release version, we won't
    // zero the Transient arena when we clear it so it wouldn't be a problem, but it is technically 
    // incorrect to clear the arena, and then access the memory later.
    ClearArena(State->Transient);
    
    HandleInput(State, InputQueue, Mouse);
    
    r32 GreenSize = 20.0f;
    r32 BlueSize = 25.0f;
    r32 RedSize = 25.0f;
    
    State->GreenIter += Context.DeltaTime * 45;
    State->BlueIter += Context.DeltaTime * 25;
    State->RedIter += Context.DeltaTime * -35;
    
    
    
#define PATTERN_THREE
    
#ifdef PATTERN_ONE
    array_entry_handle TestAssemblyHandle = *GetElementAtIndex(0, State->ActiveAssemblyIndecies);
    assembly TestAssembly = *GetElementWithHandle(TestAssemblyHandle, State->AssemblyList);
    for (s32 Range = 0; Range < TestAssembly.LEDUniverseMapCount; Range++)
    {
        leds_in_universe_range LEDUniverseRange = TestAssembly.LEDUniverseMap[Range];
        for (s32 LEDIdx = LEDUniverseRange.RangeStart;
             LEDIdx < LEDUniverseRange.RangeOnePastLast;
             LEDIdx++)
        {
            led LED = TestAssembly.LEDs[LEDIdx];
            TestAssembly.Colors[LED.Index].R = 255;
            TestAssembly.Colors[LED.Index].B = 255;
            TestAssembly.Colors[LED.Index].G = 255;
        }
    }
#endif
    
#ifdef PATTERN_TWO
    if (State->GreenIter > 2 * PI * 100) { State->GreenIter = 0; }
    r32 SinAdjusted = 0.5f + (GSSin(State->GreenIter * 0.01f) * .5f);
    u8 Brightness = (u8)(GSClamp01(SinAdjusted) * 255);
    
    array_entry_handle TestAssemblyHandle = *GetElementAtIndex(0, State->ActiveAssemblyIndecies);
    assembly TestAssembly = *GetElementWithHandle(TestAssemblyHandle, State->AssemblyList);
    for (s32 Range = 0; Range < TestAssembly.LEDUniverseMapCount; Range++)
    {
        leds_in_universe_range LEDUniverseRange = TestAssembly.LEDUniverseMap[Range];
        for (s32 LEDIdx = LEDUniverseRange.RangeStart;
             LEDIdx < LEDUniverseRange.RangeOnePastLast;
             LEDIdx++)
        {
            led LED = TestAssembly.LEDs[LEDIdx];
            TestAssembly.Colors[LED.Index].R = Brightness;
            TestAssembly.Colors[LED.Index].B = Brightness;
            TestAssembly.Colors[LED.Index].G = Brightness;
        }
    }
#endif
    
#ifdef PATTERN_THREE
    if(State->GreenIter > 100 + GreenSize) { State->GreenIter = -GreenSize; }
    if(State->BlueIter > 100 + BlueSize) { State->BlueIter = -BlueSize; }
    if(State->RedIter < 0 - RedSize) { State->RedIter = 100 + RedSize; }
    
    array_entry_handle TestAssemblyHandle = *GetElementAtIndex(0, State->ActiveAssemblyIndecies);
    assembly TestAssembly = *GetElementWithHandle(TestAssemblyHandle, State->AssemblyList);
    for (s32 Range = 0; Range < TestAssembly.LEDUniverseMapCount; Range++)
    {
        leds_in_universe_range LEDUniverseRange = TestAssembly.LEDUniverseMap[Range];
        for (s32 LEDIdx = LEDUniverseRange.RangeStart;
             LEDIdx < LEDUniverseRange.RangeOnePastLast;
             LEDIdx++)
        {
            led LED = TestAssembly.LEDs[LEDIdx];
            u8 Red = 0;
            u8 Green = 0;
            u8 Blue = 0;
            
            r32 GreenDistance = GSAbs(LED.Position.z - State->GreenIter);
            r32 GreenBrightness = GSClamp(0.0f, GreenSize - GreenDistance, GreenSize) / GreenSize;
            Green = (u8)(GreenBrightness * 255);
            
            r32 BlueDistance = GSAbs(LED.Position.z - State->BlueIter);
            r32 BlueBrightness = GSClamp(0.0f, BlueSize - BlueDistance, BlueSize) / BlueSize;
            Blue = (u8)(BlueBrightness * 255);
            
            r32 RedDistance = GSAbs(LED.Position.z - State->RedIter);
            r32 RedBrightness = GSClamp(0.0f, RedSize - RedDistance, RedSize) / RedSize;
            Red = (u8)(RedBrightness * 255);
            
            TestAssembly.Colors[LED.Index].R = Red;
            TestAssembly.Colors[LED.Index].B = Blue;
            TestAssembly.Colors[LED.Index].G = Green;
        }
    }
#endif
    
    // Update Visuals Here
    
    s32 HeaderSize = State->NetworkProtocolHeaderSize;
    dmx_buffer_list* DMXBuffers = 0;
    for (s32 i = 0; i < State->ActiveAssemblyIndecies.Used; i++)
    {
        array_entry_handle AssemblyHandle = *GetElementAtIndex(i, State->ActiveAssemblyIndecies);
        assembly Assembly = *GetElementWithHandle(AssemblyHandle, State->AssemblyList);
        dmx_buffer_list* NewDMXBuffers = CreateDMXBuffers(Assembly, HeaderSize, State->Transient);
        DMXBuffers = DMXBufferListAppend(DMXBuffers, NewDMXBuffers);
    }
    
    DEBUG_IF(GlobalDebugServices->Interface.SendSACNData)
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
                
                send_sacn_job_data* Job = PushStruct(State->Transient, send_sacn_job_data);
                Job->SendSocket = State->SACN.SendSocket;
                Job->SendTo = Context.PlatformSendTo;
                Job->DMXBuffers = DMXBuffers;
                
                Context.GeneralWorkQueue->PushWorkOnQueue(
                    Context.GeneralWorkQueue,
                    SACNSendDMXBufferListJob,
                    Job);
            }break;
            
            InvalidDefaultCase;
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
        
        // TODO(Peter): Pretty sure this isn't working right now
        m44 FaceCameraMatrix = GetLookAtMatrix(v4{0, 0, 0, 1}, V4(State->Camera.Position, 1));
        FaceCameraMatrix = FaceCameraMatrix;
        
        DEBUG_IF(GlobalDebugServices->Interface.RenderSculpture) // DebugServices RenderSculpture Toggle
        {
            DEBUG_TRACK_SCOPE(RenderSculpture);
            
            s32 MaxLEDsPerJob = 2048;
            render_quad_batch_constructor RenderLEDsBatch = PushRenderQuad3DBatch(RenderBuffer, State->TotalLEDsCount);
            
            for (s32 i = 0; i < State->ActiveAssemblyIndecies.Used; i++)
            {
                array_entry_handle AssemblyHandle = *GetElementAtIndex(i, State->ActiveAssemblyIndecies);
                assembly Assembly = *GetElementWithHandle(AssemblyHandle, State->AssemblyList);
                s32 JobsNeeded = IntegerDivideRoundUp(Assembly.LEDCount, MaxLEDsPerJob);
                
                for (s32 Job = 0; Job < JobsNeeded; Job++)
                {
                    draw_leds_job_data* JobData = PushStruct(State->Transient, draw_leds_job_data);
                    JobData->LEDs = Assembly.LEDs;
                    JobData->Colors = Assembly.Colors;
                    JobData->StartIndex = Job * MaxLEDsPerJob;
                    JobData->OnePastLastIndex = GSMin(JobData->StartIndex + MaxLEDsPerJob, Assembly.LEDCount);
                    JobData->Batch = &RenderLEDsBatch;
                    JobData->FaceCameraMatrix;
                    JobData->ModelViewMatrix = ModelViewMatrix;
                    JobData->LEDHalfWidth = LEDHalfWidth;
                    
                    Context.GeneralWorkQueue->PushWorkOnQueue(
                        Context.GeneralWorkQueue,
                        DrawLEDsInBufferRangeJob,
                        JobData);
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
                                                           State->Interface, Mouse);
            
            string InterfaceString = MakeString(PushArray(State->Transient, char, 256), 256);
            for (s32 i = 0; i < State->ActiveAssemblyIndecies.Used; i++)
            {
                array_entry_handle AssemblyHandle = *GetElementAtIndex(i, State->ActiveAssemblyIndecies);
                assembly Assembly = *GetElementWithHandle(AssemblyHandle, State->AssemblyList);
                PrintF(&InterfaceString, "Unload %.*s", Assembly.Name.Length, Assembly.Name.Memory);
                
                ButtonPos.x += ButtonDim.x + 10;
                button_result UnloadAssemblyBtn = EvaluateButton(RenderBuffer, ButtonPos, ButtonPos + ButtonDim,
                                                                 InterfaceString, State->Interface, Mouse);
                
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
        
        for (s32 m = 0; m < State->Modes.ActiveModesCount; m++)
        {
            operation_mode OperationMode = State->Modes.ActiveModes[m];
            if (OperationMode.Render != 0)
            {
                OperationMode.Render(State, RenderBuffer, OperationMode, Mouse);
            }
        }
        
        DrawDebugInterface(RenderBuffer, 25,
                           State->Interface, Context.WindowWidth, Context.WindowHeight - TopBarHeight,
                           Context.DeltaTime, State, State->Camera, Mouse, State->Transient);
    }
}

CLEANUP_APPLICATION(CleanupApplication)
{
    app_state* State = (app_state*)Context.MemoryBase;
    SACNCleanup(&State->SACN, Context);
}