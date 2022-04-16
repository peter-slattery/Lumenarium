#if !defined(LUMENARIUM_EDITOR_GRAPHICS_H)
#define LUMENARIUM_EDITOR_GRAPHICS_H

#define os_gl_no_error() os_gl_no_error_((char*)__FILE__, (u32)__LINE__)
void os_gl_no_error_(char* file, u32 line);

#define GL_NULL (u32)0

#define SHADER_MAX_ATTRS 8
#define SHADER_ATTR_LAST (u32)(1 << 31)
typedef struct Shader Shader;
struct Shader 
{ 
  u32 id; 
  u32 attrs[SHADER_MAX_ATTRS];
  u32 uniforms[SHADER_MAX_ATTRS];
};

typedef struct XPlatform_Shader_Program_Src XPlatform_Shader_Program_Src;
struct XPlatform_Shader_Program_Src
{
  String win32_vert;
  String win32_frag;

  String osx_vert;
  String osx_frag;

  String wasm_vert;
  String wasm_frag;
};

String xplatform_shader_program_get_vert(XPlatform_Shader_Program_Src src);
String xplatform_shader_program_get_frag(XPlatform_Shader_Program_Src src);

typedef struct Geometry_Buffer Geometry_Buffer;
struct Geometry_Buffer 
{
  u32 buffer_id_vao;
  u32 buffer_id_vertices;
  u32 buffer_id_indices;
  u32 vertices_len;
  u32 indices_len;
};

typedef struct Texture Texture;
struct Texture
{
  u32 id;
  
  u32 w, h, s;
};

typedef struct Graphics_Frame_Desc Graphics_Frame_Desc;
struct Graphics_Frame_Desc
{
  v4 clear_color;
  v2 viewport_min;
  v2 viewport_max;
};

void frame_begin(Graphics_Frame_Desc desc);
void frame_clear();

// Geometry
Geometry_Buffer geometry_buffer_create(r32* vertices, u32 vertices_len, u32* indices, u32 indices_len);
Shader shader_create(String code_vert, String code_frag, String* attribs, u32 attribs_len, String* uniforms, u32 uniforms_len);
void geometry_buffer_update(Geometry_Buffer* buffer, r32* verts, u32 verts_offset, u32 verts_len, u32* indices, u32 indices_offset, u32 indices_len);

// Shaders
void geometry_bind(Geometry_Buffer geo);
void shader_bind(Shader shader);
void geometry_drawi(Geometry_Buffer geo, u32 indices);
void geometry_draw(Geometry_Buffer geo);
void vertex_attrib_pointer(Geometry_Buffer geo, Shader shader, u32 count, u32 attr_index, u32 stride, u32 offset);
void set_uniform(Shader shader, u32 index, m44 u);

// Textures
Texture texture_create(u8* pixels, u32 width, u32 height, u32 stride);
void texture_bind(Texture tex);
void texture_update(Texture tex, u8* new_pixels, u32 width, u32 height, u32 stride);

//////////////////////////////////////////
//////////////////////////////////////////
//    IMPLEMENTATION
//////////////////////////////////////////
//////////////////////////////////////////

Geometry_Buffer 
geometry_buffer_create(
  r32* vertices, u32 vertices_len, 
  u32* indices, u32 indices_len
){
  Geometry_Buffer result = {};
  
  gl.glGenVertexArrays(1, &result.buffer_id_vao);
  os_gl_no_error();
  
  GLuint buffers[2];
  gl.glGenBuffers(2, (GLuint*)buffers);
  os_gl_no_error();
  
  result.buffer_id_vertices = buffers[0];
  result.buffer_id_indices = buffers[1];
  
  // Vertices
  gl.glBindVertexArray(result.buffer_id_vao);
  gl.glBindBuffer(GL_ARRAY_BUFFER, result.buffer_id_vertices);
  os_gl_no_error();
  
  gl.glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * vertices_len, vertices, GL_STATIC_DRAW);
  os_gl_no_error();
  result.vertices_len = vertices_len;
  
  // Indices
  gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.buffer_id_indices);
  os_gl_no_error();
  
  gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * indices_len, indices, GL_STATIC_DRAW);
  os_gl_no_error();
  result.indices_len = indices_len;
  
  gl.glBindBuffer(GL_ARRAY_BUFFER, GL_NULL);
  gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_NULL);
  
  return result;
}

void 
geometry_buffer_update(
  Geometry_Buffer* buffer, 
  r32* verts, 
  u32 verts_offset, 
  u32 verts_len, 
  u32* indices, 
  u32 indices_offset, 
  u32 indices_len
){
  gl.glBindVertexArray(buffer->buffer_id_vao);
  gl.glBindBuffer(GL_ARRAY_BUFFER, buffer->buffer_id_vertices);
  os_gl_no_error();
  
  if (verts_len > buffer->vertices_len)
  {
    // NOTE(PS): this is because we're going to delete the old buffer and
    // create a new one. In order to do that and not lose data, the update
    // function needs to have been passed all the buffer's data
    assert(verts_offset == 0); 
    gl.glBufferData(GL_ARRAY_BUFFER, verts_len * sizeof(r32), (void*)verts, GL_STATIC_DRAW);
  }
  else
  {
    gl.glBufferSubData(GL_ARRAY_BUFFER, verts_offset * sizeof(r32), verts_len * sizeof(r32), (void*)verts);
  }
  os_gl_no_error();
  
  gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->buffer_id_indices);
  os_gl_no_error();
  if (indices_len > buffer->indices_len)
  {
    // NOTE(PS): this is because we're going to delete the old buffer and
    // create a new one. In order to do that and not lose data, the update
    // function needs to have been passed all the buffer's data
    assert(indices_offset == 0); 
    gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_len * sizeof(u32), (void*)indices, GL_STATIC_DRAW);
  }
  else
  {
    gl.glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indices_offset * sizeof(u32), indices_len * sizeof(u32), (void*)indices);
  }
  os_gl_no_error();
}

Shader
shader_create(String code_vert, String code_frag, String* attrs, u32 attrs_len, String* uniforms, u32 uniforms_len){
  Shader result = {};
  
  GLuint shader_vert = gl.glCreateShader(GL_VERTEX_SHADER);
  s32* code_vert_len = (s32*)&code_vert.len;
  gl.glShaderSource(shader_vert, 1, (const char**)&code_vert.str, code_vert_len);
  gl.glCompileShader(shader_vert);
  { // errors
    GLint shader_vert_compiled;
    gl.glGetShaderiv(shader_vert, GL_COMPILE_STATUS, &shader_vert_compiled);
    if (!shader_vert_compiled)
    {
      GLsizei log_length = 0;
      GLchar message[1024];
      gl.glGetShaderInfoLog(shader_vert, 1024, &log_length, message);
      printf("GLSL Error: %s\n", message);
      invalid_code_path;
    }
  }
  
  GLuint shader_frag = gl.glCreateShader(GL_FRAGMENT_SHADER);
  s32* code_frag_len = (s32*)&code_frag.len;
  gl.glShaderSource(shader_frag, 1, (const char**)&code_frag.str, code_frag_len);
  gl.glCompileShader(shader_frag);
  { // errors
    GLint shader_frag_compiled;
    gl.glGetShaderiv(shader_frag, GL_COMPILE_STATUS, &shader_frag_compiled);
    if (!shader_frag_compiled)
    {
      GLsizei log_length = 0;
      GLchar message[1024];
      gl.glGetShaderInfoLog(shader_frag, 1024, &log_length, message);
      printf("GLSL Error: %s\n", message);
      printf("%.*s\n", str_varg(code_frag));
      invalid_code_path;
    }
  }
  
  result.id = (u32)gl.glCreateProgram();
  gl.glAttachShader(result.id, shader_vert);
  gl.glAttachShader(result.id, shader_frag);
  gl.glLinkProgram(result.id);
  
  GLint program_linked;
  gl.glGetProgramiv(result.id, GL_LINK_STATUS, &program_linked);
  if (program_linked != GL_TRUE)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    gl.glGetProgramInfoLog(result.id, 1024, &log_length, message);
    printf("GLSL Error: %s\n", message);
    invalid_code_path;
  }
  
  gl.glUseProgram(result.id);
  
  // TODO(PS): delete the vert and frag programs
  
  assert(attrs_len < SHADER_MAX_ATTRS);
  for (u32 i = 0; i < attrs_len; i++)
  {
    result.attrs[i] = gl.glGetAttribLocation(result.id, (char*)attrs[i].str);
    os_gl_no_error();
  }
  result.attrs[attrs_len] = SHADER_ATTR_LAST;
  
  assert(uniforms_len < SHADER_MAX_ATTRS);
  for (GLuint i = 0; i < uniforms_len; i++)
  {
    s32 len = (s32)uniforms[i].len;
    result.uniforms[i] = gl.glGetUniformLocation(result.id, (char*)uniforms[i].str);
  }
  result.uniforms[uniforms_len] = SHADER_ATTR_LAST;
  
  return result;
}

void 
set_uniform(Shader shader, u32 index, m44 u)
{
  gl.glUniformMatrix4fv(shader.uniforms[index], 1, GL_FALSE, (r32*)u.Elements);
}

void
geometry_bind(Geometry_Buffer geo)
{
  gl.glBindVertexArray(geo.buffer_id_vao);
  os_gl_no_error();
  
  gl.glBindBuffer(GL_ARRAY_BUFFER, geo.buffer_id_vertices);
  os_gl_no_error();
  
  gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geo.buffer_id_indices);
  os_gl_no_error();
}

void
shader_bind(Shader shader)
{
  gl.glUseProgram(shader.id);
  os_gl_no_error();
  for (u32 i = 0; 
       ((i < SHADER_MAX_ATTRS) && (shader.attrs[i] != SHADER_ATTR_LAST)); 
       i++)
  {
    gl.glEnableVertexAttribArray(shader.attrs[i]);
    os_gl_no_error();
  }
}

void
geometry_drawi(Geometry_Buffer geo, u32 indices){
  glDrawElements(GL_TRIANGLES, indices, GL_UNSIGNED_INT, 0);
  os_gl_no_error();
}

void
geometry_draw(Geometry_Buffer geo){
  geometry_drawi(geo, geo.indices_len);
}

void vertex_attrib_pointer(Geometry_Buffer geo, Shader shader, GLuint count, GLuint attr_index, GLuint stride, GLuint offset){
  geometry_bind(geo);
  gl.glVertexAttribPointer(shader.attrs[attr_index], count, GL_FLOAT, false, stride * sizeof(float), (void*)(offset * sizeof(float)));
  os_gl_no_error();
  gl.glEnableVertexAttribArray(shader.attrs[attr_index]);
  os_gl_no_error();
}

Texture
texture_create(u8* pixels, u32 width, u32 height, u32 stride)
{
  Texture result = {};
  glGenTextures(1, &result.id);
  os_gl_no_error();
  
  glBindTexture(GL_TEXTURE_2D, result.id);
  os_gl_no_error();
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  os_gl_no_error();
  
  glTexImage2D(
    GL_TEXTURE_2D, 
    0, 
    GL_RGBA, 
    width, 
    height, 
    0, 
    GL_RGBA, 
    GL_UNSIGNED_BYTE, 
    pixels
  );
  os_gl_no_error();
  
  result.w = width;
  result.h = height;
  result.s = stride;
  
  return result;
}

void
texture_update(Texture tex, u8* new_pixels, u32 width, u32 height, u32 stride)
{
  // NOTE(PS): this function simply replaces the entire image
  // we can write a more granular version if we need it
  
  assert(tex.w == width && tex.h == height && tex.s == stride);
  texture_bind(tex);
  glTexSubImage2D(
    GL_TEXTURE_2D,
    0,
    0, 0, // offset
    width, height, 
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    new_pixels
  );
  os_gl_no_error();
}

void
texture_bind(Texture tex)
{
  glBindTexture(GL_TEXTURE_2D, tex.id);
  os_gl_no_error();
}

void 
frame_begin(Graphics_Frame_Desc desc)
{
  v4 cc = desc.clear_color;
  glClearColor(cc.r, cc.g, cc.b, cc.a);

  v2 vmin = desc.viewport_min;
  v2 vmax = desc.viewport_max;
  glViewport((GLsizei)vmin.x, (GLsizei)vmin.y, (GLint)vmax.x, (GLint)vmax.y);
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  glDisable(GL_CULL_FACE);
  
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  
  os_gl_no_error();
}

void
frame_clear()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


String 
xplatform_shader_program_get_vert(XPlatform_Shader_Program_Src src)
{
#if defined(PLATFORM_win32)
  return src.win32_vert;
#elif defined(PLATFORM_osx)
  return src.osx_vert;
#elif defined(PLATFORM_wasm)
  return src.wasm_vert;
#endif
}

String
xplatform_shader_program_get_frag(XPlatform_Shader_Program_Src src)
{
#if defined(PLATFORM_win32)
  return src.win32_frag;
#elif defined(PLATFORM_osx)
  return src.osx_frag;
#elif defined(PLATFORM_wasm)
  return src.wasm_frag;
#endif
}


#endif // LUMENARIUM_EDITOR_GRAPHICS_H