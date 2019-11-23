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
                                 v3 RootPosition,
                                 r32 Scale,
                                 u8* MemoryBase,
                                 s32 MemorySize)
{
    assembly Assembly = {};
    Assembly.Arena = CreateMemoryArena(MemoryBase, MemorySize);
    
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
        
        v4 WS_StripStart = V4(StripDef.InterpolatePositionStart * Scale, 1);
        v4 WS_StripEnd = V4(StripDef.InterpolatePositionEnd * Scale, 1);
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