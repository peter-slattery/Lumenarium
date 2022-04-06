
#define win32_gl_no_error() win32_gl_no_error_(__FILE__, __LINE__)
void win32_gl_no_error_(char* file, u32 line) { 
  u32 error = glGetError();
  char* str = 0;
  if (error) {
    str = win32_gl_error_to_string(error);
  }
  if (error != 0)
  {
    err_write("OpenGL error: %s:%d\n\t%s :: %d\n", file, line, str, error);
    invalid_code_path;
  }
}

Platform_Geometry_Buffer 
platform_geometry_buffer_create(
                                r32* vertices, u32 vertices_len, 
                                u32* indices, u32 indices_len
                                ){
  Platform_Geometry_Buffer result = {};
  
  gl.glGenVertexArrays(1, &result.buffer_id_vao);
  win32_gl_no_error();
  
  GLuint buffers[2];
  gl.glGenBuffers(2, (GLuint*)buffers);
  win32_gl_no_error();
  
  result.buffer_id_vertices = buffers[0];
  result.buffer_id_indices = buffers[1];
  
  // Vertices
  gl.glBindVertexArray(result.buffer_id_vao);
  gl.glBindBuffer(GL_ARRAY_BUFFER, result.buffer_id_vertices);
  win32_gl_no_error();
  
  gl.glBufferData(
                  GL_ARRAY_BUFFER, sizeof(r32) * vertices_len, vertices, GL_STATIC_DRAW
                  );
  win32_gl_no_error();
  result.vertices_len = vertices_len;
  
  // Indices
  gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.buffer_id_indices);
  win32_gl_no_error();
  
  gl.glBufferData(
                  GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * indices_len, indices, GL_STATIC_DRAW
                  );
  win32_gl_no_error();
  result.indices_len = indices_len;
  
  gl.glBindBuffer(GL_ARRAY_BUFFER, NULL);
  gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, NULL);
  
  return result;
}

void platform_geometry_buffer_update(
                                     Platform_Geometry_Buffer* buffer, 
                                     r32* verts, 
                                     u32 verts_offset, 
                                     u32 verts_len, 
                                     u32* indices, 
                                     u32 indices_offset, 
                                     u32 indices_len
                                     ){
  gl.glBindVertexArray(buffer->buffer_id_vao);
  gl.glBindBuffer(GL_ARRAY_BUFFER, buffer->buffer_id_vertices);
  win32_gl_no_error();
  
  if (verts_len > buffer->vertices_len)
  {
    // NOTE(PS): this is because we're going to delete the old buffer and
    // create a new one. In order to do that and not lose data, the update
    // function needs to have been passed all the buffer's data
    assert(verts_offset == 0); 
    gl.glBufferData(
                    GL_ARRAY_BUFFER, verts_len * sizeof(r32), (void*)verts, GL_STATIC_DRAW
                    );
  }
  else
  {
    gl.glBufferSubData(
                       GL_ARRAY_BUFFER, verts_offset * sizeof(r32), verts_len * sizeof(r32), (void*)verts
                       );
  }
  win32_gl_no_error();
  
  gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->buffer_id_indices);
  win32_gl_no_error();
  if (indices_len > buffer->indices_len)
  {
    // NOTE(PS): this is because we're going to delete the old buffer and
    // create a new one. In order to do that and not lose data, the update
    // function needs to have been passed all the buffer's data
    assert(indices_offset == 0); 
    gl.glBufferData(
                    GL_ELEMENT_ARRAY_BUFFER, indices_len * sizeof(u32), (void*)indices, GL_STATIC_DRAW
                    );
  }
  else
  {
    gl.glBufferSubData(
                       GL_ELEMENT_ARRAY_BUFFER, indices_offset * sizeof(u32), indices_len * sizeof(u32), (void*)indices
                       );
  }
  win32_gl_no_error();
}

Platform_Shader
platform_shader_create(
                       String code_vert, String code_frag, String* attrs, u32 attrs_len, String* uniforms, u32 uniforms_len
                       ){
  Platform_Shader result = {};
  
  GLuint shader_vert = gl.glCreateShader(GL_VERTEX_SHADER);
  gl.glShaderSource(shader_vert, 1, (const char**)&code_vert.str, &(s32)code_vert.len);
  gl.glCompileShader(shader_vert);
  { // errors
    GLint shader_vert_compiled;
    gl.glGetShaderiv(shader_vert, GL_COMPILE_STATUS, &shader_vert_compiled);
    if (!shader_vert_compiled)
    {
      GLsizei log_length = 0;
      GLchar message[1024];
      gl.glGetShaderInfoLog(shader_vert, 1024, &log_length, message);
      invalid_code_path;
    }
  }
  
  GLuint shader_frag = gl.glCreateShader(GL_FRAGMENT_SHADER);
  gl.glShaderSource(shader_frag, 1, (const char**)&code_frag.str, &(s32)code_frag.len);
  gl.glCompileShader(shader_frag);
  { // errors
    GLint shader_frag_compiled;
    gl.glGetShaderiv(shader_frag, GL_COMPILE_STATUS, &shader_frag_compiled);
    if (!shader_frag_compiled)
    {
      GLsizei log_length = 0;
      GLchar message[1024];
      gl.glGetShaderInfoLog(shader_frag, 1024, &log_length, message);
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
    invalid_code_path;
  }
  
  gl.glUseProgram(result.id);
  
  // TODO(PS): delete the vert and frag programs
  
  assert(attrs_len < PLATFORM_SHADER_MAX_ATTRS);
  for (u32 i = 0; i < attrs_len; i++)
  {
    result.attrs[i] = gl.glGetAttribLocation(
                                             result.id, (char*)attrs[i].str
                                             );
    win32_gl_no_error();
  }
  result.attrs[attrs_len] = PLATFORM_SHADER_ATTR_LAST;
  
  assert(uniforms_len < PLATFORM_SHADER_MAX_ATTRS);
  for (GLuint i = 0; i < uniforms_len; i++)
  {
    s32 len = (s32)uniforms[i].len;
    result.uniforms[i] = gl.glGetUniformLocation(
                                                 result.id, (char*)uniforms[i].str
                                                 );
  }
  result.uniforms[uniforms_len] = PLATFORM_SHADER_ATTR_LAST;
  
  return result;
}

void 
platform_set_uniform(Platform_Shader shader, u32 index, m44 u)
{
  gl.glUniformMatrix4fv(shader.uniforms[index], 1, GL_FALSE, (r32*)u.Elements);
}

void
platform_geometry_bind(Platform_Geometry_Buffer geo)
{
  gl.glBindVertexArray(geo.buffer_id_vao);
  win32_gl_no_error();
  
  gl.glBindBuffer(GL_ARRAY_BUFFER, geo.buffer_id_vertices);
  win32_gl_no_error();
  
  gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geo.buffer_id_indices);
  win32_gl_no_error();
}

void
platform_shader_bind(Platform_Shader shader)
{
  gl.glUseProgram(shader.id);
  win32_gl_no_error();
  for (u32 i = 0; 
       ((i < PLATFORM_SHADER_MAX_ATTRS) && (shader.attrs[i] != PLATFORM_SHADER_ATTR_LAST)); 
       i++)
  {
    gl.glEnableVertexAttribArray(shader.attrs[i]);
    win32_gl_no_error();
  }
}

void
platform_geometry_draw(
                       Platform_Geometry_Buffer geo, u32 indices
                       ){
  glDrawElements(GL_TRIANGLES, indices, GL_UNSIGNED_INT, 0);
  win32_gl_no_error();
}

void
platform_geometry_draw(
                       Platform_Geometry_Buffer geo
                       ){
  platform_geometry_draw(geo, geo.indices_len);
}

void platform_vertex_attrib_pointer(
                                    Platform_Geometry_Buffer geo, Platform_Shader shader, GLuint count, GLuint attr_index, GLuint stride, GLuint offset
                                    ){
  platform_geometry_bind(geo);
  gl.glVertexAttribPointer(shader.attrs[attr_index], count, GL_FLOAT, false, stride * sizeof(float), (void*)(offset * sizeof(float)));
  win32_gl_no_error();
  gl.glEnableVertexAttribArray(shader.attrs[attr_index]);
  win32_gl_no_error();
}

Platform_Texture
platform_texture_create(u8* pixels, u32 width, u32 height, u32 stride)
{
  Platform_Texture result = {};
  glGenTextures(1, &result.id);
  win32_gl_no_error();
  
  glBindTexture(GL_TEXTURE_2D, result.id);
  win32_gl_no_error();
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  win32_gl_no_error();
  
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
  win32_gl_no_error();
  
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
                  new_pixels
                  );
}

void
platform_texture_bind(Platform_Texture tex)
{
  glBindTexture(GL_TEXTURE_2D, tex.id);
  win32_gl_no_error();
}

void 
platform_frame_begin(Platform_Graphics_Frame_Desc desc)
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
  
  win32_gl_no_error();
}

void
platform_frame_clear()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}