// Color Utils

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