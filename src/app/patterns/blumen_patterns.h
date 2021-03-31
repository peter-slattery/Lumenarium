//
// File: blumen_patterns.h
// Author: Peter Slattery
// Creation Date: 2021-01-15
//
#ifndef BLUMEN_PATTERNS_H

#define FLOWER_COLORS_COUNT 12

internal r32
Smoothstep(r32 T)
{
    r32 Result = (T * T * (3 - (2 * T)));
    return Result;
}
internal r32
Smoothstep(r32 T, r32 A, r32 B)
{
    return LerpR32(Smoothstep(T), A, B);
}
internal v3
Smoothstep(v3 P)
{
    v3 R = {};
    R.x = Smoothstep(P.x);
    R.y = Smoothstep(P.y);
    R.z = Smoothstep(P.z);
    return R;
}

internal v3
AbsV3(v3 P)
{
    v3 Result = {};
    Result.x = Abs(P.x);
    Result.y = Abs(P.y);
    Result.z = Abs(P.z);
    return Result;
}

internal v2
FloorV2(v2 P)
{
    v2 Result = {};
    Result.x = FloorR32(P.x);
    Result.y = FloorR32(P.y);
    return Result;
}
internal v3
FloorV3(v3 P)
{
    v3 Result = {};
    Result.x = FloorR32(P.x);
    Result.y = FloorR32(P.y);
    Result.z = FloorR32(P.z);
    return Result;
}

internal v2
FractV2(v2 P)
{
    v2 Result = {};
    Result.x = FractR32(P.x);
    Result.y = FractR32(P.y);
    return Result;
}
internal v3
FractV3(v3 P)
{
    v3 Result = {};
    Result.x = FractR32(P.x);
    Result.y = FractR32(P.y);
    Result.z = FractR32(P.z);
    return Result;
}

internal v2
SinV2(v2 P)
{
    v2 Result = {};
    Result.x = SinR32(P.x);
    Result.y = SinR32(P.y);
    return Result;
}
internal v3
SinV3(v3 P)
{
    v3 Result = {};
    Result.x = SinR32(P.x);
    Result.y = SinR32(P.y);
    Result.y = SinR32(P.z);
    return Result;
}

internal r32
Hash1(v2 P)
{
    v2 Result = FractV2( P * 0.3183099f ) * 50.f;
    return FractR32(P.x * P.y * (P.x + P.y));
}

internal r32
Hash1(r32 N)
{
    return FractR32(N * 17.0f * FractR32(N * 0.3183099f));
}

internal v2
Hash2(r32 N)
{
    v2 P = V2MultiplyPairwise(SinV2(v2{N,N+1.0f}), v2{43758.5453123f,22578.1459123f});
    return FractV2(P);
}

internal v2
Hash2(v2 P)
{
    v2 K = v2{ 0.3183099f, 0.3678794f };
    v2 Kp = v2{K.y, K.x};
    v2 R = V2MultiplyPairwise(P, K) + Kp;
    return FractV2( K * 16.0f * FractR32( P.x * P.y * (P.x + P.y)));
}

internal v3
Hash3(v2 P)
{
    v3 Q = v3{};
    Q.x = V2Dot(P, v2{127.1f, 311.7f});
    Q.y = V2Dot(P, v2{267.5f, 183.3f});
    Q.z = V2Dot(P, v2{419.2f, 371.9f});
    return FractV3(SinV3(Q) * 43758.5453f);
}

internal r32
HashV3ToR32(v3 P)
{
    v3 Pp = FractV3(P * 0.3183099f + v3{0.1f, 0.1f, 0.1f});
    Pp *= 17.0f;
    r32 Result = FractR32(Pp.x * Pp.y * Pp.z * (Pp.x + Pp.y + Pp.z));
    return Result;
}

internal r32
Random(v2 N)
{
    v2 V = v2{12.9898f, 4.1414f};
    return FractR32(SinR32(V2Dot(N, V)) * 43758.5453);
}

internal r32
Noise2D(v2 P)
{
    v2 IP = FloorV2(P);
    v2 U = FractV2(P);
    U = V2MultiplyPairwise(U, U);
    U = V2MultiplyPairwise(U, ((U * 2.0f) + v2{-3, -3}));
    
    r32 A = LerpR32(U.x, Random(IP), Random(IP + v2{1.0f, 0}));
    r32 B = LerpR32(U.x, Random(IP + v2{0, 1}), Random(IP + v2{1, 1}));
    r32 Res = LerpR32(U.y, A, B);
    
    return Res * Res;
}

internal r32
Noise3D(v3 P)
{
    P = AbsV3(P);
    v3 PFloor = FloorV3(P);
    v3 PFract = FractV3(P);
    v3 F = Smoothstep(PFract);
    
    r32 Result = LerpR32(F.z, 
                         LerpR32(F.y, 
                                 LerpR32(F.x,
                                         HashV3ToR32(PFloor + v3{0, 0, 0}),
                                         HashV3ToR32(PFloor + v3{1, 0, 0})),
                                 LerpR32(F.x,
                                         HashV3ToR32(PFloor + v3{0, 1, 0}),
                                         HashV3ToR32(PFloor + v3{1, 1, 0}))),
                         LerpR32(F.y,
                                 LerpR32(F.x,
                                         HashV3ToR32(PFloor + v3{0, 0, 1}),
                                         HashV3ToR32(PFloor + v3{1, 0, 1})),
                                 LerpR32(F.x,
                                         HashV3ToR32(PFloor + v3{0, 1, 1}),
                                         HashV3ToR32(PFloor + v3{1, 1, 1}))));
    
    Assert(Result >= 0 && Result <= 1);
    return Result;
}

internal r32
Noise3D_(v3 Pp)
{
    v3 P = FloorV3(Pp);
    v3 W = FractV3(Pp);
    
    //v3 U = W * W * W * (W * (W * 6.0f - 15.0f) + 10.0f);
    v3 U = V3MultiplyPairwise(W, W * 6.0f - v3{15, 15, 15});
    U = U + v3{10, 10, 10};
    U = V3MultiplyPairwise(U, W);
    U = V3MultiplyPairwise(U, W);
    U = V3MultiplyPairwise(U, W);
    
    r32 N = P.x + 317.0f * P.y + 157.0f * P.z;
    
    r32 A = Hash1(N + 0.0f);
    r32 B = Hash1(N + 1.0f);
    r32 C = Hash1(N + 317.0f);
    r32 D = Hash1(N + 317.0f);
    r32 E = Hash1(N + 157.0f);
    r32 F = Hash1(N + 158.0f);
    r32 G = Hash1(N + 474.0f);
    r32 H = Hash1(N + 475.0f);
    
    r32 K0 = A;
    r32 K1 = B - A;
    r32 K2 = C - A;
    r32 K3 = E - A;
    r32 K4 = A - B - C + D;
    r32 K5 = A - C - E + G;
    r32 K6 = A - B - E + F;
    r32 K7 = A + B + C - D + E - F - G + H;
    
    return -1.0f + 2.0f * (K0 +
                           K1 * U.x +
                           K2 * U.y +
                           K3 * U.z +
                           K4 * U.x * U.y +
                           K5 * U.y + U.z +
                           K6 * U.z * U.x +
                           K7 * U.x * U.y * U.z);
}

internal r32
Fbm2D(v2 P)
{
    r32 R = 0;
    r32 Amp = 1.0;
    r32 Freq = 1.0;
    for (u32 i = 0; i < 3; i++)
    {
        R += Amp * Noise2D(P * Freq);
        Amp *= 0.5f;
        Freq *= 1.0f / 0.5f;
    }
    return R;
}

global m44 M3 = m44{
    0.00f,  0.80f,  0.60f, 0,
    -0.80f,  0.36f, -0.48f, 0,
    -0.60f, -0.48f,  0.64f, 0,
    0, 0, 0, 1
};

internal r32
Fbm3D(v3 P)
{
    v3 X = P;
    r32 F = 2.0f;
    r32 S = 0.5f;
    r32 A = 0.0f;
    r32 B = 0.5f;
    for (u32 i = 0; i < 4; i++)
    {
        r32 N = Noise3D(X);
        A += B * N;
        B *= S;
        v4 Xp = M3 * ToV4Point(X);
        X = Xp.xyz * F;
    }
    
    return A;
}

internal r32
Fbm3D(v3 P, r32 T)
{
    v3 Tt = v3{T, T, T};
    r32 SinT = SinR32(T);
    v3 Tv = v3{SinT, SinT, SinT};
    v3 Pp = P;
    r32 F = 0.0;
    
    F += 0.500000f * Noise3D(Pp + Tt); Pp = Pp * 2.02;
    F += 0.031250f * Noise3D(Pp); Pp = Pp * 2.01;
    F += 0.250000f * Noise3D(Pp - Tt); Pp = Pp * 2.03;
    F += 0.125000f * Noise3D(Pp); Pp = Pp * 2.01;
    F += 0.062500f * Noise3D(Pp + Tt); Pp = Pp * 2.04;
    F += 0.015625f * Noise3D(Pp + Tv);
    
    F = F / 0.984375f;
    return F;
}

internal r32
Voronoise(v2 P, r32 U, r32 V)
{
    r32 K = 1.0f + 63.0f + PowR32(1.0f - V, 6.0f);
    
    v2 I = FloorV2(P);
    v2 F = FractV2(P);
    
    v2 A = v2{0, 0};
    for (s32 y = -2; y <= 2; y++)
    {
        for (s32 x = -2; x <= 2; x++)
        {
            v2 G = v2{(r32)x, (r32)y};
            v3 O = V3MultiplyPairwise(Hash3(I + G), v3{U, U, 1.0f});
            v2 D = G - F + O.xy;
            r32 W = PowR32(1.0f - Smoothstep(V2Mag(D), 0.0f, 1.414f), K);
            A += v2{O.z * W, W};
        }
    }
    
    return A.x / A.y;
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

internal pixel
PixelMix(r32 T, pixel A, pixel B)
{
    pixel Result = {
        LerpU8(T, A.R, B.R),
        LerpU8(T, A.G, B.G),
        LerpU8(T, A.B, B.B),
    };
    return Result;
}

internal pixel
PixelMix(r32 T, v4 A, v4 B)
{
    v4 Result = V4Lerp(T, A, B);
    pixel P = V4ToRGBPixel(Result);
    return P;
}

internal v4
GetColor(v4* Colors, u32 ColorsCount, r32 Percent)
{
    Percent = Clamp01(Percent);
    
    u32 LowerIndex = Percent * ColorsCount;
    
    u32 HigherIndex = LowerIndex + 1;
    if (HigherIndex >= ColorsCount) HigherIndex = 0;
    
    r32 LowerPercent = (r32)LowerIndex / (r32)ColorsCount;
    r32 StepPercent = 1.f / (r32)ColorsCount;
    r32 PercentLower = (Percent - LowerPercent) / StepPercent;
    
    v4 Result = V4Lerp(PercentLower, Colors[LowerIndex], Colors[HigherIndex]);
    
    return Result;
}

v4 RGBToHSV(v4 In)
{
    v4 Result = {};
    
    r32 Min = Min(In.r, Min(In.g, In.b));
    r32 Max = Max(In.r, Max(In.g, In.b));
    
    r32 V = Max;
    r32 Delta = Max - Min;
    r32 S = 0;
    r32 H = 0;
    if( Max != 0 )
    {
        S = Delta / Max;
        
        if( In.r == Max )
        {
            H = ( In.g - In.b ) / Delta;      // between yellow & magenta
        }
        else if( In.g == Max )
        {
            H = 2 + ( In.b - In.r ) / Delta;  // between cyan & yellow
        }
        else
        {
            H = 4 + ( In.r - In.g ) / Delta;  // between magenta & cyan
        }
        H *= 60;                // degrees
        if( H < 0 )
            H += 360;
        Result = v4{H, S, V, 1};
    }
    else
    {
        // r = g = b = 0
        // s = 0, v is undefined
        S = 0;
        H = -1;
        Result = v4{H, S, 1, 1};
    }
    
    return Result;
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