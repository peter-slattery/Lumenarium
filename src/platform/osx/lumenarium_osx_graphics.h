#define OSX_GL_ERROR_CASE(e) case e: { result = #e; } break
char* 
osx_gl_error_to_string(u32 error)
{
  char* result = 0;
  switch (error)
  {
    OSX_GL_ERROR_CASE(GL_INVALID_VALUE);
    OSX_GL_ERROR_CASE(GL_INVALID_ENUM );
    OSX_GL_ERROR_CASE(GL_INVALID_OPERATION);
    default: { result = "unknown"; }
  }
  return result;
}

void 
os_gl_no_error_(char* file, u32 line) { 
  u32 error = glGetError();
  char* str = 0;
  if (error) {
    str = osx_gl_error_to_string(error);
  }
  if (error != 0)
  {
    fprintf(stderr, "OpenGL error: %s:%d\n\t%s :: %d\n", file, line, str, error);
    invalid_code_path;
  }
}

#define load_ext(r,n) r.n = (proc_##n*)glfwGetProcAddress(#n); assert((r.n) != 0)
OpenGL_Extensions
osx_load_opengl_ext()
{
  OpenGL_Extensions result = {};
  load_ext(result, glGenVertexArrays);
  load_ext(result, glBindVertexArray);
  load_ext(result, glGenBuffers);
  load_ext(result, glBindBuffer);
  load_ext(result, glBufferData);
  load_ext(result, glBufferSubData);
  load_ext(result, glCreateShader);
  load_ext(result, glShaderSource);
  load_ext(result, glCompileShader);
  load_ext(result, glCreateProgram);
  load_ext(result, glAttachShader);
  load_ext(result, glLinkProgram);
  load_ext(result, glUseProgram);
  load_ext(result, glGetAttribLocation);
  load_ext(result, glVertexAttribPointer);
  load_ext(result, glEnableVertexAttribArray);
  load_ext(result, glGetShaderiv);
  load_ext(result, glGetShaderInfoLog);
  load_ext(result, glGetProgramiv);
  load_ext(result, glGetProgramInfoLog);
  load_ext(result, glGetUniformLocation);
  load_ext(result, glUniformMatrix4fv);
  return result;
}