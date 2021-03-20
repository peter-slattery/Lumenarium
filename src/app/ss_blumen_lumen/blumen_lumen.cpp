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

internal gs_data*
MessageQueue_GetWrite(blumen_network_msg_queue* Queue)
{
    u32 Index = Queue->WriteHead++;
    gs_data* Result = &Queue->Buffers[Index];
    Assert(Result->Size > 0);
    
    if (Queue->WriteHead >= PACKETS_MAX)
    {
        Queue->WriteHead = 0;
    }
    return Result;
}

internal bool
MessageQueue_Write(blumen_network_msg_queue* Queue, gs_data Msg)
{
    gs_data* Dest = MessageQueue_GetWrite(Queue);
    Assert(Msg.Size <= DEFAULT_QUEUE_ENTRY_SIZE);
    CopyMemoryTo(Msg.Memory, Dest->Memory, Msg.Size);
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
                
                OutputDebugString("Sending Motor Packet\n");
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

internal pixel
TEMP_Saturate(pixel P)
{
    v4 CRGB = v4{ (r32)P.R / 255.f, (r32)P.G / 255.f, (r32)P.B / 255.f, 1.f };
    v4 CHSV = RGBToHSV(CRGB);
    if (CHSV.g > .3f)
    {
        CHSV.g = 1;
        CRGB = HSVToRGB(CHSV);
    }
    return V4ToRGBPixel(CRGB);
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
    
#if 0
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
        FlowerAColors[i] = TEMP_Saturate(FlowerAColors[i]);
        FlowerBColors[i] = TEMP_Saturate(FlowerBColors[i]);
        FlowerCColors[i] = TEMP_Saturate(FlowerCColors[i]);
    }
    
    return Result;
}

internal void
BlumenLumen_CustomUpdate(gs_data UserData, app_state* State, context* Context)
{
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData.Memory;
    
    MotorTimeElapsed += Context->DeltaTime;
#if 0
    BLState->TimeElapsed += Context->DeltaTime;
    
    if (BLState->TimeElapsed > 5)
    {
        u32 NextIndex = ++BLState->CurrAnim % 3;
        animation_handle Next = BLState->AnimHandles[NextIndex];
        AnimationFadeGroup_FadeTo(&State->AnimationSystem.ActiveFadeGroup, Next, 5);
        BLState->TimeElapsed = 0;
    }
#endif
    
    gs_string BlueString = MakeString("blue");
    gs_string GreenString = MakeString("green");
    gs_string ILoveYouString = MakeString("i_love_you");
    
    while (MessageQueue_CanRead(&BLState->IncomingMsgQueue))
    {
        gs_data PacketData = MessageQueue_Read(&BLState->IncomingMsgQueue);
        
        blumen_packet Packet = *(blumen_packet*)PacketData.Memory;
        switch (Packet.Type) {
            case PacketType_PatternCommand:
            {
                microphone_packet Mic = Packet.MicPacket;
                
                u32 NameLen = CStringLength(Mic.AnimationFileName);
                if (StringEqualsCharArray(BlueString.ConstString, Mic.AnimationFileName, NameLen))
                {
                    State->AnimationSystem.ActiveFadeGroup.From.Index = 0;
                }
                else if (StringEqualsCharArray(GreenString.ConstString, Mic.AnimationFileName, NameLen))
                {
                    State->AnimationSystem.ActiveFadeGroup.From.Index = 1;
                }
                else if (StringEqualsCharArray(ILoveYouString.ConstString, Mic.AnimationFileName, NameLen))
                {
                    State->AnimationSystem.ActiveFadeGroup.From.Index = 2;
                }
                
                OutputDebugStringA("Received Pattern Packet\n");
            }break;
            
            case PacketType_MotorState:
            {
                motor_packet Motor = Packet.MotorPacket;
                BLState->LastKnownMotorState = Motor;
                
                gs_string Temp = PushStringF(State->Transient, 256, "Received Motor States: %d %d %d\n",
                                             Motor.FlowerPositions[0],
                                             Motor.FlowerPositions[1],
                                             Motor.FlowerPositions[2]);
                NullTerminate(&Temp);
                
                OutputDebugStringA(Temp.Str);
            }break;
            
            case PacketType_Temperature:
            {
                temp_packet Temp = Packet.TempPacket;
                
                if (Temp.Temperature > 21)
                {
                    BLState->BrightnessPercent = .5f;
                }
                else
                {
                    BLState->BrightnessPercent = 1.f;
                }
                
                gs_string TempStr = PushStringF(State->Transient, 256, "Temperature: %d\n",
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
        for (u32 i = 0; i < MotorOpenTimesCount; i++)
        {
            time_range Range = MotorOpenTimes[i];
            
            bool CurrTimeInRange = SystemTimeIsInTimeRange(Context->SystemTime_Current, Range);
            bool LastTimeInRange = SystemTimeIsInTimeRange(Context->SystemTime_Last, Range);
            
            if (CurrTimeInRange && !LastTimeInRange)
            {
                OutputDebugString("Open\n");
                gs_data* Msg = MessageQueue_GetWrite(&BLState->OutgoingMsgQueue);
                
                blumen_packet* Packet = (blumen_packet*)Msg->Memory;
                Packet->Type = PacketType_MotorState;
                Packet->MotorPacket.FlowerPositions[0] = 2;
                Packet->MotorPacket.FlowerPositions[1] = 2;
                Packet->MotorPacket.FlowerPositions[2] = 2;
            }
            else if (!CurrTimeInRange && LastTimeInRange)
            {
                OutputDebugString("Close\n");
                gs_data* Msg = MessageQueue_GetWrite(&BLState->OutgoingMsgQueue);
                
                blumen_packet* Packet = (blumen_packet*)Msg->Memory;
                Packet->Type = PacketType_MotorState;
                Packet->MotorPacket.FlowerPositions[0] = 1;
                Packet->MotorPacket.FlowerPositions[1] = 1;
                Packet->MotorPacket.FlowerPositions[2] = 1;
            }
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
    
    // Send Status Packet
    {
        system_time LastSendTime = BLState->LastStatusUpdateTime;
        s64 NanosSinceLastSend = ((s64)Context->SystemTime_Current.NanosSinceEpoch - (s64)LastSendTime.NanosSinceEpoch);
        s64 SecondsSinceLastSend = NanosSinceLastSend * 1000000000;
        if (SecondsSinceLastSend >= STATUS_PACKET_FREQ_SECONDS)
        {
            BLState->LastStatusUpdateTime = Context->SystemTime_Current;
            gs_data* Msg = MessageQueue_GetWrite(&BLState->OutgoingMsgQueue);
            
            OutputDebugString("Sending Status\n");
            
            blumen_packet* Packet = (blumen_packet*)Msg->Memory;
            Packet->Type = PacketType_LumenariumStatus;
            Packet->StatusPacket.NextMotorEventType = 0;
            Packet->StatusPacket.NextEventTime = 0;
            
            animation* ActiveAnim = AnimationSystem_GetActiveAnimation(&State->AnimationSystem);
            CopyMemoryTo(ActiveAnim->Name.Str, Packet->StatusPacket.AnimFileName,
                         Min(ActiveAnim->Name.Length, 32));
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