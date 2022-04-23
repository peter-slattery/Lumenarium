internal void
ed_load_font_cb(File_Async_Job_Args result, u8* user_data)
{
  App_State* state = (App_State*)user_data;
  UI* ui = &state->editor->ui;
  
  u8* f = result.data.base;
  stbtt_fontinfo font;
  if (!stbtt_InitFont(&font, f, stbtt_GetFontOffsetForIndex(f, 0)))
  {
    invalid_code_path;
  }
  
  r32 scale = stbtt_ScaleForPixelHeight(&font, 16.0f);
  s32 ascent, descent, line_gap;
  stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);
  ui->font_ascent = (r32)ascent * scale;
  ui->font_descent = (r32)descent * scale;
  ui->font_line_gap = (r32)line_gap * scale;
  if (ui->font_line_gap == 0) ui->font_line_gap = 5;
  
  ui->font_texture_scale = 2;
  scale *= ui->font_texture_scale;
  String c = lit_str("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-=_+[]{}\\|;:'\",<.>/?`~");
  for (u64 i = 0; i < c.len; i++)
  {
    s32 w, h, xoff, yoff;
    u32 id = (u32)c.str[i];
    u8* bp  = stbtt_GetCodepointBitmap(&font, 0, scale, (char)c.str[i], &w, &h, &xoff, &yoff);
    s32 x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBoxSubpixel(&font, (char)c.str[i], scale, scale, 0, 0, &x0, &y0, &x1, &y1);
    
    v2 offset = (v2){ 0, (r32)y0 / ui->font_texture_scale };
    texture_atlas_register(&state->editor->ui.atlas, bp, (u32)w, (u32)h, id, offset, TextureAtlasRegistration_PixelFormat_Alpha);
    stbtt_FreeBitmap(bp, 0);
  }
  
  Texture_Atlas_Sprite m_sprite = texture_atlas_sprite_get(&state->editor->ui.atlas, (u32)'m');
  ui->font_space_width = (r32)(m_sprite.max_x - m_sprite.min_x);
  
  texture_update(ui->atlas_texture, ui->atlas.pixels, 1024, 1024, 1024);
}

internal void
ed_draw_panel(u8* user_data, BSP_Node_Id id, BSP_Node node, BSP_Area area)
{
  App_State* state = (App_State*)user_data;
  UI* ui = &state->editor->ui;
  scratch_get(scratch);
  
  UI_Layout title_layout = {};
  ui_layout_set_row_info(ui, &title_layout);
  title_layout.bounds_min = (v2){ area.min.x, area.max.y - title_layout.row_height };
  title_layout.bounds_max = area.max;
  title_layout.at = title_layout.bounds_min;
  
  UI_Layout panel_layout = {};
  ui_layout_set_row_info(ui, &panel_layout);
  panel_layout.bounds_min = area.min;
  panel_layout.bounds_max = (v2){ area.max.x, title_layout.bounds_max.y };
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
  bg.style.color_bg = (v4){.4f,.4f,.4f,1};
  bg.style.sprite = WHITE_SPRITE_ID;
  bg.string = string_f(scratch.a, "%.*s_%u_title_bg", str_varg(title), id.value);
  bg.p_min = title_layout.bounds_min;
  bg.p_max = title_layout.bounds_max;
  UI_Widget_Result r = ui_widget_push(ui, bg);
  ui_layout_row_begin(&title_layout, 4);
  {
    ui_textc(ui, title, BLACK_V4);
  }
  ui_layout_row_end(&title_layout);
  ui_widget_pop(ui, r.id);
  ui_layout_pop(ui);

  scratch_release(scratch);
}

Geometry_Buffer b;
Shader shd;
Texture tex;

internal void
ed_init(App_State* state, Editor_Desc* desc)
{
  lumenarium_env_validate();

  Editor* editor = allocator_alloc_struct(permanent, Editor);
  zero_struct(*editor);
  state->editor = editor;
  editor->window_dim = desc->init_window_dim;
  editor->content_scale = desc->content_scale;
  editor->ui = ui_create(4096, 4096, state->input_state, permanent);
  editor->ui.draw_panel_cb = ed_draw_panel;
  editor->ui.draw_panel_cb_data = (u8*)state;
  //bsp_split(&editor->ui.panels, editor->ui.panels.root, 700, BSPSplit_YAxis, 0, 1);

  file_async_read(lit_str("data/font.ttf"), ed_load_font_cb);
  
  r32 w = 1400;
  r32 h = 700;
  r32 z = -1;
  r32 tri[] = {
    0, 0, 0,  0, 0,
    w, 0, z,  1, 0,
    w, h, z,  1, 1,
    0, h, z,  0, 1,
  };
  u32 indices[] = { 0, 1, 2, 0, 2, 3 };

  String shd_v = lit_str(
    "#version 330 core\n"
    "layout (location = 0) in vec3 pos;\n"
    "layout (location = 1) in vec2 auv;\n"
    "out vec2 uv;\n"
    "uniform mat4 proj;\n"
    "void main(void) { \n"
    "  gl_Position = proj * vec4(pos, 1); \n"
    "  uv = auv;\n"
    "}\n"
  );
  String shd_f = lit_str(
    "#version 330 core\n"
    "in vec2 uv;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D tex;\n"
    "void main(void) { FragColor = texture(tex, uv) * vec4(1,1,1,1); }"
  );
  String shd_a[] = { lit_str("pos"), lit_str("auv") };
  String shd_u[] = { lit_str("proj") };
  b = geometry_buffer_create(
    tri, sizeof(tri) / sizeof(r32),
    indices, 6
  );
  shd = shader_create(shd_v, shd_f, shd_a, 2, shd_u, 1);
  vertex_attrib_pointer(b, shd, 3, shd.attrs[0], 5, 0);
  vertex_attrib_pointer(b, shd, 2, shd.attrs[1], 5, 3);

  u32 tex_pix[] = { 0xFFFFFFFF, 0xFF00FFFF, 0xFFFFFF00, 0xFFFF00FF };
  tex = texture_create(texture_desc_default(2, 2), (u8*)tex_pix);

  ed_sculpture_visualizer_init(state);
  lumenarium_env_validate();
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
  return (u8*)pixels;
}

internal void
ed_sculpture_updated(App_State* state, r32 scale, r32 led_size)
{
  lumenarium_env_validate();
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
  pixels_dim = round_up_to_pow2_u32(pixels_dim);
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
  geo_vertex_buffers_validate(&geo.buffer_vertex);

  r32 r = led_size;
  u32 pixels_created = 0;
  for (u32 a = 0; a < state->assemblies.len; a++)
  {
    Assembly_Pixel_Buffer pixels = state->assemblies.pixel_buffers[a];
    for (u32 p = 0; p < pixels.len; p++)
    {
      v3 c = pixels.positions[p].xyz;
      c = HMM_MultiplyVec3f(c, scale);
      u32 pixel_count = pixels_created++;
      u32 pixel_x = pixel_count % pixels_dim;
      u32 pixel_y = pixel_count / pixels_dim;
      
      r32 texel_x_min = (r32)pixel_x / (r32)pixels_dim;
      r32 texel_y_min = (r32)pixel_y / (r32)pixels_dim;
      r32 texel_x_max = texel_x_min + (texel_dim / 2);
      r32 texel_y_max = texel_y_min + (texel_dim / 2);
      
      v2 t0 = (v2){texel_x_max, texel_y_max};
      v2 t1 = (v2){texel_x_max, texel_y_max};
      v2 t2 = (v2){texel_x_max, texel_y_max};
      v2 t3 = (v2){texel_x_max, texel_y_max};

      v3 p0 = HMM_AddVec3(c, (v3){ -r, -r, 0 });
      v3 p1 = HMM_AddVec3(c, (v3){  r, -r, 0 });
      v3 p2 = HMM_AddVec3(c, (v3){  r,  r, 0 });
      v3 p3 = HMM_AddVec3(c, (v3){ -r,  r, 0 });
      geo_quad_buffer_builder_push_vt(&geo, p0, p1, p2, p3, t0, t1, t2, t3);
    }
  }
  geo_vertex_buffers_validate(&geo.buffer_vertex);

  if (ed->sculpture_geo.indices_len != 0)
  {
    invalid_code_path;
    // TODO(PS): destroy the old geometry buffer or update it
  }
  ed->sculpture_geo = geometry_buffer_create(
    geo.buffer_vertex.values, 
    geo.buffer_vertex.len * geo.buffer_vertex.stride, 
    geo.buffer_index.values, 
    geo.buffer_index.len
  );
  vertex_attrib_pointer(ed->sculpture_geo, ed->sculpture_shd, 3, ed->sculpture_shd.attrs[0], 5, 0);
  vertex_attrib_pointer(ed->sculpture_geo, ed->sculpture_shd, 2, ed->sculpture_shd.attrs[1], 5, 3);
  
  // TODO(PS): make this have enough pixels for the sculpture
  // TODO(PS): map leds to pixels
  
  if (ed->sculpture_tex.desc.w != 0)
  { 
    invalid_code_path;
    // TODO(PS): destroy the old texture
  }
  
  u8* pixels = ed_leds_to_texture(state, &scratch, pixels_dim);
  Texture_Desc pixel_texture_desc = {
    .w = pixels_dim,
    .h = pixels_dim,
    .s = pixels_dim,
    .min_filter = GL_NEAREST,
    .mag_filter = GL_NEAREST,
    .fmt_internal = GL_RGBA,
    .fmt_data = GL_RGBA
  };
  ed->sculpture_tex = texture_create(pixel_texture_desc, pixels);

  scratch_release(scratch);
  lumenarium_env_validate();
}

internal void
ed_frame_prepare(App_State* state)
{
  lumenarium_env_validate();
  ui_frame_prepare(&state->editor->ui, state->editor->window_dim);
  lumenarium_env_validate();
}

internal void
ed_frame(App_State* state)
{
  lumenarium_env_validate();
  Editor* ed = state->editor;
  UI* ui = &ed->ui;
  
  {
    scratch_get(scratch);
    u32 w = ed->sculpture_tex.desc.w;
    u8* pixels = ed_leds_to_texture(state, &scratch, w);
    texture_update(ed->sculpture_tex, pixels, w, w, w);
    scratch_release(scratch);
  }

  edr_render_begin(state);
  ui_draw(&state->editor->ui);
  edr_render(state);
  lumenarium_env_validate();
}

internal void
ed_cleanup(App_State* state)
{
  lumenarium_env_validate();
}

