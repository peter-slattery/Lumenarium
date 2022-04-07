#define WHITE_SPRITE_ID 511

static String ui_shader_vert_win32 = lit_str(
                                             "#version 330 core\n"
                                             "layout (location = 0) in vec4 a_pos;\n"
                                             "layout (location = 1) in vec2 a_uv;\n"
                                             "layout (location = 2) in vec4 a_color;\n"
                                             "out vec2 uv;\n"
                                             "out vec4 color;\n"
                                             "uniform mat4 proj;\n"
                                             "void main(void) {\n"
                                             "  gl_Position = proj * a_pos;\n"
                                             "  uv = a_uv;\n"
                                             "  color = a_color;\n"
                                             "}"
                                             );

static String ui_shader_frag_win32 = lit_str(
                                             "#version 330 core\n"
                                             "in vec2 uv;\n"
                                             "in vec4 color;\n"
                                             "out vec4 FragColor;\n"
                                             "uniform sampler2D texture;\n"
                                             "void main(void) {\n"
                                             "  FragColor = texture(texture, uv) * color;\n"
                                             "}"
                                             );

internal UI
ui_create(u32 widget_pool_cap, u32 verts_cap, Input_State* input, Allocator* a)
{
  UI result = {};
  result.input = input;
  
  // Widgets
  result.widgets.free = allocator_alloc_array(a, UI_Widget, widget_pool_cap);
  result.widgets.free_cap = widget_pool_cap;
  
  // Per Frame Vertex Buffer
  result.verts_cap = verts_cap;
  result.verts = allocator_alloc_array(a, UI_Vertex, verts_cap);
  result.indices_cap = verts_cap * 2;
  result.indices = allocator_alloc_array(a, u32, result.indices_cap);
  
  result.per_frame_buffer = platform_geometry_buffer_create(
                                                            (r32*)result.verts, 
                                                            result.verts_cap, 
                                                            result.indices, 
                                                            result.indices_cap
                                                            );
  
  String attrs[] = { lit_str("a_pos"), lit_str("a_uv"), lit_str("a_color") };
  String uniforms[] = { lit_str("proj") };
  result.shader = platform_shader_create(
                                         ui_shader_vert_win32,
                                         ui_shader_frag_win32,
                                         attrs, 3,
                                         uniforms, 1
                                         );
  
  platform_vertex_attrib_pointer(
                                 result.per_frame_buffer, result.shader, 4, result.shader.attrs[0], 10, 0
                                 );
  platform_vertex_attrib_pointer(
                                 result.per_frame_buffer, result.shader, 2, result.shader.attrs[1], 10, 4
                                 );
  platform_vertex_attrib_pointer(
                                 result.per_frame_buffer, result.shader, 4, result.shader.attrs[2], 10, 6
                                 );
  
  // Texture Atlas
  result.atlas = texture_atlas_create(1024, 1024, 512, permanent);
  result.atlas_texture = platform_texture_create(result.atlas.pixels, 1024, 1024, 1024);
  
  u32 white_sprite[] = { 
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
  };
  ui_sprite_register(&result, (u8*)white_sprite, 4, 4, WHITE_SPRITE_ID);
  
  ui_create_default_style_sheet();
  return result;
}

internal u32
ui_vert_push(UI* ui, v3 p, v2 t, v4 c)
{
  assert(ui->verts_len < ui->verts_cap);
  u32 index = ui->verts_len++;
  
  ui->verts[index].pos = {p.x, p.y, p.z, 1};
  ui->verts[index].uv = t;
  ui->verts[index].color = c;
  
  return index;
}

internal void
ui_index_push(UI* ui, u32 i)
{
  assert(ui->indices_len < ui->indices_cap);
  ui->indices[ui->indices_len++] = i;
}

internal void
ui_quad_push(UI* ui, v3 pmin, v3 pmax, v2 tmin, v2 tmax, v4 c)
{
  u32 bl = ui_vert_push(ui, pmin, tmin, c);
  u32 br = ui_vert_push(ui, v3{pmax.x, pmin.y, pmin.z}, v2{tmax.x,tmin.y}, c);
  u32 tr = ui_vert_push(ui, pmax, tmax, c);
  u32 tl = ui_vert_push(ui, v3{pmin.x, pmax.y, pmin.z}, v2{tmin.x,tmax.y}, c);
  
  ui_index_push(ui, bl);
  ui_index_push(ui, br);
  ui_index_push(ui, tr);
  
  ui_index_push(ui, bl);
  ui_index_push(ui, tr);
  ui_index_push(ui, tl);
}

internal void
ui_sprite_register(UI* ui, u8* pixels, u32 w, u32 h, u32 id)
{
  texture_atlas_register(&ui->atlas, pixels, w, h, id, v2{0,0}, TextureAtlasRegistration_PixelFormat_RGBA);
  platform_texture_update(ui->atlas_texture, ui->atlas.pixels, ui->atlas.width, ui->atlas.height, ui->atlas.width);
}

internal void 
ui_sprite_push(UI* ui, v3 pmin, v3 pmax, u32 id, v4 color)
{
  Texture_Atlas_Sprite sprite = texture_atlas_sprite_get(&ui->atlas, id);
  v4 uv = texture_atlas_sprite_get_uvs(&ui->atlas, sprite);
  pmin.XY += sprite.draw_offset;
  pmax.XY += sprite.draw_offset;
  ui_quad_push(ui, pmin, pmax, uv.xy, uv.zw, color);
}

internal void 
ui_sprite_push(UI* ui, v3 pmin, v3 pmax, u32 id)
{
  ui_sprite_push(ui, pmin, pmax, id, v4{1,1,1,1});
}

struct UI_Char_Draw_Cmd
{
  v4 uv;
  v3 pmin;
  v3 pmax;
  v3 baseline_after;
};

internal UI_Char_Draw_Cmd
ui_sprite_char_get_draw_cmd(UI* ui, v3 at, u32 codepoint)
{
  UI_Char_Draw_Cmd result = {};
  
  Texture_Atlas_Sprite sprite = texture_atlas_sprite_get(&ui->atlas, codepoint);
  result.uv = texture_atlas_sprite_get_uvs(&ui->atlas, sprite);
  
  v3 dim = v3{ 
    (r32)(sprite.max_x - sprite.min_x), 
    (r32)(sprite.max_y - sprite.min_y),
    0,
  };
  result.pmin = at;
  result.pmin.XY += sprite.draw_offset;
  result.pmin.XY = v2_floor(result.pmin.XY);
  result.pmax = result.pmin + dim;
  
  result.baseline_after = v3{ result.pmax.x, at.y, at.z };
  
  return result;
}

internal void
ui_frame_prepare(UI* ui, v2 window_dim)
{
  ui->verts_len = 0;
  ui->indices_len = 0;
  
  ui->widgets.free_len = 0;
  ui->widgets.active_parent = 0;
  ui->widgets.root = ui_widget_pool_push(&ui->widgets, lit_str("root"));
  ui->widgets.active_parent = ui->widgets.root;
  
  v2 half_d = window_dim * 0.5f;
  ui->proj = HMM_Orthographic(0, window_dim.x, window_dim.y, 0, 0.01f, 100);
  
  if (ui->widget_next_hot.value != 0)
  {
    ui->widget_next_hot_frames += 1;
    if (ui->widget_hot_frames > 1) 
    {
      ui->widget_next_hot = UI_Widget_Id{0};
    }
  }
  if (ui->widget_hot.value != 0)
  {
    ui->widget_hot_frames += 1;
    if (ui->widget_hot_frames > 1) ui->widget_hot = UI_Widget_Id{0};
  }
}

global bool show = false;

internal void
ui_draw(UI* ui)
{
  u32 widget_count = ui->widgets.free_len;
  r32 range_min = -10;
  r32 range_max = -1;
  r32 range_step = (range_max - range_min) / (r32)(widget_count * 4);
  ui_widgets_to_geometry_recursive(ui, ui->widgets.root, -10, range_step);
  
  platform_geometry_buffer_update(
                                  &ui->per_frame_buffer, 
                                  (r32*)ui->verts,
                                  0,
                                  ui->verts_len * 10,
                                  ui->indices,
                                  0,
                                  ui->indices_len
                                  );
  platform_shader_bind(ui->shader);
  platform_set_uniform(ui->shader, 0, ui->proj);
  platform_texture_bind(ui->atlas_texture);
  platform_geometry_bind(ui->per_frame_buffer);
  platform_geometry_draw(ui->per_frame_buffer, ui->indices_len);
}

////////////////////////////////////////////
// Widgets

internal UI_Widget_Id
ui_widget_id_create(String string, u32 index)
{
  assert(string.len != 0 && string.str != 0);
  UI_Widget_Id result = {};
  result.value = hash_djb2_to_u32(string);
  result.index = index;
  return result;
}

internal UI_Widget*
ui_widget_pool_push(UI_Widget_Pool* pool, String string)
{
  assert(pool->free_len < pool->free_cap);
  UI_Widget* result = pool->free + pool->free_len++;
  
  result->id = ui_widget_id_create(string, pool->free_len);
  result->parent = 0;
  result->next = 0;
  result->child_first = 0;
  result->child_last = 0;
  
  if (pool->active_parent)
  {
    result->parent = pool->active_parent;
    sll_push(
             pool->active_parent->child_first, 
             pool->active_parent->child_last, 
             result
             );
  }
  
  return result;
}

internal void
ui_widget_pool_pop(UI_Widget_Pool* pool)
{
  if (pool->active_parent->parent)
  {
    pool->active_parent = pool->active_parent->parent;
  }
}

internal bool
ui_widget_id_equals(UI_Widget_Id a, UI_Widget_Id b)
{
  return (a.value == b.value);
}

internal bool
ui_widget_id_is_valid(UI_Widget_Id h)
{
  return h.value != 0;
}

internal void
ui_widget_next_hot_set(UI* ui, UI_Widget* w)
{
  ui->widget_next_hot = w->id;
  ui->widget_next_hot_frames = 0;
}

internal void
ui_widget_hot_set(UI* ui, UI_Widget* w)
{
  ui->widget_hot = w->id;
  ui->widget_hot_frames = 0;
}

internal UI_Widget_Result
ui_widget_push(UI* ui, UI_Widget_Desc desc)
{
  UI_Widget_Result result = {};
  
  UI_Widget* w = ui_widget_pool_push(&ui->widgets, desc.string);
  w->desc = desc;
  
  if (has_flag(desc.style.flags, UIWidgetStyle_MouseClick))
  {
    // CASES:
    // Mouse Over | Mouse Clicked | Is Next Hot | Response
    //      f     |      f        |      t      | clear next hot
    //      f     |      f        |      f      | do nothing
    //      f     |      t        |      f      | do nothing
    //      t     |      f        |      f      | beome next hot
    //      t     |      t        |      f      | become next hot
    //      t     |      t        |      t      | become hot
    
    v2 mouse_p = ui->input->frame_hot->mouse_pos;
    bool mouse_over = (
                       mouse_p.x >= desc.p_min.x && mouse_p.x <= desc.p_max.x &&
                       mouse_p.y >= desc.p_min.y && mouse_p.y <= desc.p_max.y
                       );
    
    if (mouse_over)
    {
      if (ui_widget_id_equals(w->id, ui->widget_next_hot))
      {
        if (input_key_is_down(ui->input, KeyCode_MouseLeftButton))
        {
          ui_widget_hot_set(ui, w);
          result.flags |= UIWidgetResult_MouseLeft_IsDown;
        }
        if (input_key_went_up(ui->input, KeyCode_MouseLeftButton))
        {
          result.flags |= UIWidgetResult_MouseLeft_WentUp;
          ui->widget_hot = UI_Widget_Id{0};
        }
      }
      else if ((w->id.index >= ui->widget_next_hot.index) && ui->widget_hot.value == 0)
      {
        ui_widget_next_hot_set(ui, w);
      }
    }
    else
    {
      if (ui_widget_id_equals(w->id, ui->widget_next_hot))
      {
        ui->widget_next_hot = UI_Widget_Id{0};
      }
    }
  }
  
  return result;
}

internal void
ui_widget_pop(UI* ui, UI_Widget* widget)
{
  assert(ui_widget_id_equals(widget->id, ui->widgets.active_parent->id));
  ui_widget_pool_pop(&ui->widgets);
}

internal r32
ui_widgets_to_geometry_recursive(UI* ui, UI_Widget* widget, r32 z_start, r32 z_step)
{
  r32 z_at = z_start;
  for (UI_Widget* child = widget->child_first; child != 0; child = child->next)
  {
    UI_Widget_Desc desc = child->desc;
    v3 bg_min = v2_to_v3(desc.p_min, z_at); 
    v3 bg_max = v2_to_v3(desc.p_max, z_at); 
    
    v4 color_fg = desc.style.color_fg;
    v4 color_bg = desc.style.color_bg;
    if (ui_widget_id_equals(ui->widget_next_hot, child->id))
    {
      color_fg = desc.style.color_bg;
      color_bg = desc.style.color_fg;
    }
    if (ui_widget_id_equals(ui->widget_hot, child->id))
    {
      color_fg = desc.style.color_fg;
      color_bg = desc.style.color_bg;
    }
    
    if (has_flag(child->desc.style.flags, UIWidgetStyle_Outline))
    {
      ui_sprite_push(ui, bg_min, bg_max, WHITE_SPRITE_ID, color_fg);
      z_at += z_step;
      bg_min += v3{ 1, 1, 0};
      bg_max -= v3{ 1, 1, 0};
      
    }
    
    if (has_flag(child->desc.style.flags, UIWidgetStyle_Bg))
    {
      bg_min.z = z_at;
      bg_max.z = z_at;
      ui_sprite_push(ui, bg_min, bg_max, desc.style.sprite, color_bg);
      z_at += z_step;
    }
    
    if (has_flag(child->desc.style.flags, UIWidgetStyle_TextWrap | UIWidgetStyle_TextClip))
    {
      r32 space_width = ui->font_space_width;
      r32 to_baseline = ui->font_line_gap + ui->font_ascent;
      v3 line_offset = { 5, 3 + to_baseline, 0 };
      r32 baseline_x_start = desc.p_min.x + line_offset.x;
      r32 baseline_y_start = desc.p_min.y + line_offset.y;
      v3 baseline = { baseline_x_start, baseline_y_start, z_at };
      for (u64 i = 0; i < child->desc.string.len; i++)
      {
        u8 at = child->desc.string.str[i];
        UI_Char_Draw_Cmd cmd = {};
        if (!char_is_space(at))
        {
          cmd = ui_sprite_char_get_draw_cmd(ui, baseline, (u32)at);
        }
        else
        {
          cmd.baseline_after = baseline;
          cmd.baseline_after.x += space_width;
        }
        
        if (cmd.baseline_after.x >= desc.p_max.x - 5)
        {
          if (has_flag(child->desc.style.flags, UIWidgetStyle_TextClip)) break;
          
          baseline.x = baseline_x_start;
          baseline.y += ui->font_ascent + ui->font_descent + ui->font_line_gap;
          cmd = ui_sprite_char_get_draw_cmd(ui, baseline, (u32)at);
        }
        
        if (!char_is_space(at))
        {
          ui_quad_push(ui, cmd.pmin, cmd.pmax, cmd.uv.xy, cmd.uv.zw, color_fg);
        }
        baseline = cmd.baseline_after;
      }
    }
    
    if (child->child_first)
    {
      z_at = ui_widgets_to_geometry_recursive(ui, child, z_at + z_step, z_step);
    }
    
    z_at += z_step;
  }
  return z_at;
}

///////////////////////////////////////////
// Layout Manager

internal void
ui_layout_row_begin(UI_Layout* layout, u32 cols)
{
  layout->mode = UILayout_Rows;
  layout->cols = cols;
}
internal void
ui_layout_row_begin(UI* ui, u32 cols) { ui_layout_row_begin(ui->layout, cols); }

internal void
ui_layout_row_end(UI_Layout* layout)
{
  layout->mode = UILayout_Columns;
}
internal void
ui_layout_row_end(UI* ui) { ui_layout_row_end(ui->layout); }

internal UI_Layout_Bounds
ui_layout_get_next(UI_Layout* layout)
{
  UI_Layout_Bounds result = {};
  switch (layout->mode)
  {
    case UILayout_Columns:
    {
      result.min = layout->at;
      result.max = v2{ layout->bounds_max.x, layout->at.y + layout->row_height };
      layout->at = v2{ layout->bounds_min.x, result.max.y + layout->row_gap};
    } break;
    
    case UILayout_Rows:
    {
      r32 col_width = (layout->bounds_max.x - layout->bounds_min.x) / layout->cols;
      col_width -= (layout->cols - 1) * layout->col_gap;
      result.min = layout->at;
      result.max = v2{ layout->at.x + col_width, layout->at.y + layout->row_height };
      layout->at = v2{ result.max.x + layout->col_gap, layout->at.y };
      if (layout->at.x >= layout->bounds_max.x)
      {
        layout->at = v2{ 
          layout->bounds_min.x, 
          layout->at.y + layout->row_height + layout->row_gap
        };
      }
    } break;
    
    invalid_default_case;
  }
  return result;
}
internal UI_Layout_Bounds 
ui_layout_get_next(UI* ui)  { return ui_layout_get_next(ui->layout); }


///////////////////////////////////////////
// Specific Widget Implementations
//
// These all rely on a layout manager to make calling them simpler

global UI_Style_Sheet ui_default_style_sheet = {};

internal void
ui_create_default_style_sheet()
{
  ui_default_style_sheet.styles[UIWidget_Text] = {
    (UIWidgetStyle_TextWrap), v4{0,0,0,0}, WHITE_V4, WHITE_SPRITE_ID
  };
  ui_default_style_sheet.styles[UIWidget_Button] = {
    (UIWidgetStyle_TextClip | UIWidgetStyle_Bg | UIWidgetStyle_Outline | UIWidgetStyle_MouseClick), BLACK_V4, WHITE_V4, WHITE_SPRITE_ID
  };
  ui_default_style_sheet.styles[UIWidget_Toggle] = {
    (UIWidgetStyle_TextClip | UIWidgetStyle_Bg | UIWidgetStyle_MouseClick), BLACK_V4, WHITE_V4, WHITE_SPRITE_ID
  };
  ui_default_style_sheet.styles[UIWidget_Menu] = ui_default_style_sheet.styles[UIWidget_Toggle];
  ui_default_style_sheet.styles[UIWidget_Dropdown] = ui_default_style_sheet.styles[UIWidget_Toggle];
  
  ui_default_style_sheet.styles[UIWidget_HSlider] = {
    (UIWidgetStyle_TextClip | UIWidgetStyle_Bg | UIWidgetStyle_Outline | UIWidgetStyle_MouseDrag), BLACK_V4, WHITE_V4, WHITE_SPRITE_ID
  };
  ui_default_style_sheet.styles[UIWidget_VSlider] = {
    (UIWidgetStyle_TextClip | UIWidgetStyle_Bg | UIWidgetStyle_Outline | UIWidgetStyle_MouseDrag), BLACK_V4, WHITE_V4, WHITE_SPRITE_ID
  };
  ui_default_style_sheet.styles[UIWidget_HScroll] = {
    (UIWidgetStyle_TextClip | UIWidgetStyle_Bg | UIWidgetStyle_Outline | UIWidgetStyle_MouseDrag), BLACK_V4, WHITE_V4, WHITE_SPRITE_ID
  };
  ui_default_style_sheet.styles[UIWidget_VScroll] = {
    (UIWidgetStyle_TextClip | UIWidgetStyle_Bg | UIWidgetStyle_Outline | UIWidgetStyle_MouseDrag), BLACK_V4, WHITE_V4, WHITE_SPRITE_ID
  };
  
  ui_default_style_sheet.styles[UIWidget_Window] = {
    (UIWidgetStyle_TextWrap), BLACK_V4, WHITE_V4, WHITE_SPRITE_ID
  };
  
};

internal UI_Widget_Style
ui_get_style(UI* ui, UI_Widget_Kind kind)
{
  if (ui->style_sheet) return ui->style_sheet->styles[kind];
  return ui_default_style_sheet.styles[kind];
}

internal UI_Widget_Desc
ui_layout_next_widget(UI* ui, UI_Widget_Kind kind)
{
  UI_Layout_Bounds b = ui_layout_get_next(ui);
  UI_Widget_Desc d = {};
  d.p_min = b.min;
  d.p_max = b.max;
  d.style = ui_get_style(ui, kind);
  return d;
}

internal void
ui_text(UI* ui, String string)
{
  UI_Widget_Desc d = ui_layout_next_widget(ui, UIWidget_Text);
  d.string = string;
  ui_widget_push(ui, d);
}

internal bool
ui_button(UI* ui, String string)
{
  UI_Widget_Desc d = ui_layout_next_widget(ui, UIWidget_Button);
  d.string = string;
  UI_Widget_Result r = ui_widget_push(ui, d);
  return has_flag(r.flags, UIWidgetResult_MouseLeft_WentUp);
}

internal bool
ui_toggle(UI* ui, String string, bool value)
{
  UI_Widget_Desc d = ui_layout_next_widget(ui, UIWidget_Button);
  if (value) {
    v4 t = d.style.color_fg;
    d.style.color_fg = d.style.color_bg;
    d.style.color_bg = t;
  }
  d.string = string;
  UI_Widget_Result r = ui_widget_push(ui, d);
  bool result = value;
  if (has_flag(r.flags, UIWidgetResult_MouseLeft_WentUp)) result = !result;
  return result;
}
