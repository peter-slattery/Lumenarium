/* date = March 24th 2022 6:05 pm */

#ifndef LUMENARIUM_WIN32_OPENGL_H
#define LUMENARIUM_WIN32_OPENGL_H

// glext.h - https://github.com/KhronosGroup/OpenGL-Registry/blob/main/api/GL/glext.h 
// wglext.h - 

// OpenGL 3.3+ Context Creation
typedef const char *WINAPI proc_wglGetExtensionsStringARB(HDC hdc);
typedef BOOL proc_wglChoosePixelFormatARB(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef HGLRC proc_wglCreateContextAttribsARB(HDC hDC, HGLRC hshareContext, const int *attribList);

// OpenGL 3.3+ Extensions
typedef void proc_glGenVertexArrays(GLsizei n, GLuint *arrays);
typedef void proc_glBindVertexArray(GLuint array);
typedef void proc_glGenBuffers (GLsizei n, GLuint *buffers);
typedef void proc_glBindBuffer(GLenum target, GLuint buffer);
typedef void proc_glBufferData(GLenum target, size_t size, const void *data, GLenum usage);
typedef void proc_glBufferSubData(GLenum target, size_t offset, size_t size, const void* data);
typedef GLuint proc_glCreateShader(GLenum type);
typedef void proc_glShaderSource(GLuint shader, u32 count, const char* const* string, const GLint *length);
typedef void proc_glCompileShader(GLuint shader);
typedef GLuint proc_glCreateProgram(void);
typedef void proc_glAttachShader(GLuint program, GLuint shader);
typedef void proc_glLinkProgram(GLuint program);
typedef void proc_glUseProgram(GLuint program);
typedef GLuint proc_glGetAttribLocation(GLuint program, const char* name);
typedef void proc_glVertexAttribPointer(GLuint attr, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void proc_glEnableVertexAttribArray(GLuint index);
typedef void proc_glGetShaderiv(GLuint shader, GLenum ele, GLint* value_out);
typedef void proc_glGetShaderInfoLog(GLuint shader, GLuint buf_len, GLsizei* len_out, GLchar* buf);
typedef void proc_glGetProgramiv(GLuint program, GLenum ele, GLint* value_out);
typedef void proc_glGetProgramInfoLog(GLuint program, GLuint cap, GLsizei* len_out, GLchar* buf);
typedef GLuint proc_glGetUniformLocation(GLuint program, const char* name);
typedef void proc_glUniformMatrix4fv(GLuint uniform, GLuint count, GLenum normalize, GLfloat* elements);
struct Win32_OpenGL_Extensions
{
  proc_wglGetExtensionsStringARB* wglGetExtensionsStringARB;
  proc_wglChoosePixelFormatARB* wglChoosePixelFormatARB;
  proc_wglCreateContextAttribsARB* wglCreateContextAttribsARB;
  
  proc_glGenVertexArrays* glGenVertexArrays;
  proc_glBindVertexArray* glBindVertexArray;
  proc_glGenBuffers* glGenBuffers;
  proc_glBindBuffer* glBindBuffer;
  proc_glBufferData* glBufferData;
  proc_glBufferSubData* glBufferSubData;
  proc_glCreateShader* glCreateShader;
  proc_glShaderSource* glShaderSource;
  proc_glCompileShader* glCompileShader;
  proc_glCreateProgram* glCreateProgram;
  proc_glAttachShader* glAttachShader;
  proc_glLinkProgram* glLinkProgram;
  proc_glUseProgram* glUseProgram;
  proc_glGetAttribLocation* glGetAttribLocation;
  proc_glVertexAttribPointer* glVertexAttribPointer;
  proc_glEnableVertexAttribArray* glEnableVertexAttribArray;
  proc_glGetShaderiv* glGetShaderiv;
  proc_glGetShaderInfoLog* glGetShaderInfoLog;
  proc_glGetProgramiv* glGetProgramiv;
  proc_glGetProgramInfoLog* glGetProgramInfoLog;
  proc_glGetUniformLocation* glGetUniformLocation;
  proc_glUniformMatrix4fv* glUniformMatrix4fv;
};

////////////////////////////////////////
// error strings

#define WIN32_GL_ERROR_CASE(code) case (code): { result = #code; } break

char* 
win32_gl_error_to_string(u32 error)
{
  char* result = 0;
  switch (error)
  {
    WIN32_GL_ERROR_CASE(GL_INVALID_VALUE);
    WIN32_GL_ERROR_CASE(GL_INVALID_ENUM );
    WIN32_GL_ERROR_CASE(GL_INVALID_OPERATION);
    default: { result = "unknown"; }
  }
  return result;
}
#endif //LUMENARIUM_WIN32_OPENGL_H
