
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
ed_init(App_State* state)
{
  Editor* editor = allocator_alloc_struct(permanent, Editor);
  state->editor = editor;
  state->editor->ui = ui_create(4096, 4096, state->input_state, permanent);
  
  // make the default quad for us to draw with
  // TODO(PS): this might be unnecessary with the per-frame buffer we use now
  make_quad(&editor->renderer.geo, &editor->renderer.shd, &editor->renderer.tex);
  
  platform_file_async_read(lit_str("data/font.ttf"), ed_load_font_cb);
  
}

internal void
ed_frame_prepare(App_State* state)
{
  ui_frame_prepare(&state->editor->ui, state->editor->window_dim);
}

internal void
ed_frame(App_State* state)
{
  edr_render_begin(state);
  ui_draw(&state->editor->ui);
  edr_render(state);
}

internal void
ed_cleanup(App_State* state)
{
  
}

