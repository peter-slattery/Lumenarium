
static String sculpture_shd_vert_win32 = lit_str(
                                                 "#version 330 core\n"
                                                 "layout (location = 0) in vec3 a_pos;\n"
                                                 "layout (location = 1) in vec2 a_uv;\n"
                                                 "out vec2 uv;\n"
                                                 "uniform mat4 proj;\n"
                                                 "void main(void) {\n"
                                                 "  gl_Position = proj * vec4(a_pos, 1.0);\n"
                                                 "  uv = a_uv;\n"
                                                 "}"
                                                 );

static String sculpture_shd_vert_wasm = lit_str(
                                                "precision highp float;\n"
                                                "attribute vec3 a_pos;\n"
                                                "attribute vec2 a_uv;\n"
                                                "varying vec2 uv;\n"
                                                "uniform mat4 proj;\n"
                                                "void main(void) {\n"
                                                "  gl_Position = proj * vec4(a_pos, 1.0);\n"
                                                "  uv = a_uv;\n"
                                                "}"
                                                );

static String sculpture_shd_frag_win32 = lit_str(
                                                 "#version 330 core\n"
                                                 "in vec2 uv;\n"
                                                 "out vec4 FragColor;\n"
                                                 "uniform sampler2D texture;\n"
                                                 "void main(void) {\n"
                                                 "  FragColor = texture(texture, uv);\n"
                                                 "}"
                                                 );

static String sculpture_shd_frag_wasm = lit_str(
                                                "precision highp float;\n"
                                                "varying vec2 uv;\n"
                                                "uniform sampler2D texture;\n"
                                                "void main(void) {\n"
                                                "  //gl_FragColor = texture2D(texture, uv) * color;\n"
                                                "  gl_FragColor = vec4(uv.x,1,uv.y,1);\n"
                                                "}"
                                                );

internal void
ed_sculpture_visualizer_init(App_State* state)
{
  Editor* editor = state->editor;
  
  
#if defined(PLATFORM_win32)
  String vert = sculpture_shd_vert_win32;
  String frag = sculpture_shd_frag_win32;
#elif defined(PLATFORM_wasm)
  String vert = sculpture_shd_vert_wasm;
  String frag = sculpture_shd_frag_wasm;
#endif
  
  String attrs[] = { lit_str("a_pos"), lit_str("a_uv") };
  String uniforms[] = { lit_str("proj") };
  editor->sculpture_shd = platform_shader_create(
                                                 vert, frag, attrs, 2, uniforms, 1
                                                 );
}

r32 cam_theta = 0;

internal void
ed_sculpture_visualizer(App_State* state)
{
  Editor* ed = state->editor;
  
  // Set the viewport to the current layout's region so that the sculpture
  // never overlaps any other ui elements
  UI_Layout l = *ed->ui.layout;
  v2 view_dim = l.bounds_max - l.bounds_min;
  glViewport(
             (s32)l.bounds_min.x, 
             (s32)l.bounds_min.y, 
             (s32)view_dim.x, 
             (s32)view_dim.y
             );
  
  // TODO(PS): TEMPORARY CAMERA CODE
  cam_theta += 0.05f;
  v3 camera_pos = v3{sinf(cam_theta) * 50, 0, cosf(cam_theta) * 50};
  r32 aspect = view_dim.x / view_dim.y;
  m44 proj = HMM_Perspective(45.0, aspect, 0.01f, 500);
  m44 view = HMM_LookAt(camera_pos, v3{0,0,0}, v3{0,1,0});
  
  platform_shader_bind(ed->sculpture_shd);
  platform_set_uniform(ed->sculpture_shd, 0, proj * view);
  platform_texture_bind(ed->sculpture_tex);
  platform_geometry_bind(ed->sculpture_geo);
  platform_geometry_draw(ed->sculpture_geo, ed->sculpture_geo.indices_len);
  
  // reset the viewport for all other rendering
  glViewport(0, 0, (s32)ed->window_dim.x, (s32)ed->window_dim.y);
}