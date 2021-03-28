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
    F += 0.250000f * Noise3D(Pp); Pp = Pp * 2.03;
    F += 0.125000f * Noise3D(Pp); Pp = Pp * 2.01;
    F += 0.062500f * Noise3D(Pp); Pp = Pp * 2.04;
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

v4 FlowerAColors[FLOWER_COLORS_COUNT] = {
    { 232 / 255.f, 219 / 255.f, 88 / 255.f },
    { 232 / 255.f, 219 / 255.f, 88 / 255.f },
    { 232 / 255.f, 219 / 255.f, 88 / 255.f },
    { 147 / 255.f, 75 / 255.f, 176 / 255.f },
    { 193 / 255.f, 187 / 255.f, 197 / 255.f },
    { 223 / 255.f, 190 / 255.f, 49 / 255.f },
    { 198 / 255.f, 76 / 255.f, 65 / 255.f },
    { 198 / 255.f, 76 / 255.f, 65 / 255.f },
    { 198 / 255.f, 76 / 255.f, 65 / 255.f },
    { 226 / 255.f, 200 / 255.f, 17 / 255.f },
    { 116 / 255.f, 126 / 255.f, 39 / 255.f },
    { 61 / 255.f, 62 / 255.f, 31 / 255.f }
};
v4 FlowerBColors[FLOWER_COLORS_COUNT] = {
    { 62 / 255.f, 56 / 255.f, 139 / 255.f },
    { 93 / 255.f, 87 / 255.f, 164 / 255.f },
    { 93 / 255.f, 87 / 255.f, 164 / 255.f },
    { 93 / 255.f, 87 / 255.f, 164 / 255.f },
    { 155 / 255.f, 140 / 255.f, 184 / 255.f },
    { 191 / 255.f, 201 / 255.f, 204 / 255.f },
    { 45 / 255.f, 31 / 255.f, 116 / 255.f },
    { 201 / 255.f, 196 / 255.f, 156 / 255.f },
    { 191 / 255.f, 175 / 255.f, 109 / 255.f },
    { 186 / 255.f, 176 / 255.f, 107 / 255.f },
    { 89 / 255.f, 77 / 255.f, 17 / 255.f },
    { 47 / 255.f, 49 / 255.f, 18 / 255.f },
};
v4 FlowerCColors[FLOWER_COLORS_COUNT] = {
    { 220 / 255.f, 217 / 255.f, 210 / 255.f },
    { 220 / 255.f, 217 / 255.f, 210 / 255.f },
    { 220 / 255.f, 217 / 255.f, 210 / 255.f },
    { 225 / 255.f, 193 / 255.f, 110 / 255.f },
    { 225 / 255.f, 193 / 255.f, 110 / 255.f },
    { 227 / 255.f, 221 / 255.f, 214 / 255.f },
    { 227 / 255.f, 221 / 255.f, 214 / 255.f },
    { 230 / 255.f, 218 / 255.f, 187 / 255.f },
    { 230 / 255.f, 218 / 255.f, 187 / 255.f },
    { 172 / 255.f, 190 / 255.f, 211 / 255.f },
    { 172 / 255.f, 190 / 255.f, 211 / 255.f },
    { 172 / 255.f, 190 / 255.f, 211 / 255.f },
};

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

internal void
SolidColorPattern(led_buffer* Leds, led_buffer_range Range, pixel Color)
{
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        Leds->Colors[LedIndex] = Color;
    }
}

internal void
Pattern_Blue(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    pixel Blue = pixel{0, 0, 255};
    SolidColorPattern(Leds, Range, Blue);
}

internal void
Pattern_Green(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    pixel Green = pixel{0, 255, 0};
    SolidColorPattern(Leds, Range, Green);
}

internal void
Pattern_FlowerColors(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    r32 CycleTime = 10;
    r32 CyclePercent = ModR32(Time, CycleTime) / CycleTime;
    
    v4 CA = GetColor(FlowerAColors, FLOWER_COLORS_COUNT, CyclePercent);
    v4 CB = GetColor(FlowerAColors, FLOWER_COLORS_COUNT, 1.0f - CyclePercent);
    
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        v4 P = Leds->Positions[LedIndex];
        r32 Pct = (Abs(ModR32(P.y, 150) / 150) + CycleTime) * PiR32;
        
        r32 APct = RemapR32(SinR32(Pct), -1, 1, 0, 1);
        Leds->Colors[LedIndex] = PixelMix(APct, CA, CB);
    }
}

internal void
TestPatternOne(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
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
TestPatternTwo(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
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
    
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
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
TestPatternThree(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    v4 GreenCenter = v4{0, 0, 150, 1};
    r32 GreenRadius = Abs(SinR32(Time)) * 200;
    
    v4 TealCenter = v4{0, 0, 150, 1};
    r32 TealRadius = Abs(SinR32(Time + 1.5)) * 200;
    
    r32 FadeDist = 35;
    
    
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
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
Pattern_HueShift(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
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
Pattern_HueFade(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
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
Pattern_AllGreen(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        u32 I = LedIndex + 1;
        Leds->Colors[LedIndex] = {0, 255, 0};
    }
}

internal r32
PatternHash(r32 Seed)
{
    return FractR32(Seed * 17.0 * FractR32(Seed * 0.3183099));
}

internal void
Pattern_Spots(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    pixel ColorA = { 0, 255, 255 };
    pixel ColorB = { 255, 0, 255 };
    
    r32 Speed = .5f;
    Time *= Speed;
    r32 ScaleA = 2 * SinR32(Time / 5);
    r32 ScaleB = 2.4f * CosR32(Time / 2.5f);
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
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
Pattern_LighthouseRainbow(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    v2 RefVector = V2Normalize(v2{ SinR32(Time), CosR32(Time) });
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
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

internal void
Pattern_SmoothGrowRainbow(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
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
Pattern_GrowAndFade(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    r32 PercentCycle = ModR32(Time, 10) / 10;
    v4 HSV = { PercentCycle * 360, 1, 1, 1 };
    v4 RGB = HSVToRGB(HSV);
    
    r32 RefHeight = -100 + (Smoothstep(PercentCycle * 1.4f) * 400);
    r32 RefBrightness = 1.0f - Smoothstep(PercentCycle);
    
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
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
Pattern_ColorToWhite(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
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

internal void
Pattern_FlowerColorToWhite(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    r32 FadeBottomBase = 50;
    r32 FadeTop = 125;
    
    for (u32 StripIndex = 0; StripIndex < Assembly.StripCount; StripIndex++)
    {
        v2_strip Strip = Assembly.Strips[StripIndex];
        
#if 0
        // All flowers same flower type
        pixel* Colors = &FlowerAColors[0];
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
#else
        // Each flower different
        v4* Colors = &FlowerAColors[0];
        r32 FlowerOffset = 0;
        if (AssemblyStrip_HasTagValueSLOW(Strip, "flower", "center"))
        {
            Colors = &FlowerBColors[0];
        }
        else if (AssemblyStrip_HasTagValueSLOW(Strip, "flower", "right"))
        {
            Colors = &FlowerCColors[0];
        }
#endif
        r32 PercentCycle = ModR32(Time + FlowerOffset, 10) / 10;
        
        r32 FadeBottom = FadeBottomBase + RemapR32(SinR32((PercentCycle * 4) * TauR32), -1, 1, -50, 50);
        
        for (u32 i = 0; i < Strip.LedCount; i++)
        {
            u32 LedIndex = Strip.LedLUT[i];
            v4 P = Leds->Positions[LedIndex];
            
            v4 FinalColor = {};
            r32 B = RemapR32(SinR32((P.y / 15.f) + (PercentCycle * TauR32)), -1, 1, .5f, 1.f);
            r32 HNoise = RemapR32(SinR32((P.y / 31.f) + (PercentCycle * TauR32)), -1, 1, 0.f, 1.f);
            
            v4 BottomColor = GetColor(Colors, FLOWER_COLORS_COUNT, (PercentCycle + HNoise) / 2);
            
            FinalColor = BottomColor;
            
            Leds->Colors[LedIndex] = V4ToRGBPixel(FinalColor);
        }
    }
}

r32 TLastFrame = 0;
v4* FAC = &FlowerAColors[0];
v4* FBC = &FlowerBColors[0];
v4* FCC = &FlowerCColors[0];

internal void
Pattern_BasicFlowers(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    if (TLastFrame > Time)
    {
        v4 * Temp = FAC;
        FAC = FBC;
        FBC = FCC;
        FCC = Temp;
    }
    TLastFrame = Time;
    
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
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
    phrase_hue Hue = BLState->AssemblyColors[Assembly.AssemblyIndex % 3];
    v4 C0 = HSVToRGB({Hue.Hue0, 1, 1, 1});
    v4 C1 = HSVToRGB({Hue.Hue1, 1, 1, 1});
    
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        v4 P = Leds->Positions[LedIndex];
        
        v3 Pp = P.xyz;
        
        r32 Noise = Fbm3D((Pp / 1000) + (v3{Time, -Time, Time} * 0.01f));
        Noise = RemapR32(Noise, -1, 1, 0, 1);
        Noise = Smoothstep(Noise, 0, 1);
        u8 NV = (u8)(Noise * 255);
        
        v3 BSeed = v3{P.z, P.x, P.y};
        r32 BNoise = 1.0f; //Fbm3D(BSeed / 50);
        
        v4 C = V4Lerp(BNoise, C0, C1);
        C = C * BNoise;
        
        //Leds->Colors[LedIndex] = V4ToRGBPixel(v4{Noise, Noise, Noise, 1});
        Leds->Colors[LedIndex] = V4ToRGBPixel(C);
        //Leds->Colors[LedIndex] = pixel{NV, NV, NV};
    }
}

internal void
Pattern_Patchy(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    DEBUG_TRACK_FUNCTION;
    
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    phrase_hue Hue = BLState->AssemblyColors[Assembly.AssemblyIndex % 3];
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
        
        v4 C = CA + CB;
        Leds->Colors[LedIndex] = V4ToRGBPixel(C);
    }
}

internal void
Pattern_Leafy(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    DEBUG_TRACK_FUNCTION;
    
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    phrase_hue Hue = BLState->AssemblyColors[Assembly.AssemblyIndex % 3];
    v4 C0 = HSVToRGB({Hue.Hue0, 1, 1, 1});
    v4 C1 = HSVToRGB({Hue.Hue1, 1, 1, 1});
    
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        v4 P = Leds->Positions[LedIndex];
        
#if 0
        r32 RefPos = P.y + Noise2D(v2{P.x, P.z} * 10);
        
        v4 C = {};
        r32 B = 0;
        
        r32 BandWidth = 5;
        r32 TransitionPeriod = 30.0f;
        u32 BandCount = 10;
        for (u32 Band = 0; Band < BandCount; Band++)
        {
            r32 BandSeed = Hash1((r32)Band);
            r32 BandSeedPos = RemapR32(BandSeed, -1, 1, 0, 1);
            r32 BandDelay = BandSeedPos * TransitionPeriod;
            r32 BandTransitionPeriod = RemapR32(Hash1((r32)Band * 3.413f), -1, 1, 0, 1) * TransitionPeriod;
            r32 BandOffset = Time + BandDelay;
            r32 BandPercent = ModR32(BandOffset, BandTransitionPeriod) / BandTransitionPeriod;
            r32 BandSmoothed = Smoothstep(BandPercent, 0, 1);
            
            r32 BandHeight = -125 + BandPercent * 250;
            
            r32 BandDist = Abs(RefPos - BandHeight);
            
            B += Max(0, (BandWidth + BandSeed * 2.5) - BandDist);
        }
        B = Clamp(0, B, 1);
        
        r32 BandCP = (P.y + 100) / 200;
        BandCP = 0.8f;
        v4 BandC = GetColor(&FlowerBColors[0], FLOWER_COLORS_COUNT, BandCP);
        
        v4 GradientC = GetColor(&FlowerBColors[0], FLOWER_COLORS_COUNT, 0);
        r32 GradientB = RemapR32(P.y, 200, 0, 1, 0);
        GradientB = Clamp(0, GradientB, 1);
        
        C = (GradientC * GradientB) + (BandC * B);
#endif
        //v4 C = GetColor(&FlowerBColors[0], FLOWER_COLORS_COUNT, 0);
        v4 C = v4{ 255, 100, 3 };
        C /= 255.f;
        //r32 B = Fbm3D(P.xyz / 200);
        //C *= B;
        if (P.y < 75) {
            C = v4{ 139 / 255.f, 69 / 255.f, 19 / 255.f, 1.0f} * .25f;
        }
        Leds->Colors[LedIndex] = V4ToRGBPixel(C);
    }
}

internal void
Pattern_LeafyPatchy(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    DEBUG_TRACK_FUNCTION;
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    phrase_hue Hue = BLState->AssemblyColors[Assembly.AssemblyIndex % 3];
    v4 C0 = HSVToRGB({Hue.Hue0, 1, 1, 1});
    v4 C1 = HSVToRGB({Hue.Hue1, 1, 1, 1});
    for (u32 LedIndex = Range.First; LedIndex < Range.OnePastLast; LedIndex++)
    {
        v4 P = Leds->Positions[LedIndex];
        r32 LedRange = 300.0f;
        r32 ScaleFactor = 1.0f / LedRange;
        v3 Pp = P.xyz + v3{150, 100, 0};
        
        r32 NoiseA = Fbm3D((Pp / 35), Time * 0.5f);
        //NoiseA = PowR32(NoiseA, 3);
        NoiseA = Smoothstep(NoiseA);
        v4 CA = C0 * NoiseA;
        
        r32 NoiseB = Noise3D((Pp / 35) + v3{0, 0, Time * 5});
        NoiseB = PowR32(NoiseB, 3);
        NoiseB = Smoothstep(NoiseB);
        v4 CB = C1;
        
        v4 C = V4Lerp(NoiseB, CA, CB);
        Leds->Colors[LedIndex] = V4ToRGBPixel(C);
    }
}

internal void
Pattern_WavyPatchy(led_buffer* Leds, led_buffer_range Range, assembly Assembly, r32 Time, gs_memory_arena* Transient, u8* UserData)
{
    DEBUG_TRACK_FUNCTION;
    blumen_lumen_state* BLState = (blumen_lumen_state*)UserData;
    gs_random_series Random = InitRandomSeries(24601);
    
    r32 LightSpeedMin = 1;
    r32 LightSpeedMax = 5;
    
#if 0
    r32 LightHueMin = (ModR32(Time, 10) / 10) * 360;
    r32 LightHueMax = ModR32((LightHueMin + 45), 360) ;
#else
    r32 CenterHue = BLState->AssemblyColors[Assembly.AssemblyIndex % 3].Hue0;
    r32 LightHueMin = ModR32(CenterHue + 30, 360);;
    r32 LightHueMax = ModR32(CenterHue - 30, 360) ;
#endif
    s32 LightTailLength = 10;
    for (u32 StripIndex = 0; StripIndex < Assembly.StripCount; StripIndex++)
    {
        v2_strip Strip = Assembly.Strips[StripIndex];
        
        r32 LightHue = LerpR32(NextRandomUnilateral(&Random),
                               LightHueMin,
                               LightHueMax);
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
            v4 C = HSVToRGB(v4{LightHue, 1, 1, 1}) * PctTail;
            Leds->Colors[LedIndex] = V4ToRGBPixel(C);
        }
    }
}

#define BLUMEN_PATTERNS_H
#endif // BLUMEN_PATTERNS_H