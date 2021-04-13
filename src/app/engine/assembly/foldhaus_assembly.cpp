//
// File: foldhaus_assembly.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_ASSEMBLY_CPP

///////////////////////////
//
//  Assembly Array
//
///////////////////////////

internal assembly_array
AssemblyArray_Create(u32 CountMax, gs_memory_arena* Storage)
{
    assembly_array Result = {0};
    Result.CountMax = CountMax;
    Result.Values = PushArray(Storage, assembly, Result.CountMax);
    return Result;
}

internal u32
AssemblyArray_Push(assembly_array* Array, assembly Assembly)
{
    Assert(Array->Count < Array->CountMax);
    u32 Index = Array->Count++;
    Array->Values[Index] = Assembly;
    Array->Values[Index].AssemblyIndex = Index;
    return Index;
}

internal assembly*
AssemblyArray_Take(assembly_array* Array)
{
    u32 Index = AssemblyArray_Push(Array, {});
    assembly* Result = Array->Values + Index;
    return Result;
}

internal void
AssemblyArray_RemoveAt(assembly_array* Array, u32 Index)
{
    u32 LastAssemblyIndex = --Array->Count;
    Array->Values[Index] = Array->Values[LastAssemblyIndex];
}

typedef bool assembly_array_filter_proc(assembly A);
bool AssemblyFilter_OutputsViaSACN(assembly A) { return A.OutputMode == NetworkProtocol_SACN; }
bool AssemblyFilter_OutputsViaUART(assembly A) { return A.OutputMode == NetworkProtocol_UART; }

internal assembly_array
AssemblyArray_Filter(assembly_array Array, assembly_array_filter_proc* Filter, gs_memory_arena* Storage)
{
    assembly_array Result = AssemblyArray_Create(Array.Count, Storage);
    
    for (u32 i = 0; i < Array.Count; i++)
    {
        assembly At = Array.Values[i];
        if (Filter(At))
        {
            AssemblyArray_Push(&Result, At);
        }
    }
    
    return Result;
}

///////////////////////////
//
//  LedSystem
//
///////////////////////////

internal led_system
LedSystem_Create(gs_allocator PlatformMemory, u32 BuffersMax)
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

internal void
LedBufferSetLed(led_buffer* Buffer, u32 Led, v4 Position)
{
    Assert(Led < Buffer->LedCount);
    Buffer->Positions[Led] = Position;
}

internal u32
Assembly_ConstructStrip(assembly* Assembly, led_buffer* LedBuffer, v2_strip* StripAt, strip_gen_data GenData, v4 RootPosition, u32 LedStartIndex, u32 LedLUTStartIndex)
{
    u32 LedsAdded = 0;
    
    switch (GenData.Method)
    {
        case StripGeneration_InterpolatePoints:
        {
            strip_gen_interpolate_points InterpPoints = GenData.InterpolatePoints;
            v4 WS_StripStart = RootPosition + ToV4Point(InterpPoints.StartPosition * Assembly->Scale);
            v4 WS_StripEnd = RootPosition + ToV4Point(InterpPoints.EndPosition * Assembly->Scale);
            
            v4 SingleStep = (WS_StripEnd - WS_StripStart) / (r32)InterpPoints.LedCount;
            for (u32 Step = 0; Step < InterpPoints.LedCount; Step++)
            {
                s32 LedIndex = LedStartIndex + LedsAdded++;
                v4 LedPosition = WS_StripStart + (SingleStep * Step);
                LedBufferSetLed(LedBuffer, LedIndex, LedPosition);
                StripAt->LedLUT[Step + LedLUTStartIndex] = LedIndex;
            }
        }break;
        
        case StripGeneration_Sequence:
        {
            strip_gen_sequence Sequence = GenData.Sequence;
            for (u32 i = 0; i < Sequence.ElementsCount; i++)
            {
                strip_gen_data SegmentGenData = Sequence.Elements[i];
                LedsAdded += Assembly_ConstructStrip(Assembly, LedBuffer, StripAt, SegmentGenData, RootPosition, LedStartIndex + LedsAdded, LedsAdded);
            }
        }break;
        
        InvalidDefaultCase;
    }
    
    return LedsAdded;
}

internal void
ConstructAssemblyFromDefinition (assembly* Assembly, led_system* LedSystem)
{
    Assembly->LedBufferIndex = LedSystemTakeFreeBuffer(LedSystem, Assembly->LedCountTotal);
    led_buffer* LedBuffer = LedSystemGetBuffer(LedSystem, Assembly->LedBufferIndex);
    
    v4 RootPosition = ToV4Vec(Assembly->Center);
    
    // Add Leds
    u32 LedsAdded = 0;
    for (u32 StripIdx = 0; StripIdx < Assembly->StripCount; StripIdx++)
    {
        v2_strip* StripAt = &Assembly->Strips[StripIdx];
        StripAt->LedLUT = PushArray(&Assembly->Arena, u32, StripAt->LedCount);
        
        strip_gen_data GenData = StripAt->GenerationData;
        LedsAdded += Assembly_ConstructStrip(Assembly, LedBuffer, StripAt, GenData, RootPosition, LedsAdded, 0);
    }
}

internal assembly*
LoadAssembly (assembly_array* Assemblies, led_system* LedSystem, gs_memory_arena* Scratch, context Context, gs_const_string Path, log_buffer* GlobalLog)
{
    assembly* NewAssembly = 0;
    
    gs_file AssemblyFile = ReadEntireFile(Context.ThreadContext.FileHandler, Path);
    if (FileNoError(AssemblyFile))
    {
        gs_string AssemblyFileText = MakeString((char*)AssemblyFile.Memory);
        
        s32 IndexOfLastSlash = FindLast(Path, '\\');
        gs_const_string FileName = Substring(Path, IndexOfLastSlash + 1, Path.Length);
        
        NewAssembly = AssemblyArray_Take(Assemblies);
        NewAssembly->Arena = CreateMemoryArena(Context.ThreadContext.Allocator, "Assembly Arena");
        
        parser AssemblyParser = ParseAssemblyFile(NewAssembly, FileName, AssemblyFileText, Scratch);
        if (AssemblyParser.Success)
        {
            ConstructAssemblyFromDefinition(NewAssembly, LedSystem);
        }
        else
        {
            FreeMemoryArena(&NewAssembly->Arena);
            Assemblies->Count -= 1;
        }
        
        for (parser_error* ErrorAt = AssemblyParser.ErrorsRoot;
             ErrorAt != 0;
             ErrorAt = ErrorAt->Next)
        {
            OutputDebugString(ErrorAt->Message.Str);
        }
        
    }
    else
    {
        Log_Error(GlobalLog, "Unable to load assembly file");
    }
    
    return NewAssembly;
}

internal void
UnloadAssembly (u32 AssemblyIndex, app_state* State, context Context)
{
    Assert(AssemblyIndex < State->Assemblies.Count);
    assembly* Assembly = &State->Assemblies.Values[AssemblyIndex];
    LedSystemFreeBuffer(&State->LedSystem, Assembly->LedBufferIndex);
    FreeMemoryArena(&Assembly->Arena);
    AssemblyArray_RemoveAt(&State->Assemblies, AssemblyIndex);
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
        if (AssemblyStrip_HasTagValue(StripAt, NameHash, ValueHash))
        {
            Result.StripIndices[Result.Count++] = StripIndex;
        }
    }
    
    return Result;
}

#define FOLDHAUS_ASSEMBLY_CPP
#endif // FOLDHAUS_ASSEMBLY_CPP