/* date = March 28th 2022 10:52 pm */

#ifndef LUMENARIUM_UI_H
#define LUMENARIUM_UI_H

/////////////////////////////////////////////////////////////
// Interface

typedef struct UI_Vertex UI_Vertex;
struct UI_Vertex
{
  v4 pos;
  v2 uv;
  v4 color;
};

#define UI_WIDGET_ID_VALID_BIT 1 << 31

typedef union UI_Widget_Id UI_Widget_Id;
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
  UIWidgetStyle_None        = 0,
  UIWidgetStyle_Bg          = 1 << 0,
  UIWidgetStyle_TextClip    = 1 << 1,
  UIWidgetStyle_TextWrap    = 1 << 2,
  UIWidgetStyle_Outline     = 1 << 3,
  UIWidgetStyle_MouseClick  = 1 << 4,
  UIWidgetStyle_MouseDragH  = 1 << 5,
  UIWidgetStyle_MouseDragV  = 1 << 6,
  UIWidgetStyle_FillH       = 1 << 7,
  UIWidgetStyle_FillV       = 1 << 8,
  UIWidgetStyle_LineInsteadOfFill = 1 << 9,
};

// akin to a css class, could be used to style multiple
// elements
typedef struct UI_Widget_Style UI_Widget_Style;
struct UI_Widget_Style
{
  UI_Widget_Style_Flags flags;
  v4 color_bg;
  v4 color_fg;
  u32 sprite;
};

// combination of style info and per-instance data
typedef struct UI_Widget_Desc UI_Widget_Desc;
struct UI_Widget_Desc
{
  UI_Widget_Style style;
  v2 fill_pct;
  String string;
  v2 p_min;
  v2 p_max;
};

typedef struct UI_Widget UI_Widget;
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

typedef struct UI_Widget_Result UI_Widget_Result;
struct UI_Widget_Result
{
  UI_Widget_Id id;
  UI_Widget_Result_Flags flags;
  v2 drag;
};

typedef u32 UI_Widget_Kind;
enum
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

typedef struct UI_Style_Sheet UI_Style_Sheet;
struct UI_Style_Sheet
{
  UI_Widget_Style styles[UIWidget_Count];
};

typedef struct UI_Widget_State UI_Widget_State;
struct UI_Widget_State
{
  v2 scroll;
};

typedef struct UI_Widget_Pool UI_Widget_Pool;
struct UI_Widget_Pool
{
  UI_Widget* free;
  u32        free_cap;
  u32        free_len;
  
  UI_Widget* root;
  UI_Widget* active_parent;
  
  UI_Widget_State* states;
  u32* states_hash;
  u32 states_cap;
};

typedef u8 UI_Layout_Mode;
enum
{
  // each element takes up a whole row
  UILayout_Columns,
  
  // each element takes up one column in the row. If you overflow, 
  // the layout manager overflows to the next row 
  UILayout_Rows,
};

typedef struct UI_Layout UI_Layout;
struct UI_Layout
{
  UI_Layout_Mode mode;
  UI_Layout* parent;
  
  v2 bounds_min;
  v2 bounds_max;
  r32 row_height;
  r32 row_gap;
  r32 col_gap;
  v2 at;
  u32 cols;
};

typedef struct UI_Layout_Bounds UI_Layout_Bounds;
struct UI_Layout_Bounds
{
  v2 min;
  v2 max;
};

typedef void UI_Draw_Panel_Cb(u8* user_data, BSP_Node_Id id, BSP_Node node, BSP_Area area);

typedef struct UI UI;
struct UI
{
  Geo_Quad_Buffer_Builder geo;
  
  Texture_Atlas atlas;
  r32 font_ascent, font_descent, font_line_gap, font_space_width;
  r32 font_texture_scale;
  
  UI_Widget_Pool widgets;
  UI_Style_Sheet* style_sheet;
  
  UI_Widget_Id widget_next_hot;
  UI_Widget_Id widget_hot;
  
  UI_Layout* layout;
  
  BSP panels;
  UI_Draw_Panel_Cb* draw_panel_cb;
  u8*               draw_panel_cb_data;
  
  // frames since these values were set
  u16          widget_next_hot_frames;
  u16          widget_hot_frames;
  
  Input_State* input;
  
  m44 proj;
  Shader shader;
  Texture atlas_texture;
  Geometry_Buffer per_frame_buffer;

  Allocator* per_frame;
};

// Interface

internal UI ui_create();
internal void ui_quad_push(UI* ui, v3 pmin, v3 pmax, v2 tmin, v2 tmax, v4 c);
internal void ui_sprite_register(UI* ui, u8* pixels, u32 w, u32 h, u32 id);
internal void ui_sprite_push_color(UI* ui, v3 pmin, v3 pmax, u32 id, v4 color);
internal void ui_sprite_push(UI* ui, v3 pmin, v3 pmax, u32 id);
internal v3   ui_sprite_char_push(UI* ui, v2 at, u32 codepoint, v4 color);
internal void ui_draw(UI* ui);

// Widgets

internal void ui_create_default_style_sheet();

internal UI_Widget_Id     ui_widget_id_create(String string, u32 index_in_parent);
internal bool             ui_widget_id_equals(UI_Widget_Id a, UI_Widget_Id b);
internal bool             ui_widget_id_is_valid(UI_Widget_Id h);

internal void ui_widget_next_hot_set(UI* ui, UI_Widget* w);
internal void ui_widget_hot_set(UI* ui, UI_Widget* w);

internal UI_Widget* ui_widget_pool_push(UI_Widget_Pool* pool, String string);
internal void       ui_widget_pool_pop(UI_Widget_Pool* pool);

internal UI_Widget_Result ui_widget_push(UI* ui, UI_Widget_Desc desc);
internal void             ui_widget_pop(UI* ui, UI_Widget_Id widget_id);

internal r32 ui_widgets_to_geometry_recursive(UI* ui, UI_Widget* widget, r32 z_start, r32 z_step);

#endif //LUMENARIUM_UI_H
