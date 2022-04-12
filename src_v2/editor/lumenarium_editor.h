/* date = March 27th 2022 0:50 pm */

#ifndef LUMENARIUM_EDITOR_H
#define LUMENARIUM_EDITOR_H

struct Editor
{
  v2 window_dim;
  Editor_Renderer renderer;
  UI ui;
  
  v3 camera_pos;
  
  Platform_Geometry_Buffer sculpture_geo;
  Platform_Shader          sculpture_shd;
  Platform_Texture         sculpture_tex;
};

// NOTE(PS): call this any time sculpture data is updated if 
// you want to see the sculpture in the visualizer
internal void ed_sculpture_updated(App_State* state);

#endif //LUMENARIUM_EDITOR_H
