r32
curve_ease_in_out(r32 t)
{
  r32 tc = clamp(0, t, 1);
  r32 theta = (tc * r32_pi) - (r32_pi / 2.0f);
  r32 s = sinf(theta);
  r32 result = 0.5f + (s / 2.0f);
  return result;
}

#define incenter_pos_to_unit(p) (v4){ ((p.x / 6.0f) + 0.5f), ((p.y / 6.0f) + 0.5f), ((p.z / 6.0f) + 0.5f), 1.0f }

void 
test_pattern(Assembly_Pixel_Buffer pixels)
{
  for (u32 j = 0; j < pixels.len; j++)
  {
    v4 p = pixels.positions[j];
    pixels.pixels[j].r = (u8)(((sinf((5 * tt) + (p.x * 10)) + 1) * 0.5f) * 255);
    pixels.pixels[j].b = (u8)(((sinf((5 * tt) + (p.z * 10)) + 1) * 0.5f) * 255);
  }
}

void
pattern_debug(Assembly_Pixel_Buffer pixels)
{
  r32 scale = 6;
  r32 offset = 0;

  for (u32 j = 0; j < pixels.len; j++)
  {
    v4 p = pixels.positions[j];
    v4 pp = incenter_pos_to_unit(p);
    pixels.pixels[j].r = pp.x * 255;
    pixels.pixels[j].g = pp.y * 255;
    pixels.pixels[j].b = pp.z * 255;
  }
}

Assembly_Pixel
color_v3_to_assembly_pixel(v3 c)
{
  return (Assembly_Pixel){
    .r = (u8)(c.x * 255),
    .g = (u8)(c.y * 255),
    .b = (u8)(c.z * 255),
  };
}

u8
u8_add_safe(u8 a, u8 b)
{
  u8 r = a + b;
  if (r < a || r < b) r = 255;
  return r;
}

Assembly_Pixel
assembly_pixel_add(Assembly_Pixel a, Assembly_Pixel b)
{
  Assembly_Pixel result = {
    .r = u8_add_safe(a.r, b.r),
    .g = u8_add_safe(a.g, b.g),
    .b = u8_add_safe(a.b, b.b),
  };
  return result;
}

void
sun_center(Assembly_Pixel_Buffer pixels, v4 center, r32 radius, r32 falloff)
{
  for (u32 j = 0; j < pixels.len; j++)
  {
    v4 p = incenter_pos_to_unit(pixels.positions[j]);
    r32 d0 = HMM_LengthVec4(HMM_SubtractVec4(p, center));
    r32 d1 = falloff - fabsf(d0 - (radius + (0.02f * sinf(tt))));
    r32 b = d1 / falloff;

    if (b > 0)
    {
      v3 p0 = pixels.positions[j].xyz;
      v3 p1 = HMM_AddVec3(p0, (v3){ tt, -tt, 0 });

      v3 color = {
        .x = remap_r32(pm_fmb_3d(p0, tt), 0, 1, 0.5, 1),
        .y = remap_r32(pm_noise_v3_to_r32(p1), 0, 1, 0, 0.3f),
        .z = 0,
      };
      Assembly_Pixel cc = color_v3_to_assembly_pixel(color);
      v3 color_b = HMM_MultiplyVec3f(color, b);
      Assembly_Pixel color_0 = color_v3_to_assembly_pixel(color_b);
      Assembly_Pixel color_1 = assembly_pixel_add(color_0, pixels.pixels[j]);
      pixels.pixels[j] = color_1;
    }
  }
}

void
grow_pattern_sphere_function(Assembly_Pixel_Buffer pixels, v4 center, r32 radius, r32 falloff)
{
  for (u32 j = 0; j < pixels.len; j++)
  {
    v4 p = incenter_pos_to_unit(pixels.positions[j]);
    r32 d0 = HMM_LengthVec4(HMM_SubtractVec4(p, center));
    r32 d1 = falloff - fabsf(d0 - (radius + (0.02f * sinf(tt))));
    r32 b = d1 / falloff;

    v3 color = {
      .x = 0.5f + 0.5f * sinf(p.x * r32_tau * 4.313f + tt * 1.3f),
      .y = 0.5f + 0.5f * cosf(0.2314f + p.y * r32_tau * 3.915f + tt),
      .z = 0.2f + 0.8f * p.z,
    };
    v3 color_b = HMM_MultiplyVec3f(color, b);
    Assembly_Pixel color_0 = color_v3_to_assembly_pixel(color_b);
    Assembly_Pixel color_1 = assembly_pixel_add(color_0, pixels.pixels[j]);
    pixels.pixels[j] = color_1;
  }
}

void 
grow_pattern(Assembly_Pixel_Buffer pixels, r32 time)
{
  v4 center = (v4){};
  r32 radius = 0;
  r32 falloff = 0;
  //phase 1 - light up to center
  if (time < 6)
  {
    r32 height = -0.2f + curve_ease_in_out(time / 4) * 0.5f;
    center = (v4){ 0.5f, 0.2f + height, 0.5f, 1 };
    radius = lerp(0.01f, (time / 6), 0.05f);
    falloff = 0.1f;
  }
  else if (time >= 6)
  {
    r32 t = (time - 6) / 4;
    center = (v4){ 0.5f, 0.5f, 0.5f, 1 };
    radius = 0.05f + curve_ease_in_out(t) * 0.4f;
    falloff = 0.1f - (curve_ease_in_out(t) * 0.05f);
  }

  grow_pattern_sphere_function(pixels, center, radius, falloff);
}

void
pattern_color(Assembly_Pixel_Buffer pixels, u8 r, u8 g, u8 b)
{
  for (u32 j = 0; j < pixels.len; j++)
  {
    pixels.pixels[j].r = r;
    pixels.pixels[j].g = g; 
    pixels.pixels[j].b = b; 
  }
}

void
pattern_blink(Assembly_Pixel_Buffer pixels)
{
  r32 t0 = clamp(0, tt, 1.5f) / 1.5f;
  r32 t1 = pm_smoothstep_r32(t0);
  r32 t2 = t1 * 0.43f;
  v4 center = (v4){
    .x = 0.5f,
    .y = t2,
    .z = 0.5f,
    .w = 1,
  };
  r32 o = remap_r32(sinf(tt), -1, 1, 0.0015f, 0.002f);
  for (u32 i = 0; i < pixels.len; i++)
  {
    v4 p = incenter_pos_to_unit(pixels.positions[i]);
    v2 ho = (v2){ .x = fabsf(p.x - 0.5f), .y = fabsf(p.z - 0.5f) };
    r32 dh = HMM_LengthVec2(ho);
    if (p.y < center.y && dh < 0.01f) 
    {
      pixels.pixels[i].r = 0; 
      pixels.pixels[i].g = 128;
      pixels.pixels[i].b = 255;
    }
  }
  grow_pattern_sphere_function(pixels, center, o, 0.01f); 
}

void
pattern_demo(Assembly_Pixel_Buffer pixels)
{
  pattern_color(pixels, 255, 255, 255); // reset
  return;

  r32 sun_limit = 5;
  if (tt < sun_limit)
  {
    r32 r = 0.1f;
    if (tt > (sun_limit - 1))
    {
      r32 ttt = tt - (sun_limit - 1);
      r = lerp(0.1f, ttt, 0.0f);
    }
    sun_center(pixels, (v4){0.5f, 0.5f, 0.5f, 1}, r, r); 
  }
  pattern_blink(pixels);

  r32 grow_delay = 2;
  if (tt > grow_delay)
  {
    grow_pattern(pixels, tt - grow_delay);
  }
}