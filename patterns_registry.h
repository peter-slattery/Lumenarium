pattern_registry_entry PatternRegistry[] = 
{
    {"Solid", SolidPatternInitProc, SolidPatternUpdateProc},
    {"Rainbow", InitRainbowPatternProc, RainbowPatternProc},
    {"Radial", InitRadialProc, UpdateRadialProc},
};