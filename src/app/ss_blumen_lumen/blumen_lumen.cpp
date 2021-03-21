//
// File: blumen_lumen.cpp
// Author: Peter Slattery
// Creation Date: 2021-01-23
//
#ifndef BLUMEN_LUMEN_CPP

internal bool
MessageQueue_CanRead(blumen_network_msg_queue* Queue)
{
    bool Result = (Queue->ReadHead != Queue->WriteHead);
    return Result;
}

internal gs_data
MessageQueue_Read(blumen_network_msg_queue* Queue)
{
    gs_data Result = {};
    u32 ReadIndex = Queue->ReadHead++;
    if (Queue->ReadHead >= BLUMEN_MESSAGE_QUEUE_COUNT)
    {
        Queue->ReadHead = 0;
    }
    Result = Queue->Buffers[ReadIndex];
    return Result;
}

// KB(1) is just bigger than any packet we send. Good for now
#define DEFAULT_QUEUE_ENTRY_SIZE KB(1)

internal void
MessageQueue_Init(blumen_network_msg_queue* Queue, gs_memory_arena* Arena)
{
    for (u32 i = 0; i < BLUMEN_MESSAGE_QUEUE_COUNT; i++)
    {
        Queue->Buffers[i] = PushSizeToData(Arena, DEFAULT_QUEUE_ENTRY_SIZE);
    }
}

internal bool
MessageQueue_Write(blumen_network_msg_queue* Queue, gs_data Msg)
{
    Assert(Msg.Size <= DEFAULT_QUEUE_ENTRY_SIZE);
    
    u32 Index = Queue->WriteHead;
    gs_data* Dest = Queue->Buffers + Index;
    CopyMemoryTo(Msg.Memory, Dest->Memory, Msg.Size);
    Dest->Size = Msg.Size;
    
    // NOTE(pjs): We increment write head at the end of writing so that
    // a reader thread doesn't pull the message off before we've finished
    // filling it out
    Queue->WriteHead++;
    return true;
}

internal bool
MessageQueue_CanWrite(blumen_network_msg_queue Queue)
{
    bool Result = ((Queue.WriteHead >= Queue.ReadHead) ||
                   (Queue.WriteHead < Queue.ReadHead));
    return Result;
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
    
    while (*Data->Running)
    {
        if (SocketQueryStatus(Data->SocketManager, Data->ListenSocket))
        {
            if (SocketPeek(Data->SocketManager, Data->ListenSocket))
            {
                // TODO(pjs): Make this a peek operation
                Msg = SocketRecieve(Data->SocketManager, Data->ListenSocket, Ctx->Transient);
                if (Msg.Size > 0)
                {
                    MessageQueue_Write(Data->IncomingMsgQueue, Msg);
                }
            }
            
            while (MessageQueue_CanRead(Data->OutgoingMsgQueue))
            {
                Msg = MessageQueue_Read(Data->OutgoingMsgQueue);
                u32 Address = WeathermanIPV4;
                u32 Port = WeathermanPort;
                s32 Flags = 0;
                SocketSend(Data->SocketManager, Data->ListenSocket, Address, Port, Msg, Flags);
            }
        }
    }
    
    CloseSocket(Data->SocketManager, Data->ListenSocket);
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
    Patterns_PushPattern(Patterns, TestPatternOne);
    Patterns_PushPattern(Patterns, TestPatternTwo);
    Patterns_PushPattern(Patterns, TestPatternThree);
    Patterns_PushPattern(Patterns, Pattern_AllGreen);
    Patterns_PushPattern(Patterns, Pattern_HueShift);
    Patterns_PushPattern(Patterns, Pattern_HueFade);
    Patterns_PushPattern(Patterns, Pattern_Spots);
    Patterns_PushPattern(Patterns, Pattern_LighthouseRainbow);
    Patterns_PushPattern(Patterns, Pattern_SmoothGrowRainbow);
    Patterns_PushPattern(Patterns, Pattern_GrowAndFade);
    Patterns_PushPattern(Patterns, Pattern_ColorToWhite);
    Patterns_PushPattern(Patterns, Pattern_Blue);
    Patterns_PushPattern(Patterns, Pattern_Green);
    Patterns_PushPattern(Patterns, Pattern_FlowerColors);
    Patterns_PushPattern(Patterns, Pattern_FlowerColorToWhite);
    Patterns_PushPattern(Patterns, Pattern_BasicFlowers);
    // 15
    Patterns_PushPattern(Patterns, Pattern_Patchy);
    Patterns_PushPattern(Patterns, Pattern_Leafy);
}

internal v4
TEMP_Saturate(v4  P)
{
    v4 CRGB = P;
    v4 CHSV = RGBToHSV(CRGB);
    if (CHSV.g > .3f)
    {
        CHSV.g = 1;
        CRGB = HSVToRGB(CHSV);
    }
    return CRGB;
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
    BLState->MicListenJobData.ListenSocket = CreateSocket(Context.SocketManager, "127.0.0.1", "20185");
    
#if 1
    BLState->MicListenThread = CreateThread(Context.ThreadManager, BlumenLumen_MicListenJob, (u8*)&BLState->MicListenJobData);
#endif
    
    gs_const_string SculpturePath = ConstString("data/test_blumen.fold");
    LoadAssembly(&State->Assemblies, &State->LedSystem, State->Transient, Context, SculpturePath, State->GlobalLog);
    
    { // Animation PLAYGROUND
        animation Anim0 = {0};
        Anim0.Name = PushStringF(&State->Permanent, 256, "test_anim_zero");
        Anim0.Layers = AnimLayerArray_Create(State->AnimationSystem.Storage, 8);
        Anim0.Blocks_ = AnimBlockArray_Create(State->AnimationSystem.Storage, 8);
        Anim0.PlayableRange.Min = 0;
        Anim0.PlayableRange.Max = SecondsToFrames(15, State->AnimationSystem);
        Animation_AddLayer(&Anim0, MakeString("Base Layer"), BlendMode_Overwrite, &State->AnimationSystem);
        
        Animation_AddBlock(&Anim0, 0, Anim0.PlayableRange.Max, Patterns_IndexToHandle(15), 0);
        
        BLState->AnimHandles[0] = AnimationArray_Push(&State->AnimationSystem.Animations, Anim0);
        
        animation Anim1 = {0};
        Anim1.Name = PushStringF(&State->Permanent, 256, "test_anim_one");
        Anim1.Layers = AnimLayerArray_Create(State->AnimationSystem.Storage, 8);
        Anim1.Blocks_ = AnimBlockArray_Create(State->AnimationSystem.Storage, 8);
        Anim1.PlayableRange.Min = 0;
        Anim1.PlayableRange.Max = SecondsToFrames(15, State->AnimationSystem);
        Animation_AddLayer(&Anim1, MakeString("Base Layer"), BlendMode_Overwrite, &State->AnimationSystem);
        
        Animation_AddBlock(&Anim1, 0, Anim0.PlayableRange.Max, Patterns_IndexToHandle(12), 0);
        
        BLState->AnimHandles[1] = AnimationArray_Push(&State->AnimationSystem.Animations, Anim1);
        
        animation Anim2 = {0};
        Anim2.Name = PushStringF(&State->Permanent, 256, "i_love_you");
        Anim2.Layers = AnimLayerArray_Create(State->AnimationSystem.Storage, 8);
        Anim2.Blocks_ = AnimBlockArray_Create(State->AnimationSystem.Storage, 8);
        Anim2.PlayableRange.Min = 0;
        Anim2.PlayableRange.Max = SecondsToFrames(15, State->AnimationSystem);
        Animation_AddLayer(&Anim2, MakeString("Base Layer"), BlendMode_Overwrite, &State->AnimationSystem);
        
        Animation_AddBlock(&Anim2, 0, Anim0.PlayableRange.Max, Patterns_IndexToHandle(17), 0);
        
        BLState->AnimHandles[2] = AnimationArray_Push(&State->AnimationSystem.Animations, Anim2);
        
        State->AnimationSystem.ActiveFadeGroup.From = BLState->AnimHandles[2];
        State->AnimationSystem.TimelineShouldAdvance = true;
    } // End Animation Playground
    
    for (u32 i = 0; i < FLOWER_COLORS_COUNT; i++)
    {
        //FlowerAColors[i] = TEMP_Saturate(FlowerAColors[i]);
        //FlowerBColors[i] = TEMP_Saturate(FlowerBColors[i]);
        //FlowerCColors[i] = TEMP_Saturate(FlowerCColors[i]);
    }
    
    return Result;
}

internal void
BlumenLumen_CustomUpdate(gs_data UserData, app_state* State, context* Context)
{
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData.Memory;
    
    bool SendMotorCommand = false;
    blumen_packet MotorCommand = {};
    
#if 0
    MotorTimeElapsed += Context->DeltaTime;
    BLState->TimeElapsed += Context->DeltaTime;
    
    if (BLState->TimeElapsed > 5)
    {
        u32 NextIndex = ++BLState->CurrAnim % 3;
        animation_handle Next = BLState->AnimHandles[NextIndex];
        AnimationFadeGroup_FadeTo(&State->AnimationSystem.ActiveFadeGroup, Next, 5);
        BLState->TimeElapsed = 0;
    }
#endif
    
    while (MessageQueue_CanRead(&BLState->IncomingMsgQueue))
    {
        gs_data PacketData = MessageQueue_Read(&BLState->IncomingMsgQueue);
        
        blumen_packet Packet = *(blumen_packet*)PacketData.Memory;
        switch (Packet.Type) {
            case PacketType_PatternCommand:
            {
                microphone_packet Mic = Packet.MicPacket;
                u32 NameLen = CStringLength(Mic.AnimationFileName);
                
                for (u32 i = 0; i < PhraseToAnimMapCount; i++)
                {
                    gs_const_string PhraseStr = ConstString(PhraseToAnimMap[i].Phrase);
                    u32 PhraseIndex = PhraseToAnimMap[i].PatternIndex;
                    if (StringEqualsCharArray(PhraseStr, Mic.AnimationFileName, NameLen))
                    {
                        AnimationFadeGroup_FadeTo(&State->AnimationSystem.ActiveFadeGroup,
                                                  animation_handle{(s32)PhraseIndex},
                                                  3.0f);
                        OutputDebugStringA("\nReceived Pattern Packet\n");
                        
                        {
                            // DEBUG CODE
                            u8 MotorState = BLState->LastKnownMotorState.FlowerPositions[0];
                            if (MotorState == 2) {
                                OutputDebugStringA("Sending 1\n");
                                MotorState = 1;
                            }
                            else
                            {
                                OutputDebugStringA("Sending 1\n");
                                MotorState = 2;
                            }
                            
                            blumen_packet MPacket = {};
                            MPacket.Type = PacketType_MotorState;
                            MPacket.MotorPacket.FlowerPositions[0] = MotorState;
                            MPacket.MotorPacket.FlowerPositions[1] = MotorState;
                            MPacket.MotorPacket.FlowerPositions[2] = MotorState;
                            MotorCommand = MPacket;
                            SendMotorCommand = true;
                        }
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
                
                BLState->LastKnownMotorState = Motor.Pos;
                
                gs_string Temp = PushStringF(State->Transient, 256, "\nReceived Motor States: \n\tPos: %d %d %d\n\tErr: %d %d %d\n\tTemp: %d\n\n",
                                             Motor.Pos.FlowerPositions[0],
                                             Motor.Pos.FlowerPositions[1],
                                             Motor.Pos.FlowerPositions[2],
                                             Motor.MotorStatus[0],
                                             Motor.MotorStatus[1],
                                             Motor.MotorStatus[2],
                                             (u32)Motor.Temperature
                                             );
                NullTerminate(&Temp);
                
                OutputDebugStringA(Temp.Str);
            }break;
            
            case PacketType_Temperature:
            {
                temp_packet Temp = Packet.TempPacket;
                
                if (Temp.Temperature > 21)
                {
                    BLState->BrightnessPercent = .25f;
                }
                else
                {
                    BLState->BrightnessPercent = 1.f;
                }
                
                gs_string TempStr = PushStringF(State->Transient, 256, "\nTemperature: %d\n",
                                                Temp.Temperature);
                NullTerminate(&TempStr);
                OutputDebugStringA(TempStr.Str);
            }break;
            
            InvalidDefaultCase;
        }
    }
    
    
    // Open / Close the Motor
    
    if (MessageQueue_CanWrite(BLState->OutgoingMsgQueue))
    {
#if 0
        for (u32 i = 0; i < MotorOpenTimesCount; i++)
        {
            time_range Range = MotorOpenTimes[i];
            
            bool CurrTimeInRange = SystemTimeIsInTimeRange(Context->SystemTime_Current, Range);
            
            bool LastTimeInRange = SystemTimeIsInTimeRange(Context->SystemTime_Last, Range);
            
            bool SendOpen = CurrTimeInRange && !LastTimeInRange;
            
            r64 NanosSinceLastSend = ((r64)Context->SystemTime_Current.NanosSinceEpoch - (r64)BLState->LastSendTime.NanosSinceEpoch);
            r64 SecondsSinceLastSend = NanosSinceLastSend / PowR32(10, 8);
            
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
            else if (!CurrTimeInRange && LastTimeInRange)
            {
                SendMotorCommand = true;
                OutputDebugString("Motors: Close\n");
                
                blumen_packet Packet = {};
                Packet.Type = PacketType_MotorState;
                Packet.MotorPacket.FlowerPositions[0] = 1;
                Packet.MotorPacket.FlowerPositions[1] = 1;
                Packet.MotorPacket.FlowerPositions[2] = 1;
                MotorCommand = Packet;
            }
        }
#endif
        
        if (SendMotorCommand)
        {
            gs_data Msg = StructToData(&MotorCommand, blumen_packet);
            MessageQueue_Write(&BLState->OutgoingMsgQueue, Msg);
        }
    }
    // Dim the leds based on temp data
#if DIM_LED_BRIGHTNESS
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
    
    // TODO(pjs): dim stem to 50%
#endif
    
    // Send Status Packet
    {
        system_time LastSendTime = BLState->LastStatusUpdateTime;
        r64 NanosSinceLastSend = ((r64)Context->SystemTime_Current.NanosSinceEpoch - (r64)LastSendTime.NanosSinceEpoch);
        r64 SecondsSinceLastSend = NanosSinceLastSend / PowR32(10, 8);
        if (SecondsSinceLastSend >= STATUS_PACKET_FREQ_SECONDS)
        {
            BLState->LastStatusUpdateTime = Context->SystemTime_Current;
            OutputDebugString("Sending Status\n");
            
            blumen_packet Packet = {};
            Packet.Type = PacketType_LumenariumStatus;
            Packet.StatusPacket.NextMotorEventType = 0;
            Packet.StatusPacket.NextEventTime = 0;
            
            animation* ActiveAnim = AnimationSystem_GetActiveAnimation(&State->AnimationSystem);
            CopyMemoryTo(ActiveAnim->Name.Str, Packet.StatusPacket.AnimFileName,
                         Min(ActiveAnim->Name.Length, 32));
            Packet.StatusPacket.AnimFileName[ActiveAnim->Name.Length] = 0;
            
            gs_data Msg = StructToData(&Packet, blumen_packet);
            MessageQueue_Write(&BLState->OutgoingMsgQueue, Msg);
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
    Result.CustomCleanup = BlumenLumen_CustomCleanup;
    return Result;
}

#define BLUMEN_LUMEN_CPP
#endif // BLUMEN_LUMEN_CPP