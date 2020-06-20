//
// File: foldhaus_assembly.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_ASSEMBLY_CPP

// Led System

internal led_system
LedSystemInitialize(platform_memory_handler PlatformMemory, u32 BuffersMax)
{
    led_system Result = {};
    Result.PlatformMemory = PlatformMemory;
    // TODO(Peter): Since we have access to PlatformMemory, just realloc Buffers when we fill it up
    Result.BuffersCountMax = BuffersMax;
    Result.Buffers = PlatformAllocArray(PlatformMemory, led_buffer, Result.BuffersCountMax);
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
    Buffer->Colors = PlatformAllocArray(System->PlatformMemory, pixel, Buffer->LedCount);
    Buffer->Positions = PlatformAllocArray(System->PlatformMemory, v4, Buffer->LedCount);
    
    System->LedsCountTotal += LedCount;
    
    return (u32)Result;
}

internal void
LedSystemFreeBuffer(led_system* System, u32 BufferIndex)
{
    Assert(BufferIndex < System->BuffersCountMax);
    led_buffer* Buffer = &System->Buffers[BufferIndex];
    PlatformFreeArray(System->PlatformMemory, Buffer->Colors, pixel, Buffer->LedCount);
    PlatformFreeArray(System->PlatformMemory, Buffer->Positions, v4, Buffer->LedCount);
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

// Assembly

internal void
ConstructAssemblyFromDefinition (assembly* Assembly, string AssemblyName, v4 RootPosition, led_system* LedSystem)
{
    Assembly->LedBufferIndex = LedSystemTakeFreeBuffer(LedSystem, Assembly->LedCountTotal);
    led_buffer* LedBuffer = LedSystemGetBuffer(LedSystem, Assembly->LedBufferIndex);
    
    // Add Leds
    u32 LedsAdded = 0;
    for (u32 StripIdx = 0; StripIdx < Assembly->StripCount; StripIdx++)
    {
        //led_strip_definition StripDef = Definition.LedStrips[StripIdx];
        v2_strip* StripAt = &Assembly->Strips[StripIdx];
        StripAt->LedLUT = PushArray(&Assembly->Arena, u32, StripAt->LedCount);
        
        v4 WS_StripStart = RootPosition + V4(StripAt->StartPosition * Assembly->Scale, 1);
        v4 WS_StripEnd = RootPosition + V4(StripAt->EndPosition * Assembly->Scale, 1);
        
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

// TODO(Peter): Don't reference State, pull back to the Led buffer and Assembly Array
internal void
LoadAssembly (app_state* State, context Context, string Path)
{
    platform_memory_result AssemblyFile = ReadEntireFile(Context, Path);
    if (AssemblyFile.Error == PlatformMemory_NoError && AssemblyFile.Size > 0)
    {
        string AssemblyFileText = MakeString((char*)AssemblyFile.Base);
        
        Assert(State->Assemblies.Count < State->Assemblies.CountMax);
        assembly* NewAssembly = &State->Assemblies.Values[State->Assemblies.Count++];
        
        s32 IndexOfLastSlash = FastLastIndexOfCharInCharArray(Path.Memory, Path.Length, '\\');
        string FileName = Substring(Path, IndexOfLastSlash + 1);
        
        NewAssembly->Arena.PlatformMemory = Context.PlatformMemory;
        if (ParseAssemblyFile(NewAssembly, AssemblyFileText, &State->Transient))
        {
            v4 Offset = TempAssemblyOffsets[State->Assemblies.Count % TempAssemblyOffsetsCount];
            ConstructAssemblyFromDefinition(NewAssembly, FileName, Offset, &State->LedSystem);
            PlatformFree(Context.PlatformMemory, AssemblyFile.Base, AssemblyFile.Size);
        }
        else
        {
            FreeMemoryArena(&NewAssembly->Arena);
            State->Assemblies.Count -= 1;
        }
    }
    else
    {
        LogError(State->GlobalLog, "Unable to load assembly file");
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


#define FOLDHAUS_ASSEMBLY_CPP
#endif // FOLDHAUS_ASSEMBLY_CPP