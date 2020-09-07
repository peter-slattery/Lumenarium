//
// File: foldhaus_assembly.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_ASSEMBLY_CPP

internal led_system
LedSystemInitialize(gs_allocator PlatformMemory, u32 BuffersMax)
{
    led_system Result = {};
    Result.PlatformMemory = PlatformMemory;
    // TODO(Peter): Since we have access to PlatformMemory, just realloc Buffers when we fill it up
    Result.BuffersCountMax = BuffersMax;
    Result.Buffers = AllocatorAllocArray(PlatformMemory, led_buffer, Result.BuffersCountMax);
    return Result;
}

internal u32
LedSystemTakeFreeBuffer(led_system* System, u32 LedCount)
{
    s32 Result = -1;
    
    if (System->BuffersCount < System->BuffersCountMax)
    {
        Result = System->BuffersCount++;
    }
    else
    {
        // NOTE(Peter): Look for a buffer that's flagged as empty
        for (u32 i = 0; i < System->BuffersCount; i++)
        {
            if (System->Buffers[i].LedCount == 0
                && System->Buffers[i].Colors == 0
                && System->Buffers[i].Positions == 0)
            {
                Result = i;
                break;
            }
        }
        Assert(Result >= 0); // NOTE(Peter): We ran out of room for led buffers
    }
    
    led_buffer* Buffer = &System->Buffers[Result];
    Buffer->LedCount = LedCount;
    Buffer->Colors = AllocatorAllocArray(System->PlatformMemory, pixel, Buffer->LedCount);
    Buffer->Positions = AllocatorAllocArray(System->PlatformMemory, v4, Buffer->LedCount);
    
    System->LedsCountTotal += LedCount;
    
    return (u32)Result;
}

internal void
LedSystemFreeBuffer(led_system* System, u32 BufferIndex)
{
    Assert(BufferIndex < System->BuffersCountMax);
    led_buffer* Buffer = &System->Buffers[BufferIndex];
    AllocatorFreeArray(System->PlatformMemory, Buffer->Colors, pixel, Buffer->LedCount);
    AllocatorFreeArray(System->PlatformMemory, Buffer->Positions, v4, Buffer->LedCount);
    System->LedsCountTotal -= Buffer->LedCount;
    *Buffer = {};
}

internal led_buffer*
LedSystemGetBuffer(led_system* System, u32 Index)
{
    led_buffer* Result = &System->Buffers[Index];
    return Result;
}

internal void
LedBufferSetLed(led_buffer* Buffer, u32 Led, v4 Position)
{
    Assert(Led < Buffer->LedCount);
    Buffer->Positions[Led] = Position;
}

internal void
ConstructAssemblyFromDefinition (assembly* Assembly, gs_const_string AssemblyName, led_system* LedSystem)
{
    Assembly->LedBufferIndex = LedSystemTakeFreeBuffer(LedSystem, Assembly->LedCountTotal);
    led_buffer* LedBuffer = LedSystemGetBuffer(LedSystem, Assembly->LedBufferIndex);
    
    v4 RootPosition = ToV4Vec(Assembly->Center);
    
    // Add Leds
    u32 LedsAdded = 0;
    for (u32 StripIdx = 0; StripIdx < Assembly->StripCount; StripIdx++)
    {
        //led_strip_definition StripDef = Definition.LedStrips[StripIdx];
        v2_strip* StripAt = &Assembly->Strips[StripIdx];
        StripAt->LedLUT = PushArray(&Assembly->Arena, u32, StripAt->LedCount);
        
        v4 WS_StripStart = RootPosition + ToV4Point(StripAt->StartPosition * Assembly->Scale);
        v4 WS_StripEnd = RootPosition + ToV4Point(StripAt->EndPosition * Assembly->Scale);
        
        v4 SingleStep = (WS_StripEnd - WS_StripStart) / (r32)StripAt->LedCount;
        for (u32 Step = 0; Step < StripAt->LedCount; Step++)
        {
            s32 LedIndex = LedsAdded++;
            v4 LedPosition = WS_StripStart + (SingleStep * Step);
            LedBufferSetLed(LedBuffer, LedIndex, LedPosition);
            StripAt->LedLUT[Step] = LedIndex;
        }
    }
}

// NOTE(Peter): These are here so that if we load 2+ sculptures, they don't all
// end up on top of one another. Purely aesthetic. Can remove once we implement
// scene editing tools
static v4 TempAssemblyOffsets[] = { v4{0, 0, 0, 0}, v4{250, 0, 75, 0}, v4{-250, 0, 75, 0} };
s32 TempAssemblyOffsetsCount = 3;

internal void
LoadAssembly (assembly_array* Assemblies, led_system* LedSystem, gs_memory_arena* Scratch, context Context, gs_const_string Path, event_log* GlobalLog)
{
    gs_file AssemblyFile = ReadEntireFile(Context.ThreadContext.FileHandler, Path);
    if (FileNoError(AssemblyFile))
    {
        gs_string AssemblyFileText = MakeString((char*)AssemblyFile.Memory);
        
        Assert(Assemblies->Count < Assemblies->CountMax);
        assembly* NewAssembly = &Assemblies->Values[Assemblies->Count++];
        
        s32 IndexOfLastSlash = FindLast(Path, '\\');
        gs_const_string FileName = Substring(Path, IndexOfLastSlash + 1, Path.Length);
        
        NewAssembly->Arena = CreateMemoryArena(Context.ThreadContext.Allocator);
        if (ParseAssemblyFile(NewAssembly, FileName, AssemblyFileText, Scratch))
        {
            ConstructAssemblyFromDefinition(NewAssembly, FileName, LedSystem);
            AllocatorFree(Context.ThreadContext.Allocator, AssemblyFile.Memory, AssemblyFile.Size);
        }
        else
        {
            FreeMemoryArena(&NewAssembly->Arena);
            Assemblies->Count -= 1;
        }
    }
    else
    {
        LogError(GlobalLog, "Unable to load assembly file");
    }
}

internal void
UnloadAssembly (u32 AssemblyIndex, app_state* State, context Context)
{
    Assert(AssemblyIndex < State->Assemblies.Count);
    assembly* Assembly = &State->Assemblies.Values[AssemblyIndex];
    LedSystemFreeBuffer(&State->LedSystem, Assembly->LedBufferIndex);
    FreeMemoryArena(&Assembly->Arena);
    u32 LastAssemblyIndex = --State->Assemblies.Count;
    State->Assemblies.Values[AssemblyIndex] = State->Assemblies.Values[LastAssemblyIndex];
}

// Querying Assemblies

internal led_strip_list
AssemblyStripsGetWithTagValue(assembly Assembly, gs_const_string TagName, gs_const_string TagValue, gs_memory_arena* Storage)
{
    led_strip_list Result = {0};
    // TODO(pjs): @Optimization
    // We can probably come back here and do this allocation procedurally, or in buckets, or with
    // a linked list. But for now, I just want to get this up and running
    Result.CountMax = Assembly.StripCount;
    Result.StripIndices = PushArray(Storage, u32, Result.CountMax);
    
    u64 NameHash = HashDJB2ToU32(StringExpand(TagName));
    u64 ValueHash = 0;
    if (TagValue.Length > 0)
    {
        ValueHash = HashDJB2ToU32(StringExpand(TagValue));
    }
    
    for (u32 StripIndex = 0; StripIndex < Assembly.StripCount; StripIndex++)
    {
        v2_strip StripAt = Assembly.Strips[StripIndex];
        for (u32 j = 0; j < StripAt.TagsCount; j++)
        {
            v2_tag TagAt = StripAt.Tags[j];
            if (TagAt.NameHash == NameHash)
            {
                // NOTE(pjs): We can pass an empty string to the Value parameter,
                // and it will match all values of Tag
                if (ValueHash == 0 || ValueHash == TagAt.ValueHash)
                {
                    Result.StripIndices[Result.Count++] = StripIndex;
                }
            }
        }
    }
    
    return Result;
}

#define FOLDHAUS_ASSEMBLY_CPP
#endif // FOLDHAUS_ASSEMBLY_CPP