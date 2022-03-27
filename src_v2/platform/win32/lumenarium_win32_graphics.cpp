
#define win32_gl_no_error() win32_gl_no_error_()
void win32_gl_no_error_() { 
  u32 error = glGetError();
  char* str = 0;
  if (error) {
    str = win32_gl_error_to_string(error);
  }
  assert(error == 0);
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

Platform_Shader
platform_shader_create(
                       String code_vert, String code_frag, String* attrs, u32 attrs_len
                       ){
  Platform_Shader result = {};
  
  GLuint shader_vert = gl.glCreateShader(GL_VERTEX_SHADER);
  gl.glShaderSource(shader_vert, 1, (const char**)&code_vert.str, &(s32)code_vert.len);
  gl.glCompileShader(shader_vert);
  { // errors
    GLint shader_vert_compiled;
    gl.glGetShaderiv(shader_vert, GL_COMPILE_STATUS, &shader_vert_compiled);
    if (shader_vert_compiled != GL_TRUE)
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
    if (shader_frag_compiled != GL_TRUE)
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
  }
  result.attrs[attrs_len] = PLATFORM_SHADER_ATTR_LAST;
  
  return result;
}

void
platform_geometry_bind(Platform_Geometry_Buffer geo)
{
  gl.glBindVertexArray(geo.buffer_id_vertices);
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
                       Platform_Geometry_Buffer geo
                       ){
  glDrawElements(GL_TRIANGLES, geo.indices_len, GL_UNSIGNED_INT, 0);
  win32_gl_no_error();
}

void platform_vertex_attrib_pointer(
                                    Platform_Geometry_Buffer geo, Platform_Shader shader, u32 attr_index
                                    ){
  platform_geometry_bind(geo);
  gl.glVertexAttribPointer(shader.attrs[attr_index], 4, GL_FLOAT, false, 0, 0);
  win32_gl_no_error();
  gl.glEnableVertexAttribArray(shader.attrs[attr_index]);
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
  
  //glDisable(GL_TEXTURE_2D);
  
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
}

void
platform_frame_clear()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}