//
// File: blumen_lumen.cpp
// Author: Peter Slattery
// Creation Date: 2021-01-23
//
#ifndef BLUMEN_LUMEN_CPP

internal void
BlumenLumen_MicListenJob(gs_thread_context* Ctx, u8* UserData)
{
    mic_listen_job_data* Data = (mic_listen_job_data*)UserData;
    
    gs_data Msg = {};
    
    while (true)
    {
#if 0
        // TODO(pjs): Make this a peek operation
        Msg = SocketRecieve(Data->SocketManager, Data->ListenSocket, Ctx->Transient);
        if (Msg.Size > 0)
        {
            OutputDebugStringA("Listened");
            Data->MicPacketBuffer->Values[Data->MicPacketBuffer->WriteHead++] = Msg;
            if (Data->MicPacketBuffer->WriteHead >= PACKETS_MAX)
            {
                Data->MicPacketBuffer->WriteHead = 0;
            }
        }
#endif
        
        while (Data->OutgoingMsgQueue->ReadHead != Data->OutgoingMsgQueue->WriteHead)
        {
            u32 ReadIndex = Data->OutgoingMsgQueue->ReadHead++;
            if (Data->OutgoingMsgQueue->ReadHead >= BLUMEN_MESSAGE_QUEUE_COUNT)
            {
                Data->OutgoingMsgQueue->ReadHead = 0;
            }
            
            Msg = Data->OutgoingMsgQueue->Buffers[ReadIndex];
            u32 Address = 0;
            u32 Port = 0;
            s32 Flags = 0;
            SocketSend(Data->SocketManager, Data->ListenSocket, Address, Port, Msg, Flags);
            OutputDebugStringA("Wrote\n");
        }
    }
    
    CloseSocket(Data->SocketManager, Data->ListenSocket);
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
    BLState->MicListenJobData.SocketManager = Context.SocketManager;
    BLState->MicListenJobData.MicPacketBuffer = &BLState->MicPacketBuffer;
    BLState->MicListenJobData.OutgoingMsgQueue = &BLState->OutgoingMsgQueue;
    BLState->MicListenJobData.ListenSocket = CreateSocket(Context.SocketManager, "127.0.0.1", "20185");
    
    BLState->MicListenThread = CreateThread(Context.ThreadManager, BlumenLumen_MicListenJob, (u8*)&BLState->MicListenJobData);
    
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
        
        Animation_AddBlock(&Anim0, 0, Anim0.PlayableRange.Max, Patterns_IndexToHandle(11), 0);
        
        AnimationArray_Push(&State->AnimationSystem.Animations, Anim0);
        
        animation Anim1 = {0};
        Anim1.Name = PushStringF(&State->Permanent, 256, "test_anim_one");
        Anim1.Layers = AnimLayerArray_Create(State->AnimationSystem.Storage, 8);
        Anim1.Blocks_ = AnimBlockArray_Create(State->AnimationSystem.Storage, 8);
        Anim1.PlayableRange.Min = 0;
        Anim1.PlayableRange.Max = SecondsToFrames(15, State->AnimationSystem);
        Animation_AddLayer(&Anim1, MakeString("Base Layer"), BlendMode_Overwrite, &State->AnimationSystem);
        
        Animation_AddBlock(&Anim1, 0, Anim0.PlayableRange.Max, Patterns_IndexToHandle(12), 0);
        
        AnimationArray_Push(&State->AnimationSystem.Animations, Anim1);
        
        animation Anim2 = {0};
        Anim2.Name = PushStringF(&State->Permanent, 256, "i_love_you");
        Anim2.Layers = AnimLayerArray_Create(State->AnimationSystem.Storage, 8);
        Anim2.Blocks_ = AnimBlockArray_Create(State->AnimationSystem.Storage, 8);
        Anim2.PlayableRange.Min = 0;
        Anim2.PlayableRange.Max = SecondsToFrames(15, State->AnimationSystem);
        Animation_AddLayer(&Anim2, MakeString("Base Layer"), BlendMode_Overwrite, &State->AnimationSystem);
        
        Animation_AddBlock(&Anim2, 0, Anim0.PlayableRange.Max, Patterns_IndexToHandle(10), 0);
        
        AnimationArray_Push(&State->AnimationSystem.Animations, Anim2);
        
        State->AnimationSystem.TimelineShouldAdvance = true;
    } // End Animation Playground
    
    return Result;
}

internal void
BlumenLumen_CustomUpdate(gs_data UserData, app_state* State, context* Context)
{
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData.Memory;
    
    gs_string BlueString = MakeString("blue");
    gs_string GreenString = MakeString("green");
    gs_string ILoveYouString = MakeString("i_love_you");
    
    while (BLState->MicPacketBuffer.ReadHead != BLState->MicPacketBuffer.WriteHead)
    {
        gs_data PacketData = BLState->MicPacketBuffer.Values[BLState->MicPacketBuffer.ReadHead++];
        microphone_packet Packet = *(microphone_packet*)PacketData.Memory;
        
        u32 NameLen = CStringLength(Packet.AnimationFileName);
        if (StringEqualsCharArray(BlueString.ConstString, Packet.AnimationFileName, NameLen))
        {
            State->AnimationSystem.ActiveAnimationIndex = 0;
        }
        else if (StringEqualsCharArray(GreenString.ConstString, Packet.AnimationFileName, NameLen))
        {
            State->AnimationSystem.ActiveAnimationIndex = 1;
        }
        else if (StringEqualsCharArray(ILoveYouString.ConstString, Packet.AnimationFileName, NameLen))
        {
            State->AnimationSystem.ActiveAnimationIndex = 2;
        }
        
        if (BLState->MicPacketBuffer.ReadHead >= PACKETS_MAX)
        {
            BLState->MicPacketBuffer.ReadHead = 0;
        }
    }
    
    if ((BLState->OutgoingMsgQueue.WriteHead >= BLState->OutgoingMsgQueue.ReadHead) ||
        (BLState->OutgoingMsgQueue.WriteHead < BLState->OutgoingMsgQueue.ReadHead))
    {
        u32 WriteIndex = BLState->OutgoingMsgQueue.WriteHead;
        if (BLState->OutgoingMsgQueue.WriteHead >= BLUMEN_MESSAGE_QUEUE_COUNT)
        {
            BLState->OutgoingMsgQueue.WriteHead = 0;
        }
        
        gs_data* Msg = BLState->OutgoingMsgQueue.Buffers + WriteIndex;
        if (Msg->Size == 0)
        {
            *Msg = PushSizeToData(&State->Permanent, sizeof(motor_packet));
        }
        motor_packet* Packet = (motor_packet*)Msg->Memory;
        Packet->FlowerPositions[0] = 5;
        Packet->FlowerPositions[0] = 4;
        Packet->FlowerPositions[0] = 9;
        
        // NOTE(pjs): We increment the write head AFTER we've written so that
        // the network thread doesn't think the buffer is ready to send before
        // the data is set. We want to avoid the case of:
        //     1. Main Thread increments write head to 1
        //     2. Network Thread thinks theres a new message to send at 0
        //     3. Network Thread sends the message at 0
        //     4. Main Thread sets the message at 0
        BLState->OutgoingMsgQueue.WriteHead += 1;
    }
}


#define BLUMEN_LUMEN_CPP
#endif // BLUMEN_LUMEN_CPP