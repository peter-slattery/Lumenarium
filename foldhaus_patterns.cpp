NODE_STRUCT(multiply_data)
{
    NODE_IN(r32, A);
    NODE_IN(r32, B);
    NODE_OUT(r32, Result);
};

NODE_PROC(MultiplyNodeProc, multiply_data)
{
    Data->Result = Data->A * Data->B;
}

NODE_STRUCT(add_data)
{
    NODE_IN(v4, A);
    NODE_IN(v4, B);
    NODE_OUT(v4, Result);
};

NODE_PROC(AddNodeProc, add_data)
{
    Data->Result = Data->A + Data->B;
}

//////////////////////////////////////
//
//       OLD - Pre Node System
//
///////////////////////////////////////

PATTERN_PUSH_COLOR_PROC(PushColor_Override)
{
    Colors[LED->Index].R = R;
    Colors[LED->Index].G = G;
    Colors[LED->Index].B = B;
}

PATTERN_PUSH_COLOR_PROC(PushColor_Add)
{
    Colors[LED->Index].R += R;
    Colors[LED->Index].G += G;
    Colors[LED->Index].B += B;
}

PATTERN_PUSH_COLOR_PROC(PushColor_Multiply)
{
    Colors[LED->Index].R *= R;
    Colors[LED->Index].G *= G;
    Colors[LED->Index].B *= B;
}

inline u32
PackColorStruct (u8* Channels)
{
    u32 Result = 0;
    Result |= (*Channels++ << 24);
    Result |= (*Channels++ << 16);
    Result |= (*Channels++ << 8);
    Result |= (255 << 0); // Alpha
    return Result;
};

inline u8
ToColorU8 (r32 V)
{
    return (u8)(V * 255.f);
}

inline u32
PackColorStructFromVector (v4 Color)
{
    u32 Result = ((ToColorU8(Color.r) << 24) |
                  (ToColorU8(Color.g) << 16) |
                  (ToColorU8(Color.b) << 8) |
                  (ToColorU8(Color.a) << 0));
    return Result;
}

internal void
InitLEDPatternSystem (led_pattern_system* PatternSystem, memory_arena* ParentStorage, 
                      s32 MaxPatternsCount, s32 PatternWorkingMemoryStorageSize)
{
    PatternSystem->Patterns = PushArray(ParentStorage, led_pattern, MaxPatternsCount);
    PatternSystem->PatternsUsed = 0;
    PatternSystem->PatternsMax = MaxPatternsCount;
    
    InitMemoryArena(&PatternSystem->PatternWorkingMemoryStorage, PushSize(ParentStorage, PatternWorkingMemoryStorageSize), PatternWorkingMemoryStorageSize, 0);
}

internal pattern_index_id_key
AddPattern (led_pattern_system* PatternSystem, 
            pattern_registry_entry* PatternSpec)
{
    Assert(PatternSystem->PatternsUsed < PatternSystem->PatternsMax);
    
    pattern_index_id_key Result = {};
    
    led_pattern* NewPattern = &PatternSystem->Patterns[PatternSystem->PatternsUsed];
    NewPattern->ID = PatternSystem->IDAccumulator++;
    NewPattern->Name = PatternSpec->Name;
    NewPattern->UpdateProc = PatternSpec->Update;
    PatternSpec->Init(NewPattern, &PatternSystem->PatternWorkingMemoryStorage);
    
    Result.Index = PatternSystem->PatternsUsed++;
    Result.ID = NewPattern->ID;
    
    return Result;
}

internal b32
RemovePattern (pattern_index_id_key Key, led_pattern_system* PatternSystem)
{
    b32 Result = false;
    s32 ActualIndex = -1;
    
    if (PatternSystem->Patterns[Key.Index].ID == Key.ID)
    {
        ActualIndex = Key.Index;
    }
    else
    {
        for (s32 i = 0; i < PatternSystem->PatternsUsed; i++)
        {
            if (PatternSystem->Patterns[i].ID == Key.ID)
            {
                ActualIndex = i;
                break;
            }
        }
    }
    
    if (ActualIndex >= 0)
    {
        for (s32 j = ActualIndex; j < PatternSystem->PatternsUsed - 1; j++)
        {
            PatternSystem->Patterns[j] = PatternSystem->Patterns[j + 1];
        }
        
        PatternSystem->PatternsUsed--;
        
        Result = true;
    }
    else
    {
        Result = false;
    }
    
    return Result;
}

internal led_pattern*
FindPatternAndUpdateIDKey (pattern_index_id_key* Key, led_pattern_system* PatternSystem)
{
    led_pattern* Result = 0;
    
    if (Key->Index < PatternSystem->PatternsUsed &&
        PatternSystem->Patterns[Key->Index].ID == Key->ID)
    {
        Result = &PatternSystem->Patterns[Key->Index];
    }
    else
    {
        for (s32 i = 0; i < PatternSystem->PatternsUsed; i++)
        {
            if (PatternSystem->Patterns[i].ID == Key->ID)
            {
                Result = &PatternSystem->Patterns[i];
                Key->Index = i;
                break;
            }
        }
    }
    
    return Result;
}

#if 0
internal void
UpdateAllPatterns_ (patterns_update_list* UpdateList, 
                    led_pattern_system* PatternSystem, 
                    pattern_led* LEDs, 
                    s32 LEDsCount,
                    r32 DeltaTime,
                    memory_arena* Transient)
{
    pattern_push_color_proc* PushColorProc = 0;
    
    for (s32 PatternKeyIdx = 0; PatternKeyIdx < UpdateList->Used; PatternKeyIdx++)
    {
        pattern_update_list_entry ListEntry = UpdateList->Patterns[PatternKeyIdx];
        pattern_index_id_key Key = ListEntry.Key;
        led_pattern* Pattern = FindPatternAndUpdateIDKey(&Key, PatternSystem);
        
        if (!Pattern)
        {
            Pattern = FindPatternAndUpdateIDKey(&Key, PatternSystem);
        }
        
        Pattern->UpdateProc(LEDs, LEDsCount,
                            Pattern->Memory,
                            DeltaTime, ListEntry.PushColorProc);
    }
    
    if (UpdateList->Next) 
    { 
        UpdateAllPatterns(UpdateList->Next, PatternSystem, LEDs, LEDsCount, DeltaTime, Transient); 
    }
}
#endif

internal void
UpdateAllPatterns (patterns_update_list* UpdateList, 
                   led_pattern_system* PatternSystem, 
                   led_buffer* LEDBuffer,
                   r32 DeltaTime,
                   memory_arena* Transient)
{
    pattern_push_color_proc* PushColorProc = 0;
    
    for (s32 PatternKeyIdx = 0; PatternKeyIdx < UpdateList->Used; PatternKeyIdx++)
    {
        pattern_update_list_entry ListEntry = UpdateList->Patterns[PatternKeyIdx];
        pattern_index_id_key Key = ListEntry.Key;
        led_pattern* Pattern = FindPatternAndUpdateIDKey(&Key, PatternSystem);
        
        if (!Pattern)
        {
            Pattern = FindPatternAndUpdateIDKey(&Key, PatternSystem);
        }
        
        led_buffer* LEDBufferIter = LEDBuffer;
        while(LEDBufferIter)
        {
            Pattern->UpdateProc(LEDBufferIter->LEDs, LEDBufferIter->Colors, LEDBufferIter->Count,
                                Pattern->Memory,
                                DeltaTime, ListEntry.PushColorProc);
            LEDBufferIter = LEDBufferIter->Next;
        }
    }
    
    if (UpdateList->Next) 
    { 
        UpdateAllPatterns(UpdateList->Next, PatternSystem, LEDBuffer, DeltaTime, Transient); 
    }
}


