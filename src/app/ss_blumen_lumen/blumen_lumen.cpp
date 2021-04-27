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
    Log_Message(GlobalLogBuffer, 
                "Motor Command Sent\nRequested Positions: %d %d %d\n", 
                Packet.FlowerPositions[0],
                Packet.FlowerPositions[1],
                Packet.FlowerPositions[2]);
}

internal void
DEBUG_ReceivedMotorPositions(blumen_lumen_state* BLState, 
                             motor_status_packet NewPos, 
                             gs_thread_context Ctx)
{
    motor_packet LastPos = BLState->LastKnownMotorState;
    bool PosChanged = (LastPos.FlowerPositions[0] != NewPos.Pos.FlowerPositions[0] ||
                       LastPos.FlowerPositions[1] != NewPos.Pos.FlowerPositions[1] ||
                       LastPos.FlowerPositions[2] != NewPos.Pos.FlowerPositions[2]);
    
    if (PosChanged) 
    {
        Log_Message(GlobalLogBuffer, 
                    "Motor Status Received\nCurrent Positions: %d %d %d\n", 
                    NewPos.Pos.FlowerPositions[0],
                    NewPos.Pos.FlowerPositions[1],
                    NewPos.Pos.FlowerPositions[2]);
    }
}

internal void
DEBUG_ReceivedTemperature(temp_packet Temp, gs_thread_context Ctx)
{
    Log_Message(GlobalLogBuffer, 
                "\nTemperature: %d\n",
                Temp.Temperature);
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
            Data->IsConnected = false;
            if (SocketHandleIsValid(ListenSocket))
            {
                Log_Message(GlobalLogBuffer, "Disconnected from Python Server\n");
                CloseSocket(Data->SocketManager, ListenSocket);
            }
            ListenSocket = CreateSocket(Data->SocketManager, "127.0.0.1", "20185");
            if (ListenSocket.Index != 0)
            {
                Log_Message(GlobalLogBuffer, "Connected to Python Server\n");
                Data->IsConnected = true;
            }
        }
        
        if (SocketQueryStatus(Data->SocketManager, ListenSocket))
        {
            if (SocketPeek(Data->SocketManager, ListenSocket))
            {
                Msg = SocketRecieve(Data->SocketManager, ListenSocket, Ctx->Transient);
                if (Msg.Size > 0)
                {
                    MessageQueue_Write(Data->IncomingMsgQueue, Msg);
                }
            }
            
            while (MessageQueue_CanRead(*Data->OutgoingMsgQueue))
            {
                Msg = MessageQueue_Peek(Data->OutgoingMsgQueue);
                
                u32 Address = WeathermanIPV4;
                u32 Port = WeathermanPort;
                s32 Flags = 0;
                s32 LengthSent = SocketSend(Data->SocketManager, ListenSocket, Address, Port, Msg, Flags);
                if (LengthSent != 0)
                {
                    // if we actually sent the message, THEN we pull it off the
                    // message queue
                    MessageQueue_Read(Data->OutgoingMsgQueue);
                } else {
                    break;
                }
            }
        }
    }
    
    CloseSocket(Data->SocketManager, ListenSocket);
}

internal void
BlumenLumen_SetPatternMode(bl_pattern_mode Mode, r32 FadeDuration, animation_system* System, blumen_lumen_state* BLState)
{
    BLState->PatternMode = Mode;
    animation_handle_array Playlist = BLState->ModeAnimations[Mode];
    System->RepeatMode = AnimationRepeat_Loop;
    System->PlaylistFadeTime = FadeDuration;
    AnimationSystem_FadeToPlaylist(System, Playlist);
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
    Patterns_PushPattern(Patterns, Pattern_None, PATTERN_SINGLETHREADED);
    Patterns_PushPattern(Patterns, Pattern_HueShift, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_Rainbow, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_BasicFlowers, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_Wavy, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_Patchy, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_Leafy, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_LeafyPatchy, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_WavyPatchy, PATTERN_SINGLETHREADED);
    Patterns_PushPattern(Patterns, Pattern_VerticalLines, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_Rotary, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_AllOnMask, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_BulbMask, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_VoicePattern, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_VoiceAddIns, PATTERN_MULTITHREADED);
    
    Patterns_PushPattern(Patterns, Pattern_StemSolid, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_PrimaryHue, PATTERN_MULTITHREADED);
    
    Patterns_PushPattern(Patterns, Pattern_GrowFadeMask, PATTERN_MULTITHREADED);
    Patterns_PushPattern(Patterns, Pattern_RainbowLoadingBar, PATTERN_MULTITHREADED);
    
    Patterns_PushPattern(Patterns, Pattern_Blue, PATTERN_MULTITHREADED);
}

internal void
AppendPrintDate(gs_string* WriteStr, system_time Time)
{
    AppendPrintF(WriteStr, "%d-%d-%d : %d:%d:%d\n\n",
                 Time.Year, Time.Month, Time.Day,
                 Time.Hour, Time.Minute, Time.Second);
}

internal void
BlumenLumen_AppendBootupLog(app_state* State, blumen_lumen_state* BLState, context Context)
{
    gs_thread_context Ctx = Context.ThreadContext;
    gs_const_string BootupLogPath = ConstString("lumenarium_boot_log.log");
    
    gs_file BootLog = ReadEntireFile(Ctx.FileHandler, BootupLogPath);
    gs_string WriteStr = {};
    
    // we don't want the log file getting huge
    // if it gets above some threshold, instead of appending,
    // copy what there is to an _old file, and start this one over
    // 
    // The thinking is that without the copy operation, when we reached
    // our threshold, we'd overwrite the whole log. If something went
    // wrong at that point, we'd have nothing to go on. This way, there is
    // always some historical data present on the system
    // 
    if (BootLog.Size < MB(4))
    {
        WriteStr = PushString(State->Transient, BootLog.Size + 1024);
    }
    else
    {
        if (!WriteEntireFile(Ctx.FileHandler, ConstString("lumenarium_boot_log_old.log"),
                             BootLog.Data))
        {
            InvalidCodePath;
        }
        WriteStr = PushString(State->Transient, 1024);
    }
    
    
    // Copy old entries in
    if (BootLog.Size > 0)
    {
        PrintF(&WriteStr, "%.*s", BootLog.Size, BootLog.Memory);
    }
    
    // New Entry
    AppendPrintF(&WriteStr, "Lumenarium Restarted\n");
    AppendPrintF(&WriteStr, "* Time: ");
    AppendPrintDate(&WriteStr, Context.SystemTime_Current);
    
    gs_data Data = StringToData(WriteStr);
    WriteEntireFile(Ctx.FileHandler, BootupLogPath, Data);
}

internal void
BlumenLumen_UpdateLog(app_state* State, blumen_lumen_state* BLState, context Context)
{
    if (!BLState->ShouldUpdateLog) return;
    
    gs_string FileStr = PushString(State->Transient, 1024);
    AppendPrintF(&FileStr, "Lumenarium Status\n");
    
    AppendPrintF(&FileStr, "Last Updated At:");
    AppendPrintDate(&FileStr, Context.SystemTime_Current);
    AppendPrintF(&FileStr, "\n\n");
    
    animation* CurrAnim = AnimationSystem_GetActiveAnimation(&State->AnimationSystem);
    AppendPrintF(&FileStr, "Curr Animation: %S\n", CurrAnim->Name);
    
    bool IsPlaying = State->AnimationSystem.TimelineShouldAdvance;
    AppendPrintF(&FileStr, "\tIs Playing: %s\n", 
                 IsPlaying ? "True" : "False");
    
    char* Connected = BLState->MicListenJobData.IsConnected ? "Connected" : "Disconnected";
    AppendPrintF(&FileStr, "Connected to Python: %s\n", Connected);
    
    u8 MP0 = BLState->LastKnownMotorState.FlowerPositions[0];
    u8 MP1 = BLState->LastKnownMotorState.FlowerPositions[1];
    u8 MP2 = BLState->LastKnownMotorState.FlowerPositions[2];
    AppendPrintF(&FileStr, "Last Known Motor State: %d %d %d\n", MP0, MP1, MP2);
    
    time_range MotorRange = {};
    if (SystemTimeIsInTimeRangeList(Context.SystemTime_Current,
                                    MotorOpenTimes,
                                    MotorOpenTimesCount,
                                    &MotorRange))
    {
        AppendPrintF(&FileStr, "\tIn Motor-Open Time Range: ( %d:%d - %d:%d)\n",
                     MotorRange.StartHour, MotorRange.StartMinute,
                     MotorRange.EndHour, MotorRange.EndMinute);
    }
    else
    {
        AppendPrintF(&FileStr, "\tIn Motor-Open Time Range: None\n");
    }
    
    char* PatternMode = 0;
    switch (BLState->PatternMode)
    {
        case BlumenPattern_Standard: { PatternMode = "Standard"; } break;
        case BlumenPattern_VoiceCommand: { PatternMode = "Voice Command"; } break;
        case BlumenPattern_NoControl: { PatternMode = "No Control: Someone's doing the Awaken sequence!"; } break;
    }
    AppendPrintF(&FileStr, "Pattern Mode: %s\n", PatternMode);
    
    phrase_hue LastHuePhrase = BLState->LastHuePhrase;
    AppendPrintF(&FileStr, "Last Mic Phrase: %S\n", LastHuePhrase.Phrase);
    
    AppendPrintF(&FileStr, "Pattern Speed: %f\n", BLState->PatternSpeed);
    
    AppendPrintF(&FileStr, "Pattern Brightness: %f\n", BLState->BrightnessPercent);
    
    time_range RangeIn = {};
    if (SystemTimeIsInTimeRangeList(Context.SystemTime_Current,
                                    LedOnTimes,
                                    LedOnTimesCount,
                                    &RangeIn))
    {
        AppendPrintF(&FileStr, "\tIn Leds-On Time Range: ( %d:%d - %d:%d)\n",
                     RangeIn.StartHour, RangeIn.StartMinute,
                     RangeIn.EndHour, RangeIn.EndMinute);
    }
    else
    {
        AppendPrintF(&FileStr, "\tIn Leds-On Time Range: None\n");
    }
    
    AppendPrintF(&FileStr, "\tTemp Dimming: %s\n",
                 Blumen_TempShouldDimLeds(BLState) ? "On" : "Off");
    
    AppendPrintF(&FileStr, "Last Temp Received: %d\n", BLState->LastTemperatureReceived);
    
    gs_data LogMem = StringToData(FileStr);
    if (!WriteEntireFile(Context.ThreadContext.FileHandler, ConstString("lumenarium_status.log"), LogMem))
    {
        InvalidCodePath;
    }
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
    
    BLState->PatternSpeed = GlobalAnimSpeed;
    
#if 1
    BLState->MicListenThread = CreateThread(Context.ThreadManager, BlumenLumen_MicListenJob, (u8*)&BLState->MicListenJobData, Context.ThreadContext);
#endif
    
    assembly* Flower0 = LoadAssembly(Flower0AssemblyPath, State, Context);
    assembly* Flower1 = LoadAssembly(Flower1AssemblyPath, State, Context);
    assembly* Flower2 = LoadAssembly(Flower2AssemblyPath, State, Context);
    
    for (u32 i = 0; i < BL_FLOWER_COUNT; i++)
    {
        assembly Assembly = State->Assemblies.Values[i];
        BLState->StemStrips[Assembly.AssemblyIndex] = AssemblyStripsGetWithTagValue(Assembly, ConstString("section"), ConstString("stem"), &State->Permanent);
    }
    
    BLState->AssemblyNameToClearCoreMapCount = 3;
    BLState->AssemblyNameToClearCore_Names = PushArray(&State->Permanent, 
                                                       u64,
                                                       BLState->AssemblyNameToClearCoreMapCount);
    BLState->AssemblyNameToClearCore_Names[0] = HashDJB2ToU32(StringExpand(Flower2->Name));
    BLState->AssemblyNameToClearCore_Names[1] = HashDJB2ToU32(StringExpand(Flower1->Name));
    BLState->AssemblyNameToClearCore_Names[2] = HashDJB2ToU32(StringExpand(Flower0->Name));
    
    gs_file_handler FileHandler = Context.ThreadContext.FileHandler;
    gs_file ColorPhraseCSVFile = ReadEntireFile(FileHandler, PhraseMapCSVPath);
    if (ColorPhraseCSVFile.Memory != 0)
    {
        gs_const_string ColorPhraseMapStr = DataToString(ColorPhraseCSVFile.Data);
        gscsv_sheet ColorPhraseSheet = CSV_Parse(ColorPhraseMapStr, 
                                                 { PhraseMapCSVSeparator },
                                                 State->Transient);
        
        BLState->PhraseHueMap = PhraseHueMap_GenFromCSV(ColorPhraseSheet, 
                                                        &State->Permanent);
    }
    
#if 0
    animation_handle DemoPatternsAnim = AnimationSystem_LoadAnimationFromFile(&State->AnimationSystem,
                                                                              State->Patterns,
                                                                              Context,
                                                                              ConstString("data/demo_patterns.foldanim"));
    State->AnimationSystem.ActiveFadeGroup.From = DemoPatternsAnim;
#else
    
    BLState->ModeAnimations[BlumenPattern_Standard] = LoadAllAnimationsInDir(AmbientPatternFolder, BLState, State, Context);
    BLState->ModeAnimations[BlumenPattern_VoiceCommand] = LoadAllAnimationsInDir(VoicePatternFolder, BLState, State, Context);
    AnimationSystem_LoadAnimationFromFile(&State->AnimationSystem, State->Patterns, Context, ConstString("data/blumen_animations/anim_demo.foldanim"));
    
    BlumenLumen_SetPatternMode(BlumenPattern_Standard, GlobalAnimTransitionSpeed, &State->AnimationSystem, BLState);
    
    BLState->AwakenHandle = AnimationSystem_LoadAnimationFromFile(&State->AnimationSystem,
                                                                  State->Patterns,
                                                                  Context,
                                                                  ConstString("data/blumen_animations/awaken.foldanim"));
    BLState->OffAnimHandle = AnimationSystem_LoadAnimationFromFile(&State->AnimationSystem,
                                                                   State->Patterns,
                                                                   Context,
                                                                   ConstString("data/blumen_animations/off_anim.foldanim"));
    
#endif
    State->AnimationSystem.TimelineShouldAdvance = true;
    
    BLState->StandardPatternHues.Granularity = 1;
    BLState->StandardPatternHues.Speed = 1;
    BLState->StandardPatternHues.AddIn = AddIn_Rotary;
    BLState->StandardPatternHues.Pattern = HuePattern_Wavy;
    
    BLState->DebugHue.Hue0.HSV = v4{0, 1, 1, 1};
    BLState->DebugHue.Hue1.HSV = v4{0, 1, 1, 1};
    BLState->DebugHue.Hue2.HSV = v4{0, 1, 1, 1};
    
    BlumenLumen_AppendBootupLog(State, BLState, Context);
    return Result;
}

internal void
BlumenLumen_UpdateMotorState(blumen_lumen_state* BLState, motor_status_packet Motor, context Context)
{
    DEBUG_ReceivedMotorPositions(BLState, Motor, Context.ThreadContext);
    
    motor_packet LastPos = BLState->LastKnownMotorState;
    motor_packet CurrPos = Motor.Pos;
    for (u32 i = 0; i < BL_FLOWER_COUNT; i++)
    {
        if (LastPos.FlowerPositions[i] != CurrPos.FlowerPositions[i])
        {
            BLState->LastTimeMotorStateChanged[i] = Context.SystemTime_Current.NanosSinceEpoch;
        }
    }
    
    BLState->LastKnownMotorState = Motor.Pos;
    BLState->ShouldUpdateLog = true;
}

internal void
BlumenLumen_ApplyNextHotHue(blumen_lumen_state* BLState, context Context, gs_string* DebugStr, app_state* State)
{
    // if we are in standard color mode, shift all flowers to the new color
    // otherwise, only shift the next flower in the sequence to the new color
    phrase_hue NewHue = BLState->NextHotHue;
    Log_Message(GlobalLogBuffer, "Switching To: %S\n", NewHue.Phrase);
    
    
    if (BLState->PatternMode == BlumenPattern_Standard ||
        NewHue.OverrideAll)
    {
        BlumenLumen_SetNextHue(BLState, 0, NewHue);
        BlumenLumen_SetNextHue(BLState, 1, NewHue);
        BlumenLumen_SetNextHue(BLState, 2, NewHue);
    }
    else
    {
        u32 AssemblyIdx = BLState->LastAssemblyColorSet;
        BlumenLumen_SetNextHue(BLState, AssemblyIdx, NewHue);
        BLState->LastAssemblyColorSet = (BLState->LastAssemblyColorSet + 1) % 3;
    }
    
    BlumenLumen_SetPatternMode(BlumenPattern_VoiceCommand, GlobalAnimTransitionSpeed, &State->AnimationSystem, BLState);
    BLState->TimeLastSetToVoiceMode = Context.SystemTime_Current;
    BLState->LastHuePhrase = NewHue;
    BLState->ShouldUpdateLog = true;
    BLState->InPhraseReceptionMode = false;
}

internal void
BlumenLumen_CustomUpdate(gs_data UserData, app_state* State, context* Context)
{
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData.Memory;
    BLState->ShouldUpdateLog = false;
    
    gs_string DebugStr = PushString(State->Transient, 256);
    
    while (MessageQueue_CanRead(BLState->IncomingMsgQueue))
    {
        gs_data PacketData = MessageQueue_Read(&BLState->IncomingMsgQueue);
        if (PacketData.Memory == 0) continue;
        
        blumen_packet Packet = *(blumen_packet*)PacketData.Memory;
        switch (Packet.Type) {
            case PacketType_PatternCommand:
            {
                microphone_packet Mic = Packet.MicPacket;
                u64 NameHash = HashDJB2ToU32(Mic.AnimationFileName);
                u32 NameLen = CStringLength(Mic.AnimationFileName);
                
                phrase_hue NewHue = PhraseHueMap_Find(BLState->PhraseHueMap, NameHash);
                if (NewHue.Phrase.Length > 0)
                {
                    bool IsLonger = (BLState->NextHotHue.Phrase.Length < NewHue.Phrase.Length);
                    bool IsntInPhraseReceptionMode = !BLState->InPhraseReceptionMode;
                    if (IsLonger || IsntInPhraseReceptionMode)
                    {
                        Log_Message(GlobalLogBuffer, "Queueing: %S\n", NewHue.Phrase);
                        
                        BLState->NextHotHue = NewHue;
                        if (SecondsElapsed(BLState->TimePhraseReceptionBegan,
                                           Context->SystemTime_Current) > PhrasePriorityMessageGroupingTime)
                        {
                            BLState->TimePhraseReceptionBegan = Context->SystemTime_Current;
                            BLState->InPhraseReceptionMode = true;
                        }
                        BLState->TimeLastPhraseReceived = Context->SystemTime_Current;
                    }
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
                
                BlumenLumen_UpdateMotorState(BLState, Motor, *Context);
            }break;
            
            case PacketType_Temperature:
            {
                temp_packet Temp = Packet.TempPacket;
                BLState->LastTemperatureReceived = Temp.Temperature;
                DEBUG_ReceivedTemperature(Temp, Context->ThreadContext);
                BLState->ShouldUpdateLog = true;
            }break;
            
            InvalidDefaultCase;
        }
    }
    
    if (BLState->InPhraseReceptionMode)
    {
        r32 SecondsSincePhraseBegan = SecondsElapsed(BLState->TimePhraseReceptionBegan, Context->SystemTime_Current);
        if (SecondsSincePhraseBegan > PhrasePriorityMessageGroupingTime)
        {
            BlumenLumen_ApplyNextHotHue(BLState, *Context, &DebugStr, State);
        }
    }
    
    BlumenLumen_AdvanceHueFade(BLState, *Context);
    
    // Update next frames Hues
    if (!BLState->DebugOverrideHue)
    {
        r32 AnimTime = AnimationSystem_GetCurrentTime(State->AnimationSystem);
        AnimTime = (r32)Context->TotalTime;
        r32 BaseTime = AnimTime * BLState->PatternSpeed;
        
        r32 ColorSpeed = 1; //.001;
        r32 ColorOscSpeed = .05 * ColorSpeed;
        r32 ColorRelOscSpeed = 1 * ColorSpeed;;
        r32 ColorOscillation = (SinR32(BaseTime * ColorOscSpeed) + 1) / 2;
        r32 ColorRelationship = 30 + (((1 + SinR32(BaseTime * ColorRelOscSpeed)) / 2) * 300);
        
        r32 H0 = ModR32(ColorOscillation * 360, 360);
        r32 H1 = ModR32(BaseTime + ColorRelationship, 360);
        // TODO(PS): use our new HSV lerp
        r32 H2 = LerpR32(.3f, H0, H1);
        
        BLState->StandardPatternHues.Hue0.HSV = v4{ H0, 1, 1, 1 };
        BLState->StandardPatternHues.Hue1.HSV = v4{ H1, 1, 1, 1 };
        BLState->StandardPatternHues.Hue2.HSV = v4{ H2, 1, 1, 1 };
        
        // Transition back to standard mode after some time
        if (BLState->PatternMode == BlumenPattern_VoiceCommand)
        {
            u64 LastChangeClock = BLState->TimeLastSetToVoiceMode.NanosSinceEpoch;
            u64 NowClocks = Context->SystemTime_Current.NanosSinceEpoch;
            s64 NanosSinceChange = NowClocks - LastChangeClock;
            r64 SecondsSinceChange = (r64)NanosSinceChange * NanosToSeconds;
            
            if (SecondsSinceChange > VoiceCommandSustainDuration)
            {
                BlumenLumen_SetPatternMode(BlumenPattern_Standard, GlobalAnimTransitionSpeed, &State->AnimationSystem, BLState);
                BLState->ShouldUpdateLog = true;
            }
        }
        
    }
    else
    {
        BLState->StandardPatternHues = BLState->DebugHue;
        AnimationSystem_FadeToPlaylist(&State->AnimationSystem, BLState->ModeAnimations[BlumenPattern_VoiceCommand]);
        
    }
    
    // Open / Close the Motor
    if (MessageQueue_CanWrite(BLState->OutgoingMsgQueue) &&
        !BLState->IgnoreTimeOfDay_MotorState)
    {
        bool SendMotorCommand = false;
        
        u64 NanosSinceLastSend = Context->SystemTime_Current.NanosSinceEpoch - BLState->LastSendTime.NanosSinceEpoch;
        r32 SecondsSinceLastSend = (r64)NanosSinceLastSend * NanosToSeconds;
        bool ShouldSendCurrentState = SecondsSinceLastSend >= MotorResendStatePeriod;
        
        bl_motor_state_value NewMotorState = MotorState_Closed;
        bool SendOpen = false;
        for (u32 i = 0; i < MotorOpenTimesCount; i++)
        {
            time_range Range = MotorOpenTimes[i];
            bool CurrTimeInRange = SystemTimeIsInTimeRange(Context->SystemTime_Current, Range);
            if (CurrTimeInRange) {
                NewMotorState = MotorState_Open;
            }
        }
        
        if (NewMotorState != BLState->LastSendState) 
        {
            ShouldSendCurrentState = true;
        }
        
        if (ShouldSendCurrentState)
        {
            BLState->LastSendTime = Context->SystemTime_Current;
            BLState->LastSendState = NewMotorState;
            
            blumen_packet Packet = {};
            Packet.Type = PacketType_MotorState;
            Packet.MotorPacket.FlowerPositions[0] = NewMotorState;
            Packet.MotorPacket.FlowerPositions[1] = NewMotorState;
            Packet.MotorPacket.FlowerPositions[2] = NewMotorState;
            gs_data Msg = StructToData(&Packet, blumen_packet);
            MessageQueue_Write(&BLState->OutgoingMsgQueue, Msg);
            
            DEBUG_SentMotorCommand(Packet.MotorPacket, Context->ThreadContext);
        }
    }
    
    // When a motor state changes to being open, wait to turn Upper Leds on 
    // in order to hide the fact that they are turning off
    motor_packet CurrMotorPos = BLState->LastKnownMotorState;
    u64 NowNanos = Context->SystemTime_Current.NanosSinceEpoch;
    for (u32 i = 0; i < BL_FLOWER_COUNT; i++)
    {
        // have to map from "assembly load order" to 
        // the order that the clear core is referencing the 
        // motors by
        assembly Assembly = State->Assemblies.Values[i];
        u64 AssemblyCCIndex = GetCCIndex(Assembly, BLState);
        u8 MotorPos = CurrMotorPos.FlowerPositions[AssemblyCCIndex];
        
        if ((MotorPos == MotorState_Open || MotorPos == MotorState_MostlyOpen) &&
            !BLState->ShouldDimUpperLeds[i])
        {
            u64 ChangedNanos = BLState->LastTimeMotorStateChanged[i];
            u64 NanosSinceChanged = NowNanos - ChangedNanos;
            r64 SecondsSinceChanged = (r64)NanosSinceChanged * NanosToSeconds;
            if (SecondsSinceChanged > TurnUpperLedsOffAfterMotorCloseCommandDelay)
            {
                BLState->ShouldDimUpperLeds[i] = true;
            }
            else
            {
                BLState->ShouldDimUpperLeds[i] = false;
            }
        }
        else if (MotorPos == MotorState_Closed ||
                 MotorPos == MotorState_HalfOpen)
        {
            BLState->ShouldDimUpperLeds[i] = false;
        }
    }
    
    // NOTE(PS): If the flowers are mostly open or full open
    // we mask off the top leds to prevent them from overheating
    // while telescoped inside the flower
    for (u32 a = 0; a < BL_FLOWER_COUNT; a++)
    {
        assembly Assembly = State->Assemblies.Values[a];
        if (!BLState->ShouldDimUpperLeds[a]) continue;
        
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
    
    if (!BLState->IgnoreTimeOfDay_LedDimming)
    {
        bool TimelineShouldAdvance = false;
        r32 OverrideBrightness = 0.0f;
        
        time_range RangeIn = {};
        if (SystemTimeIsInTimeRangeList(Context->SystemTime_Current,
                                        LedOnTimes,
                                        LedOnTimesCount,
                                        &RangeIn))
        {
            
            if (Blumen_TempShouldDimLeds(BLState))
            {
                OverrideBrightness = HighTemperatureBrightnessPercent;
            }
            else
            {
                OverrideBrightness = FullBrightnessPercent;
            }
            TimelineShouldAdvance = true;
        }
        
        State->AnimationSystem.TimelineShouldAdvance = TimelineShouldAdvance;
        BLState->BrightnessPercent = OverrideBrightness;
    }
    
    // Dim the leds based on temp data
    if (!BLState->DEBUG_IgnoreWeatherDimmingLeds)
    {
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
    }
    
    // Send Status Packet
    {
        system_time LastSendTime = BLState->LastStatusUpdateTime;
        r64 NanosSinceLastSend = ((r64)Context->SystemTime_Current.NanosSinceEpoch - (r64)LastSendTime.NanosSinceEpoch);
        r64 SecondsSinceLastSend = NanosSinceLastSend * NanosToSeconds;
        if (SecondsSinceLastSend >= STATUS_PACKET_FREQ_SECONDS)
        {
            BLState->LastStatusUpdateTime = Context->SystemTime_Current;
            Log_Message(GlobalLogBuffer, "Attempting to Send Lumenarium Status\n");
            
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
            
            // there's no new information here, but by updating the log here,
            // we're updating it at some infrequent but regular period that isnt
            // every single frame
            BLState->ShouldUpdateLog = true;
        }
    }
    
    BlumenLumen_UpdateLog(State, BLState, *Context);
}

US_CUSTOM_DEBUG_UI(BlumenLumen_DebugUI)
{
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData.Memory;
    ui_interface* I = &State->Interface;
    
    ui_BeginRow(I, BlumenDebug_Count);
    for (u32 i = 0; i < BlumenDebug_Count; i++)
    {
        if (ui_Button(I, MakeString(BlDebugUiModeStrings[i])))
        {
            BLState->DebugMode = (bl_debug_ui_mode)i;
        }
    }
    ui_EndRow(I);
    
    switch (BLState->DebugMode)
    {
        case BlumenDebug_Motors:
        {
            motor_packet PendingPacket = BLState->DEBUG_PendingMotorPacket;
            
            BLState->IgnoreTimeOfDay_MotorState = ui_ToggleText(I, MakeString("Motors Ignore Time Limit"), BLState->IgnoreTimeOfDay_MotorState);
            
            for (u32 MotorIndex = 0; MotorIndex < BL_FLOWER_COUNT; MotorIndex++)
            {
                gs_string Label = PushStringF(State->Transient, 32, "Motor %d", MotorIndex);
                ui_BeginRow(I, 5);
                {
                    ui_Label(I, Label);
                    
                    bool IsClosed = PendingPacket.FlowerPositions[MotorIndex] == MotorState_Closed;
                    if (ui_ToggleText(I, MakeString("Closed (1)"), IsClosed))
                    {
                        PendingPacket.FlowerPositions[MotorIndex] = MotorState_Closed;
                    }
                    bool IsHOpen = PendingPacket.FlowerPositions[MotorIndex] == MotorState_HalfOpen;
                    if (ui_ToggleText(I, MakeString("Half Open (3)"), IsHOpen))
                    {
                        PendingPacket.FlowerPositions[MotorIndex] = MotorState_HalfOpen;
                    }
                    bool IsMOpen = PendingPacket.FlowerPositions[MotorIndex] == MotorState_MostlyOpen;
                    if (ui_ToggleText(I, MakeString("Mostly Open (4)"), IsMOpen))
                    {
                        PendingPacket.FlowerPositions[MotorIndex] = MotorState_MostlyOpen;
                    }
                    bool IsOpen = PendingPacket.FlowerPositions[MotorIndex] == MotorState_Open;
                    if (ui_ToggleText(I, MakeString("Open (2)"), IsOpen))
                    {
                        PendingPacket.FlowerPositions[MotorIndex] = MotorState_Open;
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
            
            motor_packet MotorPos = BLState->LastKnownMotorState;
            ui_Label(I, MakeString("Current Motor Positions"));
            {
                for (u32 i = 0; i < BL_FLOWER_COUNT; i++)
                {
                    ui_BeginRow(I, 2);
                    gs_string MotorStr = PushStringF(State->Transient, 32, 
                                                     "Motor %d", 
                                                     i);
                    ui_Label(I, MotorStr);
                    
                    gs_string StateStr = {};
                    switch (MotorPos.FlowerPositions[i])
                    {
                        case MotorState_Closed: { 
                            StateStr = MakeString("Closed"); 
                        } break;
                        case MotorState_HalfOpen: { 
                            StateStr = MakeString("Half Open"); 
                        } break;
                        case MotorState_MostlyOpen: { 
                            StateStr = MakeString("Mostly Open"); 
                        } break;
                        case MotorState_Open: { 
                            StateStr = MakeString("Open"); 
                        } break;
                        
                        default:
                        {
                            StateStr = MakeString("Invalid Value");
                        } break;
                    }
                    
                    ui_Label(I, StateStr);
                    ui_EndRow(I);
                }
            }
            
            ui_Label(I, MakeString("Set Internal Motor State:"));
            if (ui_Button(I, MakeString("Closed")))
            {
                motor_status_packet Motor = {};
                Motor.Pos.FlowerPositions[0] = MotorState_Closed;
                Motor.Pos.FlowerPositions[1] = MotorState_Closed;
                Motor.Pos.FlowerPositions[2] = MotorState_Closed;
                Motor.Temperature = 16;
                
                BlumenLumen_UpdateMotorState(BLState, Motor, Context);
            }
            if (ui_Button(I, MakeString("Open")))
            {
                motor_status_packet Motor = {};
                Motor.Pos.FlowerPositions[0] = MotorState_Open;
                Motor.Pos.FlowerPositions[1] = MotorState_Open;
                Motor.Pos.FlowerPositions[2] = MotorState_Open;
                Motor.Temperature = 16;
                
                BlumenLumen_UpdateMotorState(BLState, Motor, Context);
            }
        } break;
        
        case BlumenDebug_Leds:
        {
            BLState->DEBUG_IgnoreWeatherDimmingLeds = ui_LabeledToggle(I, MakeString("Ignore Weather Dimming Leds"), BLState->DEBUG_IgnoreWeatherDimmingLeds);
            
            BLState->IgnoreTimeOfDay_LedDimming = ui_ToggleText(I, MakeString("Leds Ignore Time Limit"), BLState->IgnoreTimeOfDay_LedDimming);
            
            if (ui_BeginLabeledDropdown(I, MakeString("Phrase"), MakeString(BLState->PendingPhrase.Phrase)))
            {
                u32 ListCount = BLState->PhraseHueMap.Count;
                ui_BeginList(I, MakeString("Phrase List"), 5, ListCount);
                for (u32 i = 0; i < ListCount; i++)
                {
                    gs_string Str = MakeString(BLState->PhraseHueMap.Phrases[i]);
                    if (ui_Button(I, Str))
                    {
                        BLState->PendingPhrase = PhraseHueMap_Get(BLState->PhraseHueMap, i);
                        BLState->DebugHue = BLState->PendingPhrase;
                    }
                }
                ui_EndList(I);
            }
            ui_EndLabeledDropdown(I);
            if (ui_Button(I, MakeString("Say Phrase")))
            {
                gs_string DebugStr = PushString(State->Transient, 256);
                BLState->NextHotHue = BLState->PendingPhrase;
                BlumenLumen_ApplyNextHotHue(BLState, Context, &DebugStr, State);
            }
            
            ui_Label(I, MakeString("Phrase Constructor"));
            BLState->DebugOverrideHue = ui_ToggleText(I, MakeString("Override Hue"), BLState->DebugOverrideHue);
            if (BLState->DebugOverrideHue)
            {
                phrase_hue PHue = BLState->DebugHue;
                PHue.Hue0.HSV.x = (r64)ui_LabeledRangeSlider(I, MakeString("Hue0"), (r32)PHue.Hue0.HSV.x, 0, 360);
                PHue.Hue1.HSV.x = (r64)ui_LabeledRangeSlider(I, MakeString("Hue1"), (r32)PHue.Hue1.HSV.x, 0, 360);
                PHue.Hue2.HSV.x = (r64)ui_LabeledRangeSlider(I, MakeString("Hue2"), (r32)PHue.Hue2.HSV.x, 0, 360);
                PHue.Granularity = (u32)ui_LabeledRangeSlider(I, MakeString("Granularity"), (r32)PHue.Granularity, 0, 5);
                PHue.Speed = ui_LabeledRangeSlider(I, MakeString("Speed"), PHue.Speed, 0, 4);
                
                gs_string PatternOptions[HuePattern_Count] = {};
                PatternOptions[HuePattern_Patchy] = MakeString("patchy");
                PatternOptions[HuePattern_Wavy] = MakeString("wavy");
                
                gs_string CPattern = PatternOptions[PHue.Pattern];
                if (ui_BeginLabeledDropdown(I, MakeString("Pattern"), CPattern))
                {
                    for (u32 i = 0; i < HuePattern_Count; i++)
                    {
                        if (ui_Button(I, PatternOptions[i]))
                        {
                            PHue.Pattern = i;
                        }
                    }
                }
                ui_EndLabeledDropdown(I);
                
                
                gs_string AddInOptions[AddIn_Count] = {};
                AddInOptions[AddIn_None] = MakeString("NA");
                AddInOptions[AddIn_Waves] = MakeString("waves");
                AddInOptions[AddIn_Rotary] = MakeString("rotary");
                
                gs_string CAddIn = AddInOptions[PHue.AddIn];
                if (ui_BeginLabeledDropdown(I, MakeString("Add In"), CAddIn))
                {
                    for (u32 i = 0; i < AddIn_Count; i++)
                    {
                        if (ui_Button(I, AddInOptions[i]))
                        {
                            PHue.AddIn = i;
                        }
                    }
                }
                ui_EndLabeledDropdown(I);
                BLState->DebugHue = PHue;
            }
            
            
            InterfaceAssert(I->PerFrameMemory);
        }break;
        
        case BlumenDebug_Awaken:
        {
            ui_Label(I, MakeString("Step 1:"));
            ui_Label(I, MakeString("Leds off, flowers closed"));
            if (ui_Button(I, MakeString("Prepare")))
            {
                // motors closed
                blumen_packet M = {};
                M.Type = PacketType_MotorState;
                M.MotorPacket.FlowerPositions[0] = MotorState_Closed;
                M.MotorPacket.FlowerPositions[1] = MotorState_Closed;
                M.MotorPacket.FlowerPositions[2] = MotorState_Closed;
                gs_data D = StructToData(&M, blumen_packet);
                MessageQueue_Write(&BLState->OutgoingMsgQueue, D);
                
                // animation
                State->AnimationSystem.RepeatMode = AnimationRepeat_Single;
                AnimationFadeGroup_FadeTo(&State->AnimationSystem.ActiveFadeGroup,
                                          BLState->OffAnimHandle, 
                                          VoiceCommandFadeDuration);
                
                BLState->PatternMode = BlumenPattern_NoControl;
                BLState->IgnoreTimeOfDay_LedDimming = true;
                BLState->IgnoreTimeOfDay_MotorState = true;
            }
            
            ui_Label(I, MakeString("Step 2:"));
            if (ui_Button(I, MakeString("Begin Light Show")))
            {
                AnimationFadeGroup_FadeTo(&State->AnimationSystem.ActiveFadeGroup,
                                          BLState->AwakenHandle, 
                                          VoiceCommandFadeDuration);
            }
            
            ui_Label(I, MakeString("Step 3:"));
            if (ui_Button(I, MakeString("Open Flowers")))
            {
                // motors closed
                blumen_packet M = {};
                M.Type = PacketType_MotorState;
                M.MotorPacket.FlowerPositions[0] = MotorState_Open;
                M.MotorPacket.FlowerPositions[1] = MotorState_Open;
                M.MotorPacket.FlowerPositions[2] = MotorState_Open;
                gs_data D = StructToData(&M, blumen_packet);
                MessageQueue_Write(&BLState->OutgoingMsgQueue, D);
            }
            
            ui_Label(I, MakeString("Step 4:"));
            ui_Label(I, MakeString("Resets Lumenarium"));
            if (ui_Button(I, MakeString("Complete")))
            {
                BLState->IgnoreTimeOfDay_LedDimming = false;
                BLState->IgnoreTimeOfDay_MotorState = false;
                BlumenLumen_SetPatternMode(BlumenPattern_Standard, GlobalAnimTransitionSpeed, &State->AnimationSystem,
                                           BLState);
            }
        }break;
        
        InvalidDefaultCase;
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
