internal s32
GetAssemblyMemorySizeFromDefinition(assembly_definition Definition, string Name)
{
    s32 Result = (sizeof(led) + sizeof(pixel)) * Definition.TotalLEDCount;
    Result += sizeof(leds_in_universe_range) * Definition.LEDStripCount;
    Result += Name.Length;
    return Result;
}

internal assembly
ConstructAssemblyFromDefinition (assembly_definition Definition,
                                 string AssemblyName,
                                 v4 RootPosition,
                                 r32 Scale,
                                 memory_arena Arena)
{
    assembly Assembly = {};
    Assembly.Arena = Arena;
    
    Assembly.Name = MakeString(PushArray(&Assembly.Arena, char, AssemblyName.Length), AssemblyName.Length);
    CopyStringTo(AssemblyName, &Assembly.Name);
    
    // NOTE(Peter): Setting this to zero so we can check at the end of the loop that creates leds
    // and make sure we created to correct number. By the time this function returns it should be
    // the case that: (Assembly.LEDCount == Definition.TotalLEDCount)
    Assembly.LEDCount = 0;
    Assembly.Colors = PushArray(&Assembly.Arena, pixel, Definition.TotalLEDCount);
    Assembly.LEDs = PushArray(&Assembly.Arena, led, Definition.TotalLEDCount);
    Assembly.LEDUniverseMapCount = Definition.LEDStripCount;
    Assembly.LEDUniverseMap = PushArray(&Assembly.Arena, leds_in_universe_range, Definition.LEDStripCount);
    
    // Add LEDs
    for (s32 StripIdx = 0; StripIdx < Definition.LEDStripCount; StripIdx++)
    {
        led_strip_definition StripDef = Definition.LEDStrips[StripIdx];
        
        leds_in_universe_range* LEDUniverseRange = Assembly.LEDUniverseMap + StripIdx;
        LEDUniverseRange->Universe = StripDef.StartUniverse;
        LEDUniverseRange->RangeStart = Assembly.LEDCount;
        LEDUniverseRange->RangeOnePastLast = Assembly.LEDCount + StripDef.LEDsPerStrip;
        
        // NOTE(Peter): this should be a switch on the type, but we only have one for
        // now. The assert is to remind you to create more cases when necessary
        Assert(StripDef.InterpolationType == StripInterpolate_Points);
        
        v4 WS_StripStart = RootPosition + V4(StripDef.InterpolatePositionStart * Scale, 1);
        v4 WS_StripEnd = RootPosition + V4(StripDef.InterpolatePositionEnd * Scale, 1);
        s32 LEDsInStripCount = StripDef.LEDsPerStrip;
        
        Assert(Assembly.LEDCount + LEDsInStripCount <= Definition.TotalLEDCount);
        
        v4 SingleStep = (WS_StripEnd - WS_StripStart) / (r32)LEDsInStripCount;
        for (s32 Step = 0; Step < LEDsInStripCount; Step++)
        {
            s32 LEDIndex = Assembly.LEDCount++;
            Assembly.LEDs[LEDIndex].Position = WS_StripStart + (SingleStep * Step);
            Assembly.LEDs[LEDIndex].Index = LEDIndex;
        }
    }
    
    // NOTE(Peter): Did we create the correct number of LEDs?
    Assert(Assembly.LEDCount == Definition.TotalLEDCount);
    return Assembly;
}

static v4 TempAssemblyOffsets[] = { v4{0, 0, 0, 0}, v4{250, 0, 75, 0}, v4{-250, 0, 75, 0} };
s32 TempAssemblyOffsetsCount = 3;

internal void
LoadAssembly (app_state* State, context Context, char* Path)
{
    platform_memory_result TestAssemblyFile = Context.PlatformReadEntireFile(Path);
    Assert(TestAssemblyFile.Size > 0);
    
    assembly_definition AssemblyDefinition = ParseAssemblyFile(TestAssemblyFile.Base, TestAssemblyFile.Size, &State->Transient);
    
    Context.PlatformFree(TestAssemblyFile.Base, TestAssemblyFile.Size);
    
    string PathString = MakeStringLiteral(Path);
    s32 IndexOfLastSlash = FastLastIndexOfCharInCharArray(PathString.Memory, PathString.Length, '\\');
    string FileName = Substring(PathString, IndexOfLastSlash + 1);
    
    r32 Scale = 100;
    memory_arena AssemblyArena = {};
    AssemblyArena.Alloc = (gs_memory_alloc*)Context.PlatformAlloc;
    AssemblyArena.Realloc = (gs_memory_realloc*)Context.PlatformRealloc;
    
    v4 Offset = TempAssemblyOffsets[State->ActiveAssemblyIndecies.Used % TempAssemblyOffsetsCount];
    assembly NewAssembly = ConstructAssemblyFromDefinition(AssemblyDefinition, 
                                                           FileName, 
                                                           Offset, 
                                                           Scale, 
                                                           AssemblyArena);
    gs_list_handle NewAssemblyHandle = State->AssemblyList.PushElementOnList(NewAssembly);
    State->ActiveAssemblyIndecies.PushElementOnList(NewAssemblyHandle);
    
    State->TotalLEDsCount += NewAssembly.LEDCount;
}

internal void
UnloadAssembly (u32 AssemblyIndex, app_state* State, context Context)
{
    assembly* Assembly = State->AssemblyList.GetElementAtIndex(AssemblyIndex);
    State->TotalLEDsCount -= Assembly->LEDCount;
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
