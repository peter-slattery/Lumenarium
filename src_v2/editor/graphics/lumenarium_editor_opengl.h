/* date = March 24th 2022 6:05 pm */

#ifndef LUMENARIUM_EDITOR_OPENGL_H
#define LUMENARIUM_EDITOR_OPENGL_H

// glext.h - https://github.com/KhronosGroup/OpenGL-Registry/blob/main/api/GL/glext.h 
// wglext.h - 

void os_gl_no_error();

// type mocking
// so far, this is only for platforms that won't be using the editor
// but which still need to compile it.
#if !defined(GL_GLEXT_VERSION)
typedef u32 GLsizei;
typedef u32 GLuint;
typedef u32 GLenum;
typedef s32 GLint;
typedef float GLfloat;
typedef u8 GLchar;
typedef bool GLboolean;

#define GL_ARRAY_BUFFER 0
#define GL_ELEMENT_ARRAY_BUFFER 0
#define GL_VERTEX_SHADER 0
#define GL_COMPILE_STATUS 0
#define GL_FRAGMENT_SHADER 0
#define GL_LINK_STATUS 0
#define GL_TRUE true
#define GL_FALSE false
#define GL_UNSIGNED_INT 0
#define GL_FLOAT 0
#define GL_NEAREST 0
#define GL_LINEAR 0
#define GL_RGBA 0
#define GL_TEXTURE_2D 0
#define GL_TEXTURE_WRAP_S 0
#define GL_TEXTURE_WRAP_T 0
#define GL_TEXTURE_MIN_FILTER 0
#define GL_TEXTURE_MAG_FILTER 0
#define GL_UNSIGNED_BYTE 0
#define GL_SRC_ALPHA 0
#define GL_ONE_MINUS_SRC_ALPHA 0
#define GL_CULL_FACE 0
#define GL_DEPTH_TEST 0
#define GL_LESS 0
#define GL_COLOR_BUFFER_BIT 0
#define GL_DEPTH_BUFFER_BIT 0
#define GL_STATIC_DRAW 0
#define GL_TRIANGLES 0
#define GL_REPEAT 0
#define GL_BLEND 0

#endif

// OpenGL 3.3+ Context Creation
#if defined(PLATFORM_win32)
typedef const char *WINAPI proc_wglGetExtensionsStringARB(HDC hdc);
typedef BOOL proc_wglChoosePixelFormatARB(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef HGLRC proc_wglCreateContextAttribsARB(HDC hDC, HGLRC hshareContext, const int *attribList);
#endif

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
typedef struct OpenGL_Extensions OpenGL_Extensions;
struct OpenGL_Extensions
{
  #if defined(PLATFORM_win32)
  proc_wglGetExtensionsStringARB* wglGetExtensionsStringARB;
  proc_wglChoosePixelFormatARB* wglChoosePixelFormatARB;
  proc_wglCreateContextAttribsARB* wglCreateContextAttribsARB;
  #endif
  
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

global OpenGL_Extensions gl = {};

#endif //LUMENARIUM_EDITOR_OPENGL_H