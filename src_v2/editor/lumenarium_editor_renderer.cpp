
internal void
edr_init(App_State* state)
{
  v4 quad_verts[] = {
    -0.5f,  0.5f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.0f,  1.0f,
    00.5f, -0.5f,  0.0f,  1.0f,
    00.5f,  0.5f,  0.0f,  1.0f,
  };
  
  u32 quad_indices[] = {
    3, 2, 1,
    3, 1, 0,
  };
  
  char* shader_code_vert = 
    "#version 140\n"
    "attribute vec4 coordinates;\n"
    "void main(void) {\n"
    "  gl_Position = coordinates;\n"
    "}";
  
  char* shader_code_frag = 
    "#version 140\n"
    "void main(void) {\n"
    "  gl_FragColor = vec4(1, 0, 1, 1);\n"
    "}";
  
#if 0
  /* =======              Geometry                =======*/
  
  glCreateBuffers(1, &buffer_vertex);
  glBindBuffer(GL_ARRAY_BUFFER, buffer_vertex);
  glBufferData(GL_ARRAY_BUFFER, quad_verts, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, NULL);
  
  glCreateBuffer(1, &buffer_index);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_index);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, quad_indices, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, NULL);
  
  /* =======                Shaders                =======*/
  
  shader_vert = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(shader_vertex, shader_code_vert);
  glCompileShader(shader_vert);
  
  shader_frag = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(shader_frag, shader_code_frag);
  glCompileShader(shader_frag);
  
  shader_prog = glCreateProgram();
  glAttachShader(shader_prog, shader_vert);
  glAttachShader(shader_prog, shader_frag);
  glLinkProgram(shader_prog);
  glUseProgram(shader_prog);
  
  /* ======= Associating shaders to buffer objects =======*/
  
  glBindBuffer(GL_ARRAY_BUFFER, buffer_vertex);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_index);
  coord = glGetAttribLocation(shader_prog, "coordinates");
  glVertexAttribPointer(coord, 4, GL_FLOAT, false, 0, 0);
  glEnableVertexAttribArray(coord);
#endif
}

internal void
edr_render_quad()
{
#if 0
  glBindBuffer(GL_ARRAY_BUFFER, buffer_vertex);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_index);
  glEnableVertexAttribArray(coord);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
#endif
}


internal void
edr_render(App_State* state)
{
  Platform_Graphics_Frame_Desc desc = {};
  desc.clear_color = { 0.1f, 0.1f, 0.1f, 1 };
  desc.viewport_min = { 0, 0 };
  desc.viewport_max = { 1600, 900 };
  platform_frame_begin(desc);
  platform_frame_clear();
  
#if 0
  edr_render_quad();
#endif
}