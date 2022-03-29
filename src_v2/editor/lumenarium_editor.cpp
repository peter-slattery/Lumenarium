
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
                                shader_code_vert, shader_code_frag, attribs, 2
                                );
  
  platform_vertex_attrib_pointer(*geo, *shd, 4, shd->attrs[0], 6, 0);
  platform_vertex_attrib_pointer(*geo, *shd, 2, shd->attrs[1], 6, 4);
  
  *tex = platform_texture_create((u8*)pix, 4, 4, 4);
}

internal void
ed_init(App_State* state)
{
  Editor* editor = allocator_alloc_struct(permanent, Editor);
  state->editor = editor;
  make_quad(&editor->renderer.geo, &editor->renderer.shd, &editor->renderer.tex);
}

internal void
ed_frame_prepare(App_State* state)
{
  
}

internal void
ed_frame(App_State* state)
{
  for (u32 i = 0; i < 16; i++)
  {
    if (i % 2 == 1) continue;
    pix[i] += 1;
  }
  platform_texture_update(state->editor->renderer.tex, (u8*)pix, 4, 4, 4);
  
  edr_render(state);
}

internal void
ed_cleanup(App_State* state)
{
  
}

