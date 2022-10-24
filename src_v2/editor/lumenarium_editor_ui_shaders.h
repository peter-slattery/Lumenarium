global XPlatform_Shader_Program_Src ui_shader = {
  .win32_vert = lit_str(
    "#version 330 core\n"
    "layout (location = 0) in vec3 a_pos;\n"
    "layout (location = 1) in vec2 a_uv;\n"
    "layout (location = 2) in vec4 a_color;\n"
    "out vec2 uv;\n"
    "out vec4 color;\n"
    "uniform mat4 proj;\n"
    "void main(void) {\n"
    "  gl_Position = proj * vec4(a_pos, 1.0);\n"
    "  uv = a_uv;\n"
    "  color = a_color;\n"
    "}"
  ),
  .win32_frag = lit_str(
    "#version 330 core\n"
    "in vec2 uv;\n"
    "in vec4 color;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D tex;\n"
    "void main(void) {\n"
    "  FragColor = texture(tex, uv) * color;\n"
    "}"
  ),

  .osx_vert = lit_str(
    "#version 330 core\n"
    "layout (location = 0) in vec3 a_pos;\n"
    "layout (location = 1) in vec2 a_uv;\n"
    "layout (location = 2) in vec4 a_color;\n"
    "out vec2 uv;\n"
    "out vec4 color;\n"
    "uniform mat4 proj;\n"
    "void main(void) {\n"
    "  gl_Position = proj * vec4(a_pos, 1.0);\n"
    "  uv = a_uv;\n"
    "  color = a_color;\n"
    "}"
  ),
  .osx_frag = lit_str(
    "#version 330 core\n"
    "in vec2 uv;\n"
    "in vec4 color;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D tex;\n"
    "void main(void) {\n"
    "  FragColor = texture(tex, uv) * color;\n"
    "}"
  ),

  .wasm_vert = lit_str(
    "precision highp float;\n"
    "attribute vec3 a_pos;\n"
    "attribute vec2 a_uv;\n"
    "attribute vec4 a_color;\n"
    "varying vec2 uv;\n"
    "varying vec4 color;\n"
    "uniform mat4 proj;\n"
    "void main(void) {\n"
    "  gl_Position = proj * vec4(a_pos, 1.0);\n"
    "  uv = a_uv;\n"
    "  color = a_color;\n"
    "}"
  ),
  .wasm_frag = lit_str(
    "precision highp float;\n"
    "varying vec2 uv;\n"
    "varying vec4 color;\n"
    "uniform sampler2D tex;\n"
    "void main(void) {\n"
    "  gl_FragColor = texture2D(tex, uv) * color;\n"
    "}"
  ),
};

