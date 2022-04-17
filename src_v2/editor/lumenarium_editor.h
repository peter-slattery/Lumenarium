/* date = March 27th 2022 0:50 pm */

#ifndef LUMENARIUM_EDITOR_H
#define LUMENARIUM_EDITOR_H

typedef struct Editor_Desc Editor_Desc;
struct Editor_Desc
{
  v2 content_scale;
  v2 init_window_dim;
};

typedef struct Editor Editor;
struct Editor
{
  v2 content_scale;
  v2 window_dim;
  Editor_Renderer renderer;
  UI ui;
  
  v3 camera_pos;
  
  Geometry_Buffer sculpture_geo;
  Shader          sculpture_shd;
  Texture         sculpture_tex;
};

// NOTE(PS): call this any time sculpture data is updated if 
// you want to see the sculpture in the visualizer
internal void ed_sculpture_updated(App_State* state, r32 scale, r32 led_size);

#endif //LUMENARIUM_EDITOR_H
