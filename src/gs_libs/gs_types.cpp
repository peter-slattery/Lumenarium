//
// File: gs_types.cpp
// Author: Peter Slattery
// Creation Date: 2020-04-18
//
#ifndef GS_TYPES_CPP

#define StructToData(ptr, type) StructToData_((u8*)(ptr), sizeof(type))
internal gs_data
StructToData_(u8* Memory, u64 Size)
{
  gs_data Result = {0};
  Result.Memory = Memory;
  Result.Size = Size;
  return Result;
}

internal u32
U32DivideRoundUp (u32 A, u32 B)
{
  r32 Result = (r32)A / (r32)B;
  Result += .99999f;
  return (u32)Result;
}

inline bool XOR(bool A, bool B) { return (A == !B); }
inline bool XOR(b8 A, b8 B) { return (A == !B); }
inline bool XOR(b32 A, b32 B) { return (A == !B); }
inline bool XOR(b64 A, b64 B) { return (A == !B); }

internal u32
RoundUpToMultiple(u32 Value, u32 MultipleOf)
{
  u32 Result = Value;
  if (MultipleOf != 0)
  {
    u32 Remainder = Value % MultipleOf;
    Result = Value + (MultipleOf - Remainder);
  }
  return Result;
}
internal u32
RoundUpToPow2U32(u32 Value)
{
  u32 Result = Value - 1;
  Result |= Result >> 1;
  Result |= Result >> 2;
  Result |= Result >> 4;
  Result |= Result >> 8;
  Result |= Result >> 16;
  Result++;
  return Result;
}
internal u32
RoundUpToPow2U64(u64 Value)
{
  u64 Result = Value - 1;
  Result |= Result >> 1;
  Result |= Result >> 2;
  Result |= Result >> 4;
  Result |= Result >> 8;
  Result |= Result >> 16;
  Result |= Result >> 32;
  Result++;
  return Result;
}
internal u64
RoundUpTo64(u64 Value, u64 Alignment)
{
  Value += Alignment - 1;
  Value -= Value % Alignment;
  return Value;
}

internal u8
PowU8(u8 X, u32 Power)
{
  u8 Result = X;
  for (u32 i = 1; i < Power; i++) { Result *= X; }
  return Result;
}
internal u16
PowU16(u16 X, u32 Power)
{
  u16 Result = X;
  for (u32 i = 1; i < Power; i++) { Result *= X; }
  return Result;
}
internal u32
PowU32(u32 X, u32 Power)
{
  u32 Result = X;
  for (u32 i = 1; i < Power; i++) { Result *= X; }
  return Result;
}
internal u64
PowU64(u64 X, u32 Power)
{
  u64 Result = X;
  for (u32 i = 1; i < Power; i++) { Result *= X; }
  return Result;
}
internal s8
PowS8(s8 X, u32 Power)
{
  s8 Result = X;
  for (u32 i = 1; i < Power; i++) { Result *= X; }
  return Result;
}
internal s16
PowS16(s16 X, u32 Power)
{
  s16 Result = X;
  for (u32 i = 1; i < Power; i++) { Result *= X; }
  return Result;
}
internal s32
PowS32(s32 X, u32 Power)
{
  s32 Result = X;
  for (u32 i = 1; i < Power; i++) { Result *= X; }
  return Result;
}
internal s64
PowS64(s64 X, u32 Power)
{
  s64 Result = X;
  for (u32 i = 1; i < Power; i++) { Result *= X; }
  return Result;
}
internal r32
PowR32(r32 X, u32 Power)
{
  r32 Result = X;
  for (u32 i = 1; i < Power; i++) { Result *= X; }
  return Result;
}
internal r64
PowR64(r64 X, u32 Power)
{
  r64 Result = X;
  for (u32 i = 1; i < Power; i++) { Result *= X; }
  return Result;
}

internal u8
LerpU8(r32 T, u8 A, u8 B)
{
  return (u8)((A * (1.0f - T)) + (B * T));
}
internal u16
LerpU16(r32 T, u16 A, u16 B)
{
  return (u16)((A * (1.0f - T)) + (B * T));
}
internal u32
LerpU32(r32 T, u32 A, u32 B)
{
  return (u32)((A * (1.0f - T)) + (B * T));
}
internal u64
LerpU64(r32 T, u64 A, u64 B)
{
  return (u64)((A * (1.0f - T)) + (B * T));
}
internal s8
LerpS8(r32 T, s8 A, s8 B)
{
  return (s8)((A * (1.0f - T)) + (B * T));
}
internal s16
LerpS16(r32 T, s16 A, s16 B)
{
  return (s16)((A * (1.0f - T)) + (B * T));
}
internal s32
LerpS32(r32 T, s32 A, s32 B)
{
  return (s32)((A * (1.0f - T)) + (B * T));
}
internal s64
LerpS64(r32 T, s64 A, s64 B)
{
  return (s64)((A * (1.0f - T)) + (B * T));
}
internal r32
LerpR32(r32 T, r32 A, r32 B)
{
  return (r32)((A * (1.0f - T)) + (B * T));
}
internal r64
LerpR64(r32 T, r64 A, r64 B)
{
  return (r64)((A * (1.0f - T)) + (B * T));
}

internal u8
UnlerpU8(u8 Value, u8 Min, u8 Max)
{
  return (u8)((r64)(Value - Min) / (r64)(Max - Min));
}
internal u16
UnlerpU16(u16 Value, u16 Min, u16 Max)
{
  return (u16)((r64)(Value - Min) / (r64)(Max - Min));
}
internal u32
UnlerpU32(u32 Value, u32 Min, u32 Max)
{
  return (u32)((r64)(Value - Min) / (r64)(Max - Min));
}
internal u64
UnlerpU64(u64 Value, u64 Min, u64 Max)
{
  return (u64)((r64)(Value - Min) / (r64)(Max - Min));
}
internal s8
UnlerpS8(s8 Value, s8 Min, s8 Max)
{
  return (s8)((r64)(Value - Min) / (r64)(Max - Min));
}
internal s16
UnlerpS16(s16 Value, s16 Min, s16 Max)
{
  return (s16)((r64)(Value - Min) / (r64)(Max - Min));
}
internal s32
UnlerpS32(s32 Value, s32 Min, s32 Max)
{
  return (s32)((r64)(Value - Min) / (r64)(Max - Min));
}
internal s64
UnlerpS64(s64 Value, s64 Min, s64 Max)
{
  return (s64)((r64)(Value - Min) / (r64)(Max - Min));
}
internal r32
UnlerpR32(r32 Value, r32 Min, r32 Max)
{
  return (Value - Min) / (Max - Min);
}
internal r64
UnlerpR64(r64 Value, r64 Min, r64 Max)
{
  return (Value - Min) / (Max - Min);
}


internal u8
RemapU8(u8 Value, u8 OldMin, u8 OldMax, u8 NewMin, u8 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  u8 Result = (u8)((A * (NewMax - NewMin)) + NewMin);
  return Result;
}
internal u16
RemapU16(u16 Value, u16 OldMin, u16 OldMax, u16 NewMin, u16 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  u16 Result = (u16)((A * (NewMax - NewMin)) + NewMin);
  return Result;
}
internal u32
RemapU32(u32 Value, u32 OldMin, u32 OldMax, u32 NewMin, u32 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  u32 Result = (u32)((A * (NewMax - NewMin)) + NewMin);
  return Result;
}
internal u64
RemapU64(u64 Value, u64 OldMin, u64 OldMax, u64 NewMin, u64 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  u64 Result = (u64)((A * (NewMax - NewMin)) + NewMin);
  return Result;
}
internal s8
RemapS8(s8 Value, s8 OldMin, s8 OldMax, s8 NewMin, s8 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  s8 Result = (s8)((A * (NewMax - NewMin)) + NewMin);
  return Result;
}
internal s16
RemapS16(s16 Value, s16 OldMin, s16 OldMax, s16 NewMin, s16 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  s16 Result = (s16)((A * (NewMax - NewMin)) + NewMin);
  return Result;
}
internal s32
RemapS32(s32 Value, s32 OldMin, s32 OldMax, s32 NewMin, s32 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  s32 Result = (s32)((A * (NewMax - NewMin)) + NewMin);
  return Result;
}
internal s64
RemapS64(s64 Value, s64 OldMin, s64 OldMax, s64 NewMin, s64 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  s64 Result = (s64)((A * (NewMax - NewMin)) + NewMin);
  return Result;
}
internal r32
RemapR32(r32 Value, r32 OldMin, r32 OldMax, r32 NewMin, r32 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  r32 Result = (r32)((A * (NewMax - NewMin)) + NewMin);
  return Result;
}
internal r64
RemapR64(r64 Value, r64 OldMin, r64 OldMax, r64 NewMin, r64 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  r64 Result = (r64)((A * (NewMax - NewMin)) + NewMin);
  return Result;
}

internal u8
RemapClampedU8(u8 Value, u8 OldMin, u8 OldMax, u8 NewMin, u8 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  r64 AClamped = Clamp01(A);
  r64 UnclampedResult = ((AClamped * (NewMax - NewMin)) + NewMin);
  u8 Result = (u8)Clamp(NewMin, UnclampedResult, NewMax);
  return Result;
}
internal u16
RemapClampedU16(u16 Value, u16 OldMin, u16 OldMax, u16 NewMin, u16 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  r64 AClamped = Clamp01(A);
  r64 UnclampedResult = ((AClamped * (NewMax - NewMin)) + NewMin);
  u16 Result = (u16)Clamp(NewMin, UnclampedResult, NewMax);
  return Result;
}
internal u32
RemapClampedU32(u32 Value, u32 OldMin, u32 OldMax, u32 NewMin, u32 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  r64 AClamped = Clamp01(A);
  r64 UnclampedResult = ((AClamped * (NewMax - NewMin)) + NewMin);
  u32 Result = (u32)Clamp(NewMin, UnclampedResult, NewMax);
  return Result;
}
internal u64
RemapClampedU64(u64 Value, u64 OldMin, u64 OldMax, u64 NewMin, u64 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  r64 AClamped = Clamp01(A);
  r64 UnclampedResult = ((AClamped * (NewMax - NewMin)) + NewMin);
  u64 Result = (u64)Clamp(NewMin, UnclampedResult, NewMax);
  return Result;
}
internal s8
RemapClampedS8(s8 Value, s8 OldMin, s8 OldMax, s8 NewMin, s8 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  r64 AClamped = Clamp01(A);
  r64 UnclampedResult = ((AClamped * (NewMax - NewMin)) + NewMin);
  s8 Result = (s8)Clamp(NewMin, UnclampedResult, NewMax);
  return Result;
}
internal s16
RemapClampedS16(s16 Value, s16 OldMin, s16 OldMax, s16 NewMin, s16 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  r64 AClamped = Clamp01(A);
  r64 UnclampedResult = ((AClamped * (NewMax - NewMin)) + NewMin);
  s16 Result = (s16)Clamp(NewMin, UnclampedResult, NewMax);
  return Result;
}
internal s32
RemapClampedS32(s32 Value, s32 OldMin, s32 OldMax, s32 NewMin, s32 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  r64 AClamped = Clamp01(A);
  r64 UnclampedResult = ((AClamped * (NewMax - NewMin)) + NewMin);
  s32 Result = (s32)Clamp(NewMin, UnclampedResult, NewMax);
  return Result;
}
internal s64
RemapClampedS64(s64 Value, s64 OldMin, s64 OldMax, s64 NewMin, s64 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  r64 AClamped = Clamp01(A);
  r64 UnclampedResult = ((AClamped * (NewMax - NewMin)) + NewMin);
  s64 Result = (s64)Clamp(NewMin, UnclampedResult, NewMax);
  return Result;
}
internal r32
RemapClampedR32(r32 Value, r32 OldMin, r32 OldMax, r32 NewMin, r32 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  r64 AClamped = Clamp01(A);
  r64 UnclampedResult = ((AClamped * (NewMax - NewMin)) + NewMin);
  r32 Result = (r32)Clamp(NewMin, UnclampedResult, NewMax);
  return Result;
}
internal r64
RemapClampedR64(r64 Value, r64 OldMin, r64 OldMax, r64 NewMin, r64 NewMax)
{
  r64 A = (r64)(Value - OldMin) / (r64)(OldMax - OldMin);
  r64 AClamped = Clamp01(A);
  r64 UnclampedResult = ((AClamped * (NewMax - NewMin)) + NewMin);
  r64 Result = (r64)Clamp(NewMin, UnclampedResult, NewMax);
  return Result;
}

internal r32
FloorR32(r32 V)
{
  return (r32)((s64)V);
}
internal r64
FloorR64(r64 V)
{
  return (r64)((s64)V);
}

internal r32
FractR32(r32 V)
{
  return V - FloorR32(V);
}
internal r64
FractR64(r64 V)
{
  return V - FloorR64(V);
}

internal r32
SqrtR32(r32 V)
{
  return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(V)));
}
internal u32
SqrtU32(u32 V)
{
  return sqrt(V);
}

internal r32
ModR32(r32 Value, r32 Int)
{
  r32 Div = Value / Int;
  r32 Fract = Abs(FractR32(Div));
  return Int * Fract;
}
internal r64
ModR64(r64 Value, r64 Int)
{
  r64 Div = Value / Int;
  r64 Fract = Abs(FractR64(Div));
  return Int * Fract;
}

internal r32
SinR32(r32 Rad)
{
  return sinf(Rad);
}
internal r64
SinR64(r64 Rad)
{
  return sin(Rad);
}

internal r32
CosR32(r32 Rad)
{
  return cosf(Rad);
}
internal r64
CosR64(r64 Rad)
{
  return cos(Rad);
}

internal r32
TanR32(r32 Rad)
{
  return tanf(Rad);
}
internal r64
TanR64(r64 Rad)
{
  return tan(Rad);
}

internal r32
ASinR32(r32 Rad)
{
  return asinf(Rad);
}
internal r64
ASinR64(r64 Rad)
{
  return asin(Rad);
}

internal r32
ACosR32(r32 Rad)
{
  return acosf(Rad);
}
internal r64
ACosR64(r64 Rad)
{
  return acos(Rad);
}

internal r32
ATanR32(r32 Rad)
{
  return atanf(Rad);
}
internal r64
ATanR64(r64 Rad)
{
  return atan(Rad);
}

///////////////////////////
//
// Vector

internal v2
V2MultiplyPairwise(v2 A, v2 B)
{
  v2 Result = v2{
    A.x * B.x,
    A.y * B.y,
  };
  return Result;
}

internal v3
V3MultiplyPairwise(v3 A, v3 B)
{
  v3 Result = v3{
    A.x * B.x,
    A.y * B.y,
    A.z * B.z,
  };
  return Result;
}

internal v4
V4MultiplyPairwise(v4 A, v4 B)
{
  v4 Result = v4{
    A.x * B.x,
    A.y * B.y,
    A.z * B.z,
    A.w * B.w,
  };
  return Result;
}


v2 operator- (v2 A) { return { -A.x, -A.y }; }
v3 operator- (v3 A) { return { -A.x, -A.y, -A.z }; }
v4 operator- (v4 A) { return { -A.x, -A.y, -A.z, -A.w }; }

v2 operator+ (v2 A, v2 B) { return { A.x + B.x, A.y + B.y }; }
v3 operator+ (v3 A, v3 B) { return { A.x + B.x, A.y + B.y, A.z + B.z }; }
v4 operator+ (v4 A, v4 B) { return { A.x + B.x, A.y + B.y, A.z + B.z, A.w + B.w }; }

v2 operator- (v2 A, v2 B) { return { A.x - B.x, A.y - B.y }; }
v3 operator- (v3 A, v3 B) { return { A.x - B.x, A.y - B.y, A.z - B.z }; }
v4 operator- (v4 A, v4 B) { return { A.x - B.x, A.y - B.y, A.z - B.z, A.w - B.w }; }

void operator+= (v2& A, v2 B) { A.x += B.x; A.y += B.y; }
void operator+= (v3& A, v3 B) { A.x += B.x; A.y += B.y; A.z += B.z; }
void operator+= (v4& A, v4 B) { A.x += B.x; A.y += B.y; A.z += B.z; A.w += B.w; }

void operator-= (v2& A, v2 B) { A.x -= B.x; A.y -= B.y; }
void operator-= (v3& A, v3 B) { A.x -= B.x; A.y -= B.y; A.z -= B.z; }
void operator-= (v4& A, v4 B) { A.x -= B.x; A.y -= B.y; A.z -= B.z; A.w -= B.w; }

v2 operator* (v2 A, r32 B) { return { A.x * B, A.y * B }; }
v3 operator* (v3 A, r32 B) { return { A.x * B, A.y * B, A.z * B }; }
v4 operator* (v4 A, r32 B) { return { A.x * B, A.y * B, A.z * B, A.w * B }; }

v2 operator/ (v2 A, r32 B) { return { A.x / B, A.y / B }; }
v3 operator/ (v3 A, r32 B) { return { A.x / B, A.y / B, A.z / B }; }
v4 operator/ (v4 A, r32 B) { return { A.x / B, A.y / B, A.z / B, A.w / B }; }

void operator*= (v2& A, r32 B) { A.x *= B; A.y *= B; }
void operator*= (v3& A, r32 B) { A.x *= B; A.y *= B; A.z *= B; }
void operator*= (v4& A, r32 B) { A.x *= B; A.y *= B; A.z *= B; A.w *= B; }

void operator/= (v2& A, r32 B) { A.x /= B; A.y /= B; }
void operator/= (v3& A, r32 B) { A.x /= B; A.y /= B; A.z /= B; }
void operator/= (v4& A, r32 B) { A.x /= B; A.y /= B; A.z /= B; A.w /= B; }

bool operator == (v2 A, v2 B) { return ((A.x == B.x) && (A.y == B.y)); }
bool operator == (v3 A, v3 B) { return ((A.x == B.x) && (A.y == B.y) && (A.z == B.z)); }
bool operator == (v4 A, v4 B) { return ((A.x == B.x) && (A.y == B.y) && (A.z == B.z) && (A.w == B.w)); }

bool operator != (v2 A, v2 B) { return !((A.x == B.x) && (A.y == B.y)); }
bool operator != (v3 A, v3 B) { return !((A.x == B.x) && (A.y == B.y) && (A.z == B.z)); }
bool operator != (v4 A, v4 B) { return !((A.x == B.x) && (A.y == B.y) && (A.z == B.z) && (A.w == B.w)); }

internal v3 ToV3(v2 V, r32 Z = 0) { return v3{V.x, V.y, Z}; }
internal v4 V2ToV4(v2 V, r32 Z = 0, r32 W = 0) { return v4{V.x, V.y, Z, W}; }
internal v4 ToV4_(v3 V, r32 W)
{
  return v4{V.x, V.y, V.z, W};
}
#define ToV4Point(v) ToV4_((v), 1.0f) // all points have a w value of 1
#define ToV4Vec(v) ToV4_((v), 0.0f) // all vectors have a w value of 0 ie. they cannot be translated

internal r32 V2MagSquared(v2 V) { return ((V.x * V.x) + (V.y * V.y)); }
internal r32 V3MagSquared(v3 V) { return ((V.x * V.x) + (V.y * V.y) + (V.z * V.z)); }
internal r32 V4MagSquared(v4 V) { return ((V.x * V.x) + (V.y * V.y) + (V.z * V.z) + (V.w * V.w)); }

internal r32 V2Mag(v2 V) { return SqrtR32((V.x * V.x) + (V.y * V.y)); }
internal r32 V3Mag(v3 V) { return SqrtR32((V.x * V.x) + (V.y * V.y) + (V.z * V.z)); }
internal r32 V4Mag(v4 V) { return SqrtR32((V.x * V.x) + (V.y * V.y) + (V.z * V.z) + (V.w * V.w)); }

internal r32 V2DistanceSquared(v2 A, v2 B) { return V2MagSquared(A - B); }
internal r32 V3DistanceSquared(v3 A, v3 B) { return V3MagSquared(A - B); }
internal r32 V4DistanceSquared(v4 A, v4 B) { return V4MagSquared(A - B); }

internal r32 V2Distance(v2 A, v2 B) { return V2Mag(A - B); }
internal r32 V3Distance(v3 A, v3 B) { return V3Mag(A - B); }
internal r32 V4Distance(v4 A, v4 B) { return V4Mag(A - B); }

internal v2
V2Normalize(v2 A)
{
  r32 Magnitude = V2Mag(A);
  return A / Magnitude;
}
internal v3
V3Normalize(v3 A)
{
  r32 Magnitude = V3Mag(A);
  return A / Magnitude;
}
internal v4
V4Normalize(v4 A)
{
  r32 Magnitude = V4Mag(A);
  return A / Magnitude;
}

internal r32 V2Dot(v2 A, v2 B) { return ((A.x * B.x) + (A.y * B.y)); }
internal r32 V3Dot(v3 A, v3 B) { return ((A.x * B.x) + (A.y * B.y) + (A.z * B.z)); }
internal r32 V4Dot(v4 A, v4 B) { return ((A.x * B.x) + (A.y * B.y) + (A.z * B.z) + (A.w * B.w)); }

internal v2 V2PerpendicularCW(v2 A) { return v2{A.y, -A.x}; }
internal v2 V2PerpendicularCCW(v2 A) { return v2{A.y, A.x}; }

internal r32
V2Cross(v2 A, v2 B)
{
  return ((A.x * B.y) - (A.y * B.x));
}

internal v3
V3Cross(v3 A, v3 B)
{
  v3 Result = {
    (A.y * B.z) - (A.z * B.y),
    (A.z * B.x) - (A.x * B.z),
    (A.x * B.y) - (A.y * B.x)
  };
  return Result;
}

internal v4
V4Cross(v4 A, v4 B)
{
  v4 Result = {
    (A.y * B.z) - (A.z * B.y),
    (A.z * B.x) - (A.x * B.z),
    (A.x * B.y) - (A.y * B.x),
    0
  };
  return Result;
}

internal v2
V2Lerp(r32 T, v2 A, v2 B)
{
  v2 Result = v2{
    LerpR32(T, A.x, B.x),
    LerpR32(T, A.y, B.y),
  };
  return Result;
}

internal v3
V3Lerp(r32 T, v3 A, v3 B)
{
  v3 Result = v3{
    LerpR32(T, A.x, B.x),
    LerpR32(T, A.y, B.y),
    LerpR32(T, A.z, B.z),
  };
  return Result;
}

internal v4
V4Lerp(r32 T, v4 A, v4 B)
{
  v4 Result = v4{
    LerpR32(T, A.x, B.x),
    LerpR32(T, A.y, B.y),
    LerpR32(T, A.z, B.z),
    LerpR32(T, A.w, B.w),
  };
  return Result;
}

internal v2
V2Remap(v2 P, v2 OldMin, v2 OldMax, v2 NewMin, v2 NewMax)
{
  v2 Result = {0};
  Result.x = RemapR32(P.x, OldMin.x, OldMax.x, NewMin.x, NewMax.x);
  Result.y = RemapR32(P.y, OldMin.y, OldMax.y, NewMin.y, NewMax.y);
  return Result;
}

internal v3
V3Remap(v3 P, v3 OldMin, v3 OldMax, v3 NewMin, v3 NewMax)
{
  v3 Result = {0};
  Result.x = RemapR32(P.x, OldMin.x, OldMax.x, NewMin.x, NewMax.x);
  Result.y = RemapR32(P.y, OldMin.y, OldMax.y, NewMin.y, NewMax.y);
  Result.z = RemapR32(P.z, OldMin.z, OldMax.z, NewMin.z, NewMax.z);
  return Result;
}

internal v4
V4Remap(v4 P, v4 OldMin, v4 OldMax, v4 NewMin, v4 NewMax)
{
  v4 Result = {0};
  Result.x = RemapR32(P.x, OldMin.x, OldMax.x, NewMin.x, NewMax.x);
  Result.y = RemapR32(P.y, OldMin.y, OldMax.y, NewMin.y, NewMax.y);
  Result.z = RemapR32(P.z, OldMin.z, OldMax.z, NewMin.z, NewMax.z);
  Result.w = RemapR32(P.w, OldMin.w, OldMax.w, NewMin.w, NewMax.w);
  return Result;
}

internal v4
V4RemapAsV3(v4 P, v4 OldMin, v4 OldMax, v4 NewMin, v4 NewMax)
{
  v4 Result = {0};
  Result.xyz = V3Remap(P.xyz, OldMin.xyz, OldMax.xyz, NewMin.xyz, NewMax.xyz);
  Result.w = P.w;
  return Result;
}

///////////////////////////
//
// Ranges

internal rect2 MakeRect2MinDim(v2 Min, v2 Dim)
{
  rect2 Result = {0};
  Result.Min = Min;
  Result.Max = Min + Dim;
  return Result;
}

internal rect2 MakeRect2CenterDim(v2 Center, v2 Dim)
{
  v2 HalfDim = Dim / 2;
  rect2 Result = {0};
  Result.Min = Center - HalfDim;
  Result.Max = Center + HalfDim;
  return Result;
}

internal b32 ValueInRangeR32(r32 Min, r32 Max, r32 V)
{
  return ((V >= Min) && (V <= Max));
}

internal b32 ValueInRange1(range1 Range, r32 V)
{
  return ValueInRangeR32(Range.Min, Range.Max, V);
}
internal b32 ValueInRange2(range2 Range, v2 V)
{
  return (ValueInRangeR32(Range.Min.x, Range.Max.x, V.x) &&
          ValueInRangeR32(Range.Min.y, Range.Max.y, V.y));
}
internal b32 ValueInRange3(range3 Range, v3 V)
{
  return (ValueInRangeR32(Range.Min.x, Range.Max.x, V.x) &&
          ValueInRangeR32(Range.Min.y, Range.Max.y, V.y) &&
          ValueInRangeR32(Range.Min.z, Range.Max.z, V.z));
}
internal b32 ValueInRange4(range4 Range, v4 V)
{
  return (ValueInRangeR32(Range.Min.x, Range.Max.x, V.x) &&
          ValueInRangeR32(Range.Min.y, Range.Max.y, V.y) &&
          ValueInRangeR32(Range.Min.z, Range.Max.z, V.z) &&
          ValueInRangeR32(Range.Min.w, Range.Max.w, V.w));
}

#define PointIsInRect(range, point) ValueInRange2((range), (point))

internal r32 Range1SizeX(range1 Range) { return Range.Max - Range.Min; }
internal r32 Range2SizeX(range2 Range) { return Range.Max.x - Range.Min.x; }
internal r32 Range3SizeX(range3 Range) { return Range.Max.x - Range.Min.x; }
internal r32 Range4SizeX(range4 Range) { return Range.Max.x - Range.Min.x; }

#define Rect2Width(r) Range2SizeX((r))

internal r32 Range2SizeY(range2 Range) { return Range.Max.y - Range.Min.y; }
internal r32 Range3SizeY(range3 Range) { return Range.Max.y - Range.Min.y; }
internal r32 Range4SizeY(range4 Range) { return Range.Max.y - Range.Min.y; }

#define Rect2Height(r) Range2SizeY((r))

internal r32 Range3SizeZ(range3 Range) { return Range.Max.z - Range.Min.z; }
internal r32 Range4SizeZ(range4 Range) { return Range.Max.z - Range.Min.z; }

internal r32 Range4SizeW(range4 Range) { return Range.Max.w - Range.Min.w; }

internal r32 Range1Center(range1 Range) { return (Range.Max + Range.Min) / 2.0f; }
internal v2  Range2Center(range2 Range) { return (Range.Max + Range.Min) / 2.0f; }
internal v3  Range3Center(range3 Range) { return (Range.Max + Range.Min) / 2.0f; }
internal v4  Range4Center(range4 Range) { return (Range.Max + Range.Min) / 2.0f; }

#define Rect2Center(r) Range2Center((r))

internal range1 Range1Offset(range1 Range, r32 Delta) { return range1{ Range.Min + Delta, Range.Max + Delta }; }
internal range2 Range2Offset(range2 Range, v2 Delta) { return range2{ Range.Min + Delta, Range.Max + Delta }; }
internal range3 Range3Offset(range3 Range, v3 Delta) { return range3{ Range.Min + Delta, Range.Max + Delta }; }
internal range4 Range4Offset(range4 Range, v4 Delta) { return range4{ Range.Min + Delta, Range.Max + Delta }; }

#define Rect2Translate(r, d) Range2Offset((r), (d))
#define Rect2TranslateX(r, dx) Range2Offset((r), v2{(dx), 0})
#define Rect2TranslateY(r, dy) Range2Offset((r), v2{0, (dy)})

internal v2 RectTopLeft(rect2 Rect)
{
  return v2{ Rect.Min.x, Rect.Max.y };
}
internal v2 RectTopRight(rect2 Rect)
{
  return Rect.Max;
}
internal v2 RectBottomLeft(rect2 Rect)
{
  return Rect.Min;
}
internal v2 RectBottomRight(rect2 Rect)
{
  return v2{ Rect.Max.x, Rect.Min.y };
}

internal r32 AspectRatio(r32 Width, r32 Height) { return Width / Height; }
internal r32 RectAspectRatio(rect2 Rect) { return Range2SizeX(Rect) / Range2SizeY(Rect); }

internal void
RectHSplit(rect2 Rect, r32 YValue, rect2* Top, rect2* Bottom)
{
  r32 ClampedYValue = Clamp(Rect.Min.y, YValue, Rect.Max.y);
  Top->Max = Rect.Max;
  Top->Min = { Rect.Min.x, ClampedYValue };
  Bottom->Max = { Rect.Max.x, ClampedYValue };
  Bottom->Min = Rect.Min;
}

internal void
RectVSplit(rect2 Rect, r32 XValue, rect2* Left, rect2* Right)
{
  r32 ClampedXValue = Clamp(Rect.Min.x, XValue, Rect.Max.x);
  Left->Max = { ClampedXValue, Rect.Max.y};
  Left->Min = Rect.Min;
  Right->Max = Rect.Max;
  Right->Min = { ClampedXValue, Rect.Min.y };
}

internal void
RectHSplitAtDistanceFromTop(rect2 Rect, r32 YDist, rect2* Top, rect2* Bottom)
{
  RectHSplit(Rect, Rect.Max.y - YDist, Top, Bottom);
}
internal void
RectHSplitAtDistanceFromBottom(rect2 Rect, r32 YDist, rect2* Top, rect2* Bottom)
{
  RectHSplit(Rect, Rect.Min.y + YDist, Top, Bottom);
}
internal void
RectVSplitAtDistanceFromRight(rect2 Rect, r32 XDist, rect2* Left, rect2* Right)
{
  RectVSplit(Rect, Rect.Max.x - XDist, Left, Right);
}
internal void
RectVSplitAtDistanceFromLeft(rect2 Rect, r32 XDist, rect2* Left, rect2* Right)
{
  RectVSplit(Rect, Rect.Min.x + XDist, Left, Right);
}
internal void
RectHSplitAtPercent(rect2 Rect, r32 YPercent, rect2* Top, rect2* Bottom)
{
  RectHSplit(Rect, LerpR32(YPercent, Rect.Min.y, Rect.Max.y), Top, Bottom);
}
internal void
RectVSplitAtPercent(rect2 Rect, r32 XPercent, rect2* Left, rect2* Right)
{
  RectVSplit(Rect, LerpR32(XPercent, Rect.Min.x, Rect.Max.x), Left, Right);
}

internal rect2
RectInset(rect2 Outer, v2 Amount)
{
  rect2 Result = { Outer.Min + Amount, Outer.Max - Amount };
  return Result;
}
internal rect2
RectInset(rect2 Outer, r32 UniformAmount)
{
  return RectInset(Outer, v2{UniformAmount, UniformAmount});
}


internal range1
Range1Union(range1 A, range1 B)
{
  range1 Result = {};
  Result.Min = Max(A.Min, B.Min);
  Result.Max = Min(A.Max, B.Max);
  return Result;
}
#define Rect2Union(a,b) Range2Union((a), (b))
internal range2
Range2Union(range2 A, range2 B)
{
  range2 Result = {};
  Result.Min.x = Max(A.Min.x, B.Min.x);
  Result.Min.y = Max(A.Min.y, B.Min.y);
  Result.Max.x = Min(A.Max.x, B.Max.x);
  Result.Max.y = Min(A.Max.y, B.Max.y);
  
  if (Rect2Width(Result) < 0) { Result.Min.x = Result.Max.x; }
  if (Rect2Height(Result) < 0) { Result.Min.y = Result.Max.y; }
  
  return Result;
}
internal range3
Range3Union(range3 A, range3 B)
{
  range3 Result = {};
  Result.Min.x = Max(A.Min.x, B.Min.x);
  Result.Min.y = Max(A.Min.y, B.Min.y);
  Result.Min.z = Max(A.Min.z, B.Min.z);
  Result.Max.x = Min(A.Max.x, B.Max.x);
  Result.Max.y = Min(A.Max.y, B.Max.y);
  Result.Max.z = Min(A.Max.z, B.Max.z);
  return Result;
}

internal v2
Rect2GetRectLocalPoint(rect2 Rect, v2 Point)
{
  v2 Result = Point - Rect.Min;
  return Result;
}

internal r32
Rect2Area(rect2 Rect)
{
  r32 Result = Rect2Width(Rect) * Rect2Height(Rect);
  return Result;
}

internal v2
Rect2BottomLeft(rect2 Rect)
{
  v2 Result = Rect.Min;
  return Result;
}

internal v2
Rect2BottomRight(rect2 Rect)
{
  v2 Result = v2{ Rect.Max.x, Rect.Min.y };
  return Result;
}

internal v2
Rect2TopRight(rect2 Rect)
{
  v2 Result = Rect.Max;
  return Result;
}

internal v2
Rect2TopLeft(rect2 Rect)
{
  v2 Result = v2{ Rect.Min.x, Rect.Max.y };
  return Result;
}

///////////////////////////
//
// Ray

internal v4
RayGetPointAlong(v4 RayOrigin, v4 RayDirection, r32 T)
{
  v4 Result = RayOrigin + (RayDirection * T);
  return Result;
}

internal r32
RayPlaneIntersectionDistance(v4 RayOrigin, v4 RayDirection, v4 PlanePoint, v4 PlaneNormal)
{
  r32 T = 0.0f;
  float Denominator = V4Dot(PlaneNormal, RayDirection);
  if (Abs(Denominator) > 0.00001f)
  {
    T = V4Dot(PlanePoint - RayDirection, PlaneNormal) / Denominator;
  }
  return T;
}

internal v4
GetRayPlaneIntersectionPoint(v4 RayOrigin, v4 RayDirection, v4 PlanePoint, v4 PlaneNormal)
{
  v4 Result = {0};
  r32 T = RayPlaneIntersectionDistance(RayOrigin, RayDirection, PlanePoint, PlaneNormal);
  if (T >= 0)
  {
    Result = RayGetPointAlong(RayOrigin, RayDirection, T);
  }
  return Result;
}
internal v4
GetRayPlaneIntersectionPoint(v4_ray Ray, v4 PlanePoint, v4 PlaneNormal)
{
  return GetRayPlaneIntersectionPoint(Ray.Origin, Ray.Direction, PlanePoint, PlaneNormal);
}

internal bool
RayIntersectsPlane(v4 RayOrigin, v4 RayDirection, v4 PlanePoint, v4 PlaneNormal, v4* OutPoint)
{
  bool Result = false;
  r32 T = RayPlaneIntersectionDistance(RayOrigin, RayDirection, PlanePoint, PlaneNormal);
  if (T >= 0)
  {
    Result = true;
    *OutPoint = RayGetPointAlong(RayOrigin, RayDirection, T);
  }
  return Result;
}
internal bool
RayIntersectsPlane(v4_ray Ray, v4 PlanePoint, v4 PlaneNormal, v4* OutPoint)
{
  return RayIntersectsPlane(Ray.Origin, Ray.Direction, PlanePoint, PlaneNormal, OutPoint);
}

///////////////////////////
//
// Matrices

internal m44
M44Identity()
{
  m44 M = {0};
  M.AXx = 1.0f;
  M.AYy = 1.0f;
  M.AZz = 1.0f;
  M.Tw = 1.0f;
  return M;
}

internal m44
M44Transpose(m44 M)
{
  m44 Result = {0};
  for (u32 Y = 0; Y < 4; Y++)
  {
    for (u32 X = 0; X < 4; X++)
    {
      Result.Array[(X * 4) + Y] = M.Array[(Y * 4) + X];
    }
  }
  return Result;
}

// Matrix * Matrix

m44 operator* (m44 L, m44 R)
{
  m44 M = {0};
  
  //          ci      ic        ci      ic        ci      ic        i      ic
  M.AXx = (L.AXx * R.AXx) + (L.AYx * R.AXy) + (L.AZx * R.AXz) + (L.Tx * R.AXw);
  M.AXy = (L.AXy * R.AXx) + (L.AYy * R.AXy) + (L.AZy * R.AXz) + (L.Ty * R.AXw);
  M.AXz = (L.AXz * R.AXx) + (L.AYz * R.AXy) + (L.AZz * R.AXz) + (L.Tz * R.AXw);
  M.AXw = (L.AXw * R.AXx) + (L.AYw * R.AXy) + (L.AZw * R.AXz) + (L.Tw * R.AXw);
  
  M.AYx = (L.AXx * R.AYx) + (L.AYx * R.AYy) + (L.AZx * R.AYz) + (L.Tx * R.AYw);
  M.AYy = (L.AXy * R.AYx) + (L.AYy * R.AYy) + (L.AZy * R.AYz) + (L.Ty * R.AYw);
  M.AYz = (L.AXz * R.AYx) + (L.AYz * R.AYy) + (L.AZz * R.AYz) + (L.Tz * R.AYw);
  M.AYz = (L.AXw * R.AYx) + (L.AYw * R.AYy) + (L.AZw * R.AYz) + (L.Tw * R.AYw);
  
  M.AZx = (L.AXx * R.AZx) + (L.AYx * R.AZy) + (L.AZx * R.AZz) + (L.Tx * R.AZw);
  M.AZy = (L.AXy * R.AZx) + (L.AYy * R.AZy) + (L.AZy * R.AZz) + (L.Ty * R.AZw);
  M.AZz = (L.AXz * R.AZx) + (L.AYz * R.AZy) + (L.AZz * R.AZz) + (L.Tz * R.AZw);
  M.AZw = (L.AXw * R.AZx) + (L.AYw * R.AZy) + (L.AZw * R.AZz) + (L.Tw * R.AZw);
  
  M.Tx = (L.AXx * R.Tx) + (L.AYx * R.Ty) + (L.AZx * R.Tz) + (L.Tx * R.Tw);
  M.Ty = (L.AXy * R.Tx) + (L.AYy * R.Ty) + (L.AZy * R.Tz) + (L.Ty * R.Tw);
  M.Tz = (L.AXz * R.Tx) + (L.AYz * R.Ty) + (L.AZz * R.Tz) + (L.Tz * R.Tw);
  M.Tw = (L.AXw * R.Tx) + (L.AYw * R.Ty) + (L.AZw * R.Tz) + (L.Tw * R.Tw);
  
  return M;
}

// Matrix * Vector

v4 operator* (m44 M, v4 V)
{
  v4 Result = {0};
  Result.x = (V.x * M.AXx) + (V.y * M.AYx) + (V.z * M.AZx) + (V.w * M.Tx);
  Result.y = (V.x * M.AXy) + (V.y * M.AYy) + (V.z * M.AZy) + (V.w * M.Ty);
  Result.z = (V.x * M.AXz) + (V.y * M.AYz) + (V.z * M.AZz) + (V.w * M.Tz);
  Result.w = (V.x * M.AXw) + (V.y * M.AYw) + (V.z * M.AZw) + (V.w * M.Tw);
  return Result;
}

internal m44
M44Translation(v4 Offset)
{
  m44 Result = M44Identity();
  Result.Tx = Offset.x;
  Result.Ty = Offset.y;
  Result.Tz = Offset.z;
  return Result;
}

internal m44
M44RotationX(r32 Radians)
{
  r32 CosRad = CosR32(Radians);
  r32 SinRad = SinR32(Radians);
  m44 Result = M44Identity();
  Result.AYy = CosRad;
  Result.AZy = SinRad;
  Result.AYz = -SinRad;
  Result.AZz = CosRad;
  return Result;
}

internal m44
M44RotationY(r32 Radians)
{
  r32 CosRad = CosR32(Radians);
  r32 SinRad = SinR32(Radians);
  m44 Result = M44Identity();
  Result.AXx = CosRad;
  Result.AZx = SinRad;
  Result.AXz = -SinRad;
  Result.AZz = CosRad;
  return Result;
}

internal m44
M44RotationZ(r32 Radians)
{
  r32 CosRad = CosR32(Radians);
  r32 SinRad = SinR32(Radians);
  m44 Result = M44Identity();
  Result.AXx = CosRad;
  Result.AYx = -SinRad;
  Result.AXy = SinRad;
  Result.AYy = CosRad;
  return Result;
}

internal m44
M44Rotation(v3 Radians)
{
  r32 CosX = CosR32(Radians.x);
  r32 SinX = SinR32(Radians.x);
  r32 CosY = CosR32(Radians.y);
  r32 SinY = SinR32(Radians.y);
  r32 CosZ = CosR32(Radians.z);
  r32 SinZ = SinR32(Radians.z);
  
  m44 Result = {0};
  Result.AXx = CosY * CosZ;
  Result.AXy = -(SinX * SinY * CosZ) + (CosX * SinZ);
  Result.AXz = -(CosX * SinY * CosZ) - (SinX * SinZ);
  Result.AXw = 0;
  
  Result.AYx = -(SinZ * CosY);
  Result.AYy = (SinX * SinY * SinZ) + (CosX * CosZ);
  Result.AYz = (CosX * SinY * SinZ) - (SinX * CosZ);
  Result.AYw = 0;
  
  Result.AZx = SinY;
  Result.AZy = SinX * CosY;
  Result.AZz = CosX * CosY;
  Result.AZw = 0;
  
  Result.Tx = 0;
  Result.Ty = 0;
  Result.Tz = 0;
  Result.Tw = 1;
  
  return Result;
}

internal m44
M44Scale(v3 Scale)
{
  m44 Result = M44Identity();
  Result.AXx = Scale.x;
  Result.AYy = Scale.y;
  Result.AZz = Scale.z;
  return Result;
}

internal m44
M44ScaleUniform(r32 Scale)
{
  m44 Result = M44Identity();
  Result.AXx = Scale;
  Result.AYy = Scale;
  Result.AZz = Scale;
  return Result;
}

internal m44
M44CoordinateFrame(v4 Forward, v4 Right, v4 Up)
{
  m44 Result = {0};
  Result.AXx = Right.x;
  Result.AYx = Right.y;
  Result.AZx = Right.z;
  Result.Tx = Right.w;
  
  Result.AXy = Up.x;
  Result.AYy = Up.y;
  Result.AZy = Up.z;
  Result.Ty = Up.w;
  
  Result.AXz = Forward.x;
  Result.AYz = Forward.y;
  Result.AZz = Forward.z;
  Result.Tz = Forward.w;
  
  Result.Tw = 1.0f;
  return Result;
}

internal m44
M44ModelMatrix(v4 Forward, v4 Right, v4 Up, v4 Position)
{
  m44 RotationMatrix = M44CoordinateFrame(Forward, Right, Up);
  m44 PositionMatrix = M44Translation(-Position);
  m44 ModelViewMatrix = PositionMatrix * RotationMatrix;
  return ModelViewMatrix;
}

internal m44
M44ProjectionOrtho(r32 Width, r32 Height, r32 Near, r32 Far, r32 Right, r32 Left, r32 Top, r32 Bottom)
{
  m44 Result = {0};
  Result.AXx = 2.0f / Width;
  Result.AYy = 2.0f / Height;
  Result.AZz = 2.0f / (Near - Far);
  Result.AXw = -(Right + Left) / (Right - Left);
  Result.AYw = -(Top + Bottom) / (Top - Bottom);
  Result.AZw = -(Far + Near) / (Far - Near);
  Result.Tw = 1;
  return Result;
}

internal m44
M44ProjectionOrtho(r32 Aspect, r32 Scale, r32 Near, r32 Far)
{
  m44 Result = {0};
  r32 Width = Scale * Aspect;
  r32 Height = Scale;
  r32 Right = Width / 2.0f;
  r32 Left = -Right;
  r32 Top = Height / 2.0f;
  r32 Bottom = -Top;
  Result = M44ProjectionOrtho(Width, Height, Near, Far, Right, Left, Top, Bottom);
  return Result;
}

internal m44
M44ProjectionInterfaceOrtho(r32 Width, r32 Height, r32 Near, r32 Far)
{
  m44 Result = {0};
  r32 Aspect = Width / Height;
  r32 Right = Width;
  r32 Left = 0;
  r32 Top = Height;
  r32 Bottom = 0;
  Result = M44ProjectionOrtho(Width, Height, Near, Far, Right, Left, Top, Bottom);
  return Result;
}

internal m44
M44ProjectionPerspective(r32 FieldOfViewDegrees, r32 AspectRatio, r32 Near, r32 Far)
{
  m44 Result = M44Identity();
  
  // The perspective divide step involves dividing x and y by -z
  // Making Tz = -1 will make Tw of the result = -z
  Result.Tw = 0;
  Result.AZw = -1;
  
  // Remap z' from the range [near clip : far clip] to [0 : 1]
  r32 ViewRange = Far - Near;
  Result.AZz = -((Far + Near) / ViewRange);
  Result.Tz = -(2 * Near * Far) / ViewRange;
  
  // Adjust for field of view - adjust the x' and y coordinates based
  // on how
  r32 FovBasedScale = TanR32(DegToRadR32(FieldOfViewDegrees / 2));
  r32 Top = Near * FovBasedScale;
  r32 Bottom = -Top;
  r32 Right = Top * AspectRatio;
  r32 Left = -Right;
  Result.AXx = (2 * Near) / (Right - Left);
  Result.AZx = (Right + Left) / (Right - Left);
  Result.AYy = (2 * Near) / (Top - Bottom);
  Result.AZy = (Top + Bottom) / (Top - Bottom);
  
  return Result;
}

internal m44
M44LookAt(v4 Position, v4 Target)
{
  // NOTE(Peter): the camera usually points along the -z axis, hence
  // Forward = a ray that points from the target back towards your position
  v4 Forward = V4Normalize(Position - Target);
  v4 Right = V4Normalize(V4Cross(v4{0, 1, 0, 0}, Forward));
  v4 Up = V4Normalize(V4Cross(Forward, Right));
  m44 Result = M44CoordinateFrame(Forward, Right, Up);
  return Result;
}

///////////////////////////
//
// Strings

internal gs_const_string ConstString(char* Data, u64 Length) { return gs_const_string{Data, Length}; }
internal gs_const_string ConstString(char* Data) { return gs_const_string{Data, CStringLength(Data)}; }
internal gs_string MakeString(char* Data, u64 Length, u64 Size)
{
  Assert(Length <= Size);
  gs_string Result = {0};
  Result.Str = Data;
  Result.Length = Length;
  Result.Size = Size;
  return Result;
}
internal gs_string MakeString(char* Data, u64 Length)
{
  return MakeString(Data, Length, Length);
}
internal gs_string MakeString(char* Data)
{
  u64 StringLength = CStringLength(Data);
  return MakeString(Data, StringLength, StringLength);
}
internal gs_string MakeString(gs_const_string ConstString)
{
  return MakeString(ConstString.Str, ConstString.Length);
}

internal gs_data StringToData(gs_const_string String)
{
  gs_data Result = gs_data{0};
  Result.Memory = (u8*)String.Str;
  Result.Size = String.Length * sizeof(char);
  return Result;
}
internal gs_data StringToData(gs_string String)
{
  return StringToData(String.ConstString);
}
internal gs_const_string DataToString(gs_data Data)
{
  gs_const_string Result = {};
  Result.Str = (char*)Data.Memory;
  Result.Length = Data.Size;
  return Result;
}

internal bool IsSlash(char C) { return ((C == '/') || (C == '\\')); }
internal bool IsUpper(char C) { return(('A' <= C) && (C <= 'Z')); }
internal bool IsLower(char C) { return(('a' <= C) && (C <= 'z')); }
internal bool IsWhitespace(char C) { return (C == ' ' || C == '\n' || C == '\r' || C == '\t' || C == '\f' || C == '\v'); }
internal bool IsNewline(char C) { return (C == '\n') || (C == '\r'); }
internal bool IsNewlineOrWhitespace (char C) { return IsNewline(C) || IsWhitespace(C); }
internal bool IsBase8(char C) { return (C >= '0' && C <= '7'); }
internal bool IsBase10(char C) { return (C >= '0' && C <= '9'); }
internal bool IsBase16(char C) { return (C >= '0' && C <= '9') || (C >= 'A' && C <= 'F'); }
internal bool IsNumericDecimal(char C) { return IsBase10(C) || (C == '.'); }
internal bool IsNumericExtended(char C) { return IsNumericDecimal(C) || (C == 'x') || (C == 'f') || (C == '-'); }
internal bool IsAlpha(char C) { return( (('a' <= C) && (C <= 'z')) || (('A' <= C) && (C <= 'Z')) || C == '_'); }
internal bool IsAlphaNumeric(char C) { return((('a' <= C) && (C <= 'z')) || (('A' <= C) && (C <= 'Z')) || (('0' <= C) && (C <= '9')) || C == '_'); }
internal bool IsOperator(char C) {
  return ((C == '+') || (C == '-') || (C == '*') || (C == '/') ||
          (C == '=') || (C == '%') || (C == '<') || (C == '>'));
}

internal char
ToUpper(char C)
{
  if ((C >= 'a') && (C <= 'z'))
  {
    C -= 'a' - 'A';
  }
  return C;
}
internal char
ToLower(char C)
{
  if ((C >= 'A') && (C <= 'Z'))
  {
    C += 'a' - 'A';
  }
  return C;
}
internal bool CharsEqualCaseInsensitive(char A, char B) { return ToLower(A) == ToLower(B); }

internal u64
CharArrayLength (char* CS)
{
  char* At = CS;
  while (*At) { At++; }
  return (u64)(At - CS);
}

internal bool
IsNullTerminated(gs_const_string String)
{
  bool Result = false;
  if (String.Str)
  {
    Result = (String.Str[String.Length] == 0);
  }
  return Result;
}
internal bool
IsNullTerminated(gs_string String)
{
  return IsNullTerminated(String.ConstString);
}

internal char
GetChar(gs_const_string String, u64 I)
{
  char Result = 0;
  if (I < String.Length)
  {
    Result = String.Str[I];
  }
  return Result;
}
internal char
GetChar(gs_string String, u64 I)
{
  char Result = 0;
  if (I < String.Length)
  {
    Result = String.Str[I];
  }
  return Result;
}

internal gs_const_string
GetStringPrefix(gs_const_string String, u64 Size)
{
  gs_const_string Result = String;
  Result.Length = Min(Size, String.Length);
  return Result;
}
internal gs_const_string
GetStringPostfix(gs_const_string String, u64 Size)
{
  gs_const_string Result = String;
  u64 PostfixSize = Min(Size, String.Length);
  Result.Str += (Result.Length - PostfixSize);
  Result.Length = PostfixSize;
  return Result;
}
internal gs_const_string
GetStringAfter(gs_const_string String, u64 Cut)
{
  gs_const_string Result = String;
  u64 CutSize = Min(Cut, String.Length);
  Result.Str += CutSize;
  Result.Length -= CutSize;
  return Result;
}
internal gs_string
GetStringAfter(gs_string String, u64 Cut)
{
  gs_string Result = {0};
  Result.ConstString = GetStringAfter(String.ConstString, Cut);
  Result.Size = String.Size - Cut;
  return Result;
}
internal gs_const_string
GetStringBefore(gs_const_string String, u64 Cut)
{
  gs_const_string Result = String;
  Result.Length = Min(Cut, String.Length);
  return Result;
}
internal gs_const_string
Substring(gs_const_string String, u64 First, u64 Last)
{
  gs_const_string Result = {0};
  Result.Str = String.Str + Min(First, String.Length);
  Result.Length = Min(Last - First, String.Length);
  return Result;
}
internal gs_const_string
Substring(gs_string String, u64 First, u64 Last)
{
  return Substring(String.ConstString, First, Last);
}

internal s64
FindFirst(gs_const_string String, u64 StartIndex, char C)
{
  s64 Result = -1;
  for(u64 i = StartIndex; i < String.Length; i++)
  {
    if (String.Str[i] == C) {
      Result = (s64)i;
      break;
    }
  }
  return Result;
}
internal s64
FindFirst(gs_const_string String, char C)
{
  return FindFirst(String, 0, C);
}
internal s64
FindFirst(gs_string String, u64 StartIndex, char C)
{
  return FindFirst(String.ConstString, StartIndex, C);
}
internal s64
FindFirst(gs_string String, char C)
{
  return FindFirst(String.ConstString, 0, C);
}

internal s64
FindLast(char* String, s64 StartIndex, char C)
{
  s64 Result = -1;
  s64 i = 0;
  while (String[i] != 0 && i < StartIndex)
  {
    i++;
  }
  while (String[i])
  {
    if (String[i] == C)
    {
      Result = i;
    }
    i++;
  }
  return Result;
}
internal s64
FindLast(gs_const_string String, u64 StartIndex, char C)
{
  s64 Result = -1;
  for(s64 i= StartIndex; i >= 0; i--)
  {
    if (String.Str[i] == C) {
      Result = i;
      break;
    }
  }
  return (u64)Result;
}
internal s64
FindLast(gs_const_string String, char C)
{
  return FindLast(String, String.Length - 1, C);
}
internal s64
FindLast(gs_string String, u64 StartIndex, char C)
{
  return FindLast(String.ConstString, StartIndex, C);
}
internal s64
FindLast(gs_string String, char C)
{
  return FindLast(String.ConstString, String.Length - 1, C);
}

internal s64
FindFirstFromSet(gs_const_string String, char* SetArray)
{
  gs_const_string Set = ConstString(SetArray);
  s64 Result = -1;
  
  s64 CurrMin = String.Length;
  for (u64 SetAt = 0; SetAt < Set.Length; SetAt++)
  {
    s64 Index = FindFirst(String, Set.Str[SetAt]);
    if (Index >= 0 && Index < CurrMin)
    {
      CurrMin = Index;
    }
  }
  
  if (CurrMin < (s64)String.Length)
  {
    Result = CurrMin;
  }
  
  return Result;
}

internal s64
FindLastFromSet(gs_const_string String, char* SetArray)
{
  gs_const_string Set = ConstString(SetArray);
  s64 Result = -1;
  for(s64 At = String.Length - 1; At >= 0; At--)
  {
    char CharAt = String.Str[At];
    for (u64 SetAt = 0; SetAt < Set.Length; SetAt++)
    {
      if (CharAt == Set.Str[SetAt])
      {
        Result = (u64)At;
        // NOTE(Peter): The alternative to this goto is a break in the inner loop
        // followed by an if check in the outer loop, that must be evaluated
        // every character you check. This is more efficient
        goto find_first_from_set_complete;
      }
    }
  }
  find_first_from_set_complete:
  return Result;
}

internal bool
StringContains(gs_const_string Str, char C)
{
  bool Result = false;
  for (u32 i = 0; i < Str.Length; i++)
  {
    if (Str.Str[i] == C)
    {
      Result = true;
      break;
    }
  }
  return Result;
}

internal bool
StringsEqualUpToLength(gs_const_string A, gs_const_string B, u64 Length)
{
  bool Result = false;
  if (A.Length >= Length &&  B.Length >= Length)
  {
    Result = true;
    Length = Min(Length, A.Length);
    for (u64 i = 0; i < Length; i++)
    {
      if (A.Str[i] != B.Str[i])
      {
        Result = false;
        break;
      }
    }
  }
  return Result;
}
internal bool
StringsEqual(gs_const_string A, gs_const_string B)
{
  bool Result = false;
  if (A.Length == B.Length)
  {
    Result = StringsEqualUpToLength(A, B, A.Length);
  }
  return Result;
}
internal bool
StringEqualsCharArray(gs_const_string A, char* B, u64 Length)
{
  gs_const_string BStr = ConstString(B, Length);
  return StringsEqual(A, BStr);
}
internal bool
StringEqualsCharArray(gs_const_string A, char* B)
{
  u64 Length = CStringLength(B);
  return StringEqualsCharArray(A, B, Length);
}
internal bool
StringsEqualUpToLength(gs_string A, gs_string B, u64 Length)
{
  return StringsEqualUpToLength(A.ConstString, B.ConstString, Length);
}
internal bool
StringsEqual(gs_string A, gs_string B)
{
  return StringsEqual(A.ConstString, B.ConstString);
}
internal bool
StringEqualsCharArray(gs_string A, char* B, u64 Length)
{
  return StringEqualsCharArray(A.ConstString, B, Length);
}
internal bool
StringEqualsCharArray(gs_string A, char* B)
{
  return StringEqualsCharArray(A.ConstString, B);
}

internal u64
StringSizeLeft(gs_string String)
{
  u64 Result = String.Size - String.Length;
  return Result;
}

internal void
ReverseStringInPlace(gs_string* String)
{
  char* Start = String->Str;
  char* End = String->Str + String->Length;
  while (Start < End)
  {
    End--;
    char Temp = End[0];
    End[0] = Start[0];
    Start[0] = Temp;
    Start++;
  }
}

internal gs_const_string
GetCharSetForBase(u64 Base)
{
  gs_const_string Result = {0};
  switch(Base)
  {
    case 8: { Result = Base8Chars; }break;
    case 10: { Result = Base10Chars; }break;
    case 16: { Result = Base16Chars; }break;
    InvalidDefaultCase;
  }
  return Result;
}
internal u64
CharToUInt(char C, gs_const_string CharSet)
{
  return (u64)FindFirst(CharSet, C);
}
internal u64
CharToUInt(char C)
{
  return (u64)CharToUInt(C, Base10Chars);
}
internal u64
CharToUInt(char C, u64 Base)
{
  return CharToUInt(C, GetCharSetForBase(Base));
}

struct parse_uint_result
{
  b8 Success;
  u64 Value;
  u32 ParsedLength;
};

internal parse_uint_result
ValidateAndParseUInt(gs_const_string String, u64 Base = 10)
{
  parse_uint_result Result = {0};
  
  gs_const_string CharSet = GetCharSetForBase(Base);
  
  bool StringIsValid = true;
  for (u32 i = 0; i < String.Length; i++)
  {
    if (!StringContains(CharSet, String.Str[i]))
    {
      StringIsValid = false;
      break;
    }
  }
  
  if (StringIsValid)
  {
    u64 Acc = 0;
    u64 i = 0;
    for (; i < String.Length; i++)
    {
      u64 CharIndex = FindFirst(CharSet, String.Str[i]);
      if (CharIndex  < CharSet.Length)
      {
        Acc = CharToUInt(String.Str[i], CharSet) + (Acc * Base);
      }
      else
      {
        break;
      }
    }
    
    Result.Success = true;
    Result.Value = Acc;
    Result.ParsedLength = i;
  }
  
  return Result;
}

internal u64
ParseUInt(gs_const_string String, u64 Base = 10, u64* ParsedLength = 0)
{
  parse_uint_result ParseResult = ValidateAndParseUInt(String, Base);
  Assert(ParseResult.Success);
  if (ParsedLength)
  {
    *ParsedLength = ParseResult.ParsedLength;
  }
  return ParseResult.Value;
}
internal u64
ParseUInt(u64 Length, char* String, u64 Base = 10, u64* ParsedLength = 0)
{
  return ParseUInt(ConstString(String, Length), Base, ParsedLength);
}
internal u64
ParseUInt(char* String, u64 Base = 10, u64* ParsedLength = 0)
{
  return ParseUInt(LitString(String), Base, ParsedLength);
}

internal s64
ParseInt(gs_const_string String, u64 Base = 10, u64* ParsedLength = 0)
{
  s64 Result = 0;
  u64 TempParsedLength = 0;
  if (String.Str[0] == '-')
  {
    Result = -1 * (s64)ParseUInt(GetStringAfter(String, 1), Base, &TempParsedLength);
    TempParsedLength += 1;
  }
  else
  {
    Result = (s64)ParseUInt(String, Base, &TempParsedLength);
  }
  if (ParsedLength != 0)
  {
    *ParsedLength = TempParsedLength;
  }
  return Result;
}
internal s64
ParseInt(char* String, u64 Base = 10, u64* ParsedLength = 0)
{
  return ParseInt(LitString(String), Base, ParsedLength);
}

struct parse_float_result
{
  b8 Success;
  r64 Value;
  u64 ParsedLength;
};

internal parse_float_result
ValidateAndParseFloat(gs_const_string String)
{
  parse_float_result Result = {0};
  Result.Success = false;
  
  // Validate
  bool StringIsValid = true;
  for (u64 i = 0; i < String.Length; i++)
  {
    if (!IsNumericDecimal(String.Str[i]) && String.Str[i] != '-')
    {
      StringIsValid = false;
      break;
    }
  }
  
  if (StringIsValid)
  {
    s64 DecimalIndex = FindFirst(String, '.');
    u64 TempParsedLength = 0;
    u64 PlacesAfterPoint = 0;
    
    gs_const_string IntegerString = GetStringBefore(String, DecimalIndex);
    gs_const_string DecimalString = {};
    if (DecimalIndex >= 0)
    {
      DecimalString = GetStringAfter(String, DecimalIndex + 1);
    }
    
    r32 Polarity = 1;
    if (IntegerString.Str[0] == '-')
    {
      IntegerString = GetStringAfter(IntegerString, 1);
      Polarity = -1;
    }
    
    Result.Value = (r64)ParseInt(IntegerString, 10, &TempParsedLength);
    
    if (TempParsedLength == IntegerString.Length)
    {
      r64 AfterPoint = (r64)ParseUInt(DecimalString, 10, &PlacesAfterPoint);
      r64 Decimal = (AfterPoint / PowR64(10, PlacesAfterPoint));
      Result.Value = Result.Value + Decimal;
      Result.Value *= Polarity;
    }
    
    Result.ParsedLength = TempParsedLength + PlacesAfterPoint;
    if (DecimalIndex < (s64)String.Length) { Result.ParsedLength += 1; }
    
    Result.Success = true;
  }
  
  return Result;
}

internal r64
ParseFloat(gs_const_string String, u64* ParsedLength = 0)
{
  parse_float_result Result = ValidateAndParseFloat(String);
  Assert(Result.Success);
  if (ParsedLength != 0)
  {
    *ParsedLength = Result.ParsedLength;
  }
  return Result.Value;
}
internal r64
ParseFloat(char* String, u64* ParsedLength = 0)
{
  return ParseFloat(LitString(String), ParsedLength);
}

internal u64
AppendString(gs_string* Base, gs_const_string Appendix)
{
  u64 StartIndex = Base->Length;
  u64 LengthAvailable = Base->Size - Base->Length;
  u64 Written = 0;
  for (; Written < Min(LengthAvailable, Appendix.Length); Written++)
  {
    Base->Str[StartIndex + Written] = Appendix.Str[Written];
  }
  Base->Length += Written;
  Assert(Base->Length <= Base->Size);
  return Written;
}
internal u64
AppendString(gs_string* Base, gs_string Appendix)
{
  return AppendString(Base, Appendix.ConstString);
}

internal void
InsertAt(gs_string* Str, u64 Index, char C)
{
  if (Str->Length > Index)
  {
    for (u64 i = Str->Length; i > Index; i--)
    {
      Str->Str[i] = Str->Str[i - 1];
    }
  }
  
  if (Index < Str->Size)
  {
    Str->Str[Index] = C;
    Str->Length += 1;
    Assert(Str->Length < Str->Size);
  }
}

internal void
RemoveAt(gs_string* Str, u64 Index)
{
  if (Str->Length > 0 && Index < Str->Length)
  {
    for (u64 i = Index; i < Str->Length - 1; i++)
    {
      Str->Str[i] = Str->Str[i + 1];
    }
    Str->Length -= 1;
  }
}

internal void
NullTerminate(gs_string* String)
{
  if (String->Length < String->Size)
  {
    String->Str[String->Length] = 0;
  }
  else
  {
    String->Str[String->Length - 1] = 0;
  }
}

internal void
OutChar(gs_string* String, char C)
{
  if (String->Length < String->Size)
  {
    String->Str[String->Length++] = C;
  }
}

internal void
U64ToASCII(gs_string* String, u64 Value, u64 Base, gs_const_string Digits)
{
  u64 ValueRemaining = Value;
  u64 At = 0;
  do {
    u64 Index = ValueRemaining % Base;
    char Digit = Digits.Str[Index];
    OutChar(String, Digit);
    ValueRemaining /= Base;
  }while(ValueRemaining);
  char* End = String->Str + String->Length;
  ReverseStringInPlace(String);
}

internal void
U64ToASCII(gs_string* String, u64 Value, u64 Base)
{
  U64ToASCII(String, Value, Base, GetCharSetForBase(Base));
}

internal void
R64ToASCII(gs_string* String, r64 Value, u64 Precision)
{
  if (Value < 0)
  {
    OutChar(String, '-');
    Value = Abs(Value);
  }
  u64 IntegerPart = (u64)Value;
  // NOTE(Peter): If we don't use the inner string, when U64ToASCII reverses the characters
  // it'll put the negative sign at the end.
  gs_string IntegerString = GetStringAfter(*String, String->Length);
  U64ToASCII(&IntegerString, IntegerPart, 10);
  String->Length += IntegerString.Length;
  Value -= IntegerPart;
  if (Value > 0)
  {
    OutChar(String, '.');
    for (u64 i = 0; i < Precision; i++)
    {
      Value *= 10.0f;
      u64 DecimalPlace = (u64)Value;
      Value -= DecimalPlace;
      OutChar(String, Base10Chars.Str[DecimalPlace]);
    }
  }
}

internal s64
ReadVarArgsSignedInteger (s32 Width, va_list* Args)
{
  s64 Result = 0;
  switch (Width)
  {
    // NOTE(Peter): For Width lower than 4 bytes, the C++ spec specifies
    // that it will get promoted to an int anyways
    case 1: { Result = (s64)va_arg(*Args, s32); } break;
    case 2: { Result = (s64)va_arg(*Args, s32); } break;
    case 4: { Result = (s64)va_arg(*Args, s32); } break;
    case 8: { Result = (s64)va_arg(*Args, s64); } break;
    InvalidDefaultCase;
  }
  return Result;
}

internal r64
ReadVarArgsUnsignedInteger (s32 Width, va_list* Args)
{
  u64 Result = 0;
  switch (Width)
  {
    // NOTE(Peter): For Width lower than 4 bytes, the C++ spec specifies
    // that it will get promoted to an int anyways
    case 1: { Result = (u64)va_arg(*Args, u32); } break;
    case 2: { Result = (u64)va_arg(*Args, u32); } break;
    case 4: { Result = (u64)va_arg(*Args, u32); } break;
    case 8: { Result = (u64)va_arg(*Args, u64); } break;
    InvalidDefaultCase;
  }
  return Result;
}

internal r64
ReadVarArgsFloat (s32 Width, va_list* Args)
{
  r64 Result = 0;
  switch (Width)
  {
    case 4: { Result = (r64)va_arg(*Args, r64); } break;
    case 8: { Result = (r64)va_arg(*Args, r64); } break;
    InvalidDefaultCase;
  }
  return Result;
}

internal s32
PrintFArgsList (gs_string* String, char* Format, va_list Args)
{
  char* FormatAt = Format;
  while (*FormatAt)
  {
    if (FormatAt[0] != '%')
    {
      if (FormatAt[0] == '\\')
      {
        OutChar(String, *FormatAt++);
      }
      else
      {
        OutChar(String, *FormatAt++);
      }
    }
    else if (FormatAt[0] == '%' && FormatAt[1] == '%')  // Print the % symbol
    {
      OutChar(String, '%');
      FormatAt += 2;
    }
    else
    {
      FormatAt++;
      
      // Flags
      if (FormatAt[0] == '-')
      {
        FormatAt++;
      }
      else if (FormatAt[0] == '+')
      {
        FormatAt++;
      }
      else if (FormatAt[0] == ' ')
      {
        FormatAt++;
      }
      else if (FormatAt[0] == '#')
      {
        FormatAt++;
      }
      else if (FormatAt[0] == '0')
      {
        FormatAt++;
      }
      
      // Width
      b32 WidthSpecified = false;
      s32 Width = 0;
      
      if (IsBase10(FormatAt[0]))
      {
        WidthSpecified = true;
        u64 Parsed = 0;
        AssertMessage("ParseInt assumes whole string is an integer");
        Width = (s32)ParseInt(FormatAt, 10, &Parsed);
        FormatAt += Parsed;
      }
      else if (FormatAt[0] == '*')
      {
        WidthSpecified = true;
        Width = va_arg(Args, s32);
        Assert(Width >= 0);
        FormatAt++;
      }
      
      // Precision
      b32 PrecisionSpecified = false;
      s32 Precision = 0;
      
      if (FormatAt[0] == '.')
      {
        FormatAt++;
        if (IsBase10(FormatAt[0]))
        {
          PrecisionSpecified = true;
          
          gs_const_string PrecisionStr = {};
          PrecisionStr.Str = FormatAt;
          for (char* C = FormatAt; *FormatAt && IsBase10(*C); C++)
          {
            PrecisionStr.Length++;
          }
          u64 Parsed = 0;
          Precision = (s32)ParseInt(PrecisionStr, 10, &Parsed);
          FormatAt += Parsed;
        }
        else if (FormatAt[0] == '*')
        {
          PrecisionSpecified = true;
          Precision = va_arg(Args, s32);
          Assert(Precision >= 0);
          FormatAt++;
        }
      }
      
      // Length
      b32 LengthSpecified = false;
      s32 Length = 4;
      
      if (FormatAt[0] == 'h' && FormatAt[1] == 'h')
      {
        LengthSpecified = true;
        Length = 1;
        FormatAt += 2;
      }
      else if (FormatAt[0] == 'h')
      {
        LengthSpecified = true;
        Length = 2;
        FormatAt++;
      }
      else if (FormatAt[0] == 'l' && FormatAt[1] == 'l')
      {
        LengthSpecified = true;
        Length = 8;
        FormatAt += 2;
      }
      else if (FormatAt[0] == 'l')
      {
        LengthSpecified = true;
        Length = 4;
        FormatAt++;
      }
      else if (FormatAt[0] == 'j')
      {
        LengthSpecified = true;
        Length = 8;
        FormatAt++;
      }
      else if (FormatAt[0] == 'z')
      {
        FormatAt++;
      }
      else if (FormatAt[0] == 't')
      {
        FormatAt++;
      }
      else if (FormatAt[0] == 'L')
      {
        FormatAt++;
      }
      
      // Format Specifiers
      gs_string StringRemaining = GetStringAfter(*String, String->Length);
      Assert(StringRemaining.Length == 0);
      if (FormatAt[0] == 'd' || FormatAt[0] == 'i')
      {
        s64 SignedInt = ReadVarArgsSignedInteger(Length, &Args);
        if (SignedInt < 0)
        {
          OutChar(&StringRemaining, '-');
          SignedInt *= -1;
        }
        U64ToASCII(&StringRemaining, (u64)SignedInt, 10, Base10Chars);
      }
      else if (FormatAt[0] == 'u')
      {
        u64 UnsignedInt = ReadVarArgsUnsignedInteger(Length, &Args);
        U64ToASCII(&StringRemaining, UnsignedInt, 10, Base10Chars);
      }
      else if (FormatAt[0] == 'o')
      {
        u32 UnsignedInt = ReadVarArgsUnsignedInteger(Length, &Args);
        U64ToASCII(&StringRemaining, UnsignedInt, 8, Base8Chars);
      }
      else if (FormatAt[0] == 'x' || FormatAt[0] == 'X')
      {
        u32 UnsignedInt = ReadVarArgsUnsignedInteger(Length, &Args);
        U64ToASCII(&StringRemaining, UnsignedInt, 16, Base16Chars);
      }
      else if (FormatAt[0] == 'f' || FormatAt[0] == 'F')
      {
        r64 Float = ReadVarArgsFloat(Length, &Args);
        s32 AfterPoint = 6;
        if (PrecisionSpecified)
        {
          AfterPoint = Precision;
        }
        R64ToASCII(&StringRemaining, Float, AfterPoint);
      }
      else if (FormatAt[0] == 'c')
      {
        char InsertChar = va_arg(Args, s32);
        OutChar(&StringRemaining, InsertChar);
      }
      else if (FormatAt[0] == 's')
      {
        char* InsertString = va_arg(Args, char*);
        
        s32 InsertStringLength = CStringLength(InsertString);
        if (PrecisionSpecified)
        {
          InsertStringLength = Min(InsertStringLength, Precision);
        }
        InsertStringLength = Min(StringSizeLeft(StringRemaining), InsertStringLength);
        
        for (s32 c = 0; c < InsertStringLength; c++)
        {
          OutChar(&StringRemaining, InsertString[c]);
        }
      }
      else if (FormatAt[0] == 'S')
      {
        gs_const_string InsertString = va_arg(Args, gs_const_string);
        
        for (s32 c = 0; c < InsertString.Length; c++)
        {
          OutChar(&StringRemaining, InsertString.Str[c]);
        }
      }
      else if (FormatAt[0] == 'p')
      {
        // TODO(Peter): Pointer Address
      }
      else
      {
        // NOTE(Peter): Non-specifier character found
        InvalidCodePath;
      }
      
      String->Length += StringRemaining.Length;
      FormatAt++;
    }
  }
  
  return String->Length;
}

internal void
PrintF (gs_string* String, char* Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  String->Length = 0;
  PrintFArgsList(String, Format, Args);
  va_end(Args);
}
internal void
PrintF (gs_string* String, const char* Format, ...)
{
  // NOTE(Peter): This variant is here for clang/gcc - C++ spec doesn't allow
  // implicit conversion from a const char* (a static c string) to char*, so this
  // version of the function just provides the conversion so the compiler will be quiet
  // without removing the other implementation, which is more useful
  va_list Args;
  va_start(Args, Format);
  String->Length = 0;
  PrintFArgsList(String, (char*)Format, Args);
  va_end(Args);
}

internal void
AppendPrintF (gs_string* String, char* Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  PrintFArgsList(String, Format, Args);
  va_end(Args);
}
internal void
AppendPrintF (gs_string* String, const char* Format, ...)
{
  // NOTE(Peter): This variant is here for clang/gcc - C++ spec doesn't allow
  // implicit conversion from a const char* (a static c string) to char*, so this
  // version of the function just provides the conversion so the compiler will be quiet
  // without removing the other implementation, which is more useful
  va_list Args;
  va_start(Args, Format);
  PrintFArgsList(String, (char*)Format, Args);
  va_end(Args);
}

///////////////////////////
//
// Memory

internal gs_data
CreateData(u8* Memory, u64 Size)
{
  gs_data Result = {Memory, Size};
  return Result;
}
internal bool
DataIsNonEmpty(gs_data Data)
{
  return ((Data.Size > 0) && (Data.Memory != 0));
}

#define PushStringF(a,l,f,...) PushStringF_((a),(l),(f), DEBUG_LOC, __VA_ARGS__)
internal gs_string
PushStringF_(gs_memory_arena* Arena, u32 MaxLength, char* Format, gs_debug_loc Loc, ...)
{
  gs_string Result = gs_string { 
    (char*)PushSize_(Arena, sizeof(char) * MaxLength, Loc).Memory, // Str
    0,  // Length
    MaxLength, // Size
  };
  
  va_list Args;
  va_start(Args, Loc);
  PrintFArgsList(&Result, Format, Args);
  va_end(Args);
  
  return Result;
}

internal gs_string
PushStringCopy(gs_memory_arena* Arena, gs_const_string String)
{
  gs_string Result = PushString(Arena, String.Length);
  Result.Size = String.Length;
  Result.Length = String.Length;
  for (u32 i = 0; i < String.Length; i++)
  {
    Result.Str[i] = String.Str[i];
  }
  return Result;
}

///////////////////////////
//
// Debug Print

inline void
DebugPrint(debug_output Output, gs_const_string Message)
{
  Output.Print(Output, Message);
}

inline void
DebugPrint(debug_output Output, char* Message)
{
  gs_const_string String = ConstString(Message);
  Output.Print(Output, String);
}

internal void
DebugPrintF(debug_output Output, char* Format, ...)
{
  gs_string Message = PushString(Output.Storage, 1024);
  va_list Args;
  va_start(Args, Format);
  PrintFArgsList(&Message, Format, Args);
  NullTerminate(&Message);
  Output.Print(Output, Message.ConstString);
}

#define HandlesAreEqual(ha, hb) ((ha.IndexInBuffer == hb.IndexInBuffer) && (ha.BufferIndex == hb.BufferIndex))

///////////////////////////
//
// String Builder

internal void
GrowStringBuilder_(gs_string_builder* StringBuilder)
{
  gs_string_builder_buffer* NewBuffer = PushStruct(StringBuilder->Arena, gs_string_builder_buffer);
  NewBuffer->String = PushString(StringBuilder->Arena, StringBuilder->BufferSize);
  SLLPushOrInit(StringBuilder->Root, StringBuilder->Head, NewBuffer);
}

internal void
OutChar(gs_string_builder* Builder, char C)
{
  if (Builder->Head == 0 || Builder->Head->String.Length >= Builder->Head->String.Size)
  {
    GrowStringBuilder_(Builder);
  }
  OutChar(&Builder->Head->String, C);
}

#if 0
// TODO: If you need a string builder come back here, otherwise this can stay 0'd out
// was getting in the way
internal void
StringBuilderWriteFArgsList(gs_string_builder* Builder, char* Format, va_list Args)
{
  char* FormatAt = Format;
  while (*FormatAt)
  {
    if (FormatAt[0] != '%')
    {
      if (FormatAt[0] == '\\')
      {
        FormatAt++;
        Assert(IsBase8(FormatAt[0]) || // Octal Escape Sequences - \0 is in this set
               FormatAt[0] == '\'' ||
               FormatAt[0] == '\"' ||
               FormatAt[0] == '\?' ||
               FormatAt[0] == '\\' ||
               FormatAt[0] == 'a' || // Audible Bell
               FormatAt[0] == 'b' || // Backspace
               FormatAt[0] == 'f' || // Form Feed - New Page
               FormatAt[0] == 'n' || // Line Feed - New Line
               FormatAt[0] == 'r' || // Carriage Return
               FormatAt[0] == 't' || // Tab
               FormatAt[0] == 'v');  // Vertical Tab
        
        // Not Handled (see cpp spec) \nnn \xnn \unnnn \Unnnnnnnn
        Assert(FormatAt[0] != 'x' || FormatAt[0] != 'u' || FormatAt[0] != 'U');
        
        if (IsBase8(FormatAt[0]))
        {
          // TODO(Peter): this should keep going until it finds a non-octal character code
          // but the only one we really need is \0 atm so I'm just handling that one
          Assert(FormatAt[0] == '0');
          OutChar(Builder, (char)0);
          FormatAt++;
        }
        else
        {
          OutChar(Builder, *FormatAt++);
        }
      }
      else
      {
        OutChar(Builder, *FormatAt++);
      }
    }
    else if (FormatAt[0] == '%' && FormatAt[1] == '%')  // Print the % symbol
    {
      OutChar(Builder, '%');
      FormatAt += 2;
    }
    else
    {
      FormatAt++;
      
      // Flags
      if (FormatAt[0] == '-')
      {
        FormatAt++;
      }
      else if (FormatAt[0] == '+')
      {
        FormatAt++;
      }
      else if (FormatAt[0] == ' ')
      {
        FormatAt++;
      }
      else if (FormatAt[0] == '#')
      {
        FormatAt++;
      }
      else if (FormatAt[0] == '0')
      {
        FormatAt++;
      }
      
      // Width
      b32 WidthSpecified = false;
      s32 Width = 0;
      
      if (IsBase10(FormatAt[0]))
      {
        WidthSpecified = true;
        u64 Parsed = 0;
        AssertMessage("ParseInt assumes whole string is an integer");
        Width = (s32)ParseInt(FormatAt, 10, &Parsed);
        FormatAt += Parsed;
      }
      else if (FormatAt[0] == '*')
      {
        WidthSpecified = true;
        Width = va_arg(Args, s32);
        Assert(Width >= 0);
        FormatAt++;
      }
      
      // Precision
      b32 PrecisionSpecified = false;
      s32 Precision = 0;
      
      if (FormatAt[0] == '.')
      {
        FormatAt++;
        if (IsBase10(FormatAt[0]))
        {
          
          PrecisionSpecified = true;
          u64 Parsed = 0;
          AssertMessage("ParseInt assumes whole string is an integer");
          Precision = (s32)ParseInt(FormatAt, 10, &Parsed);
          FormatAt += Parsed;
        }
        else if (FormatAt[0] == '*')
        {
          PrecisionSpecified = true;
          Precision = va_arg(Args, s32);
          Assert(Precision >= 0);
          FormatAt++;
        }
      }
      
      // Length
      b32 LengthSpecified = false;
      s32 Length = 4;
      
      if (FormatAt[0] == 'h' && FormatAt[1] == 'h')
      {
        LengthSpecified = true;
        LengthSpecified = 1;
        FormatAt += 2;
      }
      else if (FormatAt[0] == 'h')
      {
        LengthSpecified = true;
        LengthSpecified = 2;
        FormatAt++;
      }
      else if (FormatAt[0] == 'l' && FormatAt[1] == 'l')
      {
        LengthSpecified = true;
        LengthSpecified = 8;
        FormatAt += 2;
      }
      else if (FormatAt[0] == 'l')
      {
        LengthSpecified = true;
        LengthSpecified = 4;
        FormatAt++;
      }
      else if (FormatAt[0] == 'j')
      {
        LengthSpecified = true;
        LengthSpecified = 8;
        FormatAt++;
      }
      else if (FormatAt[0] == 'z')
      {
        FormatAt++;
      }
      else if (FormatAt[0] == 't')
      {
        FormatAt++;
      }
      else if (FormatAt[0] == 'L')
      {
        FormatAt++;
      }
      
      // Format Specifiers
      gs_string StringRemaining = GetStringAfter(*String, String->Length);
      Assert(StringRemaining.Length == 0);
      if (FormatAt[0] == 'd' || FormatAt[0] == 'i')
      {
        s64 SignedInt = ReadVarArgsSignedInteger(Length, &Args);
        if (SignedInt < 0)
        {
          OutChar(&StringRemaining, '-');
          SignedInt *= -1;
        }
        U64ToASCII(&StringRemaining, (u64)SignedInt, 10, Base10Chars);
      }
      else if (FormatAt[0] == 'u')
      {
        u32 UnsignedInt = ReadVarArgsUnsignedInteger(Length, &Args);
        U64ToASCII(&StringRemaining, UnsignedInt, 10, Base10Chars);
      }
      else if (FormatAt[0] == 'o')
      {
        u32 UnsignedInt = ReadVarArgsUnsignedInteger(Length, &Args);
        U64ToASCII(&StringRemaining, UnsignedInt, 8, Base8Chars);
      }
      else if (FormatAt[0] == 'x' || FormatAt[0] == 'X')
      {
        u32 UnsignedInt = ReadVarArgsUnsignedInteger(Length, &Args);
        U64ToASCII(&StringRemaining, UnsignedInt, 16, Base16Chars);
      }
      else if (FormatAt[0] == 'f' || FormatAt[0] == 'F')
      {
        r64 Float = ReadVarArgsFloat(Length, &Args);
        s32 AfterPoint = 6;
        if (PrecisionSpecified)
        {
          AfterPoint = Precision;
        }
        R64ToASCII(&StringRemaining, Float, AfterPoint);
      }
      else if (FormatAt[0] == 'c')
      {
        char InsertChar = va_arg(Args, s32);
        OutChar(&StringRemaining, InsertChar);
      }
      else if (FormatAt[0] == 's')
      {
        char* InsertString = va_arg(Args, char*);
        
        s32 InsertStringLength = CStringLength(InsertString);
        if (PrecisionSpecified)
        {
          InsertStringLength = Min(InsertStringLength, Precision);
        }
        InsertStringLength = Min(StringSizeLeft(StringRemaining), InsertStringLength);
        
        for (s32 c = 0; c < InsertStringLength; c++)
        {
          OutChar(&StringRemaining, InsertString[c]);
        }
      }
      else if (FormatAt[0] == 'S')
      {
        gs_const_string InsertString = va_arg(Args, gs_const_string);
        
        for (s32 c = 0; c < InsertString.Length; c++)
        {
          OutChar(&StringRemaining, InsertString.Str[c]);
        }
      }
      else if (FormatAt[0] == 'p')
      {
        // TODO(Peter): Pointer Address
      }
      else
      {
        // NOTE(Peter): Non-specifier character found
        InvalidCodePath;
      }
      
      String->Length += StringRemaining.Length;
      FormatAt++;
    }
  }
  
  return String->Length;
}

internal void
StringBuilderWriteF(gs_string_builder* Builder, char* Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  StringBuilderWriteFArgsList(Builder, Format, Args);
  va_end(Args);
}

#endif // String builder

///////////////////////////
//
// File Handler

internal u64
FileHandlerGetFileInfo_NoOp(gs_file_handler FileHandler, gs_const_string Path)
{
  return 0;
}

internal gs_file
FileHandlerReadFile_NoOp(gs_const_string Path)
{
  return gs_file{0};
}

internal bool
FileHandlerWriteFile_NoOp(gs_const_string Path, gs_data Data)
{
  return false;
}

internal gs_const_string_array
FileHandlerEnumerateDirectory_NoOp(gs_const_string Path, bool Recursive, bool IncludeDirs)
{
  return gs_const_string_array{0};
}

internal gs_file_handler
CreateFileHandler(file_handler_get_file_info* GetFileInfo,
                  file_handler_read_entire_file* ReadEntireFile,
                  file_handler_write_entire_file* WriteEntireFile,
                  file_handler_enumerate_directory* EnumerateDirectory,
                  gs_memory_arena* Transient)
{
  if (GetFileInfo == 0)
  {
    GetFileInfo = (file_handler_get_file_info*)FileHandlerGetFileInfo_NoOp;
  }
  if (ReadEntireFile == 0)
  {
    ReadEntireFile = (file_handler_read_entire_file*)FileHandlerReadFile_NoOp;
  }
  if (WriteEntireFile == 0)
  {
    WriteEntireFile = (file_handler_write_entire_file*)FileHandlerWriteFile_NoOp;
  }
  if (EnumerateDirectory == 0)
  {
    EnumerateDirectory = (file_handler_enumerate_directory*)FileHandlerEnumerateDirectory_NoOp;
  }
  gs_file_handler Result = {0};
  Result.GetFileInfo = GetFileInfo;
  Result.ReadEntireFile = ReadEntireFile;
  Result.WriteEntireFile = WriteEntireFile;
  Result.EnumerateDirectory = EnumerateDirectory;
  Result.Transient = Transient;
  
  return Result;
}

internal gs_const_string
GetNullTerminatedPath(gs_file_handler FileHandler, gs_const_string Path)
{
  gs_const_string Result = {};
  if (!IsNullTerminated(Path))
  {
    gs_string NullTermPath = PushString(FileHandler.Transient, Path.Length + 1);
    PrintF(&NullTermPath, "%S", Path);
    NullTerminate(&NullTermPath);
    Result = NullTermPath.ConstString;
  }
  else
  {
    Result = Path;
  }
  return Result;
}

internal gs_file_info
GetFileInfo(gs_file_handler FileHandler, gs_const_string Path)
{
  Assert(FileHandler.GetFileInfo != 0);
  
  Path = GetNullTerminatedPath(FileHandler, Path);
  gs_file_info Result = FileHandler.GetFileInfo(FileHandler, Path);
  return Result;
}

internal gs_file
ReadEntireFile(gs_file_handler FileHandler, gs_const_string Path, gs_data Memory)
{
  Assert(FileHandler.ReadEntireFile != 0);
  
  Path = GetNullTerminatedPath(FileHandler, Path);
  gs_file Result = FileHandler.ReadEntireFile(FileHandler, Path, Memory);
  return Result;
}

internal gs_file
ReadEntireFile(gs_file_handler FileHandler, gs_const_string Path)
{
  Assert(FileHandler.GetFileInfo != 0);
  Assert(FileHandler.ReadEntireFile != 0);
  
  Path = GetNullTerminatedPath(FileHandler, Path);
  gs_file Result = {0};
  gs_file_info FileInfo = FileHandler.GetFileInfo(FileHandler, Path);
  if (FileInfo.FileSize > 0)
  {
    gs_data FileMemory = PushSize(FileHandler.Transient, FileInfo.FileSize);
    Result = ReadEntireFile(FileHandler, Path, FileMemory);
  }
  return Result;
}

internal bool
WriteEntireFile(gs_file_handler FileHandler, gs_const_string Path, gs_data Memory)
{
  Assert(FileHandler.WriteEntireFile != 0);
  
  Path = GetNullTerminatedPath(FileHandler, Path);
  return FileHandler.WriteEntireFile(FileHandler, Path, Memory);
}

internal gs_file_info_array
EnumerateDirectory(gs_file_handler FileHandler, gs_memory_arena* Storage, gs_const_string Path, u32 Flags)
{
  Assert(FileHandler.EnumerateDirectory != 0);
  
  Path = GetNullTerminatedPath(FileHandler, Path);
  return FileHandler.EnumerateDirectory(FileHandler, Storage, Path, Flags);
}

internal bool
FileNoError(gs_file File)
{
  bool Result = (File.Size > 0);
  return Result;
}

//////////////////////////
//
// Timing

internal s64
TimeHandlerGetWallClock(gs_time_handler TimeHandler)
{
  s64 Result = TimeHandler.GetWallClock();
  return Result;
}

internal s64
TimeHandlerGetSecondsElapsed(gs_time_handler TimeHandler, s64 StartCycles, s64 EndCycles)
{
  s64 Result = TimeHandler.GetSecondsElapsed(StartCycles, EndCycles);
  return Result;
}

//////////////////////////
//
// Thread Manager

CREATE_THREAD(CreateThreadStub)
{
  return {};
}


KILL_THREAD(KillThreadStub)
{
  return false;
}

internal platform_thread_manager
CreatePlatformThreadManager(platform_create_thread* CreateThreadProc,
                            platform_kill_thread* KillThreadProc)
{
  platform_thread_manager Result = {};
  Result.CreateThreadProc = CreateThreadProc;
  Result.KillThreadProc = KillThreadProc;
  
  if (!CreateThreadProc)
  {
    Result.CreateThreadProc = CreateThreadStub;
  }
  if (!KillThreadProc)
  {
    Result.KillThreadProc = KillThreadStub;
  }
  
  return Result;
}

internal platform_thread_handle
CreateThread(platform_thread_manager* Manager, thread_proc_* Proc, u8* Arg, gs_thread_context Ctx)
{
  platform_thread_handle Result = {};
  
  for (u32 i = 1; i < THREADS_MAX; i++)
  {
    if (!Manager->ThreadsUsed[i])
    {
      Result.Index = i;
      break;
    }
  }
  Assert(Result.Index != 0);
  
  Manager->ThreadsUsed[Result.Index] = true;
  Manager->CreateThreadProc(&Manager->Threads[Result.Index], Proc, Arg, Ctx);
  
  return Result;
}

internal bool
KillThread(platform_thread_manager* Manager, platform_thread_handle Handle)
{
  Assert(Manager->ThreadsUsed[Handle.Index]);
  
  platform_thread* Thread = &Manager->Threads[Handle.Index];
  bool Result = Manager->KillThreadProc(Thread);
  
  if (Result)
  {
    Manager->ThreadsUsed[Handle.Index] = false;
    Manager->Threads[Handle.Index] = {};
  }
  
  return Result;
}


//////////////////////////
//
// Socket Manager

CONNECT_SOCKET(PlatformConnectSocket_Stub)
{
  return false;
}

CLOSE_SOCKET(PlatformCloseSocket_Stub)
{
  return false;
}

SOCKET_QUERY_STATUS(PlatformSocketQueryStatus_Stub)
{
  return false;
}

SOCKET_PEEK(PlatformSocketPeek_Stub)
{
  return 0;
}

SOCKET_RECEIVE(PlatformSocketRecieve_Stub)
{
  return {};
}

SOCKET_SEND(PlatformSocketSend_Stub)
{
  return false;
}

internal platform_socket_manager
CreatePlatformSocketManager(platform_connect_socket* ConnectSocketProc,
                            platform_close_socket* CloseSocketProc,
                            platform_socket_query_status* SocketQueryStatusProc,
                            platform_socket_peek* SocketPeekProc,
                            platform_socket_receive* SocketRecieveProc,
                            platform_socket_send* SocketSendProc)
{
  platform_socket_manager Result = {};
  Result.ConnectSocketProc = ConnectSocketProc;
  Result.CloseSocketProc = CloseSocketProc;
  Result.SocketQueryStatusProc = SocketQueryStatusProc;
  Result.SocketPeekProc = SocketPeekProc;
  Result.SocketRecieveProc = SocketRecieveProc;
  Result.SocketSendProc = SocketSendProc;
  
  if (!ConnectSocketProc)
  {
    Result.ConnectSocketProc = PlatformConnectSocket_Stub;
  }
  if (!CloseSocketProc)
  {
    Result.CloseSocketProc = PlatformCloseSocket_Stub;
  }
  if (!SocketQueryStatusProc)
  {
    Result.SocketQueryStatusProc = PlatformSocketQueryStatus_Stub;
  }
  if (!SocketPeekProc)
  {
    Result.SocketPeekProc = PlatformSocketPeek_Stub;
  }
  if (!SocketRecieveProc)
  {
    Result.SocketRecieveProc = PlatformSocketRecieve_Stub;
  }
  if (!SocketSendProc)
  {
    Result.SocketSendProc = PlatformSocketSend_Stub;
  }
  return Result;
}

internal bool
SocketHandleIsValid(platform_socket_handle_ Handle)
{
  return Handle.Index != 0;
}

internal platform_socket*
SocketManagerGetSocket(platform_socket_manager* Manager, platform_socket_handle_ Handle)
{
  platform_socket* Result = 0;
  if (Manager->SocketsUsed[Handle.Index])
  {
    platform_socket* Socket = &Manager->Sockets[Handle.Index];
    if (Socket->PlatformHandle != 0)
    {
      Result = Socket;
    }
  }
  return Result;
}

internal bool
ConnectSocket(platform_socket_manager* Manager, platform_socket_handle_ Handle)
{
  bool Result = false;
  platform_socket* Socket = SocketManagerGetSocket(Manager, Handle);
  if (Socket)
  {
    Result = Manager->ConnectSocketProc(Manager, Socket);
  }
  return Result;
}

internal bool
RemoveSocket (platform_socket_manager* Manager, platform_socket_handle_ Handle)
{
  bool Result = Manager->SocketsUsed[Handle.Index];
  Manager->SocketsUsed[Handle.Index] = false;
  return Result;
}

internal platform_socket_handle_
CreateSocket(platform_socket_manager* Manager, char* Addr, char* Port)
{
  platform_socket_handle_ Result = {};
  for (u32 i = 1; i < SOCKETS_COUNT_MAX; i++)
  {
    if (!Manager->SocketsUsed[i])
    {
      Result.Index = i;
      Manager->SocketsUsed[i] = true;
      break;
    }
  }
  
  Assert(Result.Index != 0);
  platform_socket* Socket = &Manager->Sockets[Result.Index];
  Socket->Handle = Result;
  CopyArray(Addr, Socket->Addr, char, CStringLength(Addr) + 1);
  CopyArray(Port, Socket->Port, char, CStringLength(Port) + 1);
  
  bool Success = Manager->ConnectSocketProc(Manager, Socket);
  if (!Success)
  {
    if (RemoveSocket(Manager, Result))
    {
      Result = {};
    }
    else
    {
      InvalidCodePath;
    }
  }
  
  return Result;
}

internal bool
CloseSocket(platform_socket_manager* Manager, platform_socket* Socket)
{
  bool Result = false;
  if (Socket)
  {
    if (Manager->CloseSocketProc(Manager, Socket))
    {
      RemoveSocket(Manager, Socket->Handle);
      *Socket = {};
      Result = true;
    }
    else
    {
      InvalidCodePath;
    }
  }
  return Result;
}

internal bool
CloseSocket(platform_socket_manager* Manager, platform_socket_handle_ Handle)
{
  bool Result = false;
  platform_socket* Socket = SocketManagerGetSocket(Manager, Handle);
  return CloseSocket(Manager, Socket);
}

// NOTE(pjs): returns true if the socket is connected
internal bool
SocketQueryStatus(platform_socket_manager* Manager, platform_socket_handle_ SocketHandle)
{
  bool Result = false;
  platform_socket* Socket = SocketManagerGetSocket(Manager, SocketHandle);
  if (Socket)
  {
    Result = Manager->SocketQueryStatusProc(Manager, Socket);
  }
  return Result;
}

internal u32
SocketPeek(platform_socket_manager* Manager, platform_socket_handle_ SocketHandle)
{
  u32 Result = 0;
  platform_socket* Socket = SocketManagerGetSocket(Manager, SocketHandle);
  if (Socket)
  {
    Result = Manager->SocketPeekProc(Manager, Socket);
  }
  return Result;
}

internal gs_data
SocketRecieve(platform_socket_manager* Manager, platform_socket_handle_ SocketHandle, gs_memory_arena* Storage)
{
  gs_data Result = {};
  platform_socket* Socket = SocketManagerGetSocket(Manager, SocketHandle);
  if (Socket)
  {
    Result = Manager->SocketRecieveProc(Manager, Socket, Storage);
  }
  return Result;
}

internal bool
SocketSend(platform_socket_manager* Manager, platform_socket_handle_ SocketHandle, u32 Address, u32 Port, gs_data Data, s32 Flags)
{
  bool Result = false;
  platform_socket* Socket = SocketManagerGetSocket(Manager, SocketHandle);
  if (Socket)
  {
    s32 SizeSent = Manager->SocketSendProc(Manager, Socket, Address, Port, Data, Flags);
    Result = (SizeSent == Data.Size);
  }
  return Result;
}

///////////////////////////
//
// Hashes

internal u32
HashAppendDJB2ToU32(u32 Hash, u8 Byte)
{
  u32 Result = Hash;
  if (Result == 0) { Result = 5381; }
  Result = ((Result << 5) + Result) + Byte;
  return Result;
}

internal u64
HashAppendDJB2ToU32(u64 Hash, u8 Byte)
{
  u64 Result = Hash;
  if (Result == 0) { Result = 5381; }
  Result = ((Result << 5) + Result) + Byte;
  return Result;
}

internal u32
HashDJB2ToU32(char* String)
{
  u32 Hash = 5381;
  char* C = String;
  while(*C)
  {
    Hash = ((Hash << 5) + Hash) + *C++;
  }
  return Hash;
}
internal u32
HashDJB2ToU32(u32 Length, char* String)
{
  u32 Hash = 5381;
  for (u32 i = 0; i < Length; i++)
  {
    Hash = ((Hash << 5) + Hash) + String[i];
  }
  return Hash;
}
internal u32
HashDJB2ToU32(gs_const_string Str)
{
  return HashDJB2ToU32(StringExpand(Str));
}
internal u32
HashDJB2ToU32(gs_string Str)
{
  return HashDJB2ToU32(StringExpand(Str));
}

internal u64
HashDJB2ToU64(char* String)
{
  u64 Hash = 5381;
  char* C = String;
  while(*C)
  {
    Hash = ((Hash << 5) + Hash) + *C++;
  }
  return Hash;
}
internal u64
HashDJB2ToU64(u32 Length, char* String)
{
  u64 Hash = 5381;
  for (u32 i = 0; i < Length; i++)
  {
    Hash = ((Hash << 5) + Hash) + String[i];
  }
  return Hash;
}
internal u64
HashDJB2ToU64(gs_const_string Str)
{
  return HashDJB2ToU64(StringExpand(Str));
}
internal u64
HashDJB2ToU64(gs_string Str)
{
  return HashDJB2ToU64(StringExpand(Str));
}

///////////////////////////
//
// Random Series

internal gs_random_series
InitRandomSeries(u32 Seed)
{
  gs_random_series Result = {0};
  Result.Value = Seed;
  return Result;
}

internal u32
NextRandom(gs_random_series* Series)
{
  u32 Result = Series->Value;
  Result ^= Result << 13;
  Result ^= Result >> 17;
  Result ^= Result << 5;
  Series->Value = Result;
  return Result;
}

internal r32
NextRandomUnilateral(gs_random_series* Series)
{
  r32 Result = (r32)NextRandom(Series) / (r32)UINT32_MAX;
  return Result;
}

internal r32
NextRandomBilateral(gs_random_series* Series)
{
  r32 Result = (r32)NextRandom(Series);
  Result = Result / (r32)0xFFFFFFFF;
  Result = (Result * 2.0f) - 1.0f;
  return Result;
}


///////////////////////////
//
// Sort


static void
RadixSortInPlace_ (gs_radix_list* List, u32 Start, u32 End, u32 Iteration)
{
  u32 Shift = Iteration;
  u32 ZerosBoundary = Start;
  u32 OnesBoundary = End - 1;
  
  for (u32 d = Start; d < End; d++)
  {
    u64 CurrentIndex = ZerosBoundary;
    u64 Radix = List->Radixes.Values[CurrentIndex];
    u64 Place = (Radix >> Shift) & 0x1;
    if (Place)
    {
      u64 EvictedIndex = OnesBoundary;
      u64 EvictedRadix = List->Radixes.Values[EvictedIndex];
      u64 EvictedID = List->IDs.Values[EvictedIndex];
      
      List->Radixes.Values[EvictedIndex] = Radix;
      List->IDs.Values[EvictedIndex] = List->IDs.Values[CurrentIndex];
      
      List->Radixes.Values[CurrentIndex] = EvictedRadix;
      List->IDs.Values[CurrentIndex] = EvictedID;
      
      OnesBoundary -= 1;
    }
    else
    {
      ZerosBoundary += 1;
    }
  }
  
  if (Iteration > 0)
  {
    RadixSortInPlace_(List, Start, ZerosBoundary, Iteration - 1);
    RadixSortInPlace_(List, ZerosBoundary, End, Iteration - 1);
  }
}

static void
RadixSortInPlace (gs_radix_list* List)
{
  u32 Highest = 0;
  for (u32 i = 0; i < List->Radixes.Count; i++)
  {
    if (List->Radixes.Values[i] > Highest)
    {
      Highest = List->Radixes.Values[i];
    }
  }
  
  u32 Iterations = 0;
  while (Highest > 1)
  {
    ++Iterations;
    Highest = Highest >> 1;
  }
  
  RadixSortInPlace_(List, 0, List->Radixes.Count, Iterations);
}


///////////////////////////
//
// Input

inline bool
KeyIsMouseButton(gs_key Key)
{
  bool Result = (Key >= gs_Key_MouseLeftButton);
  Result = Result && Key <= gs_Key_MouseRightButton;
  return Result;
}
inline u32
GetMouseButtonIndex(gs_key Button)
{
  Assert(KeyIsMouseButton(Button));
  u32 Result = Button - gs_Key_MouseLeftButton;
  return Result;
}
inline bool
MouseButtonTransitionedDown(gs_mouse_state Mouse, u32 Index)
{
  bool IsDown = (Mouse.ButtonStates[Index] & (1 << MouseButton_IsDownBit)) != 0;
  bool WasDown = (Mouse.ButtonStates[Index] & (1 << MouseButton_WasDownBit)) != 0;
  return IsDown && !WasDown;
}
inline bool
MouseButtonTransitionedDown(gs_mouse_state Mouse, gs_key Button)
{
  u32 Index = GetMouseButtonIndex(Button);
  return MouseButtonTransitionedDown(Mouse, Index);
}
inline bool
MouseButtonIsDown(gs_mouse_state Mouse, u32 Index)
{
  bool IsDown = (Mouse.ButtonStates[Index] & (1 << MouseButton_IsDownBit)) != 0;
  return IsDown;
}
inline bool
MouseButtonIsDown(gs_mouse_state Mouse, gs_key Button)
{
  u32 Index = GetMouseButtonIndex(Button);
  return MouseButtonIsDown(Mouse, Index);
}
inline bool
MouseButtonTransitionedUp(gs_mouse_state Mouse, u32 Index)
{
  bool IsDown = (Mouse.ButtonStates[Index] & (1 << MouseButton_IsDownBit)) != 0;
  bool WasDown = (Mouse.ButtonStates[Index] & (1 << MouseButton_WasDownBit)) != 0;
  return !IsDown && WasDown;
}
inline bool
MouseButtonTransitionedUp(gs_mouse_state Mouse, gs_key Button)
{
  u32 Index = GetMouseButtonIndex(Button);
  return MouseButtonTransitionedUp(Mouse, Index);
}
inline bool
MouseButtonIsUp(gs_mouse_state Mouse, u32 Index)
{
  bool IsDown = (Mouse.ButtonStates[Index] & (1 << MouseButton_IsDownBit)) != 0;
  return !IsDown;
}
inline bool
MouseButtonIsUp(gs_mouse_state Mouse, gs_key Button)
{
  u32 Index = GetMouseButtonIndex(Button);
  return MouseButtonIsUp(Mouse, Index);
}
internal void
SetMouseButtonTransitionedDown(gs_mouse_state* Mouse, gs_key Button)
{
  u32 Index = GetMouseButtonIndex(Button);
  
  Mouse->ButtonStates[Index] = 0;
  Mouse->ButtonStates[Index] |= MouseButton_IsDown << MouseButton_IsDownBit;
  Mouse->ButtonStates[Index] |= MouseButton_WasNotDown << MouseButton_WasDownBit;
}
internal void
SetMouseButtonTransitionedUp(gs_mouse_state* Mouse, gs_key Button)
{
  u32 Index = GetMouseButtonIndex(Button);
  
  Mouse->ButtonStates[Index] = 0;
  Mouse->ButtonStates[Index] |= MouseButton_IsNotDown << MouseButton_IsDownBit;
  Mouse->ButtonStates[Index] |= MouseButton_WasDown << MouseButton_WasDownBit;
}
internal void
AdvanceMouseButtonState(gs_mouse_state* Mouse, gs_key Button)
{
  u32 Index = GetMouseButtonIndex(Button);
  
  if (MouseButtonIsDown(*Mouse, Index))
  {
    Mouse->ButtonStates[Index] |= MouseButton_WasDown << MouseButton_WasDownBit;
  }
  else
  {
    Mouse->ButtonStates[Index] &= MouseButton_WasNotDown << MouseButton_WasDownBit;
  }
}
internal void
AdvanceMouseButtonsState(gs_mouse_state* Mouse)
{
  AdvanceMouseButtonState(Mouse, gs_Key_MouseLeftButton);
  AdvanceMouseButtonState(Mouse, gs_Key_MouseMiddleButton);
  AdvanceMouseButtonState(Mouse, gs_Key_MouseRightButton);
}

///////////////////////////
//
// Network


static u32
HostToNetU32(u32 In)
{
  unsigned char *s = (unsigned char *)&In;
  u32 Result = (u32)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
  return Result;
}

static u16
HostToNetU16(u16 In)
{
  unsigned char *s = (unsigned char *)&In;
  u16 Result = (u16)(s[0] << 8 | s[1]);
  return Result;
}










#define GS_TYPES_CPP
#endif // GS_TYPES_CPP