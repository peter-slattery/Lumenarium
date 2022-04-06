// 

// TODO(PS): 
// TODO(PS): 
// TODO(PS): 
// TODO(PS):  you guessed the data types and names of ALL of this
// fix it!

typedef int GLint;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef unsigned int GLenum;
typedef bool GLboolean;
typedef unsigned int GLsizei;

// NOTE(PS): these values and function signatures all come from 
// the GLES2/gl2.h header file that can be found here:
// https://www.khronos.org/registry/OpenGL/api/GLES2/gl2.h
//
// I resorted to hard coding these rather than passing them in because 
// passing them in didn't seem to be working.

#define GL_FALSE                          0
#define GL_TRUE                           1
#define GL_TEXTURE_2D                     0x0DE1
#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_STENCIL_BUFFER_BIT             0x00000400
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_BLEND                          0x0BE2
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_DEPTH_TEST                     0x0B71
#define GL_LESS                           0x0201
#define GL_ARRAY_BUFFER                   0x8892 
#define GL_ELEMENT_ARRAY_BUFFER           0x8893 
#define GL_STATIC_DRAW                    0x88e4 
#define GL_FRAGMENT_SHADER                0x8b30 
#define GL_VERTEX_SHADER                  0x8b31 
#define GL_TRIANGLES                      0x0004
#define GL_UNSIGNED_INT                   0x1405
#define GL_FLOAT                          0x1406
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803
#define GL_REPEAT                         0x2901
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_LINEAR                         0x2601
#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908
#define GL_UNSIGNED_BYTE                  0x1401

WASM_EXTERN bool glHadError();
WASM_EXTERN void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
WASM_EXTERN void glEnable(GLuint i);
WASM_EXTERN void glDisable(GLuint i);
WASM_EXTERN void glBlendFunc(GLuint a, GLuint b);
WASM_EXTERN void glViewport(GLuint xmin, GLuint ymin, GLuint xmax, GLuint ymax);
WASM_EXTERN void glDepthFunc(GLuint i);
WASM_EXTERN void glClear(GLuint i);

WASM_EXTERN GLuint glCreateBuffer();
WASM_EXTERN void glBindBuffer(GLenum buffer_kind, GLuint buffer_id);
WASM_EXTERN void glBufferData(GLenum target, size_t size, const void* data, GLenum usage);
WASM_EXTERN GLuint glCreateShader(GLenum kind);
WASM_EXTERN GLuint glShaderSource(GLuint shader_id, char* shader_code, GLuint shader_code_len);
WASM_EXTERN void glCompileShader(GLuint shader_id);
WASM_EXTERN GLuint glCreateProgram(void);
WASM_EXTERN void glAttachShader(GLuint program, GLuint shader);
WASM_EXTERN void glLinkProgram(GLuint program);
WASM_EXTERN void glUseProgram(GLuint program);
WASM_EXTERN GLuint glGetAttribLocation(GLuint program, const char* name, GLuint name_len);
WASM_EXTERN GLuint glGetUniformLocation(GLuint program, const char* name, u32 len);
WASM_EXTERN void glVertexAttribPointer(GLuint attr, GLuint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
WASM_EXTERN void glEnableVertexAttribArray(GLuint index);
WASM_EXTERN void glDrawElements(GLenum type, GLuint count, GLenum ele_type, void* indices);
WASM_EXTERN void glGenTextures(GLuint count, GLuint* ids, u32 ids_size);
WASM_EXTERN void glBindTexture(GLenum slot, GLuint id);
WASM_EXTERN void glTexParameteri(GLenum slot, GLenum param, GLenum value);
WASM_EXTERN void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * data, u32 data_size);
WASM_EXTERN void glBindTexture(GLenum target, GLuint id);
WASM_EXTERN void glTexSubImage2D(GLenum target, GLint level, GLuint offsetx, GLuint offsety, GLuint w, GLuint h, GLenum format, GLenum type, void* ptr, u32 ptr_size);

Platform_Geometry_Buffer 
platform_geometry_buffer_create(
                                r32* vertices, GLuint vertices_len, 
                                GLuint* indices, GLuint indices_len
                                ){
  Platform_Geometry_Buffer result = {};
  
  result.buffer_id_vertices = glCreateBuffer();
  result.buffer_id_indices = glCreateBuffer();
  
  // Vertices
  glBindBuffer(GL_ARRAY_BUFFER, result.buffer_id_vertices);
  glBufferData(
               GL_ARRAY_BUFFER, sizeof(r32) * vertices_len, vertices, GL_STATIC_DRAW
               );
  
  // Indices
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.buffer_id_indices);
  glBufferData(
               GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices_len, indices, GL_STATIC_DRAW
               );
  result.indices_len = indices_len;
  
  glBindBuffer(GL_ARRAY_BUFFER, (GLuint)NULL);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)NULL);
  
  return result;
}

Platform_Shader
platform_shader_create(
                       String code_vert, String code_frag, String* attrs, GLuint attrs_len, String* uniforms, GLuint uniforms_len
                       ){
  Platform_Shader result = {};
  
  GLuint shader_vert = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(shader_vert, str_expand(code_vert));
  glCompileShader(shader_vert);
  
  GLuint shader_frag = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(shader_frag, str_expand(code_frag));
  glCompileShader(shader_frag);
  
  result.id = (GLuint)glCreateProgram();
  glAttachShader(result.id, shader_vert);
  glAttachShader(result.id, shader_frag);
  glLinkProgram(result.id);
  glUseProgram(result.id);
  
  // TODO(PS): delete the vert and frag programs
  
  assert(attrs_len < PLATFORM_SHADER_MAX_ATTRS);
  for (GLuint i = 0; i < attrs_len; i++)
  {
    s32 len = (s32)attrs[i].len;
    result.attrs[i] = glGetAttribLocation(
                                          result.id, (char*)attrs[i].str, len
                                          );
  }
  result.attrs[attrs_len] = PLATFORM_SHADER_ATTR_LAST;
  
  assert(uniforms_len < PLATFORM_SHADER_MAX_ATTRS);
  for (GLuint i = 0; i < uniforms_len; i++)
  {
    s32 len = (s32)uniforms[i].len;
    result.uniforms[i] = glGetUniformLocation(
                                              result.id, (char*)uniforms[i].str, uniforms[i].len
                                              );
  }
  result.uniforms[uniforms_len] = PLATFORM_SHADER_ATTR_LAST;
  
  return result;
}

void
platform_geometry_bind(Platform_Geometry_Buffer geo)
{
  glBindBuffer(GL_ARRAY_BUFFER, geo.buffer_id_vertices);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geo.buffer_id_indices);
}

void
platform_shader_bind(Platform_Shader shader)
{
  glUseProgram(shader.id);
  for (GLuint i = 0; i < PLATFORM_SHADER_MAX_ATTRS && shader.attrs[i] != PLATFORM_SHADER_ATTR_LAST; i++)
  {
    glEnableVertexAttribArray(shader.attrs[i]);
  }
}

void 
platform_set_uniform(Platform_Shader shader, u32 index, m44 u)
{
  glUniformMatrix4fv(shader.uniforms[index], 1, GL_FALSE, &u.Elements[0]);
}

void
platform_geometry_draw(
                       Platform_Geometry_Buffer geo, u32 indices
                       ){
  glDrawElements(GL_TRIANGLES, indices, GL_UNSIGNED_INT, 0);
}

void
platform_geometry_draw(
                       Platform_Geometry_Buffer geo
                       ){
  glDrawElements(GL_TRIANGLES, geo.indices_len, GL_UNSIGNED_INT, 0);
}

void platform_vertex_attrib_pointer(
                                    Platform_Geometry_Buffer geo, Platform_Shader shader, u32 count, u32 attr_index, u32 stride, u32 offset
                                    ){
  //platform_shader_bind(shader);
  platform_geometry_bind(geo);
  glVertexAttribPointer(shader.attrs[attr_index], count, GL_FLOAT, false, stride * sizeof(float), (void*)(offset * sizeof(float)));
}

Platform_Texture
platform_texture_create(u8* pixels, u32 width, u32 height, u32 stride)
{
  Platform_Texture result = {};
  glGenTextures(1, &result.id, sizeof(u32));
  glBindTexture(GL_TEXTURE_2D, result.id);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  
  glTexImage2D(
               GL_TEXTURE_2D, 
               0, 
               GL_RGBA, 
               width, 
               height, 
               0,
               GL_RGBA, 
               GL_UNSIGNED_BYTE, 
               pixels,
               (width * height) * sizeof(u32) 
               );
  
  result.w = width;
  result.h = height;
  result.s = stride;
  
  return result;
}

void
platform_texture_update(Platform_Texture tex, u8* new_pixels, u32 width, u32 height, u32 stride)
{
  // NOTE(PS): this function simply replaces the entire image
  // we can write a more granular version if we need it
  
  assert(tex.w == width && tex.h == height && tex.s == stride);
  platform_texture_bind(tex);
  glTexSubImage2D(
                  GL_TEXTURE_2D,
                  0,
                  0, 0, // offset
                  width, height, 
                  GL_RGBA,
                  GL_UNSIGNED_BYTE,
                  new_pixels,
                  width * height * sizeof(u32)
                  );
}


void
platform_texture_bind(Platform_Texture tex)
{
  glBindTexture(GL_TEXTURE_2D, tex.id);
}

void 
platform_frame_begin(Platform_Graphics_Frame_Desc desc)
{
  v4 cc = desc.clear_color;
  glClearColor(cc.r, cc.g, cc.b, cc.a);
  
  v2 vmin = desc.viewport_min;
  v2 vmax = desc.viewport_max;
  glViewport(vmin.x, vmin.y, vmax.x, vmax.y);
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  //glDisable(GL_TEXTURE_2D);
  
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
}

void
platform_frame_clear()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}