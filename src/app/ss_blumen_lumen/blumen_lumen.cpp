//
// File: blumen_lumen.cpp
// Author: Peter Slattery
// Creation Date: 2021-01-23
//
#ifndef BLUMEN_LUMEN_CPP

internal animation_handle_array
LoadAllAnimationsInDir(gs_const_string Path, blumen_lumen_state* BLState, app_state* State, context Context)
{
    animation_handle_array Result = {};
    
    gs_thread_context Ctx = Context.ThreadContext;
    gs_file_info_array FilesInDir = EnumerateDirectory(Ctx.FileHandler, State->Transient, Path, 0);
    
    Result.Count = FilesInDir.Count;
    Result.Handles = PushArray(&State->Permanent, animation_handle, Result.Count);
    
    for (u32 i = 0; i < FilesInDir.Count; i++)
    {
        gs_file_info File = FilesInDir.Values[i];
        Result.Handles[i] = AnimationSystem_LoadAnimationFromFile(&State->AnimationSystem,
                                                                  State->Patterns,
                                                                  Context,
                                                                  File.Path);
    }
    
    return Result;
}

internal s32
GetCCIndex (assembly Assembly, blumen_lumen_state* BLState)
{
    s32 Result = 0;
    
    u64 AssemblyNameHash = HashDJB2ToU32(StringExpand(Assembly.Name));
    for (u32 i = 0; i < BLState->AssemblyNameToClearCoreMapCount; i++)
    {
        if (AssemblyNameHash == BLState->AssemblyNameToClearCore_Names[i])
        {
            Result = (s32)i;
            break;
        }
    }
    
    return Result;
}

internal void
DEBUG_AppendText(gs_string Str, gs_thread_context Ctx)
{
    gs_const_string DebugPath = ConstString("data/debug_motor_changes.txt");
    gs_file DebugFile = ReadEntireFile(Ctx.FileHandler,
                                       DebugPath);
    gs_string NewString = PushString(Ctx.Transient, DebugFile.Size + Str.Size + 16);
    if (DebugFile.Size > 0)
    {
        PrintF(&NewString, "%.*s\nENTRY:\n", DebugFile.Size, (char*)DebugFile.Memory);
    }
    AppendPrintF(&NewString, "%S\n", Str.ConstString);
    NullTerminate(&NewString);
    
    if (!WriteEntireFile(Ctx.FileHandler, DebugPath, StringToData(NewString)))
    {
        InvalidCodePath;
    }
}

internal void
DEBUG_SentMotorCommand(motor_packet Packet, gs_thread_context Ctx)
{
    gs_string Str = PushStringF(Ctx.Transient, 256, "Motor Command Sent\nRequested Positions: %d %d %d\n", 
                                Packet.FlowerPositions[0],
                                Packet.FlowerPositions[1],
                                Packet.FlowerPositions[2]);
    DEBUG_AppendText(Str, Ctx);
    
    NullTerminate(&Str);
    OutputDebugStringA(Str.Str);
}

internal void
DEBUG_ReceivedMotorPositions(motor_packet NewPos, 
                             motor_packet LastPos, 
                             gs_thread_context Ctx)
{
    bool PosChanged = (LastPos.FlowerPositions[0] != NewPos.FlowerPositions[0] ||
                       LastPos.FlowerPositions[1] != NewPos.FlowerPositions[1] ||
                       LastPos.FlowerPositions[2] != NewPos.FlowerPositions[2]);
    
    if (PosChanged) 
    {
        gs_string Str = PushStringF(Ctx.Transient, 256, "Motor Status Received\nCurrent Positions: %d %d %d\n", 
                                    NewPos.FlowerPositions[0],
                                    NewPos.FlowerPositions[1],
                                    NewPos.FlowerPositions[2]);
        DEBUG_AppendText(Str, Ctx);
        
        NullTerminate(&Str);
        OutputDebugStringA(Str.Str);
    }
}

internal void
DEBUG_ReceivedTemperature(temp_packet Temp, gs_thread_context Ctx)
{
    gs_string TempStr = PushStringF(Ctx.Transient, 256, 
                                    "\nTemperature: %d\n",
                                    Temp.Temperature);
    NullTerminate(&TempStr);
    OutputDebugStringA(TempStr.Str);
}

internal void
BlumenLumen_MicListenJob(gs_thread_context* Ctx, u8* UserData)
{
    mic_listen_job_data* Data = (mic_listen_job_data*)UserData;
    
    gs_data Msg = {};
    
    u8 WeathermanIPAddr[4] = {};
    WeathermanIPAddr[0] = 127;
    WeathermanIPAddr[1] = 0;
    WeathermanIPAddr[2] = 0;
    WeathermanIPAddr[3] = 1;
    
    u32 WeathermanIPV4 = (u32)UpackB4(WeathermanIPAddr);
    u32 WeathermanPort = 20185;
    
    platform_socket_handle_ ListenSocket = {0};
    
    while (*Data->Running)
    {
        if (!SocketQueryStatus(Data->SocketManager, ListenSocket))
        {
            if (SocketHandleIsValid(ListenSocket))
            {
                OutputDebugStringA("Disconnected from Python Server\n");
                CloseSocket(Data->SocketManager, ListenSocket);
            }
            ListenSocket = CreateSocket(Data->SocketManager, "127.0.0.1", "20185");
            if (ListenSocket.Index != 0)
            {
                OutputDebugStringA("Connected to Python Server\n");
            }
        }
        
        if (SocketQueryStatus(Data->SocketManager, ListenSocket))
        {
            if (SocketPeek(Data->SocketManager, ListenSocket))
            {
                // TODO(pjs): Make this a peek operation
                Msg = SocketRecieve(Data->SocketManager, ListenSocket, Ctx->Transient);
                if (Msg.Size > 0)
                {
                    MessageQueue_Write(Data->IncomingMsgQueue, Msg);
                }
            }
            
            while (MessageQueue_CanRead(*Data->OutgoingMsgQueue))
            {
                Msg = MessageQueue_Read(Data->OutgoingMsgQueue);
                
                u32 Address = WeathermanIPV4;
                u32 Port = WeathermanPort;
                s32 Flags = 0;
                SocketSend(Data->SocketManager, ListenSocket, Address, Port, Msg, Flags);
            }
        }
        
        MessageQueue_Clear(Data->OutgoingMsgQueue);
    }
    
    CloseSocket(Data->SocketManager, ListenSocket);
}

internal void
BlumenLumen_LoadPatterns(app_state* State)
{
    animation_pattern_array* Patterns = &State->Patterns;
    if (Patterns->CountMax == 0)
    {
        *Patterns = Patterns_Create(&State->Permanent, 32);
    }
    
    Patterns->Count = 0;
    Patterns_PushPattern(Patterns, TestPatternOne, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, TestPatternTwo, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, TestPatternThree, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_AllGreen, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_HueShift, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_HueFade, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_Spots, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_LighthouseRainbow, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_SmoothGrowRainbow, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_GrowAndFade, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_ColorToWhite, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_Blue, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_Green, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_FlowerColors, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_FlowerColorToWhite, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_BasicFlowers, PATTERN_MULTITHREADED);
    // 15
    Patterns_PushPattern(Patterns, Pattern_Wavy, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_Patchy, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_Leafy, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_LeafyPatchy, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_WavyPatchy, PATTERN_SINGLETHREADED);
}

internal gs_data
BlumenLumen_CustomInit(app_state* State, context Context)
{
    // This is memory for any custom data that we want to use
    // as a part of a particular sculpture.
    // By returning it from here, it will be sent as an argument to
    // the sculpture's CustomUpdate function;
    gs_data Result = {};
    
    Result = PushSizeToData(&State->Permanent, sizeof(blumen_lumen_state));
    
    blumen_lumen_state* BLState = (blumen_lumen_state*)Result.Memory;
    BLState->Running = true;
    BLState->BrightnessPercent = 1;
    MessageQueue_Init(&BLState->IncomingMsgQueue, &State->Permanent);
    MessageQueue_Init(&BLState->OutgoingMsgQueue, &State->Permanent);
    
    BLState->MicListenJobData.Running = &BLState->Running;
    BLState->MicListenJobData.SocketManager = Context.SocketManager;
    BLState->MicListenJobData.IncomingMsgQueue = &BLState->IncomingMsgQueue;
    BLState->MicListenJobData.OutgoingMsgQueue = &BLState->OutgoingMsgQueue;
    
#if 1
    BLState->MicListenThread = CreateThread(Context.ThreadManager, BlumenLumen_MicListenJob, (u8*)&BLState->MicListenJobData);
#endif
    
    assembly* Flower0 = LoadAssembly(Flower0AssemblyPath, State, Context);
    assembly* Flower1 = LoadAssembly(Flower1AssemblyPath, State, Context);
    assembly* Flower2 = LoadAssembly(Flower2AssemblyPath, State, Context);
    
    BLState->AssemblyNameToClearCoreMapCount = 3;
    BLState->AssemblyNameToClearCore_Names = PushArray(&State->Permanent, 
                                                       u64,
                                                       BLState->AssemblyNameToClearCoreMapCount);
    BLState->AssemblyNameToClearCore_Names[0] = HashDJB2ToU32(StringExpand(Flower2->Name));
    BLState->AssemblyNameToClearCore_Names[1] = HashDJB2ToU32(StringExpand(Flower1->Name));
    BLState->AssemblyNameToClearCore_Names[2] = HashDJB2ToU32(StringExpand(Flower0->Name));
    
    gs_file_handler FileHandler = Context.ThreadContext.FileHandler;
    gs_file ColorPhraseCSVFile = ReadEntireFile(FileHandler, PhraseMapCSVPath);
    gs_const_string ColorPhraseMapStr = DataToString(ColorPhraseCSVFile.Data);
    gscsv_sheet ColorPhraseSheet = CSV_Parse(ColorPhraseMapStr, 
                                             { PhraseMapCSVSeparator },
                                             State->Transient);
    
    BLState->PhraseHueMap = PhraseHueMap_GenFromCSV(ColorPhraseSheet, 
                                                    &State->Permanent);
    
#if 0
    { // Animation PLAYGROUND
        animation_desc Desc = {};
        Desc.NameSize = 256;
        Desc.LayersCount = 8;
        Desc.BlocksCount = 8;
        Desc.MinFrames = 0;
        Desc.MaxFrames = SecondsToFrames(15, State->AnimationSystem);
        
        animation_desc Desc0 = Desc;
        Desc.Name = "test_anim_zero";
        animation Anim0 = Animation_Create(Desc0, &State->AnimationSystem);
        Animation_AddLayer(&Anim0, MakeString("Base Layer"), BlendMode_Overwrite, &State->AnimationSystem);
        Animation_AddBlock(&Anim0, 0, Anim0.PlayableRange.Max, Patterns_IndexToHandle(15), 0);
        BLState->AnimHandles[0] = AnimationArray_Push(&State->AnimationSystem.Animations, Anim0);
        
        animation_desc Desc1 = Desc;
        Desc1.Name = "test_anim_one";
        animation Anim1 = Animation_Create(Desc1, &State->AnimationSystem);
        Animation_AddLayer(&Anim1, MakeString("Base Layer"), BlendMode_Overwrite, &State->AnimationSystem);
        Animation_AddBlock(&Anim1, 0, Anim0.PlayableRange.Max, Patterns_IndexToHandle(12), 0);
        BLState->AnimHandles[1] = AnimationArray_Push(&State->AnimationSystem.Animations, Anim1);
        
        animation_desc Desc2 = Desc;
        Desc2.Name = "i_love_you";
        animation Anim2 = Animation_Create(Desc2, &State->AnimationSystem);;
        Animation_AddLayer(&Anim2, MakeString("Base Layer"), BlendMode_Overwrite, &State->AnimationSystem);
        Animation_AddBlock(&Anim2, 0, Anim0.PlayableRange.Max, Patterns_IndexToHandle(20), 0);
        BLState->AnimHandles[2] = AnimationArray_Push(&State->AnimationSystem.Animations, Anim2);
        
        State->AnimationSystem.ActiveFadeGroup.From = BLState->AnimHandles[2];
    } // End Animation Playground
#elif 0
    animation_handle DemoPatternsAnim = AnimationSystem_LoadAnimationFromFile(&State->AnimationSystem,
                                                                              State->Patterns,
                                                                              Context,
                                                                              ConstString("data/demo_patterns.foldanim"));
    State->AnimationSystem.ActiveFadeGroup.From = DemoPatternsAnim;
#else
    BLState->ModeAnimations[BlumenPattern_Standard] = LoadAllAnimationsInDir(AmbientPatternFolder, BLState, State, Context);
    BLState->ModeAnimations[BlumenPattern_VoiceCommand] = LoadAllAnimationsInDir(VoicePatternFolder, BLState, State, Context);
    
    State->AnimationSystem.ActiveFadeGroup.From = BLState->ModeAnimations[BlumenPattern_Standard].Handles[0];
#endif
    State->AnimationSystem.TimelineShouldAdvance = true;
    
    return Result;
}

internal void
BlumenLumen_CustomUpdate(gs_data UserData, app_state* State, context* Context)
{
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData.Memory;
    
    bool SendMotorCommand = false;
    blumen_packet MotorCommand = {};
    
    while (MessageQueue_CanRead(BLState->IncomingMsgQueue))
    {
        gs_data PacketData = MessageQueue_Read(&BLState->IncomingMsgQueue);
        
        blumen_packet Packet = *(blumen_packet*)PacketData.Memory;
        switch (Packet.Type) {
            case PacketType_PatternCommand:
            {
                microphone_packet Mic = Packet.MicPacket;
                u64 NameHash = HashDJB2ToU32(Mic.AnimationFileName);
                u32 NameLen = CStringLength(Mic.AnimationFileName);
                
                phrase_hue NewHue = PhraseHueMap_Get(BLState->PhraseHueMap, NameHash);
                if (NewHue.PhraseHash != 0)
                {
                    if (BLState->PatternMode == BlumenPattern_Standard)
                    {
                        BLState->AssemblyColors[0] = NewHue;
                        BLState->AssemblyColors[1] = NewHue;
                        BLState->AssemblyColors[2] = NewHue;
                        
                        animation_handle NewAnim = BLState->ModeAnimations[BlumenPattern_VoiceCommand].Handles[0];
                        AnimationFadeGroup_FadeTo(&State->AnimationSystem.ActiveFadeGroup,
                                                  NewAnim,
                                                  VoiceCommandFadeDuration);
                    }
                    else
                    {
                        u32 AssemblyIdx = BLState->LastAssemblyColorSet;
                        BLState->AssemblyColors[AssemblyIdx] = NewHue;
                    }
                    
                    BLState->PatternMode = BlumenPattern_VoiceCommand;
                    // TODO(PS): get current time so we can fade back after
                    // a while
                }
            }break;
            
            case PacketType_MotorState:
            {
                motor_status_packet Motor = Packet.MotorStatusPacket;
                
                // NOTE(pjs): Python sends multi-byte integers in little endian
                // order. Have to unpack
                u8* T = (u8*)&Motor.Temperature;
                Motor.Temperature = (T[0] << 8 |
                                     T[1] << 0);
                
                motor_packet LastPos = BLState->LastKnownMotorState;
                DEBUG_ReceivedMotorPositions(LastPos, Motor.Pos, Context->ThreadContext);
                BLState->LastKnownMotorState = Motor.Pos;
                
            }break;
            
            case PacketType_Temperature:
            {
                temp_packet Temp = Packet.TempPacket;
                
                if (Temp.Temperature > 0)
                {
                    BLState->BrightnessPercent = .25f;
                }
                else
                {
                    BLState->BrightnessPercent = 1.f;
                }
                
                DEBUG_ReceivedTemperature(Temp, Context->ThreadContext);
            }break;
            
            InvalidDefaultCase;
        }
    }
    
    
    // Open / Close the Motor
    if (MessageQueue_CanWrite(BLState->OutgoingMsgQueue))
    {
        for (u32 i = 0; i < MotorOpenTimesCount; i++)
        {
            time_range Range = MotorOpenTimes[i];
            
            bool CurrTimeInRange = SystemTimeIsInTimeRange(Context->SystemTime_Current, Range);
            
            bool LastSendTimeInRange = SystemTimeIsInTimeRange(BLState->LastSendTime, Range);
            
            bool LastTimeInRange = SystemTimeIsInTimeRange(Context->SystemTime_Last, Range);
            
            bool SendOpen = CurrTimeInRange && !LastSendTimeInRange;
            bool SendClose = !CurrTimeInRange && LastSendTimeInRange;
            
            //SendOpen = SecondsSinceLastSend > 2;
            if (SendOpen)
            {
                SendMotorCommand = true;
                
                BLState->LastSendTime = Context->SystemTime_Current;
                OutputDebugString("Motors: Open\n");
                
                blumen_packet Packet = {};
                Packet.Type = PacketType_MotorState;
                Packet.MotorPacket.FlowerPositions[0] = 2;
                Packet.MotorPacket.FlowerPositions[1] = 2;
                Packet.MotorPacket.FlowerPositions[2] = 2;
                MotorCommand = Packet;
            }
            else if (SendClose)
            {
                SendMotorCommand = true;
                BLState->LastSendTime = Context->SystemTime_Current;
                OutputDebugString("Motors: Close\n");
                
                blumen_packet Packet = {};
                Packet.Type = PacketType_MotorState;
                Packet.MotorPacket.FlowerPositions[0] = 1;
                Packet.MotorPacket.FlowerPositions[1] = 1;
                Packet.MotorPacket.FlowerPositions[2] = 1;
                MotorCommand = Packet;
            }
        }
        
        if (SendMotorCommand)
        {
            gs_data Msg = StructToData(&MotorCommand, blumen_packet);
            MessageQueue_Write(&BLState->OutgoingMsgQueue, Msg);
            DEBUG_SentMotorCommand(MotorCommand.MotorPacket, Context->ThreadContext);
        }
    }
    // Dim the leds based on temp data
    for (u32 i = 0; i < State->LedSystem.BuffersCount; i++)
    {
        led_buffer Buffer = State->LedSystem.Buffers[i];
        for (u32 j = 0; j < Buffer.LedCount; j++)
        {
            pixel* Color = Buffer.Colors + j;
            Color->R = Color->R * BLState->BrightnessPercent;
            Color->G = Color->G * BLState->BrightnessPercent;
            Color->B = Color->B * BLState->BrightnessPercent;
        }
    }
    
    // NOTE(PS): If the flowers are mostly open or full open
    // we mask off the top leds to prevent them from overheating
    // while telescoped inside the flower
    motor_packet CurrMotorPos = BLState->LastKnownMotorState;
    for (u32 a = 0; a < State->Assemblies.Count; a++)
    {
        assembly Assembly = State->Assemblies.Values[a];
        u64 AssemblyCCIndex = GetCCIndex(Assembly, BLState);
        
        u8 MotorPos = CurrMotorPos.FlowerPositions[AssemblyCCIndex];
        
        if (MotorPos == MotorState_Closed || 
            MotorPos == MotorState_HalfOpen) 
        {
            continue;
        }
        
        led_buffer Buffer = State->LedSystem.Buffers[Assembly.LedBufferIndex];
        
        led_strip_list TopStrips = AssemblyStripsGetWithTagValue(Assembly, ConstString("section"), ConstString("inner_bloom"), State->Transient);
        for (u32 s = 0; s < TopStrips.Count; s++)
        {
            u32 SIndex = TopStrips.StripIndices[s];
            v2_strip Strip = Assembly.Strips[SIndex];
            for (u32 l = 0; l < Strip.LedCount; l++)
            {
                u32 LIndex = Strip.LedLUT[l];
                Buffer.Colors[LIndex] = {0};
            }
        }
    }
    
    // Send Status Packet
    {
        system_time LastSendTime = BLState->LastStatusUpdateTime;
        r64 NanosSinceLastSend = ((r64)Context->SystemTime_Current.NanosSinceEpoch - (r64)LastSendTime.NanosSinceEpoch);
        r64 SecondsSinceLastSend = NanosSinceLastSend / PowR32(10, 8);
        if (SecondsSinceLastSend >= STATUS_PACKET_FREQ_SECONDS)
        {
            BLState->LastStatusUpdateTime = Context->SystemTime_Current;
            OutputDebugString("Attempting to Send Lumenarium Status\n");
            
            blumen_packet Packet = {};
            Packet.Type = PacketType_LumenariumStatus;
            Packet.StatusPacket.NextMotorEventType = 0;
            Packet.StatusPacket.NextEventTime = 0;
            
            animation* ActiveAnim = AnimationSystem_GetActiveAnimation(&State->AnimationSystem);
            if (ActiveAnim)
            {
                CopyMemoryTo(ActiveAnim->Name.Str, Packet.StatusPacket.AnimFileName,
                             Min(ActiveAnim->Name.Length, 32));
                Packet.StatusPacket.AnimFileName[ActiveAnim->Name.Length] = 0;
            }
            
            gs_data Msg = StructToData(&Packet, blumen_packet);
            MessageQueue_Write(&BLState->OutgoingMsgQueue, Msg);
        }
    }
}

US_CUSTOM_DEBUG_UI(BlumenLumen_DebugUI)
{
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData.Memory;
    ui_interface* I = &State->Interface;
    
    // NOTE(PS): Motors
    {
        motor_packet PendingPacket = BLState->DEBUG_PendingMotorPacket;
        
        for (u32 MotorIndex = 0; MotorIndex < 3; MotorIndex++)
        {
            gs_string Label = PushStringF(State->Transient, 32, "Motor %d", MotorIndex);
            ui_BeginRow(I, 5);
            {
                ui_Label(I, Label);
                
                bool IsClosed = PendingPacket.FlowerPositions[MotorIndex] == 1;
                if (ui_ToggleText(I, MakeString("Closed (1)"), IsClosed))
                {
                    PendingPacket.FlowerPositions[MotorIndex] = 1;
                }
                bool IsHOpen = PendingPacket.FlowerPositions[MotorIndex] == 3;
                if (ui_ToggleText(I, MakeString("Half Open (3)"), IsHOpen))
                {
                    PendingPacket.FlowerPositions[MotorIndex] = 3;
                }
                bool IsMOpen = PendingPacket.FlowerPositions[MotorIndex] == 4;
                if (ui_ToggleText(I, MakeString("Mostly Open (4)"), IsMOpen))
                {
                    PendingPacket.FlowerPositions[MotorIndex] = 4;
                }
                bool IsOpen = PendingPacket.FlowerPositions[MotorIndex] == 2;
                if (ui_ToggleText(I, MakeString("Open (2)"), IsOpen))
                {
                    PendingPacket.FlowerPositions[MotorIndex] = 2;
                }
            }
            ui_EndRow(I);
        }
        BLState->DEBUG_PendingMotorPacket = PendingPacket;
        
        if (ui_Button(I, MakeString("Send Motor Packet")))
        {
            blumen_packet Packet = {};
            Packet.Type = PacketType_MotorState;
            Packet.MotorPacket = BLState->DEBUG_PendingMotorPacket;
            gs_data Msg = StructToData(&Packet, blumen_packet);
            MessageQueue_Write(&BLState->OutgoingMsgQueue, Msg);
            
            DEBUG_SentMotorCommand(Packet.MotorPacket, Context.ThreadContext);
        }
        
    }
}

US_CUSTOM_CLEANUP(BlumenLumen_CustomCleanup)
{
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData.Memory;
    BLState->Running = false;
}

internal user_space_desc
BlumenLumen_UserSpaceCreate()
{
    user_space_desc Result = {};
    Result.LoadPatterns = BlumenLumen_LoadPatterns;
    Result.CustomInit = BlumenLumen_CustomInit;
    Result.CustomUpdate = BlumenLumen_CustomUpdate;
    Result.CustomDebugUI = BlumenLumen_DebugUI;
    Result.CustomCleanup = BlumenLumen_CustomCleanup;
    return Result;
}

#define BLUMEN_LUMEN_CPP
#endif // BLUMEN_LUMEN_CPP
