#include "foldhaus_platform.h"
#include "foldhaus_app.h"

internal void
SetPanelDefinitionExternal(panel* Panel, s32 OldPanelDefinitionIndex, s32 NewPanelDefinitionIndex)
{
    if(OldPanelDefinitionIndex >= 0)
    {
        GlobalPanelDefs[OldPanelDefinitionIndex].Cleanup(Panel);
    }
    GlobalPanelDefs[NewPanelDefinitionIndex].Init(Panel);
}


internal void
DrawPanelFooter(panel* Panel, render_command_buffer* RenderBuffer, v2 FooterMin, v2 FooterMax, interface_config Interface, mouse_state Mouse)
{
    PushRenderQuad2D(RenderBuffer, FooterMin, v2{FooterMax.x, FooterMin.y + 25}, v4{.5f, .5f, .5f, 1.f});
    PushRenderQuad2D(RenderBuffer, FooterMin, FooterMin + v2{25, 25}, WhiteV4);
    
    v2 PanelSelectButtonMin = FooterMin + v2{30, 1};
    v2 PanelSelectButtonMax = PanelSelectButtonMin + v2{100, 23};
    
    if (Panel->PanelSelectionMenuOpen)
    {
        v2 ButtonDimension = v2{100, 25};
        v2 ButtonMin = v2{PanelSelectButtonMin.x, FooterMax.y};
        
        v2 MenuMin = ButtonMin;
        v2 MenuMax = v2{ButtonMin.x + ButtonDimension.x, ButtonMin.y + (ButtonDimension.y * GlobalPanelDefsCount)};
        if (MouseButtonTransitionedDown(Mouse.LeftButtonState)
            && !PointIsInRange(Mouse.DownPos, MenuMin, MenuMax))
    {
        Panel->PanelSelectionMenuOpen = false;
    }


for (s32 i = 0; i < GlobalPanelDefsCount; i++)
        {
            panel_definition Def = GlobalPanelDefs[i];
            string DefName = MakeString(Def.PanelName, Def.PanelNameLength);
            button_result DefinitionButton = EvaluateButton(RenderBuffer,
                                                            ButtonMin, ButtonMin + ButtonDimension,
                                                            DefName, Interface, Mouse);
            if (DefinitionButton.Pressed)
            {
                SetPanelDefinition(Panel, i);
                Panel->PanelSelectionMenuOpen = false;
            }
            
            ButtonMin.y += ButtonDimension.y;
        }
    }

button_result ButtonResult = EvaluateButton(RenderBuffer,
                                                PanelSelectButtonMin, 
PanelSelectButtonMax,
                                                MakeStringLiteral("Select"), Interface, Mouse);
    if (ButtonResult.Pressed)
    {
        Panel->PanelSelectionMenuOpen = !Panel->PanelSelectionMenuOpen;
    }
    
}

internal void
RenderPanel(panel* Panel, v2 PanelMin, v2 PanelMax, v2 WindowMin, v2 WindowMax, render_command_buffer* RenderBuffer, app_state* State, context Context, mouse_state Mouse)
{
    Assert(Panel->PanelDefinitionIndex >= 0);
    
v2 FooterMin = PanelMin;
        v2 FooterMax = v2{PanelMax.x, PanelMin.y + 25};
    v2 PanelViewMin = v2{PanelMin.x, FooterMax.y};
        v2 PanelViewMax = PanelMax;
        
panel_definition Definition = GlobalPanelDefs[Panel->PanelDefinitionIndex];
    Definition.Render(*Panel, PanelMin, PanelMax, RenderBuffer, State, Context, Mouse);
    
PushRenderOrthographic(RenderBuffer, WindowMin.x, WindowMin.y, WindowMax.x, WindowMax.y);
DrawPanelFooter(Panel, RenderBuffer, FooterMin, FooterMax, State->Interface, Mouse);
}

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
    platform_memory_result TestAssemblyFile = Context.PlatformReadEntireFile(Path);
    Assert(TestAssemblyFile.Size > 0);
    
    assembly_definition AssemblyDefinition = ParseAssemblyFile(TestAssemblyFile.Base, TestAssemblyFile.Size, &State->Transient);
    
    Context.PlatformFree(TestAssemblyFile.Base, TestAssemblyFile.Size);
    
    string PathString = MakeStringLiteral(Path);
    s32 IndexOfLastSlash = FastLastIndexOfCharInCharArray(PathString.Memory, PathString.Length, '\\');
    string FileName = Substring(PathString, IndexOfLastSlash + 1);
    
    r32 Scale = 100;
    memory_arena AssemblyArena = {};
    AssemblyArena.Alloc = (gs_memory_alloc*)Context.PlatformAlloc;
    AssemblyArena.Realloc = (gs_memory_realloc*)Context.PlatformRealloc;
    
    assembly NewAssembly = ConstructAssemblyFromDefinition(AssemblyDefinition, 
                                                           FileName, 
                                                           v3{0, 0, 0}, 
                                                           Scale, 
                                                           AssemblyArena);
    array_entry_handle NewAssemblyHandle = PushElement(NewAssembly, &State->AssemblyList);
    PushElement(NewAssemblyHandle, &State->ActiveAssemblyIndecies);
    
    State->TotalLEDsCount += NewAssembly.LEDCount;
}

internal void
UnloadAssembly (s32 AssemblyIndex, app_state* State, context Context)
{
    assembly* Assembly = GetElementAtIndex(AssemblyIndex, State->AssemblyList);
    State->TotalLEDsCount -= Assembly->LEDCount;
    FreeMemoryArena(&Assembly->Arena, (gs_memory_free*)Context.PlatformFree);
    
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
        RegisterKeyPressCommand(&State->DefaultInputCommandRegistry, KeyCode_X, Command_Ended, KeyCode_Invalid, DeleteAnimationBlock);
    }
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
    
    InitializeInputCommandRegistry(&State->DefaultInputCommandRegistry, 32, &State->Permanent);
    
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
        if (FontFile.Size)
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
    char Path[] = "radialumia.fold";
    LoadAssembly(State, Context, Path);
#endif
    
    State->PixelsToWorldScale = .01f;
    
    GlobalDebugServices->Interface.RenderSculpture = true;
    
    ReloadStaticData(Context, GlobalDebugServices, Alloc, Free);
    
    // Setup Operation Modes
    State->Modes.ActiveModesCount = 0;
    State->Modes.Arena = {};
    State->Modes.Arena.Alloc = (gs_memory_alloc*)Context.PlatformAlloc;
    State->Modes.Arena.Realloc = (gs_memory_realloc*)Context.PlatformRealloc;
    State->Modes.Arena.FindAddressRule = FindAddress_InLastBufferOnly;
    
    { // MODES PLAYGROUND
        InitializeAnimationSystem(&State->AnimationSystem);
        
        animation_block BlockZero = {0}; 
        BlockZero.StartTime = 0;
        BlockZero.EndTime = 2;
        BlockZero.Proc = TestPatternOne;
        AddAnimationBlock(BlockZero, &State->AnimationSystem);
        
        animation_block BlockOne = {0}; 
        BlockOne.StartTime = 3;
        BlockOne.EndTime = 5;
        BlockOne.Proc = TestPatternTwo;
        AddAnimationBlock(BlockOne, &State->AnimationSystem);
        
        animation_block BlockTwo = {0};
        BlockTwo.StartTime = 5;
        BlockTwo.EndTime = 8;
        BlockTwo.Proc = TestPatternThree;
        AddAnimationBlock(BlockTwo, &State->AnimationSystem);
        
        State->AnimationSystem.AnimationEnd = 10;
    } // End Animation Playground
    
    
    { // Panels Playground
        InitializePanelLayout(&State->PanelLayout);
panel* Panel = TakeNewPanel(&State->PanelLayout);
        SetPanelDefinition(Panel, 0);
        SplitPanelVertically(Panel, .5f, v2{0, 0}, v2{Context.WindowWidth, Context.WindowHeight}, &State->PanelLayout);
        SetPanelDefinition(&Panel->Right->Panel, 1);
    } // End Panels Playground
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
    ClearArena(&State->Transient);
    
    HandleInput(State, InputQueue, Mouse);
    
    if (State->AnimationSystem.TimelineShouldAdvance) { 
        State->AnimationSystem.Time += Context.DeltaTime;
        if (State->AnimationSystem.Time > State->AnimationSystem.AnimationEnd)
        {
            State->AnimationSystem.Time -= State->AnimationSystem.AnimationEnd;
        }
        
        for (u32 i = 0; i < State->AnimationSystem.BlocksCount; i++)
        {
            animation_block_entry BlockEntry = State->AnimationSystem.Blocks[i];
            if (!AnimationBlockIsFree(BlockEntry))
            {
                animation_block Block = BlockEntry.Block;
                if (State->AnimationSystem.Time >= Block.StartTime
                    && State->AnimationSystem.Time <= Block.EndTime)
                {
                    Block.Proc(State, State->AnimationSystem.Time - Block.StartTime);
                }
            }
        }
    }
    
    s32 HeaderSize = State->NetworkProtocolHeaderSize;
    dmx_buffer_list* DMXBuffers = 0;
    for (s32 i = 0; i < State->ActiveAssemblyIndecies.Used; i++)
    {
        array_entry_handle AssemblyHandle = *GetElementAtIndex(i, State->ActiveAssemblyIndecies);
        assembly Assembly = *GetElementWithHandle(AssemblyHandle, State->AssemblyList);
        dmx_buffer_list* NewDMXBuffers = CreateDMXBuffers(Assembly, HeaderSize, &State->Transient);
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
                
                send_sacn_job_data* Job = PushStruct(&State->Transient, send_sacn_job_data);
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
    
PushRenderOrthographic(RenderBuffer, 0, 0, Context.WindowWidth, Context.WindowHeight);
        PushRenderClearScreen(RenderBuffer);
    v2 WindowMin = v2{0, 0};
    v2 WindowMax = v2{Context.WindowWidth, Context.WindowHeight};
    HandleMousePanelInteraction(&State->PanelLayout, WindowMin, WindowMax, Mouse);
    DrawAllPanels(State->PanelLayout, WindowMin, WindowMax, RenderBuffer, State->Interface, Mouse, State, Context);
    
    ////////////////////////////////
    //   Render Assembly
    ///////////////////////////////
    if (Context.WindowIsVisible)
    {
    #if 0
        State->Camera.AspectRatio = (r32)Context.WindowWidth / (r32)Context.WindowHeight;
        
        m44 ModelViewMatrix = GetCameraModelViewMatrix(State->Camera);
        m44 ProjectionMatrix = GetCameraPerspectiveProjectionMatrix(State->Camera);
        
        r32 LEDHalfWidth = .5f;
        
        PushRenderPerspective(RenderBuffer, 0, 0, Context.WindowWidth / 2, Context.WindowHeight, State->Camera);
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
                    draw_leds_job_data* JobData = PushStruct(&State->Transient, draw_leds_job_data);
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
            
            string InterfaceString = MakeString(PushArray(&State->Transient, char, 256), 256);
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
        
        DrawDebugInterface(RenderBuffer, 25,
                           State->Interface, Context.WindowWidth, Context.WindowHeight - TopBarHeight,
                           Context.DeltaTime, State, State->Camera, Mouse, &State->Transient);
#endif
    }


        for (s32 m = 0; m < State->Modes.ActiveModesCount; m++)
        {
            operation_mode OperationMode = State->Modes.ActiveModes[m];
            if (OperationMode.Render != 0)
            {
                OperationMode.Render(State, RenderBuffer, OperationMode, Mouse);
            }
        }
        
    
    // Checking for overflows
    {
        DEBUG_TRACK_SCOPE(OverflowChecks);
        AssertAllocationsNoOverflow(State->Permanent);
        for (s32 i = 0; i < State->AssemblyList.Used; i++)
        {
            assembly* Assembly = GetElementAtIndex(i, State->AssemblyList);
            AssertAllocationsNoOverflow(Assembly->Arena);
        }
    }
}

CLEANUP_APPLICATION(CleanupApplication)
{
    app_state* State = (app_state*)Context.MemoryBase;
    SACNCleanup(&State->SACN, Context);
}