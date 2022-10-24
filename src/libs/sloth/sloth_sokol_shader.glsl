#pragma sokol @ctype mat4 hmm_mat4

#pragma sokol @vs sloth_viz_vs

uniform sloth_viz_vs_params {
  mat4 mvp;
};

in vec3 position;
in vec2 uv;
// TODO: This is really wasteful of data we have to 
// send to the gpu every frame but I can't figure out 
// how to pass uints to a shader
in vec4 color;

out vec2 o_uv;
out vec4 o_color;

void main()
{
  gl_Position = mvp * vec4(position, 1);
  o_uv = uv;
  o_color = color;
}
#pragma sokol @end

#pragma sokol @fs sloth_viz_fs

in vec2 o_uv;
in vec4 o_color;
out vec4 frag_color;

uniform sampler2D tex;

void main()
{
  frag_color = texture(tex, o_uv) * o_color;
}

#pragma sokol @end

#pragma sokol @program sloth_viz sloth_viz_vs sloth_viz_fs