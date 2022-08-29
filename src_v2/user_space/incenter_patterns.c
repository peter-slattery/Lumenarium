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

Assembly_Pixel
color_v3_to_assembly_pixel_faded(v3 c, r32 b)
{
  return (Assembly_Pixel){
    .r = (u8)(c.x * b * 255),
    .g = (u8)(c.y * b * 255),
    .b = (u8)(c.z * b * 255),
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

Assembly_Pixel
color_ramp_eval_pixel(Color_Ramp ramp, r32 pct)
{
  v3 c = color_ramp_eval(ramp, pct);
  Assembly_Pixel r = color_v3_to_assembly_pixel(c);
  return r;
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
r32 month = (r32)MONTH_jan;

#if 0
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

#endif

void
pattern_demo(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
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
    //pattern_test_data_scene_hombre(pixels, strips, data_less_than_color_a, data_less_than_color_b, data_border_color, data_greater_than_color);
  }
}


//////////// MOOD BOARD DEMOS ////////////

#define hex_color(r, g, b) (v3){ .x = (r32)r / 255.f, .y = (r32)g / 255.f, .z = (r32)b / 255.f }

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

global Color_Ramp sun_ramp = {
  .anchors = {
    [0] = { .pct = 0,    .color = hex_color(  0,   0,  0), },
    [1] = { .pct = 0.2,  .color = hex_color(  0,   0,  0), },
    [2] = { .pct = 0.3f, .color = hex_color(255,  85,  0), },
    [3] = { .pct = 0.6f, .color = hex_color(255,   0,  0), },
    [4] = { .pct = 0.9,  .color = hex_color(255, 209, 54), },
    [5] = { .pct = 1,    .color = hex_color(255, 209, 54), },
  },
  .anchors_count = 6
};

global Color_Ramp cities_ramp = {
  .anchors = {
    [0] = { .pct = 0, .color = { 0, 0, 0 } },
    [1] = { .pct = .75f, .color = { 0, 0, 0 } },
    [2] = { .pct = 1,    .color = { 255.f / 255.f, 194.f / 255.f, 86.f / 255.5 } },
  },
  .anchors_count = 3
};

global Color_Ramp cities_sparkle_ramp = {
  .anchors = {
    [0] = { .pct = 0, .color = { 0, 0, 0 } },
    [1] = { .pct = .7f, .color = { 0, 0, 0 } },
    [2] = { .pct = .1f, .color = { 255.f / 255.f, 100.f / 255.f, 40.f / 255.5 } },
  },
  .anchors_count = 3
};

global Color_Ramp xray_ramp = {
  .anchors = {
    [0] = { .pct = 0.0f, .color = { 32.f / 255.f,  2.f / 255.f,   186.f / 255.f } },
    [1] = { .pct = 0.5f, .color = { 230.f / 255.f, 37.f / 255.f,  7.f / 255.f } },
    [2] = { .pct = 1.0f, .color = { 255.f / 255.f, 162.f / 255.f, 0 } },
  },
  .anchors_count = 3
};

global Color_Ramp xray_ramp_rev = {
  .anchors = {
    [2] = { .pct = 1.0f, .color = { 32.f / 255.f,  2.f / 255.f,   186.f / 255.f } },
    [1] = { .pct = 0.5f, .color = { 230.f / 255.f, 37.f / 255.f,  7.f / 255.f } },
    [0] = { .pct = 0.0f, .color = { 255.f / 255.f, 162.f / 255.f, 0 } },
  },
  .anchors_count = 3
};

global Color_Ramp nature_ramp = {
  .anchors = {
    [0] = {.pct = 0.0f, .color = { 85.f / 255.f, 192.f / 255.f, 255.f / 255.f } },
    [1] = {.pct = 0.5f, .color = { 0, 1, 0.5f } },    
    [2] = {.pct = 1.0f, .color = { 85.f / 255.f, 192.f / 255.f, 255.f / 255.f } },
  },
  .anchors_count = 3,
};

void
pattern_aurora_led(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, v4 pos, u32 index, r32 scene_time)
{
  v4 p_unit = incenter_pos_to_unit(pos);
  v3 p_offset = HMM_AddVec3(pos.xyz, (v3){ 213.145f, 99.321f, 71.3f });
  v3 p_scaled = HMM_MultiplyVec3f(p_offset, 2);
  r32 v = pm_fmb_3d(p_scaled, scene_time);
  r32 vv = pm_smoothstep_r32(v);
  v3 color = color_ramp_eval(aurora_ramp, vv);
  pixels.pixels[index] = color_v3_to_assembly_pixel(color);
}

void
pattern_aurora(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  r32 scene_time = ins->scene_time;
  for (u32 j = 0; j < pixels.len; j++)
  {
    v4 p = pixels.positions[j];
    pattern_aurora_led(pixels, strips, p, j, scene_time);
  }
}

void
pattern_demo_2(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
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
pattern_demo_3(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  r32 scene_time = ins->scene_time;
  v3 sun_dir = (v3){ sinf(scene_time), 0, cosf(scene_time) };
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
      
      //v3 sun_color = sun_center_for_pos(pixels.positions[led_index], (v4){0.5f, 0.5f, 0.5f, 1}, .1f, .1f);
      v3 color = day_color; //HMM_AddVec3(day_color, sun_color);
      pixels.pixels[led_index] = color_v3_to_assembly_pixel(color);
    }
  }
}

// random fill
u32 city_hashes[city_count];
u32 city_iters[city_count];
void
pattern_random_fill_prep()
{
  Random_Series rs = random_series_create(1337);
  for (u32 i = 0; i < city_count; i++)
  {
    city_iters[i] = random_series_next(&rs) % 127;
    city_hashes[i] = hash_djb2_cstr_to_u32(city_strings[i]);
  }
}

void
pattern_random_fill(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  pattern_color(pixels, strips, 1, 38, 45); // dull green
  
  Assembly_Pixel color = color_v3_to_assembly_pixel((v3){1, .9f, 0});
  
  r32 scene_time = ins->scene_time;
  r32 dots_per_second = 5;
  u32 iter_cap = scene_time * dots_per_second;
  for (u32 city = 0; city < city_count; city++)
  {
    Random_Series rs = random_series_create(city_hashes[city]);
    u32 city_iter = min(city_iters[city], iter_cap);
    Assembly_Strip strip = strips.strips[city + 1];
    for (u32 i = 0; i < city_iter; i++)
    {
      u32 led = random_series_next(&rs) % strip.pixels_cap;
      u32 led_index = strip.pixels[led];
      pixels.pixels[led_index] = color;
    }
  }      
}

r32
hash_v3_to_r32(v3 p)
{
  u32 seed = hash_djb2_str_to_u32((char*)&p.x, 4);
  seed = hash_djb2_append_str_to_u32(seed, (char*)&p.y, 4);
  seed = hash_djb2_append_str_to_u32(seed, (char*)&p.z, 4);
  Random_Series rs = random_series_create(seed);
  return random_series_next_unilateral(&rs);
}

r32
noise_v3_to_r32(v3 p, r32 scale)
{
  p = pm_abs_v3(HMM_MultiplyVec3f(p, scale));
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
  
  r32 h0 = hash_v3_to_r32(p_fl_0);
  r32 h1 = hash_v3_to_r32(p_fl_1);
  r32 h2 = hash_v3_to_r32(p_fl_2);
  r32 h3 = hash_v3_to_r32(p_fl_3);
  r32 h4 = hash_v3_to_r32(p_fl_4);
  r32 h5 = hash_v3_to_r32(p_fl_5);
  r32 h6 = hash_v3_to_r32(p_fl_6);
  r32 h7 = hash_v3_to_r32(p_fl_7);
  
  r32 h0_1 = lerp(h0, f.x, h1);
  r32 h2_3 = lerp(h2, f.x, h3);
  r32 h4_5 = lerp(h4, f.x, h5);
  r32 h6_7 = lerp(h6, f.x, h7);
  r32 h01_23 = lerp(h0_1, f.y, h2_3);
  r32 h45_67 = lerp(h4_5, f.y, h6_7);
  // r32 result = lerp( 
  //   lerp(
  //     lerp(h0, f.x, h1),
  //     f.y, 
  //     lerp(h2, f.x, h3)
  //   ),
  //   f.z,
  //   lerp(
  //     lerp(h4, f.x, h5),
  //     f.y,
  //     lerp(h6, f.x, h7)
  //   )
  // );
  r32 result = lerp(h01_23, f.z, h45_67);
  
  assert(result >= 0 && result <= 1);
  return result;
}

r32
fbm_3d(v3 x, r32 scale)
{
  v3  pp = HMM_MultiplyVec3f(x, scale);
  r32 f  = 0.0f;    
  f += 0.500000f * noise_v3_to_r32(pp, 1); pp = HMM_MultiplyVec3f(pp, 2.02);
  f += 0.300000f * noise_v3_to_r32(pp, 1); pp = HMM_MultiplyVec3f(pp, 2.03);
  f += 0.125000f * noise_v3_to_r32(pp, 1); pp = HMM_MultiplyVec3f(pp, 2.01);
  f += 0.062500f * noise_v3_to_r32(pp, 1); pp = HMM_MultiplyVec3f(pp, 2.04);  
  r32 d = 0.9875f;
  f = f / d;
  return f;
}

// Data Flow Pattern
void
pattern_add_data_flow_ramp(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, 
  r32 period, r32 offset, r32 radius,
  Color_Ramp color_ramp)
{
  Random_Series rs = random_series_create(133753);
  
  for (u32 city = 0; city < city_count; city++)
  {
    Assembly_Strip strip = strips.strips[city + 1];
    r32 city_offset = random_series_next_unilateral(&rs) * period;
    for (u32 led = 0; led < strip.pixels_len; led++)
    {
      u32 led_index = strip.pixels[led];
      r32 led_pct = (r32)led_index / (r32)strip.pixels_len;
      r32 dist = (-1 * fmodf(led_pct + offset + city_offset, period)) + radius;
      dist = max(dist, 0) / radius;
      v3 color = color_ramp_eval(color_ramp, led_pct);
      pixels.pixels[led_index] = assembly_pixel_add(
          pixels.pixels[led_index],
        color_v3_to_assembly_pixel_faded(color, dist)
      );
    }
  }
}

void
pattern_add_data_flow_color(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, 
  r32 period, r32 offset, r32 radius,
  v3 color)
{
  Random_Series rs = random_series_create(133753);
  
  for (u32 city = 0; city < city_count; city++)
  {
    Assembly_Strip strip = strips.strips[city + 1];
    r32 city_offset = random_series_next_unilateral(&rs) * period;
    for (u32 led = 0; led < strip.pixels_len; led++)
    {
      u32 led_index = strip.pixels[led];
      r32 led_pct = (r32)led_index / (r32)strip.pixels_len;
      r32 dist = (-1 * fmodf(led_pct + offset + city_offset, period)) + radius;
      dist = max(dist, 0) / radius;
      pixels.pixels[led_index] = assembly_pixel_add(
          pixels.pixels[led_index],
        color_v3_to_assembly_pixel_faded(color, dist)
      );
    }
  }
}

void
pattern_mask_noise(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, r32 scale, r32 offset)
{
  for (u32 city = 0; city < city_count; city++)
  {
    Assembly_Strip strip = strips.strips[city + 1];    
    for (u32 led = 0; led < strip.pixels_len; led++)
    {
      u32 led_index = strip.pixels[led];
      v4 p = pixels.positions[led_index];
      r32 n = noise_v3_to_r32(HMM_AddVec3(p.xyz, (v3){offset, 0, 0}), 4);
      pixels.pixels[led_index] = assembly_pixel_scale(pixels.pixels[led_index], n);
    }
  }
}

void
pattern_data_flow(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  pattern_color(pixels, strips, 1, 38, 45); // dull green
  
  r32 tt_base = ins->scene_time;
  pattern_add_data_flow_color(pixels, strips, .6f,  tt_base,        .02f,  (v3){1, 0, .8f});  
  pattern_add_data_flow_color(pixels, strips, .8f, tt_base * .5f,  .035f, (v3){0, 1, 0});  
  pattern_add_data_flow_color(pixels, strips, 1.2f, tt_base * .35f, .06f,  (v3){0, 1, 1});  
  pattern_mask_noise(pixels, strips, 5, tt);
}


void
pattern_fast_noise_test(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  for (u32 city = 0; city < city_count; city++)
  {
    Assembly_Strip strip = strips.strips[city + 1];    
    for (u32 led = 0; led < strip.pixels_len; led++)
    {
      u32 led_index = strip.pixels[led];
      v4 p = pixels.positions[led_index];
      r32 n = noise_v3_to_r32(HMM_AddVec3(p.xyz, (v3){tt, 0, 0}), 4);
      pixels.pixels[led_index] = color_v3_to_assembly_pixel((v3){n,n,n});
    }
  }
}

// City Lights Twinkle
void
secondary_pattern_twinkle(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  for (u32 i = 0; i < secondary_strips_len; i++) {
    Assembly_Strip* strip = secondary_city_strips[i];
    for (u32 led = 0; led < strip->pixels_len; led++)
    {
      u32 led_index = strip->pixels[led];
      v4 p = pixels.positions[led_index];
      v4 p_unit = incenter_pos_to_unit(p);
      v3 p_offset = HMM_AddVec3(p.xyz, (v3){ 213.145f, 99.321f, 71.3f });
      v3 p_scaled = HMM_MultiplyVec3f(p_offset, 2);
      r32 v = pm_fmb_3d(p_scaled, ins->scene_time);
      r32 vv = (2 * pm_smoothstep_r32(v)) - 1;
      
#define TESTING_LEDS 0
#if !TESTING_LEDS
      v3 color = color_ramp_eval(cities_sparkle_ramp, vv);
      pixels.pixels[led_index] = color_v3_to_assembly_pixel(color);
#else
      pixels.pixels[led_index] = color_v3_to_assembly_pixel((v3){1, 0, 0});
#endif
    }
  }
}

Assembly_Pixel
sun(v3 pos, r32 radius2, Assembly_Pixel back_color, r32 t)
{
  Assembly_Pixel color_sun   = { 255, 0,   0   };
  r32 sun_radius = INCENTER_FEET(3);
  r32 sun_radius2 = sun_radius * sun_radius;
  r32 sun_b = sdf_sphere2_d(sun_radius2, radius2);
  Assembly_Pixel result = {0, 0, 0};
  if (sun_b > 0) {
    sun_b = max(0, sun_b);
    t *= 0.5f;
    v3 pn = HMM_NormalizeVec3(pos);
    pn = HMM_MultiplyVec3f(pn, -t);
    pos = HMM_AddVec3(pos, pn);
    pos = HMM_AddVec3(pos, (v3){ 4.1f * sin(t * 0.05f), 3 * cos(t * 0.05f), 0 });
    r32 p = fbm_3d(pos, 2);
    result = color_ramp_eval_pixel(sun_ramp, pm_smoothstep_r32(p));
    result = assembly_pixel_blend(back_color, result, sun_b);
  }
  return result;
}

void
pattern_sun_passive(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  r32 st = (r32)ins->scene_time;
  for (u32 j = 0; j < pixels.len; j++)
  {
    v4 pos = pixels.positions[j];
    v4 p = incenter_pos_to_unit(pos);
    r32 r2 = HMM_LengthSquaredVec3(pos.xyz);
#if !TESTING_LEDS
    pixels.pixels[j] = sun(pos.xyz, r2, (Assembly_Pixel){0, 0, 0}, st);
#else
    pixels.pixels[j] = color_v3_to_assembly_pixel((v3){0, 0, 1});
#endif
    
  }
  
  secondary_pattern_twinkle(pixels, strips, ins);
}

void
pattern_add_bar_chart(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins, Incenter_Scene scene, u32 year, Incenter_Month_Id month, Color_Ramp color_ramp)
{
  for (u32 row_i = 0; row_i < scene.data_len; row_i++)
  {
    Incenter_Data_Row row = scene.data[row_i];
    if (row.year != year || row.month != month) continue; 
    
    Assembly_Strip strip = strips.strips[row.id];
    for (u32 led_i = 0; led_i < strip.pixels_len; led_i++)
    {
      u32 led_index = strip.pixels[led_i];
      
      // Bar Chart
      r32 pct = ((r32)led_i / (r32)strip.pixels_len);
      if (pct < row.prop) {
        r32 cpct = pct / row.prop;
        Assembly_Pixel p = color_ramp_eval_pixel(color_ramp, cpct);
        pixels.pixels[led_index] = p;
      }
    }
  }
}

void
pattern_bar_chart(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  Incenter_Scene scene = ins->scenes[ins->scene_at];
  if (!scene.data) return;
  
  pattern_color(pixels, strips, 0, 0, 0);
  pattern_add_bar_chart(pixels, strips, ins, scene, scene.data[0].year, scene.data[0].month, xray_ramp);
}


void
pattern_bar_chart_over_time(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  Incenter_Scene scene = ins->scenes[ins->scene_at];
  if (!scene.data) return;
  
  local_persist s32 last_scene_at = -1;
  local_persist u32 year_max = 0;
  local_persist Incenter_Month_Id month_max = 0;
  local_persist u32 year_at = 0;
  local_persist Incenter_Month_Id month_at = 0;
  local_persist r32 month_start_time = 0;
  if (last_scene_at != ins->scene_at) 
  {
    last_scene_at = ins->scene_at;
    
    // Determine what the end of the data set is
    for (u32 row_i = 0; row_i < scene.data_len; row_i++)
    {
      Incenter_Data_Row row = scene.data[row_i];
      if (row.year >= year_max) {
        year_max = row.year;
        month_max = max(row.month, month_max);
      }
    }
    
    year_at = scene.data[0].year;
    month_at = scene.data[0].month;
    month_start_time = ins->scene_time;
  }
  
  r32 month_duration = 2;
  r32 time_at_month = ins->scene_time - month_start_time;
  if (time_at_month > month_duration) 
  {
    if (year_at < year_max) {
      month_at += 1;
      if (month > MONTH_dec) {
        month_at = MONTH_jan;
        year_at += 1;
      }
    } else {
      if (month < month_max) month += 1;
    }
    
    month_start_time = ins->scene_time;
  }
  
  pattern_color(pixels, strips, 0, 0, 0);
  pattern_add_bar_chart(pixels, strips, ins, scene, year_at, month_at, xray_ramp);
}


void
pattern_scene_input_pulse(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  r32 pulse_duration = 1;
  if (ins->scene_time > pulse_duration) return;

  r32 pulse_pct = min(ins->scene_time / pulse_duration, 1);
  r32 height = (pm_easeout_cubic_r32(pulse_pct) * 1.4f) - 0.2f;
  r32 brightness = 1 - pulse_pct;

  r32 strip_len = (r32)(BLACK_ROCK_LED_OPL - BLACK_ROCK_LED_FIRST);

  Assembly_Strip brc_strip = strips.strips[city_black_rock];
  for (u32 i = BLACK_ROCK_LED_FIRST; i < BLACK_ROCK_LED_OPL; i++)
  {
    u32 led_i = brc_strip.pixels[i];
    r32 pct_i = (r32)i / (r32)strip_len;
    pct_i = 1 - pct_i;
    r32 pct_d = fabsf(pct_i - height);
    r32 b = max(0, 0.1f - pct_d) / 0.1f;
    pixels.pixels[led_i] = color_v3_to_assembly_pixel_faded((v3){0, 0, 1}, b * brightness);
  }
}

////////////////////////////////////////
// ANYBODY HOME?

void
pattern_anybody_home(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  pattern_color(pixels, strips, 0, 0, 0);

  Assembly_Strip strip = strips.strips[city_black_rock];
  for (u32 i = BLACK_ROCK_LED_FIRST; i < BLACK_ROCK_LED_OPL; i++) 
  {
    u32 led_i = strip.pixels[i];
    pixels.pixels[led_i] = color_v3_to_assembly_pixel((v3){0, 0, 1});
  }
}


////////////////////////////////////////
// Felt Isolated

Assembly_Pixel
pattern_felt_isolated_color(u32 pixel_i, u32 pixels_len, r32 b)
{
  r32 pp = 1 - ((r32)pixel_i / (r32)pixels_len);
  Assembly_Pixel color0 = color_ramp_eval_pixel(xray_ramp, pp);
  Assembly_Pixel color = assembly_pixel_scale(color0, b);
  return color;
}

void
pattern_felt_isolated_intro(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  Incenter_Scene scene = ins->scenes[ins->scene_at];
  pattern_color(pixels, strips, 0, 0, 0);
  for (u32 row_i = 0; row_i < scene.data_len; row_i++)
  {
    Incenter_Data_Row row = scene.data[row_i];
    Assembly_Strip strip = strips.strips[row.id];
    u32 pixel_start = row.prop * strip.pixels_len;
    u32 pixel_index = strip.pixels[pixel_start];
    r32 row_offset = (.1439f * row_i);
    r32 b = pm_sinf_01(ins->scene_time + row_offset);    
    pixels.pixels[pixel_index] = pattern_felt_isolated_color(
        pixel_start, 
      strip.pixels_len, 
      b
    );
  }
}

void
pattern_felt_isolated_passive(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  Incenter_Scene scene = ins->scenes[ins->scene_at];
  r32 scene_speed = 4;
  r32 scene_time = ins->scene_time * scene_speed;
  
  pattern_color(pixels, strips, 0, 0, 0);
  pattern_scene_input_pulse(pixels, strips, ins);
  for (u32 row_i = 0; row_i < scene.data_len; row_i++)
  {
    Incenter_Data_Row row = scene.data[row_i];
    Assembly_Strip strip = strips.strips[row.id];
    u32 pixel_start = strip.pixels_len - (row.prop * strip.pixels_len);
    r32 row_offset = (.1439f * row_i);
    r32 b = pm_sinf_01(ins->scene_time + row_offset);
    
    r32 grow_duration = 4.0f;
    r32 grow_delay = row_offset * 5;
    r32 grow_time = (scene_time - 2.0f) - grow_delay;
    
    r32 grow_pct = clamp(0, grow_time, grow_duration) / grow_duration;
    r32 grow_pct_smoothed = pm_easeinout_cubic_r32(grow_pct);
    u32 pixels_on = (strip.pixels_len - pixel_start) * grow_pct_smoothed;
    u32 pixel_stop = clamp(pixel_start + 1, pixel_start + pixels_on, strip.pixels_len);
    for (u32 pixel_i = pixel_start; pixel_i < pixel_stop; pixel_i++)
    {
      u32 pixel_index = strip.pixels[pixel_i];
      pixels.pixels[pixel_index] = pattern_felt_isolated_color(
          pixel_i, 
        strip.pixels_len,
        b
      );
    }
    
  }
}

void
pattern_rainbow(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  Assembly_Pixel p = color_v3_to_assembly_pixel((v3){
      .x = pm_sinf_01(ins->scene_time),
      .y = pm_cosf_01(ins->scene_time),
      .z = 0.5f,
    });
  for (u32 j = 0; j < pixels.len; j++)
  {
    pixels.pixels[j] = p;
  }
}

///////////////////////////////////
// Begun to Heal

void
pattern_bar_chart_bubbly_intro(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  Incenter_Scene scene = ins->scenes[ins->scene_at];
  if (!scene.data) return;
  
  pattern_color(pixels, strips, 0, 0, 0);
  r32 scene_speed = 3;
  r32 scene_time = ins->scene_time * scene_speed;
  
  for (u32 row_i = 0; row_i < scene.data_len; row_i++)
  {
    Incenter_Data_Row row = scene.data[row_i];
    Assembly_Strip strip = strips.strips[row.id];
    u32 led_index = strip.pixels[strip.pixels_len - 1];
    Assembly_Pixel p = color_v3_to_assembly_pixel(nature_ramp.anchors[0].color);
    pixels.pixels[led_index] = p;
  }
}

void
pattern_bar_chart_bubbly_passive(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  Incenter_Scene scene = ins->scenes[ins->scene_at];
  if (!scene.data) return;
  
  pattern_color(pixels, strips, 0, 0, 0);
  r32 scene_time = ins->scene_time;
  r32 grow_time = 5;
  r32 pct_grow_time = clamp(0, ((scene_time - INCENTER_TRANSITION_DURATION) / grow_time), 1);
  
  // create a ramp that is 0 at pct_grow_time = 0 and pct_grow_time = 1,
  // but smoothly grows to 1 at pct_grow_time = 0.5
  r32 offset_influence = sinf((1 - pct_grow_time) * r32_pi);
  
  for (u32 row_i = 0; row_i < scene.data_len; row_i++)
  {
    Incenter_Data_Row row = scene.data[row_i];
    Assembly_Strip strip = strips.strips[row.id];
    
    u32 first_led = strip.pixels[strip.pixels_len - 1];
    v3 first_led_pos = pixels.positions[first_led].xyz;
    v3 root = HMM_AddVec3(first_led_pos, (v3){ ins->scene_time, 0, 0 });
    
    r32 strip_pct = row.prop * pct_grow_time;
    if (pct_grow_time < 1) {
      r32 strip_offset = (pm_noise_v3_to_r32(root) * 0.4f) - 0.2f;
      strip_pct += strip_offset * offset_influence;
      strip_pct = clamp(0, strip_pct, row.prop);
    }
    
    for (u32 led_i = 0; led_i < strip.pixels_len; led_i++)
    {
      u32 led_index = strip.pixels[led_i];
      
      // Bar Chart
      r32 pct = 1 - ((r32)led_i / (r32)strip.pixels_len);
      if (pct < strip_pct) {
        r32 cpct = fractf(((1 - pct) / row.prop) + (ins->scene_time * 0.3f));
        Assembly_Pixel p = color_ramp_eval_pixel(nature_ramp, cpct);
        pixels.pixels[led_index] = p;
      }
    }
  }
}

///////////////////////////////////////////////////
// Relationship Community Support

void
pattern_bar_chart_with_connections(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  Incenter_Scene scene = ins->scenes[ins->scene_at];
  if (!scene.data) return;
  
  pattern_color(pixels, strips, 0, 0, 0);
  
  r32 scene_time = ins->scene_time;  
  pattern_add_bar_chart(pixels, strips, ins, scene, scene.data[0].year, scene.data[0].month, xray_ramp);
  pattern_add_data_flow_color(pixels, strips, .3f,  ins->scene_time, .01f, (v3){1,1,1});
  pattern_add_data_flow_color(pixels, strips, .15f, ins->scene_time, .005f, (v3){1,1,1});
}

/////
// Believe Science Renewable Tech

void
pattern_bar_chart_random_fill(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  Incenter_Scene scene = ins->scenes[ins->scene_at];
  if (!scene.data) return;
  
  pattern_color(pixels, strips, 1, 38, 45); // dull green
  
  Assembly_Pixel color = color_v3_to_assembly_pixel((v3){1, .9f, 0});
  assert(color.r != 1);
  
  r32 scene_time = ins->scene_time * 8;
  r32 dots_per_second = 5;
  u32 iter_cap = scene_time * dots_per_second;
  for (u32 row_i = 0; row_i < scene.data_len; row_i++)
  {
    Incenter_Data_Row row = scene.data[row_i];
    Assembly_Strip strip = strips.strips[row.id];
    u32 led_max = strip.pixels_len;
    u32 led_min = strip.pixels_len * (1 - row.prop);
    u32 led_range = led_max - led_min;
    
    Random_Series rs = random_series_create(city_hashes[row.id]);
    u32 city_iter = min(iter_cap, led_range);
    for (u32 i = 0; i < city_iter; i++)
    {
      u32 led_first = (random_series_next(&rs) % led_range) + led_min;
      u32 led = led_first;
      u32 led_index = 0;
      do {
        led = ((led + 1) % led_range) + led_min;
        if (led == led_first) break;
        led_index = strip.pixels[led];        
      } while (pixels.pixels[led_index].r == 1);
      pixels.pixels[led_index] = color;
    }
  }
#if 0
  for (u32 city = 0; city < city_count; city++)
  {
    Random_Series rs = random_series_create(city_hashes[city]);
    u32 city_iter = min(city_iters[city], iter_cap);
    Assembly_Strip strip = strips.strips[city + 1];
    for (u32 i = 0; i < city_iter; i++)
    {
      u32 led = random_series_next(&rs) % strip.pixels_cap;
      u32 led_index = strip.pixels[led];
      pixels.pixels[led_index] = color;
    }
  }      
#endif
}

void
pattern_scene_input(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  Incenter_Scene scene = ins->scenes[ins->scene_at];
  Assembly_Strip brc_strip = strips.strips[city_black_rock];
  
  pattern_color(pixels, strips, 0, 0, 0);
  secondary_pattern_twinkle(pixels, strips, ins);
  
  // black out whole strip
  for (u32 led_i = 0; led_i < brc_strip.pixels_len; led_i++)
  {
    u32 led = brc_strip.pixels[led_i];
    pixels.pixels[led] = (Assembly_Pixel){0,0,0};
  }
  
  u32 on_range_start = 0;
  u32 on_range_stop = 0;
  switch (scene.kind) 
  {
    case Incenter_SceneKind_Information:
    {
    } break;
    
    case Incenter_SceneKind_YesOrNo:
    {
      u32 middle = brc_strip.pixels_len / 2;
      on_range_start = ins->input_option == 0 ? 0      : middle;
      on_range_stop  = ins->input_option == 0 ? middle : brc_strip.pixels_len;
    } break;
    
    case Incenter_SceneKind_ThreeOption:
    {
      u32 one_third = brc_strip.pixels_len / 3;
      u32 two_thirds = one_third * 2;
      u32 top = brc_strip.pixels_len;
      switch (ins->input_option) {
        case 0: { on_range_start = 0;          on_range_stop = one_third;  } break;
        case 1: { on_range_start = one_third;  on_range_stop = two_thirds; } break;
        case 2: { on_range_start = two_thirds; on_range_stop = top;        } break;
      }
    } break;
    
    case Incenter_SceneKind_SlidingScale:
    {
      on_range_start = 0;
      on_range_stop = (r32)brc_strip.pixels_len * ins->input_pct;
    } break;
    
    invalid_default_case;
  }
  
  for (u32 led_i = on_range_start; led_i < on_range_stop; led_i++)
  {
    u32 led = brc_strip.pixels[led_i];
    pixels.pixels[led] = (Assembly_Pixel){255,255,255};
  }
}

/////////////////////////////////////////////
// Transition Patterns

void
pattern_sun_transition(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins, r32 radius_start, r32 radius_end, u32 back_scene_mode)
{
  r32 st = (r32)ins->transition_time;
  r32 shrink_duration = INCENTER_TRANSITION_SUN_REVEAL_DURATION;
  r32 shrink_progress_pct = (st / shrink_duration);
  shrink_progress_pct = clamp(0, shrink_progress_pct, 1);
  r32 radius = lerp(radius_start, shrink_progress_pct, radius_end);
  r32 radius2 = radius * radius;
  
  //Assembly_Pixel color_shell = { 255, 255, 255 };
  Assembly_Pixel color_void  = { 0,   0,   0   };
  
  r32 falloff = INCENTER_FEET(1);
  r32 falloff2 = falloff * falloff;
  
  Incenter_Scene    back_scene = ins->scenes[ins->scene_at];
  Incenter_Pattern* back_pattern = 0;
  if (back_scene_mode < Incenter_SceneMode_Count) {
    back_pattern = back_scene.patterns[back_scene_mode];
  } else if (back_scene_mode == Incenter_SceneMode_Input) {
    back_pattern = pattern_scene_input;
  } 
  assert(back_pattern != 0);
  
  back_pattern(pixels, strips, ins);
  
  for (u32 j = 0; j < pixels.len; j++)
  {
    v4 pos = pixels.positions[j];
    v4 p = incenter_pos_to_unit(pos);
    r32 r2 = HMM_LengthSquaredVec3(pos.xyz);
    r32 b = sdf_sphere_hull2_d(radius2, 3, r2);
    Assembly_Pixel back_color = pixels.pixels[j];
    if (r2 > radius2) {      
      back_color = sun(pos.xyz, r2, color_void, st);
    }
    
    v3 color_shell_v3 = {
      .x = 0.5f + 0.5f * sinf(p.x * r32_tau * 4.313f + tt * 1.3f),
      .y = 0.5f + 0.5f * cosf(0.2314f + p.y * r32_tau * 3.915f + tt),
      .z = 0.2f + 0.8f * p.z,
    };
    Assembly_Pixel color_shell = color_v3_to_assembly_pixel(color_shell_v3);
    pixels.pixels[j] = assembly_pixel_blend(back_color, color_shell, b);
  }
}

void
pattern_sun_transition_shrink(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  r32 radius_start = INCENTER_FEET(15);
  r32 radius_end   = INCENTER_FEET(0);
  u32 scene_mode = Incenter_SceneMode_Passive;
  pattern_sun_transition(pixels, strips, ins, radius_start, radius_end, scene_mode);
}

void
pattern_sun_transition_grow(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, Incenter_State* ins)
{
  r32 radius_start = INCENTER_FEET(0);
  r32 radius_end   = INCENTER_FEET(40);
  u32 scene_mode = Incenter_SceneMode_Input;
  if (ins->scene_at == Incenter_Scene_WelcomeHome ||
      ins->scene_at == Incenter_Scene_AnyoneHome) {
    scene_mode = Incenter_SceneMode_Passive;
  }
  pattern_sun_transition(pixels, strips, ins, radius_start, radius_end, scene_mode);
}
