#ifndef PATTERNS_MATH_H
#define PATTERNS_MATH_H

r32 fractf(r32 v) { r64 int_part = 0; return (r32)modf((r64)v, &int_part); }

r32
pm_smoothstep_r32(r32 t)
{
  r32 r = (t * t * (3 - (2 * t)));
  return r;
}

r32
pm_smoothstep_range_r32(r32 t, r32 min, r32 max)
{
  r32 tt = pm_smoothstep_r32(t);
  r32 r = lerp(min, tt, max);
  return r;
}

v3
pm_smoothstep_v3(v3 p)
{
  v3 result = {
    .x = pm_smoothstep_r32(p.x),
    .y = pm_smoothstep_r32(p.y),
    .z = pm_smoothstep_r32(p.z),
  };
  return result;
}

///// vector extensions

v2 pm_abs_v2(v2 v) { return (v2){ .x = fabsf(v.x), .y = fabsf(v.y) }; }
v3 pm_abs_v3(v3 v) { return (v3){ .x = fabsf(v.x), .y = fabsf(v.y), .z = fabsf(v.z) }; }
v4 pm_abs_v4(v4 v) { return (v4){ .x = fabsf(v.x), .y = fabsf(v.y), .z = fabsf(v.z), .w = fabsf(v.w) }; }

v2 pm_floor_v2(v2 v) { return (v2){ .x = floorf(v.x), .y = floorf(v.y) }; }
v3 pm_floor_v3(v3 v) { return (v3){ .x = floorf(v.x), .y = floorf(v.y), .z = floorf(v.z) }; }
v4 pm_floor_v4(v4 v) { return (v4){ .x = floorf(v.x), .y = floorf(v.y), .z = floorf(v.z), .w = floorf(v.w) }; }

v2 pm_fract_v2(v2 v) { return (v2){ .x = fractf(v.x), .y = fractf(v.y) }; }
v3 pm_fract_v3(v3 v) { return (v3){ .x = fractf(v.x), .y = fractf(v.y), .z = fractf(v.z) }; }
v4 pm_fract_v4(v4 v) { return (v4){ .x = fractf(v.x), .y = fractf(v.y), .z = fractf(v.z), .w = fractf(v.w) }; }

v2 pm_sin_v2(v2 v) { return (v2){ .x = sinf(v.x), .y = sinf(v.y) }; }
v3 pm_sin_v3(v3 v) { return (v3){ .x = sinf(v.x), .y = sinf(v.y), .z = sinf(v.z) }; }
v4 pm_sin_v4(v4 v) { return (v4){ .x = sinf(v.x), .y = sinf(v.y), .z = sinf(v.z), .w = sinf(v.w) }; }

v2 pm_cos_v2(v2 v) { return (v2){ .x = cosf(v.x), .y = cosf(v.y) }; }
v3 pm_cos_v3(v3 v) { return (v3){ .x = cosf(v.x), .y = cosf(v.y), .z = cosf(v.z) }; }
v4 pm_cos_v4(v4 v) { return (v4){ .x = cosf(v.x), .y = cosf(v.y), .z = cosf(v.z), .w = cosf(v.w) }; }

////// hash functions

r32
pm_hash_v2_to_r32(v2 p)
{
  v2 r = HMM_MultiplyVec2f(p, 0.3183099f);
  r = pm_fract_v2(r);
  r = HMM_MultiplyVec2f(r, 50);
  r32 result = fractf(r.x * r.y * (r.x + r.y));
  return result;
}

r32
pm_hash_r32_to_r32(r32 n)
{
  return fractf(n * 17 * fractf(n * 0.3183099f));
}

v2
pm_hash_r32_to_v2(r32 n)
{
  v2 a = pm_sin_v2((v2){ n, n + 1.0f });
  v2 b = HMM_MultiplyVec2(a, (v2){ 43758.5453123f, 22578.1459123f });
  v2 r = pm_fract_v2(b);
  return r;
}

v2
pm_hash_v2_to_v2(v2 p)
{
  v2 k = (v2){ 0.3183099f, 0.3678794f };
  v2 kp = (v2){k.y, k.x};
  v2 r0 = HMM_MultiplyVec2(p, k);
  v2 r1 = HMM_AddVec2(r0, kp);
  r32 f = 16.0f * fractf(p.x * p.y * (p.x + p.y));
  v2 r2 = HMM_MultiplyVec2f(k, f);
  v2 r3 = pm_fract_v2(r2);
  return r3;
}

v3
pm_hash_v2_to_v3(v2 p)
{
  v3 q = (v3){
    .x = HMM_DotVec2(p, (v2){127.1f, 311.7f}),
    .y = HMM_DotVec2(p, (v2){267.5f, 183.3f}),
    .z = HMM_DotVec2(p, (v2){419.2f, 371.9f})
  };
  v3 r0 = pm_sin_v3(q);
  v3 r1 = HMM_MultiplyVec3f(r0, 43758.5453f);
  v3 r2 = pm_fract_v3(r1);
  return r2;
}

r32
pm_hash_v3_to_r32(v3 p)
{
  v3 p0 = HMM_MultiplyVec3f(p, 0.3183099f);
  v3 p1 = HMM_AddVec3(p0, (v3){ 0.1f, 0.1f, 0.1f });
  v3 p2 = pm_fract_v3(p1);
  v3 p3 = HMM_MultiplyVec3f(p, 17.0f);
  r32 r0 = fractf(p3.x * p3.y * p3.z * (p3.x + p3.y + p3.z));
  return r0;
}

r32
pm_random_v2_to_r32(v2 n)
{
  v2 v = (v2){ 12.9898f, 4.1414f };
  r32 r0 = HMM_DotVec2(n, v);
  r32 r1 = sinf(r0);
  r32 r2 = fractf(r1 * 43758.5453);
  return r2;
}

internal r32
pm_noise_v3_to_r32(v3 p)
{
    p = pm_abs_v3(p);
    v3 p_fl = pm_floor_v3(p);
    v3 p_fr = pm_fract_v3(p);
    v3 f = pm_smoothstep_v3(p_fr);
    
    v3 p_fl_0 = p_fl;
    v3 p_fl_1 = HMM_AddVec3(p_fl, (v3){1, 0, 0});
    v3 p_fl_2 = HMM_AddVec3(p_fl, (v3){0, 1, 0});
    v3 p_fl_3 = HMM_AddVec3(p_fl, (v3){1, 1, 0});
    v3 p_fl_4 = HMM_AddVec3(p_fl, (v3){0, 0, 1});
    v3 p_fl_5 = HMM_AddVec3(p_fl, (v3){1, 0, 1});
    v3 p_fl_6 = HMM_AddVec3(p_fl, (v3){0, 1, 1});
    v3 p_fl_7 = HMM_AddVec3(p_fl, (v3){1, 1, 1});

    r32 h0 = pm_hash_v3_to_r32(p_fl_0);
    r32 h1 = pm_hash_v3_to_r32(p_fl_1);
    r32 h2 = pm_hash_v3_to_r32(p_fl_2);
    r32 h3 = pm_hash_v3_to_r32(p_fl_3);
    r32 h4 = pm_hash_v3_to_r32(p_fl_4);
    r32 h5 = pm_hash_v3_to_r32(p_fl_5);
    r32 h6 = pm_hash_v3_to_r32(p_fl_6);
    r32 h7 = pm_hash_v3_to_r32(p_fl_7);

    r32 result = lerp(f.z, 
      lerp(
        lerp(h0, f.x, h1),
        f.y, 
        lerp(h2, f.x, h3)
      ),
      lerp(
        lerp(h4, f.x, h5),
        f.y,
        lerp(h6, f.x, h7)
      )
    );
    
    assert(result >= 0 && result <= 1);
    return result;
}

internal r32
pm_fmb_3d(v3 p, r32 t)
{
  v3 ts = (v3){t, t, t};
  r32 tsin = sinf(t);
  v3 tv = (v3){tsin, tsin, tsin};
  v3 pp = p;
  r32 f = 0.0;
    
  v3 pp0 = HMM_AddVec3(pp, ts);
  v3 pp1 = HMM_SubtractVec3(pp, ts);

  f += 0.500000f * pm_noise_v3_to_r32(pp0); pp = HMM_MultiplyVec3f(pp, 2.02);
  f += 0.300000f * pm_noise_v3_to_r32(pp1); pp = HMM_MultiplyVec3f(pp, 2.03);
  f += 0.125000f * pm_noise_v3_to_r32(pp);  pp = HMM_MultiplyVec3f(pp, 2.01);
  f += 0.062500f * pm_noise_v3_to_r32(pp0); pp = HMM_MultiplyVec3f(pp, 2.04);
  r32 d = 0.9875f;
  
  f = f / d;
  return f;
}

// internal r32
// pm_voronoise(v2 p, r32 u, r32 v)
// {
//     r32 k = 1.0f + 63.0f + powf(1.0f - v, 6.0f);
    
//     v2 i = pm_floor_v2(p);
//     v2 f = pm_fract_v2(p);
    
//     v2 a = (v2){0, 0};
//     for (s32 y = -2; y <= 2; y++)
//     {
//         for (s32 x = -2; x <= 2; x++)
//         {
//             v2 g = (v2){(r32)x, (r32)y};
//             v2 hi = HMM_AddVec2(g, i);
//             v3 h = pm_hash_v2_to_v3(hi);
//             v3 o = HMM_MultiplyVec3(h, (v3){ u, u, 1.0f });
//             v2 d0 = HMM_SubtractVec2(g, f);
//             v2 d1 = HMM_AddVec2(d0, o.XY);
//             r32 d1m = HMM_LengthVec2(d1);
//             r32 w = powf(1.0f - pm_smoothstep_range_r32(d1m, 0.0f, 1.414f), k);
//             a = HMM_AddVec2(a, (v2){o.z * w, w});
//         }
//     }
    
//     return a.x / a.y;
// }

#endif // PATTERNS_MATH_H