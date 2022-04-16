
global XPlatform_Shader_Program_Src sculpture_shd = {
  .win32_vert = lit_str(
    "#version 330 core\n"
    "layout (location = 0) in vec3 a_pos;\n"
    "layout (location = 1) in vec2 a_uv;\n"
    "out vec2 uv;\n"
    "uniform mat4 proj;\n"
    "void main(void) {\n"
    "  gl_Position = proj * vec4(a_pos, 1.0);\n"
    "  uv = a_uv;\n"
    "}"
  ),
  .win32_frag = lit_str(
    "#version 330 core\n"
    "in vec2 uv;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D tex;\n"
    "void main(void) {\n"
    "  FragColor = texture(tex, uv);\n"
    "}"
  ),

  .osx_vert = lit_str(
    "#version 330 core\n"
    "layout (location = 0) in vec3 a_pos;\n"
    "layout (location = 1) in vec2 a_uv;\n"
    "out vec2 uv;\n"
    "uniform mat4 proj;\n"
    "void main(void) {\n"
    "  gl_Position = proj * vec4(a_pos, 1.0);\n"
    "  uv = a_uv;\n"
    "}"
  ),
  .osx_frag = lit_str(
    "#version 330 core\n"
    "in vec2 uv;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D tex;\n"
    "void main(void) {\n"
    "  FragColor = texture(tex, uv);\n"
    "}"
  ),

  .wasm_vert = lit_str(
    "precision highp float;\n"
    "attribute vec3 a_pos;\n"
    "attribute vec2 a_uv;\n"
    "varying vec2 uv;\n"
    "uniform mat4 proj;\n"
    "void main(void) {\n"
    "  gl_Position = proj * vec4(a_pos, 1.0);\n"
    "  uv = a_uv;\n"
    "}"
  ),
  .wasm_frag = lit_str(
    "precision highp float;\n"
    "varying vec2 uv;\n"
    "uniform sampler2D tex;\n"
    "void main(void) {\n"
    "  //gl_FragColor = texture2D(tex, uv) * color;\n"
    "  gl_FragColor = vec4(uv.x,1,uv.y,1);\n"
    "}"
  ),
};