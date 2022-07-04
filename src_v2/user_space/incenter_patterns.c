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
#define incenter_pos_to_bimodal(p) (v4){ (p.x / 3.0f), (p.y / 3.0f), (p.z / 3.0f), 1.0f }

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

Assembly_Pixel
assembly_pixel_add_multi(u32 count, ...)
{
  Assembly_Pixel result = {};
  va_list args;
  va_start(args, count);
  for (u32 i = 0; i < count; i++) {
    Assembly_Pixel p = va_arg(args, Assembly_Pixel);
    result.r = u8_add_safe(result.r, p.r);
    result.g = u8_add_safe(result.g, p.g);
    result.b = u8_add_safe(result.b, p.b);
  }
  return result;
}

Assembly_Pixel
assembly_pixel_scale(Assembly_Pixel p, r32 scale)
{
  Assembly_Pixel result = {
    .r = (u8)(clamp(0, p.r * scale, 255)),
    .g = (u8)(clamp(0, p.g * scale, 255)),
    .b = (u8)(clamp(0, p.b * scale, 255)),
  };
  return result;
}

Assembly_Pixel
assembly_pixel_scale_u8(u32 r, u32 g, u32 b, r32 scale)
{
  Assembly_Pixel result = {
    .r = (u8)(clamp(0, r * scale, 255)),
    .g = (u8)(clamp(0, g * scale, 255)),
    .b = (u8)(clamp(0, b * scale, 255)),
  };
  return result;
}

Assembly_Pixel
assembly_pixel_blend(Assembly_Pixel a, Assembly_Pixel b, r32 t)
{
  r32 rf = lerp((r32)a.r, t, (r32)b.r);
  r32 gf = lerp((r32)a.g, t, (r32)b.g);
  r32 bf = lerp((r32)a.b, t, (r32)b.b);
  r32 rc = clamp(0, rf, 255);
  r32 gc = clamp(0, gf, 255);
  Assembly_Pixel result = {
    .r = (u8)rc,
    .g = (u8)clamp(0, gf, 255),
    .b = (u8)clamp(0, bf, 255),
  };
  return result;
}

v3
sun_center_for_pos(v4 p, v4 center, r32 radius, r32 falloff)
{
  v4 p_unit = incenter_pos_to_unit(p);
  r32 d0 = HMM_LengthVec4(HMM_SubtractVec4(p_unit, center));
  r32 d1 = falloff - fabsf(d0 - (radius + (0.02f * sinf(tt))));
  r32 b = d1 / falloff;

  v3 result = {};
  if (b > 0)
  {
    v3 p0 = p.xyz;
    v3 p1 = HMM_AddVec3(p0, (v3){ tt, -tt, 0 });

    v3 color = {
      .x = remap_r32(pm_fmb_3d(p0, tt), 0, 1, 0.5, 1),
      .y = remap_r32(pm_noise_v3_to_r32(p1), 0, 1, 0, 0.3f),
      .z = 0,
    };
    result = HMM_MultiplyVec3f(color, b);
  }
  return result;
}

void
sun_center(Assembly_Pixel_Buffer pixels, v4 center, r32 radius, r32 falloff)
{
  for (u32 j = 0; j < pixels.len; j++)
  {
    v3 color = sun_center_for_pos(pixels.positions[j], center, radius, falloff);
    Assembly_Pixel ac = color_v3_to_assembly_pixel(color);
    pixels.pixels[j] = assembly_pixel_add(ac, pixels.pixels[j]);

    #if 0
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
    #endif
  }
}

void
grow_pattern_sphere_function(Assembly_Pixel_Buffer pixels, v4 center, r32 radius, r32 falloff, Assembly_Pixel inner_color)
{
  for (u32 j = 0; j < pixels.len; j++)
  {
    v4 p = incenter_pos_to_unit(pixels.positions[j]);
    r32 d0 = HMM_LengthVec4(HMM_SubtractVec4(p, center));
    r32 d1 = fabsf(d0 - (radius + (0.02f * sinf(tt))));
    r32 d2 = falloff - d1;
    r32 b = d2 / falloff;
    r32 inner_b = d0 < (radius - falloff) ? 1 : 0;

    v3 color = {
      .x = 0.5f + 0.5f * sinf(p.x * r32_tau * 4.313f + tt * 1.3f),
      .y = 0.5f + 0.5f * cosf(0.2314f + p.y * r32_tau * 3.915f + tt),
      .z = 0.2f + 0.8f * p.z,
    };
    v3 color_b = HMM_MultiplyVec3f(color, b);
    Assembly_Pixel color_0 = color_v3_to_assembly_pixel(color_b);
    Assembly_Pixel color_inner = assembly_pixel_scale(inner_color, inner_b);
    Assembly_Pixel color_1 = assembly_pixel_add_multi(3, color_0, color_inner, pixels.pixels[j]);
    
    pixels.pixels[j] = color_1;
  }
}

void 
grow_pattern(Assembly_Pixel_Buffer pixels, r32 time, Assembly_Pixel inner_color)
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
    radius = 0.05f + curve_ease_in_out(t) * 0.6f;
    falloff = 0.1f - (curve_ease_in_out(t) * 0.05f);
  }

  grow_pattern_sphere_function(pixels, center, radius, falloff, inner_color);
}

void
pattern_color(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, u8 r, u8 g, u8 b)
{
  for (u32 j = 0; j < pixels.len; j++)
  {
    pixels.pixels[j].r = r;
    pixels.pixels[j].g = g; 
    pixels.pixels[j].b = b; 
  }

#if 0
  for (u32 strip_i = 0; strip_i < strips.len; strip_i++)
  {
    Assembly_Strip strip = strips.strips[strip_i];
    for (u32 led_i = 0; led_i < strip.pixels_len; led_i++)
    {
      u32 pixel_index = strip.pixels[led_i];
      pixels.pixels[pixel_index].r = 0;
      pixels.pixels[pixel_index].g = 255;
    }
  }
  #endif
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
  grow_pattern_sphere_function(pixels, center, o, 0.01f, (Assembly_Pixel){}); 
}

u32 year = 2019;
r32 month = (r32)MONTH_Jan;

s32
test_data_find_nearest_row(Incenter_City_Id city, u32 year, Incenter_Month_Id month)
{
  s32 row_index = -1;
  s32 nearest = 10000;
  s32 months = (year * 12) + month;
  for (s32 i = 0; i < test_data_len; i++) 
  {
    Incenter_Test_Data_Row row = test_data[i];
    if (row.city != city) continue;

    s32 row_months = (row.year * 12) + row.month;
    s32 months_offset = months - row_months;
    if (months_offset < nearest && months_offset >= 0) {
      nearest = months_offset;
      row_index = i;
    }
  }
  return row_index;
}

global r32 city_last_values[city_count] = { -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f, -0.2f,
};
global r32 city_last_vel[city_count];

global Assembly_Pixel pixel_white = { 255, 255, 255 };
global Assembly_Pixel pixel_black = { 0,   0,   0   };

void
pattern_test_data_scene_hombre(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Assembly_Pixel color_before_a, Assembly_Pixel color_before_b, Assembly_Pixel color_at, Assembly_Pixel color_after)
{
  Incenter_Test_Data_Row* rows = test_data;

  r32 rand_min = 1000;
  r32 rand_max = -1000;

  r32 month_delta = 1.0f / 24.0f;
  if (month >= 11) {
    s32 x = 5;
  }
  for (u32 city = 0; city < city_count; city++)
  {
    Assembly_Strip strip = strips.strips[city + 1];
    s32 data_row = test_data_find_nearest_row(city, year, month);
    if (data_row < 0) continue;
    s32 data_row_next = data_row + 1;

    Incenter_Test_Data_Row row_curr = test_data[data_row];
    r32 target_p = test_data[data_row].value_0;
    if (data_row_next < test_data_len) {
      Incenter_Test_Data_Row row_next = test_data[data_row_next];
      if (row_next.city == row_curr.city) 
      {
        r32 row_curr_months = (r32)((row_curr.year * 12) + row_curr.month);
        r32 row_next_months = (r32)((row_next.year * 12) + row_next.month);
        r32 curr_months = (r32)(year * 12) + month;

        r32 row_month_delta = row_next_months - row_curr_months;
        r32 delta_from_row_curr = curr_months - row_curr_months;
        r32 theta = delta_from_row_curr / row_month_delta;

        target_p = lerp(row_curr.value_0 * 0.8f, theta, row_next.value_0 * 0.8f);
      }
    }

    r32 last_value = city_last_values[city];
    r32 dist_to_target = (target_p - last_value);
    r32 towards_target = dist_to_target > 0 ? 1 : -1;
    dist_to_target = fabsf(dist_to_target);
    r32 force_approach = dist_to_target * towards_target * 0.1f;
    city_last_vel[city] += force_approach;
    
    r32 max_vel = 0.01f;
    r32 force_damping = -max(1 - dist_to_target, 0) * (city_last_vel[city]);
    city_last_vel[city] = clamp(-1 * max_vel, city_last_vel[city] + force_damping, max_vel);

    city_last_values[city] += city_last_vel[city];

    for (u32 led = 0; led < strip.pixels_len; led++)
    {
      r32 pct = (r32)(strip.pixels_len - led) / (r32)strip.pixels_len;
      u32 led_index = strip.pixels[led];
      v4  led_pos = incenter_pos_to_unit(pixels.positions[led_index]);

      // max(-|pct -pct_r|, 0) ^ 10
      r32 d0 = (-1 * fabsf(city_last_values[city] - pct)) + 1;
      r32 d1 = max(0, d0);
      r32 d2 = powf(d1, 20);
      r32 d2_inverse = 1 - d2;
      v3 noise_pos0 = HMM_MultiplyVec3f(HMM_AddVec3(led_pos.xyz, (v3){237, 111 + tt, 923}), 16);
      r32 at_noise = pm_fmb_3d(noise_pos0, 1);
      at_noise = pm_smoothstep_r32(at_noise);
      r32 d3 = d2;

      v3 noise_pos1 = HMM_MultiplyVec3f(HMM_AddVec3(led_pos.xyz, (v3){237, 111 + (tt * 0.3f), 923}), 16);
      r32 inner_noise = pm_smoothstep_r32(pm_fmb_3d(noise_pos1, 1));

      r32 before = pct < city_last_values[city] ? d2_inverse : 0;
      r32 after  = pct > city_last_values[city] ? d2_inverse : 0;

      Assembly_Pixel color_before_ = assembly_pixel_scale(
        assembly_pixel_blend(color_before_a, color_before_b, at_noise * at_noise * at_noise), 
        before
      );
      Assembly_Pixel color_after_  = assembly_pixel_scale(color_after, after - (0.8f * inner_noise));
      Assembly_Pixel color_at_     = assembly_pixel_scale(color_at, d3);

      pixels.pixels[led_index] = assembly_pixel_add_multi(3, color_before_, color_after_, color_at_);
    }
  }

  month += month_delta;
  if (month > (r32)MONTH_Dec + 1) {
    month = (r32)MONTH_Jan;
    year += 1;
    if (year > 2022) {
      year = 2019;
    }
  }
}

void
pattern_test_data_scene(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips)
{
  Incenter_Test_Data_Row* rows = test_data;

  r32 month_delta = 1.0f / 24.0f;
  if (month >= 11) {
    s32 x = 5;
  }
  for (u32 city = 0; city < city_count; city++)
  {
    Assembly_Strip strip = strips.strips[city + 1];
    s32 data_row = test_data_find_nearest_row(city, year, month);
    if (data_row < 0) continue;
    s32 data_row_next = data_row + 1;

    Incenter_Test_Data_Row row_curr = test_data[data_row];
    r32 percent_r = test_data[data_row].value_0;
    r32 percent_g = test_data[data_row].value_1;
    r32 percent_b = test_data[data_row].value_2;
    if (data_row_next < test_data_len) {
      Incenter_Test_Data_Row row_next = test_data[data_row_next];
      if (row_next.city == row_curr.city) 
      {
        r32 row_curr_months = (r32)((row_curr.year * 12) + row_curr.month);
        r32 row_next_months = (r32)((row_next.year * 12) + row_next.month);
        r32 curr_months = (r32)(year * 12) + month;

        r32 row_month_delta = row_next_months - row_curr_months;
        r32 delta_from_row_curr = curr_months - row_curr_months;
        r32 theta = delta_from_row_curr / row_month_delta;

        percent_r = lerp(row_curr.value_0, theta, row_next.value_0);
        percent_g = lerp(row_curr.value_1, theta, row_next.value_1);
        percent_b = lerp(row_curr.value_2, theta, row_next.value_2);
      }
    }

    // printf("%d %f - %f\n", year, month, percent_r);

    for (u32 led = 0; led < strip.pixels_len; led++)
    {
      r32 pct = (r32)(strip.pixels_len - led) / (r32)strip.pixels_len;
      u32 led_index = strip.pixels[led];

      #if 0
      pixels.pixels[led_index] = (Assembly_Pixel){
        .r = 32,
        .g = 32,
        .b = 32,
      };

      if (pct <= percent_r) pixels.pixels[led_index].r = 255;
      if (pct <= percent_g) pixels.pixels[led_index].g = 255;
      if (pct <= percent_b) pixels.pixels[led_index].b = 255;
      #else
      if (pct > percent_r) {
        pixels.pixels[led_index] = (Assembly_Pixel){
          .r = 32,
          .g = 32,
          .b = 32,
        };
      } else {
        r32 pct_dist = percent_r - pct;
        r32 ramp = (percent_r - pct_dist) / percent_r;
        ramp = ramp * ramp;

        pixels.pixels[led_index] = (Assembly_Pixel){
          .r = 32 + (128 * ramp),
          .g = 128 + (128 * ramp),
          .b = 255,
        };
      }
      #endif
    }
  }

  month += month_delta;
  if (month > (r32)MONTH_Dec + 1) {
    month = (r32)MONTH_Jan;
    year += 1;
    if (year > 2022) {
      year = 2019;
    }
  }
}

void
pattern_demo(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips)
{
  // clear previous frame
  pattern_color(pixels, strips, 0, 0, 0);

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

  Assembly_Pixel data_less_than_color_a = { .r = 32, .g = 128, .b = 255 };
  Assembly_Pixel data_less_than_color_b = { .r = 32, .g = 255, .b = 128 };
  Assembly_Pixel data_border_color = { .r = 255, .g = 255, .b = 255 };
  Assembly_Pixel data_greater_than_color = { .r = 64, .g = 0, .b = 0 };

  r32 grow_delay = 2;
  if (tt > grow_delay && tt < grow_delay + 10)
  {
    grow_pattern(pixels, tt - grow_delay, data_greater_than_color);
  }

  if (tt >= grow_delay + 9) {
    pattern_test_data_scene_hombre(pixels, strips, data_less_than_color_a, data_less_than_color_b, data_border_color, data_greater_than_color);
  }
}


//////////// MOOD BOARD DEMOS ////////////

global Color_Ramp aurora_ramp = {
  .anchors = {
    [0] = { .pct = 0,    .color = { 0, 0, 0 } },
    [1] = { .pct = .4f,  .color = { 0, 0, 0 } },
    [2] = { .pct = .55f,  .color = { 176.f / 255.f, 65.f / 255.f, 36.f / 255.f } },
    [3] = { .pct = .7f,  .color = { 237.f / 255.f, 201.f / 255.f, 138.f / 255.f } },
    [4] = { .pct = .80f, .color = { 49.f / 255.f, 156.f / 255.f, 255.f / 255.f } },
    [5] = { .pct = 1.0f, .color = { 49.f / 255.f, 156.f / 255.f, 255.f / 255.f } },
  },
  .anchors_count = 5,
};

global Color_Ramp cities_ramp = {
  .anchors = {
    [0] = { .pct = 0, .color = { 0, 0, 0 } },
    [1] = { .pct = .75f, .color = { 0, 0, 0 } },
    [2] = { .pct = 1,    .color = { 255.f / 255.f, 194.f / 255.f, 86.f / 255.5 } },
  },
  .anchors_count = 3
};

void
pattern_demo_1(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips)
{
  for (u32 j = 0; j < pixels.len; j++)
  {
    v4 p = pixels.positions[j];
    v4 p_unit = incenter_pos_to_unit(p);
    v3 p_offset = HMM_AddVec3(p.xyz, (v3){ 213.145f, 99.321f, 71.3f });
    v3 p_scaled = HMM_MultiplyVec3f(p_offset, 2);
    r32 v = pm_fmb_3d(p_scaled, tt);
    r32 vv = pm_smoothstep_r32(v);
    v3 color = color_ramp_eval(aurora_ramp, vv);
    pixels.pixels[j] = color_v3_to_assembly_pixel(color);
  }
}

void
pattern_demo_2(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips)
{
  for (u32 city = 0; city < city_count; city++)
  {
    Assembly_Strip strip = strips.strips[city + 1];
    for (u32 led = 0; led < strip.pixels_len; led++)
    {
      r32 pct = (r32)(strip.pixels_len - led) / (r32)strip.pixels_len;
      u32 led_index = strip.pixels[led];

      v3 color = color_ramp_eval(cities_ramp, 1 - pct);
      pixels.pixels[led_index] = color_v3_to_assembly_pixel(color);
    }
  }
}

// sunrise
void
pattern_demo_3(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips)
{
  v3 sun_dir = (v3){ sinf(tt), 0, cosf(tt) };
  for (u32 city = 0; city < city_count; city++)
  {
    Assembly_Strip strip = strips.strips[city + 1];
    for (u32 led = 0; led < strip.pixels_len; led++)
    {
      r32 pct = (r32)(strip.pixels_len - led) / (r32)strip.pixels_len;
      u32 led_index = strip.pixels[led];

      v3 cities_color = color_ramp_eval(cities_ramp, 1 - pct);
      v4 p = pixels.positions[led_index];
      v4 p_unit = incenter_pos_to_bimodal(p);
      r32 d = HMM_DotVec3(p_unit.xyz, sun_dir);
      r32 dc = clamp(0, d, 1);
      r32 ds = clamp(0, -1 * d, 1);
      cities_color = HMM_MultiplyVec3f(cities_color, dc);

      v3 sky_color = { 49.f / 255.f, 156.f / 255.f, 255.f / 255.f };
      sky_color = HMM_MultiplyVec3f(sky_color, ds);
      v3 day_color = HMM_AddVec3(sky_color, cities_color);

      v3 sun_color = sun_center_for_pos(pixels.positions[led_index], (v4){0.5f, 0.5f, 0.5f, 1}, .1f, .1f);
      v3 color = HMM_AddVec3(day_color, sun_color);
      pixels.pixels[led_index] = color_v3_to_assembly_pixel(color);
    }
  }
}