//
// File: foldhaus_assembly.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_ASSEMBLY_CPP

internal void
ConstructAssemblyFromDefinition (assembly* Assembly, string AssemblyName, v4 RootPosition)
{
    Assembly->LEDBuffer.LEDCount = 0;
    Assembly->LEDBuffer.Colors = PushArray(&Assembly->Arena, pixel, Assembly->LedCountTotal);
    Assembly->LEDBuffer.LEDs = PushArray(&Assembly->Arena, led, Assembly->LedCountTotal);
    Assembly->LEDUniverseMapCount = Assembly->LedCountTotal;
    Assembly->LEDUniverseMap = PushArray(&Assembly->Arena, leds_in_universe_range, Assembly->LedCountTotal);
    
    // Add LEDs
    for (u32 StripIdx = 0; StripIdx < Assembly->StripCount; StripIdx++)
    {
        //led_strip_definition StripDef = Definition.LEDStrips[StripIdx];
        v2_strip* StripAt = &Assembly->Strips[StripIdx];
        
        leds_in_universe_range* LEDUniverseRange = &Assembly->LEDUniverseMap[StripIdx];
        
        LEDUniverseRange->Universe = StripAt->StartUniverse;
        LEDUniverseRange->RangeStart = Assembly->LEDBuffer.LEDCount;
        LEDUniverseRange->RangeOnePastLast = Assembly->LEDBuffer.LEDCount + StripAt->LedCount;
        
        v4 WS_StripStart = RootPosition + V4(StripAt->StartPosition * Assembly->Scale, 1);
        v4 WS_StripEnd = RootPosition + V4(StripAt->EndPosition * Assembly->Scale, 1);
        s32 LEDsInStripCount = StripAt->LedCount;
        
        v4 SingleStep = (WS_StripEnd - WS_StripStart) / (r32)LEDsInStripCount;
        for (s32 Step = 0; Step < LEDsInStripCount; Step++)
        {
            s32 LEDIndex = Assembly->LEDBuffer.LEDCount;
            Assembly->LEDBuffer.LEDs[LEDIndex].Position = WS_StripStart + (SingleStep * Step);
            Assembly->LEDBuffer.LEDs[LEDIndex].Index = LEDIndex;
            Assembly->LEDBuffer.LEDCount += 1;
        }
    }
}

// NOTE(Peter): These are here so that if we load 2+ sculptures, they don't all
// end up on top of one another. Purely aesthetic. Can remove once we implement
// scene editing tools
static v4 TempAssemblyOffsets[] = { v4{0, 0, 0, 0}, v4{250, 0, 75, 0}, v4{-250, 0, 75, 0} };
s32 TempAssemblyOffsetsCount = 3;

internal void
LoadAssembly (app_state* State, context Context, string Path)
{
    platform_memory_result AssemblyFile = ReadEntireFile(Context, Path);
    if (AssemblyFile.Error == PlatformMemory_NoError)
    {
        string AssemblyFileText = MakeString((char*)AssemblyFile.Base);
        
        s32 IndexOfLastSlash = FastLastIndexOfCharInCharArray(Path.Memory, Path.Length, '\\');
        string FileName = Substring(Path, IndexOfLastSlash + 1);
        
        assembly NewAssembly = {};
        NewAssembly.Arena.Alloc = (gs_memory_alloc*)Context.PlatformAlloc;
        NewAssembly.Arena.Realloc = (gs_memory_realloc*)Context.PlatformRealloc;
        ParseAssemblyFile(&NewAssembly, AssemblyFileText, &State->Transient);
        
        v4 Offset = TempAssemblyOffsets[State->ActiveAssemblyIndecies.Used % TempAssemblyOffsetsCount];
        ConstructAssemblyFromDefinition(&NewAssembly, FileName, Offset);
        gs_list_handle NewAssemblyHandle = State->AssemblyList.PushElementOnList(NewAssembly);
        
        State->ActiveAssemblyIndecies.PushElementOnList(NewAssemblyHandle);
        State->TotalLEDsCount += NewAssembly.LEDBuffer.LEDCount;
        
        Context.PlatformFree(AssemblyFile.Base, AssemblyFile.Size);
    }
    else
    {
        LogError(State->GlobalLog, "Unable to load assembly file");
    }
}

internal void
UnloadAssembly (u32 AssemblyIndex, app_state* State, context Context)
{
    assembly* Assembly = State->AssemblyList.GetElementAtIndex(AssemblyIndex);
    State->TotalLEDsCount -= Assembly->LEDBuffer.LEDCount;
    FreeMemoryArena(&Assembly->Arena, (gs_memory_free*)Context.PlatformFree);
    
    State->AssemblyList.FreeElementAtIndex(AssemblyIndex);
    for (u32 i = 0; i < State->ActiveAssemblyIndecies.Used; i++)
    {
        gs_list_handle Handle = *State->ActiveAssemblyIndecies.GetElementAtIndex(i);
        if (Handle.Index == AssemblyIndex)
        {
            State->ActiveAssemblyIndecies.FreeElementAtIndex(i);
            break;
        }
    }
}


#define FOLDHAUS_ASSEMBLY_CPP
#endif // FOLDHAUS_ASSEMBLY_CPP