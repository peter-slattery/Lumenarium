#include "lumenarium_editor_sculpture_visualizer_shaders.h"

u32 fbo;
Texture fbo_tex_c;
u32 fbo_rbo;
Geometry_Buffer fs_quad;
Shader fs_shd;

internal void
ed_sculpture_visualizer_init(App_State* state)
{
  Editor* editor = state->editor;
  
  String vert = xplatform_shader_program_get_vert(sculpture_shd);
  String frag = xplatform_shader_program_get_frag(sculpture_shd);
  
  String attrs[] = { lit_str("a_pos"), lit_str("a_uv") };
  String uniforms[] = { lit_str("proj") };
  editor->sculpture_shd = shader_create(vert, frag, attrs, 2, uniforms, 1);

  {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    v2 wds = HMM_MultiplyVec2(editor->window_dim, editor->content_scale);
    s32 w = (s32)wds.x;
    s32 h = (s32)wds.y;
    fbo_tex_c = texture_create(
      (Texture_Desc){
        .w = w, .h = h, .s = w,
        .min_filter = GL_LINEAR,
        .mag_filter = GL_LINEAR,
        .fmt_internal = GL_RGBA,
        .fmt_data = GL_RGBA,
      },
      0
    );
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex_c.id, 0); 

    glGenRenderbuffers(1, &fbo_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, fbo_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fbo_rbo);

    u32 status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    u32 complete = GL_FRAMEBUFFER_COMPLETE;
    if (status != complete)
    {
      #define GL_ENUM_ERROR_CASE(e, msg) case e: { printf("Error: %s - %s\n", #e, msg); } break
      switch (status) {
        GL_ENUM_ERROR_CASE(GL_FRAMEBUFFER_UNDEFINED, "");
        GL_ENUM_ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT, "");
        GL_ENUM_ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT, "");
        GL_ENUM_ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER, "");
        GL_ENUM_ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER, "");
        GL_ENUM_ERROR_CASE(GL_FRAMEBUFFER_UNSUPPORTED, "");
        GL_ENUM_ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE, "");
        //GL_ENUM_ERROR_CASE(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS, "");
        default: {
          os_gl_no_error();
        } break;
      }
      printf("Error: unable to complete framebuffer\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    String vert = xplatform_shader_program_get_vert(sculpture_comp_shd);
    String frag = xplatform_shader_program_get_frag(sculpture_comp_shd);
    String shd_a[] = { lit_str("a_pos"), lit_str("a_uv") };
    String shd_u[] = { lit_str("proj") };
    fs_shd = shader_create(vert, frag, shd_a, 2, shd_u, 1);

    fs_quad = unit_quad_create();
    vertex_attrib_pointer(fs_quad, fs_shd, 3, fs_shd.attrs[0], 5, 0);
    vertex_attrib_pointer(fs_quad, fs_shd, 2, fs_shd.attrs[1], 5, 3);
  }
}

r32 cam_theta = 0;
u32 offset = 0;

internal void
ed_sculpture_visualizer(App_State* state)
{
  Editor* ed = state->editor;
  
#define SCULPTURE_VIZ_BLOOM 0
#if SCULPTURE_VIZ_BLOOM
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  os_gl_no_error();
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
#endif

  // Set the viewport to the current layout's region so that the sculpture
  // never overlaps any other ui elements
  UI_Layout l = *ed->ui.layout;
  v2 view_dim = HMM_SubtractVec2(l.bounds_max, l.bounds_min);
  v2 view_min = l.bounds_min;
  v2 view_max = l.bounds_max;
  v2 view_min_scaled = HMM_MultiplyVec2(view_min, ed->content_scale);
  v2 view_dim_scaled = HMM_MultiplyVec2(view_dim, ed->content_scale);
  glViewport(
    (s32)view_min_scaled.x, 
    (s32)view_min_scaled.y, 
    (u32)view_dim_scaled.x, 
    (u32)view_dim_scaled.y
  );
  
  // TODO(PS): TEMPORARY CAMERA CODE
  cam_theta += 0.01f;
  r32 cam_r = 50;
  //v3 camera_pos = (v3){sinf(cam_theta) * cam_r, 25, cosf(cam_theta) * cam_r};
  v3 camera_pos = (v3){ 0, -4.9, -cam_r };
  r32 aspect = view_dim.x / view_dim.y;
  m44 proj = HMM_Perspective(72.0, aspect, 0.01f, 500);
  m44 view = HMM_LookAt(camera_pos, (v3){0,0,0}, (v3){0,1,0});
  
  shader_bind(ed->sculpture_shd);
  set_uniform(ed->sculpture_shd, 0, HMM_MultiplyMat4(proj, view));
  texture_bind(ed->sculpture_tex);
  geometry_bind(ed->sculpture_geo);

  u32 i = 1008;
  u32 j = 2868;
  u32 k = ed->sculpture_geo.indices_len;
  u32 h = (i * 6) + 3;
  geometry_drawi(ed->sculpture_geo, k, 0);
  
  // reset the viewport for all other rendering
#if SCULPTURE_VIZ_BLOOM
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  v2 wds = HMM_MultiplyVec2(ed->window_dim, ed->content_scale);
  glViewport(0, 0, (s32)wds.x, (s32)wds.y);

  m44 ortho = HMM_Orthographic(0, ed->window_dim.x, ed->window_dim.y, 0, 0.01f, 200.0f);
  m44 scale = HMM_Scale((v3){ed->window_dim.x / 2, -ed->window_dim.y / 2, 100});
  m44 pos = HMM_Translate((v3){ed->window_dim.x / 2, ed->window_dim.y / 2, -99});
  m44 model = HMM_MultiplyMat4(pos, scale);
  m44 mvp = HMM_MultiplyMat4(ortho, model);

  shader_bind(fs_shd);
  set_uniform(fs_shd, 0, mvp);
  texture_bind(fbo_tex_c);
  geometry_bind(fs_quad);
  geometry_drawi(fs_quad, 6, 0);
#endif // SCULPTURE_VIZ_BLOOM
}