void
secondary_pattern_solid_color(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips, v3 color)
{
  Assembly_Pixel color_final = color_v3_to_assembly_pixel(color);
  Assembly_Strip strip = strips.strips[city_secondary_first];
  for (u32 led = 0; led < strip.pixels_len; led++)
  {
    u32 led_index = strip.pixels[led];
    pixels.pixels[led_index] = color_final;
  }
}

// Splash
Random_Series splash_rand = { .last_value = 5336 };
#define SPLASH_CAP 15
v3  splash_pos[SPLASH_CAP];
r32 splash_age[SPLASH_CAP] = { 0, .31f, .87f, .9f, 1.23f, 1.23, 1.212, 3.121, 4.134, 1.312, 3.31, 2.1, 3.12 };
v3  splash_col[SPLASH_CAP];
r32 splash_duration = 5;

v3 splash_colors[5] = {
  {1, 1, 1}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 1, 1},
};
u32 next_color = 0;

void
secondary_pattern_splash(Assembly_Pixel_Buffer pixels, Assembly_Strip_Array strips)
{
  Assembly_Strip strip = strips.strips[city_secondary_first];

  // Clear the last frame
  Assembly_Pixel black = color_v3_to_assembly_pixel((v3){0, 0, 0});
  for (u32 led = 0; led < strip.pixels_len; led++)
  {
    u32 led_index = strip.pixels[led];
    pixels.pixels[led_index] = black;
  }

  for (u32 i = 0; i < SPLASH_CAP; i++) 
  {
    // Update the splash
    splash_age[i] += 1.0f / 30.0f;
    if (splash_age[i] >= splash_duration) {
      u32 city_i = city_secondary_first + (random_series_next(&splash_rand) % (city_secondary_count - city_secondary_first));
      assert(city_i >= city_secondary_first && city_i < city_secondary_count);
      Incenter_City_Desc city = city_descs[city_i];
      splash_pos[i] = incenter_latlng_to_cartesian(city.lat, city.lon, 1).xyz;
      splash_age[i] = random_series_next_unilateral(&splash_rand) * -2;
      splash_col[i] = splash_colors[next_color];
      next_color = (next_color + 1) % 5;
    }

    if (splash_age[i] < 0) continue;

    // Apply to lights
    r32 pct = clamp(0, (splash_age[i] / splash_duration), 1);
    r32 radius = pct * .8f;
    r32 radius2 = radius * radius;
    r32 b = pm_smoothstep_r32(1 - pct);
    for (u32 led = 0; led < strip.pixels_len; led++)
    {
      u32 led_index = strip.pixels[led];
      v4 p = pixels.positions[led_index];
      v4 pp = incenter_pos_to_unit(p);
      r32 d2 = HMM_LengthSquaredVec3(HMM_SubtractVec3(pp.xyz, splash_pos[i]));
      if (d2 <= radius2) {
        pixels.pixels[led_index] = assembly_pixel_add(
          pixels.pixels[led_index],
          color_v3_to_assembly_pixel_faded(splash_col[i], b)
        );
      }
    }
  }
}