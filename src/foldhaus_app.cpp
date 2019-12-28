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
DrawPanelFooter(panel* Panel, render_command_buffer* RenderBuffer, rect FooterBounds, interface_config Interface, mouse_state Mouse)
{
    PushRenderQuad2D(RenderBuffer, FooterBounds.Min, v2{FooterBounds.Max.x, FooterBounds.Min.y + 25}, v4{.5f, .5f, .5f, 1.f});
    PushRenderQuad2D(RenderBuffer, FooterBounds.Min, FooterBounds.Min + v2{25, 25}, WhiteV4);
    
    v2 PanelSelectButtonMin = FooterBounds.Min + v2{30, 1};
    v2 PanelSelectButtonMax = PanelSelectButtonMin + v2{100, 23};
    
    if (Panel->PanelSelectionMenuOpen)
    {
        v2 ButtonDimension = v2{100, 25};
        v2 ButtonMin = v2{PanelSelectButtonMin.x, FooterBounds.Max.y};
        
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
RenderPanel(panel* Panel, rect PanelBounds, rect WindowBounds, render_command_buffer* RenderBuffer, app_state* State, context Context, mouse_state Mouse)
{
    Assert(Panel->PanelDefinitionIndex >= 0);
    
    rect FooterBounds = rect{
        PanelBounds.Min,
        v2{PanelBounds.Max.x, PanelBounds.Min.y + 25},
    };
    rect PanelViewBounds = rect{
        v2{PanelBounds.Min.x, FooterBounds.Max.y},
        PanelBounds.Max,
    };
    
    panel_definition Definition = GlobalPanelDefs[Panel->PanelDefinitionIndex];
    Definition.Render(*Panel, PanelViewBounds, RenderBuffer, State, Context, Mouse);
    
    PushRenderOrthographic(RenderBuffer, WindowBounds.Min.x, WindowBounds.Min.y, WindowBounds.Max.x, WindowBounds.Max.y);
    DrawPanelFooter(Panel, RenderBuffer, FooterBounds, State->Interface, Mouse);
}

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

////////////////////////////////////////////////////////////////////////

RELOAD_STATIC_DATA(ReloadStaticData)
{
    app_state* State = (app_state*)Context.MemoryBase;
    
    GlobalDebugServices = DebugServices;
    GSAlloc = Alloc;
    GSFree = Free;
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
    State->Camera.AspectRatio = AspectRatio(State->WindowBounds);
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
    
    { // Animation PLAYGROUND
        InitializeAnimationSystem(&State->AnimationSystem);
        State->AnimationSystem.SecondsPerFrame = 1.f / 24.f;
        
        animation_block BlockOne = {0}; 
        BlockOne.StartTime = 0;
        BlockOne.EndTime = 8;
        BlockOne.Proc = TestPatternTwo;
        AddAnimationBlock(BlockOne, &State->AnimationSystem);
        
        animation_block BlockTwo = {0};
        BlockTwo.StartTime = 8;
        BlockTwo.EndTime = 15;
        BlockTwo.Proc = TestPatternThree;
        AddAnimationBlock(BlockTwo, &State->AnimationSystem);
        
        State->AnimationSystem.AnimationStart = 0;
        State->AnimationSystem.AnimationEnd = 15;
    } // End Animation Playground
    
    
    InitializePanelSystem(&State->PanelSystem);
    panel* Panel = TakeNewPanel(&State->PanelSystem);
    SetPanelDefinition(Panel, 0);
}

internal void
HandleInput (app_state* State, rect WindowBounds, input_queue InputQueue, mouse_state Mouse)
{
    DEBUG_TRACK_FUNCTION;
    
    input_command_registry ActiveCommands = {};
    if (State->Modes.ActiveModesCount > 0)
    {
        ActiveCommands = State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1].Commands;
    }
    else
    {
        panel_and_bounds PanelWithMouseOverIt = GetPanelContainingPoint(Mouse.Pos, &State->PanelSystem, WindowBounds);
        if (!PanelWithMouseOverIt.Panel) { return; }
        
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

#define PANEL_EDGE_CLICK_MAX_DISTANCE 6

internal void
HandleMousePanelInteractionOrRecurse(panel* Panel, rect PanelBounds, panel_system* PanelLayout, mouse_state Mouse)
{
    // TODO(Peter): Need a way to calculate this button's position more systemically
    if (Panel->SplitDirection == PanelSplit_NoSplit 
        && PointIsInRange(Mouse.DownPos, PanelBounds.Min, PanelBounds.Min + v2{25, 25}))
    {
        r32 XDistance = GSAbs(Mouse.Pos.x - Mouse.DownPos.x);
        r32 YDistance = GSAbs(Mouse.Pos.y - Mouse.DownPos.y);
        
        if (XDistance > YDistance)
        {
            r32 XPercent = (Mouse.Pos.x - PanelBounds.Min.x) / Width(PanelBounds);
            SplitPanelVertically(Panel, XPercent, PanelBounds, PanelLayout);
        }
        else
        {
            r32 YPercent = (Mouse.Pos.y - PanelBounds.Min.y) / Height(PanelBounds);
            SplitPanelHorizontally(Panel, YPercent, PanelBounds, PanelLayout);
        }
    }
    else if (Panel->SplitDirection == PanelSplit_Horizontal)
    {
        r32 SplitY = GSLerp(PanelBounds.Min.y, PanelBounds.Max.y, Panel->SplitPercent);
        r32 ClickDistanceFromSplit = GSAbs(Mouse.DownPos.y - SplitY);
        if (ClickDistanceFromSplit < PANEL_EDGE_CLICK_MAX_DISTANCE)
        {
            r32 NewSplitY = Mouse.Pos.y;
            if (NewSplitY <= PanelBounds.Min.y)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Top, PanelLayout);
            }
            else if (NewSplitY >= PanelBounds.Max.y)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Bottom, PanelLayout);
            }
            else
            {
                Panel->SplitPercent = (NewSplitY  - PanelBounds.Min.y) / Height(PanelBounds);
            }
        }
        else
        {
            rect TopPanelBounds = GetTopPanelBounds(Panel, PanelBounds);
            rect BottomPanelBounds = GetBottomPanelBounds(Panel, PanelBounds);
            HandleMousePanelInteractionOrRecurse(&Panel->Bottom->Panel, BottomPanelBounds, PanelLayout, Mouse);
            HandleMousePanelInteractionOrRecurse(&Panel->Top->Panel, TopPanelBounds, PanelLayout, Mouse);
        }
    }
    else if (Panel->SplitDirection == PanelSplit_Vertical)
    {
        r32 SplitX = GSLerp(PanelBounds.Min.x, PanelBounds.Max.x, Panel->SplitPercent);
        r32 ClickDistanceFromSplit = GSAbs(Mouse.DownPos.x - SplitX);
        if (ClickDistanceFromSplit < PANEL_EDGE_CLICK_MAX_DISTANCE)
        {
            r32 NewSplitX = Mouse.Pos.x;
            if (NewSplitX <= PanelBounds.Min.x)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Right, PanelLayout);
            }
            else if (NewSplitX >= PanelBounds.Max.x)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Left, PanelLayout);
            }
            else
            {
                Panel->SplitPercent = (NewSplitX  - PanelBounds.Min.x) / Width(PanelBounds); 
            }
        }
        else
        {
            rect LeftPanelBounds = GetLeftPanelBounds(Panel, PanelBounds);
            rect RightPanelBounds = GetRightPanelBounds(Panel, PanelBounds);
            HandleMousePanelInteractionOrRecurse(&Panel->Left->Panel, LeftPanelBounds, PanelLayout, Mouse);
            HandleMousePanelInteractionOrRecurse(&Panel->Right->Panel, RightPanelBounds, PanelLayout, Mouse);
        }
    }
}

internal void
HandleMousePanelInteraction(panel_system* PanelSystem, rect WindowBounds,mouse_state Mouse)
{
    if (MouseButtonTransitionedUp(Mouse.LeftButtonState))
    {
        Assert(PanelSystem->PanelsUsed > 0);
        panel* FirstPanel = &PanelSystem->Panels[0].Panel;
        HandleMousePanelInteractionOrRecurse(FirstPanel, WindowBounds, PanelSystem, Mouse);
    }
}

internal void
DrawPanelBorder(panel Panel, v2 PanelMin, v2 PanelMax, v4 Color, mouse_state Mouse, render_command_buffer* RenderBuffer)
{
    r32 MouseLeftEdgeDistance = GSAbs(Mouse.Pos.x - PanelMin.x);
    r32 MouseRightEdgeDistance = GSAbs(Mouse.Pos.x - PanelMax.x);
    r32 MouseTopEdgeDistance = GSAbs(Mouse.Pos.y - PanelMax.y);
    r32 MouseBottomEdgeDistance = GSAbs(Mouse.Pos.y - PanelMin.y);
    
    PushRenderBoundingBox2D(RenderBuffer, PanelMin, PanelMax, 1, Color);
    v4 HighlightColor = v4{.3f, .3f, .3f, 1.f};
    r32 HighlightThickness = 1;
    if (MouseLeftEdgeDistance < PANEL_EDGE_CLICK_MAX_DISTANCE)
    {
        v2 LeftEdgeMin = PanelMin;
        v2 LeftEdgeMax = v2{PanelMin.x + HighlightThickness, PanelMax.y};
        PushRenderQuad2D(RenderBuffer, LeftEdgeMin, LeftEdgeMax, HighlightColor);
    }
    else if (MouseRightEdgeDistance < PANEL_EDGE_CLICK_MAX_DISTANCE)
    {
        v2 RightEdgeMin = v2{PanelMax.x - HighlightThickness, PanelMin.y};
        v2 RightEdgeMax = PanelMax;
        PushRenderQuad2D(RenderBuffer, RightEdgeMin, RightEdgeMax, HighlightColor);
    }
    else if (MouseTopEdgeDistance < PANEL_EDGE_CLICK_MAX_DISTANCE)
    {
        v2 TopEdgeMin = v2{PanelMin.x, PanelMax.y - HighlightThickness};
        v2 TopEdgeMax = PanelMax;
        PushRenderQuad2D(RenderBuffer, TopEdgeMin, TopEdgeMax, HighlightColor);
    }
    else if (MouseBottomEdgeDistance < PANEL_EDGE_CLICK_MAX_DISTANCE)
    {
        v2 BottomEdgeMin = PanelMin;
        v2 BottomEdgeMax = v2{PanelMax.x, PanelMin.y + HighlightThickness};
        PushRenderQuad2D(RenderBuffer, BottomEdgeMin, BottomEdgeMax, HighlightColor);
    }
}

internal void
DrawAllPanels(panel_layout PanelLayout, render_command_buffer* RenderBuffer, mouse_state Mouse, app_state* State, context Context)
{
    for (u32 i = 0; i < PanelLayout.PanelsCount; i++)
    {
        panel_with_layout PanelWithLayout = PanelLayout.Panels[i];
        panel* Panel = PanelWithLayout.Panel;
        rect PanelBounds = PanelWithLayout.Bounds;
        
        RenderPanel(Panel, PanelBounds, State->WindowBounds, RenderBuffer, State, Context, Mouse);
        v4 BorderColor = v4{0, 0, 0, 1};
        
        PushRenderOrthographic(RenderBuffer, State->WindowBounds.Min.x, State->WindowBounds.Min.y, State->WindowBounds.Max.x, State->WindowBounds.Max.y);
        DrawPanelBorder(*Panel, PanelBounds.Min, PanelBounds.Max, BorderColor, Mouse, RenderBuffer);
    }
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
        State->AnimationSystem.Time += Context.DeltaTime;
        if (State->AnimationSystem.Time > State->AnimationSystem.AnimationEnd)
        {
            State->AnimationSystem.Time -= State->AnimationSystem.AnimationEnd;
        }
    }
    
    s32 CurrentFrame = (s32)(State->AnimationSystem.Time / State->AnimationSystem.SecondsPerFrame);
    if (CurrentFrame != State->AnimationSystem.LastUpdatedFrame)
    {
        State->AnimationSystem.LastUpdatedFrame = CurrentFrame;
        r32 FrameTime = CurrentFrame * State->AnimationSystem.SecondsPerFrame;
        
        for (u32 i = 0; i < State->AnimationSystem.BlocksCount; i++)
        {
            animation_block_entry BlockEntry = State->AnimationSystem.Blocks[i];
            if (!AnimationBlockIsFree(BlockEntry))
            {
                animation_block Block = BlockEntry.Block;
                if (State->AnimationSystem.Time >= Block.StartTime
                    && State->AnimationSystem.Time <= Block.EndTime)
                {
                    for (s32 j = 0; j < State->ActiveAssemblyIndecies.Used; j++)
                    {
                        array_entry_handle* AssemblyHandle = GetElementAtIndex(j, State->ActiveAssemblyIndecies);
                        assembly* Assembly = GetElementWithHandle(*AssemblyHandle, State->AssemblyList);
                        Block.Proc(Assembly, FrameTime  - Block.StartTime);
                    }
                }
            }
        }
    }
    
    s32 HeaderSize = State->NetworkProtocolHeaderSize;
    dmx_buffer_list* DMXBuffers = 0;
    for (s32 i = 0; i < State->ActiveAssemblyIndecies.Used; i++)
    {
        array_entry_handle* AssemblyHandle = GetElementAtIndex(i, State->ActiveAssemblyIndecies);
        assembly* Assembly = GetElementWithHandle(*AssemblyHandle, State->AssemblyList);
        dmx_buffer_list* NewDMXBuffers = CreateDMXBuffers(*Assembly, HeaderSize, &State->Transient);
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
    
    PushRenderOrthographic(RenderBuffer, 0, 0, Width(State->WindowBounds), Height(State->WindowBounds));
    PushRenderClearScreen(RenderBuffer);
    
    HandleMousePanelInteraction(&State->PanelSystem, State->WindowBounds, Mouse);
    
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
    
    // Checking for overflows
    {
        DEBUG_TRACK_SCOPE(OverflowChecks);
        AssertAllocationsNoOverflow(State->Permanent);
        for (s32 i = 0; i < State->ActiveAssemblyIndecies.Used; i++)
        {
            array_entry_handle* AssemblyHandle = GetElementAtIndex(i, State->ActiveAssemblyIndecies);
            assembly* Assembly = GetElementWithHandle(*AssemblyHandle, State->AssemblyList);
            AssertAllocationsNoOverflow(Assembly->Arena);
        }
    }
}

CLEANUP_APPLICATION(CleanupApplication)
{
    app_state* State = (app_state*)Context.MemoryBase;
    SACNCleanup(&State->SACN, Context);
}