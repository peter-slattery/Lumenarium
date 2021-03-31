//
// File: blumen_patterns.h
// Author: Peter Slattery
// Creation Date: 2021-01-15
//
#ifndef BLUMEN_PATTERNS_H

#define FLOWER_COLORS_COUNT 12

internal pixel
V4ToRGBPixel(v4 C)
{
    pixel Result = {};
    Result.R = (u8)(C.x * 255);
    Result.G = (u8)(C.y * 255);
    Result.B = (u8)(C.z * 255);
    return Result;
}

internal void
Pattern_AltBloomMask(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    Time = Time * BLState->PatternSpeed;
    
    v3 SphereCenter = Assembly.Center - v3{0, -150, 0};
    r32 SphereRadius = Time;
    r32 SphereBrightness = 1;
    
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        v3 P = Leds->Positions[LedIndex].xyz;
        r32 Sphere = SDF_SphereNormalized(P, SphereCenter, SphereRadius);
        Sphere = Clamp01(-Sphere);
        Leds->Colors[LedIndex] = V4ToRGBPixel(WhiteV4 * Sphere);
    }
}

internal void
Pattern_HueShift(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    Time = Time * BLState->PatternSpeed;
    
    r32 Height = SinR32(Time) * 25;
    
    r32 CycleLength = 5.0f;
    r32 CycleProgress = FractR32(Time / CycleLength);
    r32 CycleBlend = (SinR32(Time) * .5f) + .5f;
    
    v4 HSV = { CycleProgress * 360, 1, 1, 1 };
    v4 RGB = HSVToRGB(HSV);
    
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        v4 Pos = Leds->Positions[LedIndex];
        r32 Dist = Pos.y - Height;
        
        //v4 HSV = { (ModR32(Dist, 25) / 25) * 360, 1, 1, 1 };
        //v4 RGB = HSVToRGB(HSV);
        
        u8 R = (u8)(RGB.x * 255);
        u8 G = (u8)(RGB.y * 255);
        u8 B = (u8)(RGB.z * 255);
        
        Leds->Colors[LedIndex].R = R;
        Leds->Colors[LedIndex].G = G;
        Leds->Colors[LedIndex].B = B;
    }
}

internal void
Pattern_Rainbow(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    Time = Time * BLState->PatternSpeed;
    
    r32 HueBase = ModR32(Time * 50, 360);
    
    r32 CycleLength = 5.0f;
    r32 CycleProgress = FractR32(Time / CycleLength);
    r32 CycleBlend = (SinR32(Time) * .5f) + .5f;
    
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        v4 Pos = Leds->Positions[LedIndex];
        r32 Hue = HueBase + Pos.y + Pos.x;
        v4 HSV = { Hue, 1, 1, 1 };
        v4 RGB = HSVToRGB(HSV);
        
        Leds->Colors[LedIndex] = V4ToRGBPixel(RGB);
    }
}

internal void
Pattern_RadialRainbow(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    Time = Time * BLState->PatternSpeed;
    
    v2 RefVector = V2Normalize(v2{ SinR32(Time), CosR32(Time) });
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        v2 Vector = v2{
            Leds->Positions[LedIndex].x,
            Leds->Positions[LedIndex].z
        };
        Vector = V2Normalize(Vector);
        
        r32 Angle = V2Dot(RefVector, Vector);
        
        v4 HSV = { (Angle * 30) + (Time * 10), 1, 1, 1 };
        v4 RGB = HSVToRGB(HSV);
        
        Leds->Colors[LedIndex] = V4ToRGBPixel(RGB);
    }
}

internal void
Pattern_BasicFlowers(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    Time = Time * BLState->PatternSpeed;
    
    phrase_hue Hue = BLState->AssemblyColors[Assembly.AssemblyIndex % 3];
    v4 C0 = HSVToRGB({Hue.Hue0, 1, 1, 1});
    v4 C1 = HSVToRGB({Hue.Hue1, 1, 1, 1});
    v4 C2 = HSVToRGB({Hue.Hue2, 1, 1, 1});
    
    for (u32 StripIndex = 0; StripIndex < Assembly.StripCount; StripIndex++)
    {
        v2_strip Strip = Assembly.Strips[StripIndex];
        r32 CycleT = ModR32(Time, 10) * 20;
        
        for (u32 i = 0; i < Strip.LedCount; i++)
        {
            u32 LedIndex = Strip.LedLUT[i];
            v4 P = Leds->Positions[LedIndex];
            
            r32 T = ModR32(P.y + CycleT, 200) / 200.f;
            T = Clamp01(T);
            
            v4 Color = {};
            if (T < 0.5f) 
            {
                Color = V4Lerp(T * 2, C0, C1);
            } 
            else 
            {
                Color = V4Lerp((T - 0.5f) * 2, C1, C2);
            }
            Leds->Colors[LedIndex] = V4ToRGBPixel(Color);
        }
    }
}

internal void
Pattern_Wavy(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    DEBUG_TRACK_FUNCTION;
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    Time = Time * BLState->PatternSpeed;
    
    phrase_hue Hue = BlumenLumen_GetCurrentHue(BLState, Assembly);
    v4 C0 = HSVToRGB({Hue.Hue0, 1, 1, 1});
    v4 C1 = HSVToRGB({Hue.Hue1, 1, 1, 1});
    v4 C2 = HSVToRGB({Hue.Hue2, 1, 1, 1});
    
    r32 Top = 120 + (SinR32(Time) * 10);
    r32 Mid = 70 + (CosR32(Time * 2.13) * 20);
    r32 Bot = 0;
    
    r32 TopD = Top - Mid;
    r32 BotD = Mid - Bot;
    r32 MidD = Min(TopD, BotD);
    
    //r32 MaxFadeDistance = 10;
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        v3 P = Leds->Positions[LedIndex].xyz;
        
        r32 PercentTop = Clamp01(1.0f - ((Top - P.y) / TopD));
        
        r32 PercentMid = Clamp01(1.0f - Abs(P.y - Mid) / MidD);
        r32 N = Noise3D((P / 17) + v3{Time, -Time, 0});
        N = Clamp01(N) * 2;
        N = Smoothstep(N);
        N *= N;
        N = Smoothstep(N);
        N *= 1.0f - PowR32(1.0f - PercentMid, 4);
        PercentMid = Clamp01(PercentMid + N);
        
        r32 PercentBot = Clamp01(1.0f - ((P.y - Bot) / BotD));
        
        v4 TopC = (C0 * PercentTop);
        v4 MidC = (C1 * PercentMid);
        v4 BotC = (C2 * PercentBot);
        
        v4 C = {};
        if (PercentTop > PercentMid && PercentTop > PercentBot)
        {
            C = C0;
        }
        else if (PercentMid > PercentBot)
        {
            C = C1;
        }
        else
        {
            C = C2;
        }
        
        r32 ScaleFactor = PercentTop + PercentMid + PercentBot;
        C = (TopC + MidC + BotC) / ScaleFactor;
        Leds->Colors[LedIndex] = V4ToRGBPixel(C);
    }
}

internal void
Pattern_Patchy(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    DEBUG_TRACK_FUNCTION;
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    Time = Time * BLState->PatternSpeed;
    
    phrase_hue Hue = BlumenLumen_GetCurrentHue(BLState, Assembly);
    v4 C0 = HSVToRGB({Hue.Hue0, 1, 1, 1});
    v4 C1 = HSVToRGB({Hue.Hue1, 1, 1, 1});
    
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        v4 P = Leds->Positions[LedIndex];
        r32 LedRange = 300.0f;
        r32 ScaleFactor = 1.0f / LedRange;
        v3 Pp = P.xyz + v3{150, 100, 0};
        
        r32 NoiseA = Noise3D((Pp / 38) + v3{0, 0, Time});
        NoiseA = PowR32(NoiseA, 3);
        NoiseA = Smoothstep(NoiseA);
        v4 CA = C0 * NoiseA;
        
        r32 NoiseB = Noise3D((Pp / 75) + v3{Time * 0.5f, 0, 0});
        NoiseB = PowR32(NoiseB, 3);
        NoiseB = Smoothstep(NoiseB);
        v4 CB = C1 * NoiseB;
        
        v4 C = (C0 * NoiseA) + (C1 * NoiseB);
        C /= (NoiseA + NoiseB);
        Leds->Colors[LedIndex] = V4ToRGBPixel(C);
    }
}

internal r32
Leafy_BandSDF(v3 P, gs_random_series* Random, r32 Time)
{
    r32 MinBandThickness = 5;
    r32 MaxBandThickness = 10;
    r32 MaxTransitionPeriod = 120.0f;
    
    r32 BandTransitionPeriod = NextRandomUnilateral(Random) * MaxTransitionPeriod;
    r32 BandTransitionBias = (1 - Clamp(0, (Time / (MaxTransitionPeriod / 2)), 0.7f)); // approaches 0.5 over time
    BandTransitionPeriod *= BandTransitionBias;
    
    r32 BandPercent = ModR32(Time, BandTransitionPeriod) / BandTransitionPeriod;
    BandPercent = Smoothstep(BandPercent);
    r32 BandY = -150 + (BandPercent * 290);
    
    r32 ThickRand = NextRandomUnilateral(Random);
    // 1 - 4((ThickRand - .5)^2) - distribution curve
    ThickRand = 1.0f - ((4 * PowR32(ThickRand, 2)) - (4 * ThickRand) + 1);
    r32 BandThickness = MinBandThickness + (ThickRand * (MaxBandThickness - MinBandThickness));
    
    // BandBrightness = 1 - ((2x - 1) ^ 8) where x is BandPercent
    r32 BandBrightness = 1.0f - PowR32((2 * BandPercent) - 1, 8);
    BandBrightness *= RemapR32(NextRandomUnilateral(Random), 0, 1, .25f, 1);
    r32 Result = 1 - Clamp01(Abs(P.y - BandY) / BandThickness);
    Result *= BandBrightness;
    return Result;
}

internal void
Pattern_Leafy(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    DEBUG_TRACK_FUNCTION;
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    Time = Time * BLState->PatternSpeed;
    
    phrase_hue Hue = BlumenLumen_GetCurrentHue(BLState, Assembly);
    v4 C0 = HSVToRGB({Hue.Hue0, 1, 1, 1});
    v4 C1 = HSVToRGB({Hue.Hue1, 1, 1, 1});
    v4 C2 = HSVToRGB({Hue.Hue2, 1, 1, 1});
    
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        v4 P = Leds->Positions[LedIndex];
        
        v4 C = {};
        r32 B = 0;
        
        // NOTE(PS): initializing the Random seed inside the Led Loop
        // so that the bands are consistently calculated for each led
        // ie. each time you calculate a band, the random numbers requested
        // will always be the same
        gs_random_series Random = InitRandomSeries(24601);
        u32 BandCount = 25;
        for (u32 Band = 0; Band < BandCount; Band++)
        {
            B += Leafy_BandSDF(P.xyz, &Random, Time);
        }
        B = Clamp01(B);
        
        C = WhiteV4 * B;
        Leds->Colors[LedIndex] = V4ToRGBPixel(C);
    }
}

internal void
Pattern_LeafyPatchy(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    DEBUG_TRACK_FUNCTION;
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    Time = Time * BLState->PatternSpeed;
    
    phrase_hue Hue = BlumenLumen_GetCurrentHue(BLState, Assembly);
    v4 C0 = HSVToRGB({Hue.Hue0, 1, 1, 1});
    v4 C1 = HSVToRGB({Hue.Hue1, 1, 1, 1});
    v4 C2 = HSVToRGB({Hue.Hue2, 1, 1, 1});
    
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        v4 P = Leds->Positions[LedIndex];
        v3 Pp = P.xyz + v3{150, 100, 0};
        
        r32 NoiseA = Fbm3D((Pp / 18), Time * 0.25f);
        NoiseA = Smoothstep(NoiseA);
        
        r32 NoiseB = Noise3D((Pp / 35) + v3{0, 0, Time * 0.5f});
        NoiseB = PowR32(NoiseB, 3);
        NoiseB = Smoothstep(NoiseB);
        
        r32 NoiseC = Noise3D((Pp / 25) + v3{0, 0, Time * 4});
        r32 CPresence = SinR32((P.y / 50) - Time) + (0.8f * SinR32((P.y / 25) - (Time * 5.0f)));
        CPresence = RemapR32(CPresence, -1.8, 1.8, 0, 1);
        CPresence = PowR32(CPresence, 4);
        NoiseC *= CPresence;
        
        v4 C = (C0 * NoiseA * 0.5f) + (C1 * NoiseB) + (C2 * NoiseC);
        C *= 1.0f / (NoiseA + NoiseB + NoiseC);
        Leds->Colors[LedIndex] = V4ToRGBPixel(C);
    }
}

internal void
Pattern_WavyPatchy(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    Time = Time * BLState->PatternSpeed;
}

internal void
Pattern_VerticalLines(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    DEBUG_TRACK_FUNCTION;
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    Time = Time * BLState->PatternSpeed;
    
    gs_random_series Random = InitRandomSeries(24601);
    
    r32 LightSpeedMin = 1;
    r32 LightSpeedMax = 5;
    
    s32 LightTailLength = 10;
    for (u32 StripIndex = 0; StripIndex < Assembly.StripCount; StripIndex++)
    {
        v2_strip Strip = Assembly.Strips[StripIndex];
        
        r32 LightStartHeight = NextRandomUnilateral(&Random);
        r32 LightSpeed = LerpR32(NextRandomUnilateral(&Random), 
                                 LightSpeedMin,
                                 LightSpeedMax);
        r32 LightCurrentHeight = LightStartHeight + (LightSpeed * Time * 0.1f);
        s32 StartIndex = (s32)(LightCurrentHeight * (r32)Strip.LedCount) % Strip.LedCount;
        
        for (s32 i = 0; i < LightTailLength; i++)
        {
            s32 StripLedIndex = StartIndex + i;
            if (StripLedIndex >= (s32)Strip.LedCount) continue;
            
            u32 LedIndex = Strip.LedLUT[StripLedIndex];
            r32 PctTail = ((r32)i / (r32)LightTailLength);
            v4 C = WhiteV4 * PctTail;
            Leds->Colors[LedIndex] = V4ToRGBPixel(C);
        }
    }
}

internal void
Pattern_Rotary(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    Time = Time * BLState->PatternSpeed;
    
    gs_random_series Random = InitRandomSeries(24601);
    
#define SphereCount 32
    v3 SphereCenter[SphereCount];
    
    r32 MaxHeightOffset = 150;
    r32 MaxSpeed = 10;
    r32 SphereRadius = 2.0f;
    for (u32 i = 0; i < SphereCount; i++)
    {
        r32 SphereSeedA = NextRandomBilateral(&Random);
        r32 SphereSeedB = NextRandomBilateral(&Random);
        r32 SphereSpeed = NextRandomUnilateral(&Random) * MaxSpeed;
        
        r32 SphereTime = Time + SphereSpeed;
        r32 HeightOffset = SphereTime + (SphereSeedA * MaxHeightOffset);
        r32 RotationOffset = SphereTime + SphereSeedB * TauR32;
        r32 SphereRotationDir = NextRandomBilateral(&Random) < 0 ? -1 : 1;
        v3 SpherePosOffset = v3{
            SinR32(RotationOffset * SphereRotationDir) * (SphereRadius * 2), 
            HeightOffset, 
            CosR32(RotationOffset * SphereRotationDir) * (SphereRadius * 2)
        };
        SphereCenter[i] = Assembly.Center + SpherePosOffset;
    }
    
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        v3 P = Leds->Positions[LedIndex].xyz;
        
        r32 Dist = 10000000;
        for (u32 i = 0; i < SphereCount; i++)
        {
            r32 SphereSDF = Abs(SDF_Sphere(P, SphereCenter[i], SphereRadius));
            SphereSDF = SphereSDF / SphereRadius;
            Dist = Min(Dist, SphereSDF);
        }
        
        v4 C = BlackV4;
        if (Dist <= 1)
        {
            r32 Brightness = Clamp01(SphereRadius - Dist);
            C = WhiteV4 * Brightness;
        }
        
        Leds->Colors[LedIndex] = V4ToRGBPixel(C);
    }
}

internal void
Pattern_AllOnMask(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    pixel White = V4ToRGBPixel(WhiteV4);
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        v3 P = Leds->Positions[LedIndex].xyz;
        Leds->Colors[LedIndex] = White;
    }
}

internal void
Pattern_BulbMask(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    Time = Time * BLState->PatternSpeed;
    
    r32 Top = 141;
    r32 BulbRange = 50;
    
    pixel White = V4ToRGBPixel(WhiteV4);
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        v3 P = Leds->Positions[LedIndex].xyz;
        
        r32 BulbSDF = 1 - Clamp01(((Top - P.y) - BulbRange) / BulbRange);
        r32 N = Noise3D((P / 17) + v3{Time, -Time, 0});
        N = Clamp01(N) * 2;
        N = Smoothstep(N);
        N *= N;
        N = Smoothstep(N);
        N *= 1.0f - PowR32(1.0f - BulbSDF, 4);
        BulbSDF += N;
        BulbSDF = Clamp01(BulbSDF);
        v4 C = WhiteV4 * BulbSDF;
        Leds->Colors[LedIndex] = V4ToRGBPixel(C);
    }
}

#define BLUMEN_PATTERNS_H
#endif // BLUMEN_PATTERNS_H