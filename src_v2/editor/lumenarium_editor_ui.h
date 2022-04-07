/* date = March 28th 2022 10:52 pm */

#ifndef LUMENARIUM_UI_H
#define LUMENARIUM_UI_H

/////////////////////////////////////////////////////////////
// Interface

struct UI_Vertex
{
  v4 pos;
  v2 uv;
  v4 color;
};

#define UI_WIDGET_ID_VALID_BIT 1 << 31

union UI_Widget_Id
{
  // equality of widget id's only relies on the value field
  // which is a hash of the widget's string
  u32 value;
  
  // this struct tracks the index of the widget only to be able
  // to override next hot in cases where an earlier element became
  // next hot that is in the same location as the current one
  u32 index;
};

typedef u32 UI_Widget_Style_Flags;
enum
{
  UIWidgetStyle_None       = 0,
  UIWidgetStyle_Bg         = 1,
  UIWidgetStyle_TextClip   = 2,
  UIWidgetStyle_TextWrap   = 4,
  UIWidgetStyle_Outline    = 8,
  UIWidgetStyle_MouseClick = 16,
  UIWidgetStyle_MouseDrag  = 32,
};

// akin to a css class, could be used to style multiple
// elements
struct UI_Widget_Style
{
  UI_Widget_Style_Flags flags;
  v4 color_bg;
  v4 color_fg;
  
  u32 sprite;
};

// combination of style info and per-instance data
struct UI_Widget_Desc
{
  UI_Widget_Style style;
  String string;
  v2 p_min;
  v2 p_max;
};

struct UI_Widget
{
  UI_Widget_Id id;
  UI_Widget_Desc desc;
  
  UI_Widget* parent;
  UI_Widget* next;
  UI_Widget* child_first;
  UI_Widget* child_last;
};

typedef u32 UI_Widget_Result_Flags;
enum
{
  UIWidgetResult_None = 0,
  UIWidgetResult_MouseLeft_IsDown = 1,
  UIWidgetResult_MouseLeft_WentUp = 2,
};

struct UI_Widget_Result
{
  UI_Widget_Result_Flags flags;
};

enum UI_Widget_Kind
{
  UIWidget_Text,
  
  // Buttons
  UIWidget_Button,
  UIWidget_Toggle,
  UIWidget_Menu,
  UIWidget_Dropdown,
  
  // Sliders
  UIWidget_HSlider,
  UIWidget_VSlider,
  UIWidget_HScroll,
  UIWidget_VScroll,
  
  // Panels
  UIWidget_Window,
  
  UIWidget_Count,
};

struct UI_Style_Sheet
{
  UI_Widget_Style styles[UIWidget_Count];
};

struct UI_Widget_Pool
{
  UI_Widget* free;
  u32        free_cap;
  u32        free_len;
  
  UI_Widget* root;
  UI_Widget* active_parent;
};

enum UI_Layout_Mode
{
  // each element takes up a whole row
  UILayout_Columns,
  
  // each element takes up one column in the row. If you overflow, 
  // the layout manager overflows to the next row 
  UILayout_Rows,
};

struct UI_Layout
{
  v2 bounds_min;
  v2 bounds_max;
  r32 row_height;
  r32 row_gap;
  r32 col_gap;
  v2 at;
  UI_Layout_Mode mode;
  u32 cols;
};

struct UI_Layout_Bounds
{
  v2 min;
  v2 max;
};

struct UI
{
  UI_Vertex* verts;
  u32        verts_len;
  u32        verts_cap;
  
  u32* indices;
  u32  indices_len;
  u32  indices_cap;
  
  Texture_Atlas atlas;
  r32 font_ascent, font_descent, font_line_gap, font_space_width;
  
  UI_Widget_Pool widgets;
  UI_Style_Sheet* style_sheet;
  
  UI_Widget_Id widget_next_hot;
  UI_Widget_Id widget_hot;
  
  UI_Layout* layout;
  
  // frames since these values were set
  u16          widget_next_hot_frames;
  u16          widget_hot_frames;
  
  Input_State* input;
  
  m44 proj;
  Platform_Shader shader;
  Platform_Texture atlas_texture;
  Platform_Geometry_Buffer per_frame_buffer;
};

// Interface

internal UI ui_create();
internal void ui_quad_push(UI* ui, v3 pmin, v3 pmax, v2 tmin, v2 tmax, v4 c);
internal void ui_sprite_register(UI* ui, u8* pixels, u32 w, u32 h, u32 id);
internal void ui_sprite_push(UI* ui, v3 pmin, v3 pmax, u32 id, v4 color);
internal void ui_sprite_push(UI* ui, v3 pmin, v3 pmax, u32 id);
internal v3   ui_sprite_char_push(UI* ui, v2 at, u32 codepoint, v4 color);
internal void ui_draw(UI* ui);

// Widgets

internal void ui_create_default_style_sheet();

internal UI_Widget_Id     ui_widget_id_create(u32 index_in_parent, String string);
internal bool             ui_widget_id_equals(UI_Widget_Id a, UI_Widget_Id b);
internal bool             ui_widget_id_is_valid(UI_Widget_Id h);

internal UI_Widget* ui_widget_pool_push(UI_Widget_Pool* pool, String string);
internal void       ui_widget_pool_pop(UI_Widget_Pool* pool);

internal UI_Widget_Result ui_widget_push(UI* ui, UI_Widget_Desc desc);
internal void             ui_widget_pop(UI* ui, UI_Widget* widget);

internal r32 ui_widgets_to_geometry_recursive(UI* ui, UI_Widget* widget, r32 z_start, r32 z_step);

#endif //LUMENARIUM_UI_H
