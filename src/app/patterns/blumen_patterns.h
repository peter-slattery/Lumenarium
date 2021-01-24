//
// File: blumen_patterns.h
// Author: Peter Slattery
// Creation Date: 2021-01-15
//
#ifndef BLUMEN_PATTERNS_H

internal void
SolidColorPattern(led_buffer* Leds, pixel Color)
{
    for (u32 LedIndex = 0; LedIndex < Leds->LedCount; LedIndex++)
    {
        Leds->Colors[LedIndex] = Color;
    }
}

internal void
Pattern_Blue(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    pixel Blue = pixel{0, 0, 255};
    SolidColorPattern(Leds, Blue);
}

internal void
Pattern_Green(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    pixel Green = pixel{0, 255, 0};
    SolidColorPattern(Leds, Green);
}

internal void
TestPatternOne(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    led_strip_list BlumenStrips = AssemblyStripsGetWithTagValue(Assembly, ConstString("assembly"), ConstString("Blumen Lumen"), Transient);
    led_strip_list RadiaStrips = AssemblyStripsGetWithTagValue(Assembly, ConstString("assembly"), ConstString("Radialumia"), Transient);
    
    for (u32 i = 0; i < BlumenStrips.Count; i++)
    {
        u32 StripIndex = BlumenStrips.StripIndices[i];
        v2_strip StripAt = Assembly.Strips[StripIndex];
        
        for (u32 j = 0; j < StripAt.LedCount; j++)
        {
            u32 LedIndex = StripAt.LedLUT[j];
            Leds->Colors[LedIndex] = { 255, 0, 0 };
            
        }
    }
    
    for (u32 i = 0; i < RadiaStrips.Count; i++)
    {
        u32 StripIndex = RadiaStrips.StripIndices[i];
        v2_strip StripAt = Assembly.Strips[StripIndex];
        
        for (u32 j = 0; j < StripAt.LedCount; j++)
        {
            u32 LedIndex = StripAt.LedLUT[j];
            Leds->Colors[LedIndex] = { 0, 255, 0 };
        }
    }
}

internal void
TestPatternTwo(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    r32 PeriodicTime = (Time / PiR32) * 2;
    
    r32 ZeroOneSin = (SinR32(PeriodicTime) * .5f) + .5f;
    r32 ZeroOneCos = (CosR32(PeriodicTime) * .5f) + .5f;
    pixel Color = { (u8)(ZeroOneSin * 255), 0, (u8)(ZeroOneCos * 255) };
    
    v4 Center = v4{0, 0, 0, 1};
    r32 ThetaZ = Time / 2;
    v4 Normal = v4{CosR32(ThetaZ), 0, SinR32(ThetaZ), 0}; // NOTE(Peter): dont' need to normalize. Should always be 1
    v4 Right = V4Cross(Normal, v4{0, 1, 0, 0});
    
    v4 FrontCenter = Center + (Normal * 25);
    v4 BackCenter = Center - (Normal * 25);
    
    r32 OuterRadiusSquared = 1000000;
    r32 InnerRadiusSquared = 0;
    
    for (u32 LedIndex = 0; LedIndex < Leds->LedCount; LedIndex++)
    {
        v4 Position = Leds->Positions[LedIndex];
        
        v4 ToFront = Position + FrontCenter;
        v4 ToBack = Position + BackCenter;
        
        r32 ToFrontDotNormal = V4Dot(ToFront, Normal);
        r32 ToBackDotNormal = V4Dot(ToBack, Normal);
        
        ToFrontDotNormal = Clamp01(ToFrontDotNormal * 1000);
        ToBackDotNormal = Clamp01(ToBackDotNormal * 1000);
        
        r32 SqDistToCenter = V4MagSquared(Position);
        if (SqDistToCenter < OuterRadiusSquared && SqDistToCenter > InnerRadiusSquared)
        {
            if (XOR(ToFrontDotNormal > 0, ToBackDotNormal > 0))
            {
                Leds->Colors[LedIndex] = Color;
            }
            else
            {
                //Leds->Colors[LedIndex] = {};
            }
        }
        else
        {
            //Leds->Colors[LedIndex] = {};
        }
    }
}

internal void
TestPatternThree(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    v4 GreenCenter = v4{0, 0, 150, 1};
    r32 GreenRadius = Abs(SinR32(Time)) * 200;
    
    v4 TealCenter = v4{0, 0, 150, 1};
    r32 TealRadius = Abs(SinR32(Time + 1.5)) * 200;
    
    r32 FadeDist = 35;
    
    
    for (u32 LedIndex = 0; LedIndex < Leds->LedCount; LedIndex++)
    {
        v4 LedPosition = Leds->Positions[LedIndex];
        u8 Red = 0;
        u8 Green = 0;
        u8 Blue = 0;
        
        r32 GreenDist = Abs(V4Mag(LedPosition - GreenCenter) - GreenRadius);
        r32 GreenBrightness = Clamp(0.f, FadeDist - Abs(GreenDist), FadeDist);
        Green = (u8)(GreenBrightness * 255);
        
        r32 TealDist = Abs(V4Mag(LedPosition - TealCenter) - TealRadius);
        r32 TealBrightness = Clamp(0.f, FadeDist - Abs(TealDist), FadeDist);
        Red = (u8)(TealBrightness * 255);
        Blue = (u8)(TealBrightness * 255);
        
        Leds->Colors[LedIndex].R = Red;
        Leds->Colors[LedIndex].B = Green;
        Leds->Colors[LedIndex].G = Green;
    }
}

v4 HSVToRGB (v4 In)
{
    float Hue = In.x;
    /*
while (Hue > 360.0f) { Hue -= 360.0f; }
    while (Hue < 0.0f) { Hue += 360.0f; }
    */
    Hue = ModR32(Hue, 360.0f);
    if (Hue < 0) { Hue += 360.0f; }
    if (Hue == MinR32) { Hue = 0; }
    if (Hue == MaxR32) { Hue = 360; }
    Assert(Hue >= 0 && Hue < 360);
    
    float Sat = In.y;
    float Value = In.z;
    
    float hh, p, q, t, ff;
    long        i;
    v4 Result = {};
    Result.a = In.a;
    
    if(Sat <= 0.0f) {       // < is bogus, just shuts up warnings
        Result.r = Value;
        Result.g = Value;
        Result.b = Value;
        return Result;
    }
    hh = Hue;
    if(hh >= 360.0f) hh = 0.0f;
    hh /= 60.0f;
    i = (long)hh;
    ff = hh - i;
    p = Value * (1.0f - Sat);
    q = Value * (1.0f - (Sat * ff));
    t = Value * (1.0f - (Sat * (1.0f - ff)));
    
    switch(i) {
        case 0:
        {Result.r = Value;
            Result.g = t;
            Result.b = p;
        }break;
        
        case 1:
        {
            Result.r = q;
            Result.g = Value;
            Result.b = p;
        }break;
        
        case 2:
        {
            Result.r = p;
            Result.g = Value;
            Result.b = t;
        }break;
        
        case 3:
        {
            Result.r = p;
            Result.g = q;
            Result.b = Value;
        }break;
        
        case 4:
        {
            Result.r = t;
            Result.g = p;
            Result.b = Value;
        }break;
        
        case 5:
        default:
        {
            Result.r = Value;
            Result.g = p;
            Result.b = q;
        }break;
    }
    
    return Result;
}

internal void
Pattern_HueShift(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    r32 Height = SinR32(Time) * 25;
    
    r32 CycleLength = 5.0f;
    r32 CycleProgress = FractR32(Time / CycleLength);
    r32 CycleBlend = (SinR32(Time) * .5f) + .5f;
    
    for (u32 LedIndex = 0; LedIndex < Leds->LedCount; LedIndex++)
    {
        v4 Pos = Leds->Positions[LedIndex];
        r32 Dist = Pos.y - Height;
        
        v4 HSV = { (ModR32(Dist, 25) / 25) * 360, 1, 1, 1 };
        v4 RGB = HSVToRGB(HSV);
        
        u8 R = (u8)(RGB.x * 255);
        u8 G = (u8)(RGB.y * 255);
        u8 B = (u8)(RGB.z * 255);
        
        Leds->Colors[LedIndex].R = R;
        Leds->Colors[LedIndex].G = G;
        Leds->Colors[LedIndex].B = B;
    }
}

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
Pattern_HueFade(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    r32 HueBase = ModR32(Time * 50, 360);
    
    r32 CycleLength = 5.0f;
    r32 CycleProgress = FractR32(Time / CycleLength);
    r32 CycleBlend = (SinR32(Time) * .5f) + .5f;
    
    for (u32 LedIndex = 0; LedIndex < Leds->LedCount; LedIndex++)
    {
        v4 Pos = Leds->Positions[LedIndex];
        r32 Hue = HueBase + Pos.y + Pos.x;
        v4 HSV = { Hue, 1, 1, 1 };
        v4 RGB = HSVToRGB(HSV);
        
        Leds->Colors[LedIndex] = V4ToRGBPixel(RGB);
    }
}

internal void
Pattern_AllGreen(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    for (u32 LedIndex = 0; LedIndex < Leds->LedCount; LedIndex++)
    {
        u32 I = LedIndex + 1;
        Leds->Colors[LedIndex] = {};
        if (I % 3 == 0)
        {
            Leds->Colors[LedIndex].R = 255;
        }
        else if (I % 3 == 1)
        {
            Leds->Colors[LedIndex].G = 255;
        }
        else if (I % 3 == 2)
        {
            Leds->Colors[LedIndex].B = 255;
        }
        
    }
}

internal r32
PatternHash(r32 Seed)
{
    return FractR32(Seed * 17.0 * FractR32(Seed * 0.3183099));
}

internal void
Pattern_Spots(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    pixel ColorA = { 0, 255, 255 };
    pixel ColorB = { 255, 0, 255 };
    
    r32 Speed = .5f;
    Time *= Speed;
    r32 ScaleA = 2 * SinR32(Time / 5);
    r32 ScaleB = 2.4f * CosR32(Time / 2.5f);
    for (u32 LedIndex = 0; LedIndex < Leds->LedCount; LedIndex++)
    {
        v4 P = Leds->Positions[LedIndex];
        r32 V = P.y;
        r32 Noise = .3f * PatternHash(V);
        r32 ThetaY = (Leds->Positions[LedIndex].y / 10) + Time + Noise;
        r32 ThetaX = (Leds->Positions[LedIndex].x / 13) + Time + Noise;
        r32 Fade = (ScaleA * SinR32(ThetaY)) + (ScaleB * CosR32(3 * ThetaX));
        Fade = RemapClampedR32(Fade, -1, 1, 0, 1);
        
        Leds->Colors[LedIndex].R = (u8)LerpR32(Fade, ColorA.R, ColorB.R);
        Leds->Colors[LedIndex].G = (u8)LerpR32(Fade, ColorA.G, ColorB.G);
        Leds->Colors[LedIndex].B = (u8)LerpR32(Fade, ColorA.B, ColorB.B);
    }
}

internal void
Pattern_LighthouseRainbow(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    v2 RefVector = V2Normalize(v2{ SinR32(Time), CosR32(Time) });
    for (u32 LedIndex = 0; LedIndex < Leds->LedCount; LedIndex++)
    {
        v2 Vector = v2{
            Leds->Positions[LedIndex].x,
            Leds->Positions[LedIndex].z
        };
        Vector = V2Normalize(Vector);
        
        r32 Angle = V2Dot(RefVector, Vector);
        
        v4 HSV = { (Angle * 30) + (Time * 10) + Leds->Positions[LedIndex].y, 1, 1, 1 };
        v4 RGB = HSVToRGB(HSV);
        
        Leds->Colors[LedIndex] = V4ToRGBPixel(RGB);
    }
}

internal r32
Smoothstep(r32 T)
{
    r32 Result = (T * T * (3 - (2 * T)));
    return Result;
}

internal void
Pattern_SmoothGrowRainbow(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    r32 FillCycleTime = ModR32(Time, 7.0f) / 7.0f;
    r32 ColorCycleTime = ModR32(Time, 21.0f) / 21.0f;
    
    v4 HSV = { 0, 1, 1, 1 };
    for (u32 s = 0; s < Assembly.StripCount; s++)
    {
        v2_strip Strip = Assembly.Strips[s];
        
        v4 RGB0 = HSVToRGB(HSV);
        for (u32 l = 0; l < Strip.LedCount; l++)
        {
            u32 LedIndex = Strip.LedLUT[l];
            Leds->Colors[LedIndex] = V4ToRGBPixel(RGB0);
        }
        
        HSV.x += 15;
    }
}

internal void
Pattern_GrowAndFade(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    r32 PercentCycle = ModR32(Time, 10) / 10;
    v4 HSV = { PercentCycle * 360, 1, 1, 1 };
    v4 RGB = HSVToRGB(HSV);
    
    r32 RefHeight = -100 + (Smoothstep(PercentCycle * 1.4f) * 400);
    r32 RefBrightness = 1.0f - Smoothstep(PercentCycle);
    
    for (u32 LedIndex = 0; LedIndex < Leds->LedCount; LedIndex++)
    {
        v4 P = Leds->Positions[LedIndex];
        
        v4 RgbFaded = v4{};
        if (P.y < RefHeight)
        {
            RgbFaded = RGB * RefBrightness;
        }
        Leds->Colors[LedIndex] = V4ToRGBPixel(RgbFaded);
    }
}

internal void
Pattern_ColorToWhite(led_buffer* Leds, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    r32 FadeBottomBase = 50;
    r32 FadeTop = 125;
    
    for (u32 StripIndex = 0; StripIndex < Assembly.StripCount; StripIndex++)
    {
        v2_strip Strip = Assembly.Strips[StripIndex];
        
        r32 FlowerSpread = .8f;
        r32 FlowerOffset = 0;
        if (AssemblyStrip_HasTagValueSLOW(Strip, "flower", "center"))
        {
            FlowerOffset = 1;
        }
        else if (AssemblyStrip_HasTagValueSLOW(Strip, "flower", "right"))
        {
            FlowerOffset = 2;
        }
        FlowerOffset *= FlowerSpread;
        
        r32 PercentCycle = ModR32(Time + FlowerOffset, 10) / 10;
        
        r32 FadeBottom = FadeBottomBase + RemapR32(SinR32((PercentCycle * 4) * TauR32), -1, 1, -50, 50);
        
        v4 TopRGB = WhiteV4;
        pixel TopColor = V4ToRGBPixel(TopRGB);
        
        for (u32 i = 0; i < Strip.LedCount; i++)
        {
            u32 LedIndex = Strip.LedLUT[i];
            v4 P = Leds->Positions[LedIndex];
            
            pixel FinalColor = {};
            if (P.y > FadeTop)
            {
                FinalColor = TopColor;
            }
            else
            {
                r32 B = RemapR32(SinR32((P.y / 15.f) + (PercentCycle * TauR32)), -1, 1, .5f, 1.f);
                r32 HNoise = RemapR32(SinR32((P.y / 31.f) + (PercentCycle * TauR32)), -1, 1, -32.f, 32.f);
                v4 BottomRGB = HSVToRGB(v4{ (PercentCycle * 360) + HNoise, 1, B, 1 });
                
                if (P.y < FadeBottom)
                {
                    FinalColor = V4ToRGBPixel(BottomRGB);
                }
                else if (P.y >= FadeBottom && P.y <= FadeTop)
                {
                    r32 FadePct = RemapR32(P.y, FadeBottom, FadeTop, 0, 1);
                    v4 MixRGB = V4Lerp(FadePct, BottomRGB, TopRGB);
                    FinalColor = V4ToRGBPixel(MixRGB);
                }
            }
            
            Leds->Colors[LedIndex] = FinalColor;
        }
    }
}

#define BLUMEN_PATTERNS_H
#endif // BLUMEN_PATTERNS_H