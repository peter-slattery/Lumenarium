
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



global XPlatform_Shader_Program_Src sculpture_comp_shd = {
  .win32_vert = lit_str(""
  ),
  .win32_frag = lit_str(
    ""
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
    "float normpdf(in float x, in float sigma) { return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma; }\n"
    "void main(void) {\n"
    "  vec4 orig_p = texture(tex, uv);\n"
    "  vec2 tex_size = textureSize(tex, 0);\n"
    "  vec2 tex_offset = 1.0 / tex_size;\n"
    "  "
    "  const int m_size = 15;\n // must be odd\n"
    "  const int k_size = (m_size - 1) / 2;\n"
    "  float kernel[m_size];\n"
    "  float sigma = 7.0;\n"
    "  float z = 0;\n"
    "  for (int i = 0; i <= k_size; i++) {\n"
    "    float v = normpdf(float(i), sigma);\n"
    "    kernel[k_size + i] = v; kernel[k_size - i] = v;\n"
    "  }\n"
    "  for (int i = 0; i < m_size; i++) {\n"
    "    z += kernel[i];\n"
    "  }\n"
    "  vec3 bloom_acc = vec3(0);\n"
    "  for (int i = -k_size; i <= k_size; i++) {\n"
    "    for (int j = -k_size; j <= k_size; j++) {\n"
    "      vec2 uvp = uv + (vec2(float(i), float(j)) / tex_size);\n"
    "      bloom_acc += kernel[k_size + j] * kernel[k_size + i] * texture(tex, uvp).xyz;\n"
    "    }\n"
    "  }\n"
    "  vec3 bloom_color = bloom_acc / (z * z);\n"
    "  vec3 final_color = orig_p.xyz + bloom_color;\n"
      // tone mapping
    "  float exposure = 1.0;\n"
    "  float gamma = 2.2;\n"
    "  vec3 result = vec3(1.0) - exp(-final_color * exposure);\n"
      // also gamma correct while we're at it       
    "  result = pow(result, vec3(1.0 / gamma));\n"
    "  FragColor = vec4(result, 1.0);\n"
    "}"
  ),

  .wasm_vert = lit_str(
    ""
  ),
  .wasm_frag = lit_str(
    ""
  ),
};

