inline void
SetTransition(pattern_transition* Transition, r32 Duration, r32 Elapsed)
{
    Transition->Duration = Duration;
    Transition->TimeElapsed = Elapsed;
}

internal void
InitLEDChannelSystem (led_channel_system* System, memory_arena* ParentStorage, s32 StorageSize)
{
    System->Channels = 0;
    System->ChannelCount = 0;
    InitMemoryArena(&System->Storage, PushSize(ParentStorage, StorageSize), StorageSize, 0);
    System->FreeList = 0;
}

inline void
ResetChannel (led_channel* Channel, led_channel_system* ChannelSystem)
{
    if (Channel->Patterns == 0)
    {
        Channel->Patterns = PushArray(&ChannelSystem->Storage, 
                                      pattern_index_id_key, 
                                      CHANNEL_MAX_PATTERNS);
    }
    Channel->ActivePatterns = 0;
    Channel->ActivePatternIndex = -1;
    
    SetTransition(&Channel->Transition, 3, 0);
    
    Channel->BlendMode = ChannelBlend_Override;
    Channel->ChannelID = ChannelIDAccumulator++;
    
    Channel->Next = 0;
}

internal led_channel*
AddLEDChannel (led_channel_system* ChannelSystem)
{
    led_channel* Channel = 0;
    
    if (ChannelSystem->FreeList)
    {
        Channel = ChannelSystem->FreeList;
        ChannelSystem->FreeList = ChannelSystem->FreeList->Next;
    }
    else
    {
        Channel = PushStruct(&ChannelSystem->Storage, led_channel);
    }
    ResetChannel(Channel, ChannelSystem);
    
    Channel->Next = ChannelSystem->Channels;
    ChannelSystem->Channels = Channel;
    ChannelSystem->ChannelCount++;
    
    return Channel;
}

internal b32
RemoveLEDChannel_ (led_channel* PrevChannel, led_channel* ToRemove, led_channel_system* ChannelSystem)
{
    b32 Result = true;
    
    if (PrevChannel->Next == ToRemove)
    {
        PrevChannel->Next = ToRemove->Next;
        ToRemove->Next = ChannelSystem->FreeList;
        ChannelSystem->FreeList = ToRemove;
        ChannelSystem->ChannelCount--;
        Result = true;
    }
    else
    {
        Result = false;
    }
    
    return Result;
}

internal b32
RemoveLEDChannel (led_channel* Channel, led_channel_system* ChannelSystem)
{
    b32 Result = true;
    
    led_channel* Cursor = ChannelSystem->Channels;
    for (s32 i = 0;
         (Cursor->Next != Channel && i < ChannelSystem->ChannelCount);
         Cursor = Cursor->Next, i++){}
    
    Result = RemoveLEDChannel_(Cursor, Channel, ChannelSystem);
    
    return Result;
}

internal b32
RemoveLEDChannel (s32 Index, led_channel_system* ChannelSystem)
{
    Assert(Index < ChannelSystem->ChannelCount);
    b32 Result = true;
    
    if (Index == 0)
    {
        led_channel* FirstChannel = ChannelSystem->Channels;
        ChannelSystem->Channels = FirstChannel->Next;
        FirstChannel->Next = ChannelSystem->FreeList;
        ChannelSystem->FreeList = FirstChannel;
        ChannelSystem->ChannelCount--;
        Result = true;
    }
    else
    {
        led_channel* PrevChannel = ChannelSystem->Channels;
        for (s32 i = 0; i < Index - 1; i++)
        {
            PrevChannel = PrevChannel->Next;
        }
        
        Result = RemoveLEDChannel_(PrevChannel, PrevChannel->Next, ChannelSystem);
    }
    
    return Result;
}

internal led_channel*
GetChannelByIndex (s32 Index, led_channel_system ChannelSystem)
{
    Assert(Index < ChannelSystem.ChannelCount);
    led_channel* Result = ChannelSystem.Channels;
    
    for (s32 i = 0; i < Index; i++)
    {
        Result = Result->Next;
    }
    
    return Result;
}

internal void
AddPatternKeyToChannel (pattern_index_id_key Key, led_channel* Channel)
{
    Assert(Channel->ActivePatterns < CHANNEL_MAX_PATTERNS);
    Channel->Patterns[Channel->ActivePatterns++] = Key;
}

internal b32
RemovePatternKeyFromChannel (pattern_index_id_key Key, led_channel* Channel)
{
    b32 Result = false;
    
    s32 RemoveFrom = -1;
    for (s32 i = 0; i < Channel->ActivePatterns; i++)
    {
        if (Channel->Patterns[i].ID == Key.ID)
        {
            RemoveFrom = i;
        }
    }
    
    if (RemoveFrom >= 0)
    {
        for (s32 j = 0; j < Channel->ActivePatterns; j++)
        {
            Channel->Patterns[j] = Channel->Patterns[j + 1];
        }
        Channel->ActivePatterns--;
        Result = true;
    }
    
    return Result;
}

internal void
PushPatternKeyOnUpdateList (pattern_index_id_key Key, 
                            pattern_push_color_proc* PushColorProc,
                            patterns_update_list* UpdateList, 
                            memory_arena* Storage)
{
    if (UpdateList->Used >= UpdateList->Size)
    {
        if (!UpdateList->Next)
        {
            UpdateList->Next = PushStruct(Storage, patterns_update_list);
            UpdateList->Next->Size = UpdateList->Size;
            UpdateList->Next->Used = 0;
            UpdateList->Next->Patterns = PushArray(Storage, pattern_update_list_entry, UpdateList->Next->Size);
        }
        PushPatternKeyOnUpdateList(Key, PushColorProc, UpdateList->Next, Storage);
    }
    else
    {
        pattern_update_list_entry Entry = {};
        Entry.Key = Key;
        Entry.PushColorProc = PushColorProc;
        UpdateList->Patterns[UpdateList->Used++] = Entry;
    }
}

internal void
UpdateChannel (led_channel* Channel, 
               patterns_update_list* PatternsNeedUpdateList, 
               r32 DeltaTime,
               memory_arena* Storage)
{
    // Update Transition
    Channel->Transition.TimeElapsed += DeltaTime;
    if (Channel->Transition.TimeElapsed >= Channel->Transition.Duration ||
        (Channel->ActivePatternIndex < 0 && Channel->ActivePatterns > 0))
    {
        Channel->Transition.TimeElapsed -= Channel->Transition.Duration;
        Channel->ActivePatternIndex++;
        if (Channel->ActivePatternIndex >= Channel->ActivePatterns)
        {
            Channel->ActivePatternIndex = 0;
        }
    }
    
    // Create Active Pattern List
    if (Channel->ActivePatterns > 0)
    {
        Assert(Channel->ActivePatternIndex >= 0 && Channel->ActivePatternIndex < Channel->ActivePatterns);
        
        pattern_push_color_proc* PushColorProc = 0;
        switch (Channel->BlendMode)
        {
            case ChannelBlend_Override: { PushColorProc = PushColor_Override; } break;
            case ChannelBlend_Add: { PushColorProc = PushColor_Add; } break;
            case ChannelBlend_Multiply: { PushColorProc = PushColor_Multiply; } break;
            default:
            {
                InvalidCodePath;
            }break;
        }
        
        PushPatternKeyOnUpdateList(Channel->Patterns[Channel->ActivePatternIndex],
                                   PushColorProc,
                                   PatternsNeedUpdateList, Storage);
    }
}

internal patterns_update_list
UpdateAllChannels (led_channel_system* ChannelSystem, r32 DeltaTime, memory_arena* Transient)
{
    patterns_update_list Result = {};
    // NOTE(Peter): The initial size of this array is ChannelCount * 2 b/c at the moment, in the worst case, 
    // we need to update the two patterns a channel is blending between, and there isn't a case where we'd
    // update 3 per channel
    Result.Size = ChannelSystem->ChannelCount * 2;
    Result.Used = 0;
    Result.Patterns = PushArray(Transient, pattern_update_list_entry, Result.Size);
    Result.Next = 0;
    
    for (led_channel* Channel = ChannelSystem->Channels;
         Channel;
         Channel = Channel->Next)
    {
        UpdateChannel(Channel, &Result, DeltaTime, Transient);
    }
    
    return Result;
}
