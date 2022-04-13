
static r32 z_ = 0;
static r32 r_ = 0.3f;
static r32 quad_verts[] = {
  -r_,  -r_,  z_,  1.0f,  0, 0,
  r_, -r_,  z_,  1.0f,    1, 0,
  r_, r_,  z_,  1.0f,     1, 1,
  -r_,  r_,  z_,  1.0f,   0, 1,
};

static u32 quad_indices[] = {
  0, 1, 2,
  0, 2, 3,
};

static String shader_code_vert_win32 = lit_str(
                                               "#version 330 core\n"
                                               "layout (location = 0) in vec4 coordinates;\n"
                                               "layout (location = 1) in vec2 uv;\n"
                                               "out vec2 tex_coord;\n"
                                               "void main(void) {\n"
                                               "  gl_Position = coordinates;\n"
                                               "  tex_coord = uv;\n"
                                               "}"
                                               );

static String shader_code_vert_wasm = lit_str(
                                              "precision highp float;\n"
                                              "attribute vec4 coordinates;\n"
                                              "attribute vec2 uv;\n"
                                              "varying vec2 tex_coord;\n"
                                              "void main(void) {\n"
                                              "  gl_Position = coordinates;\n"
                                              "  tex_coord = uv;\n"
                                              "}");

static String shader_code_frag_win32 = lit_str(
                                               "#version 330 core\n"
                                               "in vec2 tex_coord;\n"
                                               "out vec4 FragColor;\n"
                                               "uniform sampler2D texture;\n"
                                               "void main(void) {\n"
                                               "//  FragColor = vec4(1,tex_coord.x,tex_coord.y,1);\n"
                                               "  FragColor = texture(texture, tex_coord);\n"
                                               "}"
                                               );

static String shader_code_frag_wasm = lit_str(
                                              "precision highp float;\n"
                                              "varying vec2 tex_coord;\n"
                                              "uniform sampler2D texture;\n"
                                              "void main(void) {\n"
                                              "  gl_FragColor = texture2D(texture, tex_coord);\n"
                                              "  // vec4(1, tex_coord.x, tex_coord.y, 1);\n"
                                              "}");

static u32 pix[] = {
  0xFF000000, 0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF,
  0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF, 0xFF000000,
  0xFF000000, 0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF,
  0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF, 0xFF000000,
};

void make_quad(Platform_Geometry_Buffer* geo, Platform_Shader* shd, Platform_Texture* tex)
{
  // TODO(PS): TEMP
#if defined(PLATFORM_win32)
  String shader_code_vert = shader_code_vert_win32;
  String shader_code_frag = shader_code_frag_win32;
#elif defined(PLATFORM_wasm)
  String shader_code_vert = shader_code_vert_wasm;
  String shader_code_frag = shader_code_frag_wasm;
#endif
  
  *geo = platform_geometry_buffer_create(
                                         quad_verts, 24, quad_indices, 6
                                         );
  
  String attribs[] = { 
    lit_str("coordinates"),
    lit_str("uv"),
  };
  *shd = platform_shader_create(
                                shader_code_vert, shader_code_frag, attribs, 2, 0, 0
                                );
  
  platform_vertex_attrib_pointer(*geo, *shd, 4, shd->attrs[0], 6, 0);
  platform_vertex_attrib_pointer(*geo, *shd, 2, shd->attrs[1], 6, 4);
  
  *tex = platform_texture_create((u8*)pix, 4, 4, 4);
}

internal void
ed_load_font_cb(Platform_File_Async_Job_Args result, u8* user_data)
{
  App_State* state = (App_State*)user_data;
  UI* ui = &state->editor->ui;
  
  u8* f = result.data.base;
  stbtt_fontinfo font;
  if (!stbtt_InitFont(&font, f, stbtt_GetFontOffsetForIndex(f, 0)))
  {
    invalid_code_path;
  }
  
  r32 scale = stbtt_ScaleForPixelHeight(&font, 18.0f);
  s32 ascent, descent, line_gap;
  stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);
  ui->font_ascent = (r32)ascent * scale;
  ui->font_descent = (r32)descent * scale;
  ui->font_line_gap = (r32)line_gap * scale;
  if (ui->font_line_gap == 0) ui->font_line_gap = 5;
  
  String c = lit_str("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-=_+[]{}\\|;:'\",<.>/?`~");
  for (u64 i = 0; i < c.len; i++)
  {
    s32 w, h, xoff, yoff;
    u32 id = (u32)c.str[i];
    u8* bp  = stbtt_GetCodepointBitmap(&font, 0, scale, (char)c.str[i], &w, &h, &xoff, &yoff);
    s32 x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBoxSubpixel(&font, (char)c.str[i], scale, scale, 0, 0, &x0, &y0, &x1, &y1);
    
    v2 offset = v2{ 0, (r32)y0 };
    texture_atlas_register(&state->editor->ui.atlas, bp, (u32)w, (u32)h, id, offset, TextureAtlasRegistration_PixelFormat_Alpha);
    stbtt_FreeBitmap(bp, 0);
  }
  
  Texture_Atlas_Sprite m_sprite = texture_atlas_sprite_get(&state->editor->ui.atlas, (u32)'m');
  ui->font_space_width = (r32)(m_sprite.max_x - m_sprite.min_x);
  
  platform_texture_update(ui->atlas_texture, ui->atlas.pixels, 1024, 1024, 1024);
}

internal void
ed_draw_panel(u8* user_data, BSP_Node_Id id, BSP_Node node, BSP_Area area)
{
  App_State* state = (App_State*)user_data;
  UI* ui = &state->editor->ui;
  scratch_get(scratch);
  
  UI_Layout title_layout = {};
  ui_layout_set_row_info(ui, &title_layout);
  title_layout.bounds_min = v2{ area.min.x, area.max.y - title_layout.row_height };
  title_layout.bounds_max = area.max;
  title_layout.at = title_layout.bounds_min;
  
  UI_Layout panel_layout = {};
  ui_layout_set_row_info(ui, &panel_layout);
  panel_layout.bounds_min = area.min;
  panel_layout.bounds_max = v2{ area.max.x, title_layout.bounds_max.y };
  panel_layout.at = panel_layout.bounds_min;
  
  ui_layout_push(ui, &panel_layout);
  
  String title = {};
  switch (node.user_data)
  {
    case 0:
    {
      title = lit_str("None");
    } break;
    
    case 1:
    {
      ed_sculpture_visualizer(state);
      title = lit_str("Sculpture");
    } break;
    
    invalid_default_case;
  }
  ui_layout_pop(ui);
  
  ui_layout_push(ui, &title_layout);
  UI_Widget_Desc bg = {};
  bg.style.flags = UIWidgetStyle_Bg;
  bg.style.color_bg = v4{.4f,.4f,.4f,1};
  bg.style.sprite = WHITE_SPRITE_ID;
  bg.string = string_f(scratch.a, "%.*s_%u_title_bg", str_varg(title), id.value);
  bg.p_min = title_layout.bounds_min;
  bg.p_max = title_layout.bounds_max;
  UI_Widget_Result r = ui_widget_push(ui, bg);
  ui_layout_row_begin(&title_layout, 4);
  {
    ui_text(ui, title, BLACK_V4);
  }
  ui_layout_row_end(&title_layout);
  ui_widget_pop(ui, r.id);
  ui_layout_pop(ui);
}

internal void
ed_init(App_State* state)
{
  Editor* editor = allocator_alloc_struct(permanent, Editor);
  state->editor = editor;
  editor->ui = ui_create(4096, 4096, state->input_state, permanent);
  editor->ui.draw_panel_cb = ed_draw_panel;
  editor->ui.draw_panel_cb_data = (u8*)state;
  //bsp_split(&editor->ui.panels, editor->ui.panels.root, 700, BSPSplit_YAxis, 0, 1);
  
  // make the default quad for us to draw with
  // TODO(PS): this might be unnecessary with the per-frame buffer we use now
  make_quad(&editor->renderer.geo, &editor->renderer.shd, &editor->renderer.tex);
  
  platform_file_async_read(lit_str("data/font.ttf"), ed_load_font_cb);
  
  ed_sculpture_visualizer_init(state);
}

internal u8*
ed_leds_to_texture(App_State* state, Allocator_Scratch* scratch, u32 pixels_dim)
{
  u32 at = 0;
  u32* pixels = allocator_alloc_array(scratch->a, u32, pixels_dim * pixels_dim);
  for (u32 a = 0; a < state->assemblies.len; a++)
  {
    Assembly_Pixel_Buffer leds = state->assemblies.pixel_buffers[a];
    for (u32 p = 0; p < leds.len; p++)
    {
      Assembly_Pixel led = leds.pixels[p];
      u32 index = at++;
      pixels[index] = (
                       led.r << 0 |
                       led.g << 8 |
                       led.b << 16 |
                       0xFF << 24
                       );
    }
  }
#if 0
  for (u32 y = 0; y < pixels_dim; y++) 
  {
    for (u32 x = 0; x < pixels_dim; x++)
    {
      r32 rp = (r32)y / (r32)pixels_dim;
      r32 bp = (r32)x / (r32)pixels_dim;
      u8 rb = (u8)(255 * rp);
      u8 bb = (u8)(255 * bp);
      u32 c = (
               0xFF0000FF |
               (rb << 8) |
               (bb << 16)
               );
      pixels[(y * pixels_dim) + x] = c;
    }
  }
#endif
  return (u8*)pixels;
}

internal void
ed_sculpture_updated(App_State* state, r32 scale, r32 led_size)
{
  Editor* ed = state->editor;
  if (!ed) return;
  
  scratch_get(scratch);
  
  // NOTE(PS): we need to know the total number of leds in order to give them
  // texture coordinates
  u32 leds_count = 0;
  for (u32 a = 0; a < state->assemblies.len; a++)
  {
    Assembly_Pixel_Buffer pixels = state->assemblies.pixel_buffers[a];
    leds_count += pixels.len;
  }
  
  // round up to a texture whose sides are powers of two
  u32 pixels_dim = (u32)floorf(sqrtf((r32)leds_count));
  pixels_dim = round_up_to_pow2(pixels_dim);
  u32 pixels_count = pixels_dim * pixels_dim;
  r32 texel_dim = 1 / (r32)pixels_dim;
  
  // NOTE(PS): Rebuild the sculpture geometry to point to the new
  // sculpture.
  Geo_Vertex_Buffer_Storage storage = (
                                       GeoVertexBufferStorage_Position | 
                                       GeoVertexBufferStorage_TexCoord
                                       );
  u32 verts_cap = leds_count * 4;
  u32 indices_cap = leds_count * 6;
  Geo_Quad_Buffer_Builder geo = geo_quad_buffer_builder_create(scratch.a, verts_cap, storage, indices_cap);
  r32 r = led_size;
  
  u32 pixels_created = 0;
  for (u32 a = 0; a < state->assemblies.len; a++)
  {
    Assembly_Pixel_Buffer pixels = state->assemblies.pixel_buffers[a];
    for (u32 p = 0; p < pixels.len; p++)
    {
      v3 c = pixels.positions[p].xyz;
      c *= scale;
      
      u32 pixel_count = pixels_created++;
      u32 pixel_x = pixel_count % pixels_dim;
      u32 pixel_y = pixel_count / pixels_dim;
      r32 texel_x_min = (r32)pixel_x / (r32)pixels_dim;
      r32 texel_y_min = (r32)pixel_y / (r32)pixels_dim;
      r32 texel_x_max = texel_x_min + texel_dim;
      r32 texel_y_max = texel_y_min + texel_dim;
      
      v2 t0 = v2{texel_x_min, texel_y_min};
      v2 t1 = v2{texel_x_max, texel_y_min};
      v2 t2 = v2{texel_x_max, texel_y_max};
      v2 t3 = v2{texel_x_min, texel_y_max};
      
      v3 p0 = c + v3{ -r, -r, 0 };
      v3 p1 = c + v3{  r, -r, 0 };
      v3 p2 = c + v3{  r,  r, 0 };
      v3 p3 = c + v3{ -r,  r, 0 };
      geo_quad_buffer_builder_push(&geo, p0, p1, p2, p3, t0, t1, t2, t3);
    }
  }
  
  if (ed->sculpture_geo.indices_len != 0)
  {
    invalid_code_path;
    // TODO(PS): destroy the old geometry buffer or update it
  }
  ed->sculpture_geo = platform_geometry_buffer_create(
                                                      geo.buffer_vertex.values, 
                                                      geo.buffer_vertex.len, 
                                                      geo.buffer_index.values, 
                                                      geo.buffer_index.len
                                                      );
  
  platform_vertex_attrib_pointer(
                                 ed->sculpture_geo, ed->sculpture_shd, 3, ed->sculpture_shd.attrs[0], 5, 0
                                 );
  platform_vertex_attrib_pointer(
                                 ed->sculpture_geo, ed->sculpture_shd, 2, ed->sculpture_shd.attrs[1], 5, 3
                                 );
  
  // TODO(PS): make this have enough pixels for the sculpture
  // TODO(PS): map leds to pixels
  
  if (ed->sculpture_tex.w != 0)
  {
    invalid_code_path;
    // TODO(PS): destroy the old texture
  }
  
  u8* pixels = ed_leds_to_texture(state, &scratch, pixels_dim);
  ed->sculpture_tex = platform_texture_create(pixels, pixels_dim, pixels_dim, pixels_dim);
}

internal void
ed_frame_prepare(App_State* state)
{
  ui_frame_prepare(&state->editor->ui, state->editor->window_dim);
}

internal void
ed_frame(App_State* state)
{
  Editor* ed = state->editor;
  UI* ui = &ed->ui;
  
  {
    scratch_get(scratch);
    u32 w = ed->sculpture_tex.w;
    u8* pixels = ed_leds_to_texture(state, &scratch, w);
    platform_texture_update(ed->sculpture_tex, pixels, w, w, w);
  }
  
  edr_render_begin(state);
  ui_draw(&state->editor->ui);
  edr_render(state);
}

internal void
ed_cleanup(App_State* state)
{
  
}

