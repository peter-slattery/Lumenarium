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
        
        PushTri3DOnBatch(Data->Batch, P0, P1, P2, UV0, UV1, UV2, Color);
        PushTri3DOnBatch(Data->Batch, P0, P2, P3, UV0, UV2, UV3, Color);
        
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
    platform_memory_result TestAssemblyFile = Context.PlatformReadEntireFile(Path);
    if (TestAssemblyFile.Size <= 0)
    {
        InvalidCodePath;
    }
    assembly_definition AssemblyDefinition = ParseAssemblyFile((char*)TestAssemblyFile.Base, State->Transient);
    Context.PlatformFree(TestAssemblyFile.Base, TestAssemblyFile.Size);
    
    string PathString = MakeStringLiteral(Path);
    s32 IndexOfLastSlash = FastLastIndexOfChar(PathString.Memory, PathString.Length, '\\');
    string FileName = Substring(PathString, IndexOfLastSlash + 1);
    
    r32 Scale = 100;
    ConstructAssemblyFromDefinition(AssemblyDefinition, FileName, v3{0, 0, 0}, Scale, Context, State);
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

internal render_texture*
PushTexture (app_state* State)
{
    render_texture* Result = 0;
    
    if (State->LoadedTexturesUsed < State->LoadedTexturesSize)
    {
        Result = State->LoadedTextures + State->LoadedTexturesUsed++;
    }
    else
    {
        // TODO(Peter): Be able to grow this array
        for (s32 i = 0; i < State->LoadedTexturesUsed; i++)
        {
            if (State->LoadedTextures[i].Handle == 0)
            {
                Result = State->LoadedTextures + i;
            }
        }
    }
    
    Assert(Result);
    return Result;
}

internal render_texture*
StoreTexture (app_state* State, u8* Memory, s32 Width, s32 Height, s32 BytesPerPixel, s32 Stride)
{
    render_texture* Result = PushTexture(State);
    Result->Memory = Memory;
    Result->Handle = 0;
    Result->Width = Width;
    Result->Height = Height;
    Result->BytesPerPixel = BytesPerPixel;
    Result->Stride = Stride;
}

internal void
RemoveTexture (app_state* State, s32 Index)
{
    State->LoadedTextures[Index].Handle = 0;
    // TODO(Peter): Free the memory it was using
}

internal void
RemoveTexture (app_state* State, render_texture* Texture)
{
    Texture->Handle = 0;
    // TODO(Peter): Free the memory it was using
}

RELOAD_STATIC_DATA(ReloadStaticData)
{
    app_state* State = (app_state*)Context.MemoryBase;
    
    GlobalDebugServices = DebugServices;
    
    if (State->InputCommandRegistry.Size > 0)
    {
        RegisterKeyPressCommand(&State->InputCommandRegistry, KeyCode_Delete, false, KeyCode_Invalid,
                                DeleteSelectedChannelOrPattern);
        RegisterKeyPressCommand(&State->InputCommandRegistry, KeyCode_MouseLeftButton, true, KeyCode_Invalid,
                                CameraMouseControl);
        RegisterKeyPressCommand(&State->InputCommandRegistry, KeyCode_U, false, KeyCode_Invalid, ToggleUniverseDebugView);
        RegisterMouseWheelCommand(&State->InputCommandRegistry, CameraMouseZoom);
        RegisterKeyPressCommand(&State->InputCommandRegistry, KeyCode_A, false, KeyCode_Invalid, AddNode);
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
    
    State->LoadedTexturesSize = 8;
    State->LoadedTextures = PushArray(State->Permanent, render_texture, State->LoadedTexturesSize);
    State->LoadedTexturesUsed = 0;
    
    InitializeInputCommandRegistry(&State->InputCommandRegistry, 32, State->Permanent);
    
    // TODO(Peter): put in InitializeInterface?
    r32 FontSize = 14;
    {
        platform_memory_result FontFile = Context.PlatformReadEntireFile("Anonymous Pro.ttf");
        if (FontFile.Size)
        {
            stbtt_fontinfo StbFont;
            if (stbtt_InitFont(&StbFont, FontFile.Base, stbtt_GetFontOffsetForIndex(FontFile.Base, 0)))
            {
                bitmap_font* Font = PushStruct(State->Permanent, bitmap_font);
                Font->Atlas = PushTexture(State);
                RenderStbttToBitmapFont(Font, StbFont, FontSize, 512, State->Permanent, State->Transient );
                State->Interface.Font = Font;
                State->Font = Font;
                Font->Atlas->Handle = Context.PlatformGetGPUTextureHandle(Font->Atlas->Memory, Font->Atlas->Width, Font->Atlas->Height);
            }
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
    
    InitLEDPatternSystem(&State->PatternSystem, State->Permanent,
                         32, Megabytes(4));
    InitLEDChannelSystem(&State->ChannelSystem, State->Permanent,
                         sizeof(led_channel) * 32);
    
#if 1
    char Path[] = "radialumia.fold";
    LoadAssembly(State, Context, Path);
#endif
    
    State->InterfaceState.AddingPattern = false;
    State->InterfaceState.PatternSelectorStart = 0;
    State->InterfaceState.ChannelSelected = -1;
    
    State->InterfaceYMax = 200;
    State->PixelsToWorldScale = .01f;
    State->Camera_StartDragPos = {};
    
    State->UniverseOutputDisplayOffset = v2{0, 0};
    State->UniverseOutputDisplayZoom = 1.0f;
    
    GlobalDebugServices->Interface.RenderSculpture = true;
    
    State->NodeList = AllocateNodeList(State->Permanent, Kilobytes(64));
    
    State->NodeInteraction = NewNodeInteraction(); 
    State->NodeRenderSettings.PortDim = v2{20, 15};
    State->NodeRenderSettings.PortStep = State->NodeRenderSettings.PortDim.y + 10;
    State->NodeRenderSettings.PortColors[MemberType_r32] = RedV4;
    State->NodeRenderSettings.PortColors[MemberType_s32] = GreenV4;
    State->NodeRenderSettings.PortColors[MemberType_v4] = BlueV4;
    State->NodeRenderSettings.Font = State->Font;
    
    State->OutputNode = PushOutputNodeOnList(State->NodeList, v2{500, 250}, State->Transient);
    
    ReloadStaticData(Context, GlobalDebugServices);
}

UPDATE_AND_RENDER(UpdateAndRender)
{
    app_state* State = (app_state*)Context.MemoryBase;
    
    ExecuteAllRegisteredCommands(&State->InputCommandRegistry, Input, State);
    
    UpdateOutputNodeCalculations(State->OutputNode, State->NodeList, State->Transient, 
                                 State->LEDBufferList->LEDs,
                                 State->LEDBufferList->Colors, 
                                 State->LEDBufferList->Count);
    
    /*
    patterns_update_list Temp_PatternsNeedUpdate = UpdateAllChannels(&State->ChannelSystem,
                                                                     Context.DeltaTime,
                                                                     State->Transient);
                                                                     
    UpdateAllPatterns(&Temp_PatternsNeedUpdate, &State->PatternSystem, State->LEDBufferList, Context.DeltaTime, State->Transient);
    */
    
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
        
        RenderBuffer->ViewWidth = Context.WindowWidth;
        RenderBuffer->ViewHeight = Context.WindowHeight;
        
        r32 LEDHalfWidth = .5f;
        
        PushRenderPerspective(RenderBuffer, Context.WindowWidth, Context.WindowHeight, State->Camera);
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
        
        PushRenderOrthographic(RenderBuffer, Context.WindowWidth, Context.WindowHeight);
        
        // Universe Data View
        if (State->DrawUniverseOutputDisplay)
        {
            DEBUG_TRACK_SCOPE(DrawUniverseOutputDisplay);
            
            string TitleBarString = InitializeString(PushArray(State->Transient, char, 64), 64);
            
            v2 DisplayArea_Dimension = v2{600, 600};
            v2 DisplayContents_Offset = State->UniverseOutputDisplayOffset;
            v2 DisplayArea_TopLeft = v2{300, Context.WindowHeight - 50} + DisplayContents_Offset;
            v2 UniverseDisplayDimension = v2{100, 100} * State->UniverseOutputDisplayZoom;
            v2 Padding = v2{25, 50} * State->UniverseOutputDisplayZoom;
            
            v2 UniverseDisplayTopLeft = DisplayArea_TopLeft;
            
            sacn_universe_buffer* UniverseList = State->SACN.UniverseBuffer;
            while(UniverseList)
            {
                for (s32 UniverseIdx = 0;
                     UniverseIdx < UniverseList->Used;
                     UniverseIdx++)
                {
                    sacn_universe* Universe = UniverseList->Universes + UniverseIdx;
                    
                    DrawSACNUniversePixels(RenderBuffer, Universe, 
                                           UniverseDisplayTopLeft, UniverseDisplayDimension);
                    
                    if (State->UniverseOutputDisplayZoom > .5f)
                    {
                        v2 TitleDisplayStart = UniverseDisplayTopLeft + v2{0, 12};
                        PrintF(&TitleBarString, "Universe %d", Universe->Universe);
                        DrawString(RenderBuffer, TitleBarString, State->Interface.Font, 12, 
                                   TitleDisplayStart, WhiteV4);
                    }
                    
                    UniverseDisplayTopLeft.x += UniverseDisplayDimension.x + Padding.x;
                    if (UniverseDisplayTopLeft.x > DisplayArea_TopLeft.x + DisplayArea_Dimension.x)
                    {
                        UniverseDisplayTopLeft.x = DisplayArea_TopLeft.x;
                        UniverseDisplayTopLeft.y -= UniverseDisplayDimension.y + Padding.y;
                    }
                    
                    if (UniverseDisplayTopLeft.y < DisplayArea_TopLeft.y - DisplayArea_Dimension.y)
                    {
                        break;
                    }
                }
                UniverseList = UniverseList->Next;
            }
        }
        
        ///////////////////////////////////////
        //     Menu Bar
        //////////////////////////////////////
        
        r32 TopBarHeight = 40;
        {
            panel_result TopBarPanel = EvaluatePanel(RenderBuffer, 
                                                     v2{0, Context.WindowHeight - TopBarHeight},
                                                     v2{Context.WindowWidth, Context.WindowHeight},
                                                     0, State->Interface, Input);
            
            v2 ButtonDim = v2{200, State->Interface.Font->NewLineYOffset + 10};
            v2 ButtonPos = v2{State->Interface.Margin.x, Context.WindowHeight - (ButtonDim.y + 10)};
            button_result LoadAssemblyBtn = EvaluateButton(RenderBuffer, ButtonPos, ButtonPos + ButtonDim, 
                                                           MakeStringLiteral("Load Assembly"), 
                                                           State->Interface, Input);
            
            string InterfaceString = MakeString(PushArray(State->Transient, char, 256), 256);
            for (int i = 0; i < State->AssembliesUsed; i++)
            {
                PrintF(&InterfaceString, "Unload %.*s", State->AssemblyList[i].Name.Length, State->AssemblyList[i].Name.Memory);
                
                ButtonPos.x += ButtonDim.x + 10;
                button_result UnloadAssemblyBtn = EvaluateButton(RenderBuffer, ButtonPos, ButtonPos + ButtonDim,
                                                                 InterfaceString, State->Interface, Input);
                
                if (UnloadAssemblyBtn.Pressed)
                {
                    UnloadAssembly(i, State, Context);
                }
            }
            
            if (LoadAssemblyBtn.Pressed)
            {
                char FilePath[256];
                b32 Success = Context.PlatformGetFilePath(FilePath, 256);
                if (Success)
                {
                    LoadAssembly(State, Context, FilePath);
                }
            }
        }
        
        ///////////////////////////////////////
        //    Figuring Out Nodes
        //////////////////////////////////////
        {
            v2 MousePos = v2{(r32)Input.New->MouseX, (r32)Input.New->MouseY};
            v2 LastFrameMousePos = v2{(r32)Input.Old->MouseX, (r32)Input.Old->MouseY};
            
            if (KeyTransitionedDown(Input, KeyCode_MouseLeftButton))
            {
                node_offset Node = GetNodeUnderPoint(State->NodeList, MousePos, State->NodeRenderSettings);
                if (Node.Node)
                {
                    State->NodeInteraction = GetNodeInteractionType(Node.Node, Node.Offset, MousePos, State->NodeRenderSettings);
                }
            }
            else if (KeyTransitionedUp(Input, KeyCode_MouseLeftButton))
            {
                if (IsDraggingNodePort(State->NodeInteraction))
                {
                    TryConnectNodes(State->NodeInteraction, MousePos, State->NodeList, State->NodeRenderSettings);
                }
                State->NodeInteraction = NewNodeInteraction();
            }
            
            UpdateDraggingNode(MousePos, State->NodeInteraction, State->NodeList, 
                               State->NodeRenderSettings);
            UpdateDraggingNodePort(MousePos, State->NodeInteraction, State->NodeList, 
                                   State->NodeRenderSettings, RenderBuffer);
            UpdateDraggingNodeValue(MousePos, LastFrameMousePos, State->NodeInteraction, State->NodeList, State->NodeRenderSettings, State);
            
            RenderNodeList(State->NodeList, State->NodeRenderSettings, RenderBuffer);
            
            ResetNodesUpdateState(State->NodeList);
            
            if (State->InterfaceShowNodeList)
            {
                v2 TopLeft = State->NodeListMenuPosition;
                
                // Title Bar
                PushRenderQuad2D(RenderBuffer, v2{TopLeft.x, TopLeft.y - 30}, v2{TopLeft.x + 300, TopLeft.y}, 
                                 v4{.3f, .3f, .3f, 1.f});
                DrawString(RenderBuffer, MakeStringLiteral("Nodes List"), State->Font, 14, 
                           v2{TopLeft.x, TopLeft.y - 25}, WhiteV4);
                TopLeft.y -= 30;
                
                for (s32 i = 0; i < NodeSpecificationsCount; i++)
                {
                    node_specification Spec = NodeSpecifications[i];
                    
                    button_result Button = EvaluateButton(RenderBuffer, v2{TopLeft.x, TopLeft.y - 30}, v2{TopLeft.x + 300, TopLeft.y}, 
                                                          MakeStringLiteral(Spec.Name), State->Interface, Input);
                    if (Button.Pressed)
                    {
                        PushNodeOnListFromSpecification(State->NodeList, Spec, MousePos, State->Permanent);
                    }
                    
                    TopLeft.y -= 30;
                }
                
                if (KeyTransitionedDown(Input, KeyCode_MouseLeftButton) ||
                    KeyTransitionedDown(Input, KeyCode_Esc))
                {
                    State->InterfaceShowNodeList = false;
                }
            }
        }
        
        if (State->ColorPickerEditValue != 0)
        {
            b32 ShouldClose = EvaluateColorPicker(RenderBuffer, State->ColorPickerEditValue, 
                                                  v2{200, 200}, State->Interface, Input);
            
            if (ShouldClose ||
                KeyTransitionedDown(Input, KeyCode_Esc))
            {
                State->ColorPickerEditValue = 0;
            }
        }
        
#if 0
        ///////////////////////////////////////
        //     Current Patterns Panel
        //////////////////////////////////////
        
        r32 LeftPanelRightEdge = DrawLeftHandInterface(State, Input, Context.WindowHeight - TopBarHeight, RenderBuffer);
        
        if (State->InterfaceState.ChannelSelected >= 0)
        {
            led_channel* ActiveChannel = GetChannelByIndex(State->InterfaceState.ChannelSelected,
                                                           State->ChannelSystem);
            
            button_result OperationButtonState = EvaluateButton(
                RenderBuffer,
                v2{Context.WindowWidth - 150, 500},
                v2{Context.WindowWidth - 50, 550},
                MakeStringLiteral(PatternSelectorOperationsText[ActiveChannel->BlendMode]),
                State->Interface,
                Input);
            
            if (OperationButtonState.Pressed)
            {
                State->InterfaceState.ChooseOperationPanelOpen = !State->InterfaceState.ChooseOperationPanelOpen;
            }
            
            if (State->InterfaceState.ChooseOperationPanelOpen)
            {
                s32 StringLength = 128;
                s32 OperationsStart = PatternSelectorCombine_Invalid + 1;
                s32 OperationsOnePastLast = PatternSelectorCombine_Count;
                s32 OperationsCount = (OperationsOnePastLast - OperationsStart);
                string* OperationChoices = PushArray(State->Transient, string, OperationsCount);
                
                for (s32 Choice = OperationsStart;
                     Choice < OperationsOnePastLast;
                     Choice++)
                {
                    s32 Index = Choice - OperationsStart;
                    PushString(&OperationChoices[Index], State->Transient, StringLength);
                    CopyCharArrayToString(PatternSelectorOperationsText[Choice],
                                          &OperationChoices[Index]);
                }
                
                v2 Min = v2{Context.WindowWidth - 250, 250};
                v2 Max = v2{Context.WindowWidth - 50, 500};
                
                scroll_list_result OperationChoice = DrawSelectableOptionsList(RenderBuffer, Min, Max, OperationChoices, OperationsCount,
                                                                               0, ActiveChannel->BlendMode - 1,
                                                                               State->Interface, Input);
                if (OperationChoice.IndexSelected + 1 > (int)ChannelBlend_Invalid &&
                    OperationChoice.IndexSelected + 1 < (int)ChannelBlend_Count)
                {
                    ActiveChannel->BlendMode = (channel_blend_mode)(OperationChoice.IndexSelected + 1);
                }
            }
        }
        
#endif
        
        DrawDebugInterface(RenderBuffer, 25,
                           State->Interface, Context.WindowWidth, Context.WindowHeight - TopBarHeight,
                           Context.DeltaTime, State->Camera, Input, State->Transient);
    }
    
    ClearArena(State->Transient);
    EndDebugFrame(GlobalDebugServices);
}

CLEANUP_APPLICATION(CleanupApplication)
{
    app_state* State = (app_state*)Context.MemoryBase;
    SACNCleanup(&State->SACN, Context);
}