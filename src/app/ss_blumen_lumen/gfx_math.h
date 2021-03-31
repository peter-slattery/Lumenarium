/* date = March 31st 2021 2:19 am */

#ifndef GFX_MATH_H
#define GFX_MATH_H

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

#endif //GFX_MATH_H
