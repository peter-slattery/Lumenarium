#ifndef SLOTH_H
#define SLOTH_H

// DOCUMENTATION
// 
// Bare minimum usage requirements:
// Copy the following into ONE file in your codebase
//
//   #define SLOTH_IMPLEMENTATION 1
//   #include "stb_sprintf.h"
//   #include "sloth.h"
//
// Examples:
// 
// Immediately following this block comment is an example
// of setting sloth.h up with a sokol_app/sokol_gfx environment.
//
// How It Works:
//
// sloth.h exposes an immediate mode interface that allows
// you to write expressive interfaces via simple, composable
// commands.
// 
// For example, a button might look like:
//     if (my_button(&sloth_ctx).clicked) {
//       // respond to button click
//     }
//
// Internally, sloth is actually a retained mode ui that
// maintains a tree of widgets which make up the interface.
// That tree is pruned and growns each frame based on the 
// immediate mode function calls.
//
// So if on Frame N, you call my_button for the first time
// a widget will be added to the widget tree.
// On Frame N+1, if you call my_button again, the existing
// widget is used.
// On Frame N+2, if you don't call my_button, the widget
// corresponding to the button will be removed from the tree.
//
// This allows for an immediate mode interface (simple, easy to understand)
// that is rendered with arbitrarily complex layout routines
// (expressive, hand tailored).
//
// The one drawback of this is that all user input must happen
// on a one frame delay, since a widget doesn't know its
// size and position until the layout routine has been run,
// which happens at the end of the frame. 
// This is usually a small price to pay.

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// TODO
// - CLEANUP
//   - remove _OLD functions
//   - remove widget_tree_last_addition, since we don't use that anymore?
//     (pending)
// 
// - copying selected text
// - UTF-8 and UTF-32 Support in fonts and glyphs
//   - Glpyh IDs should have an 8 bit family and a 32 bit id, so that
//     the id supports utf8 
// - Figure out things like weight and bold/italics for font rendering
// - Widget contents scrolling
// - glyph atlas should use premultiplied alpha
// - if we use a separate texture for different glyph families, is there 
//   a way we could use one bit alpha mask textures for families that
//   only have text glyphs?
//   - possibly, we could default to one bit textures until it contains
//     an image that has rgb data in it, and then upgrade the texture
//     to rgb?
// - popup buffer - a way to push widgets onto a buffer that gets added
//   to the vertex buffer later. Either that or a way to make the buffer
//   respect z distance to the camera?
// - widget pool growing reallocates in the middle of a frame. We need to 
//   turn it into a buffer arena
// - Constant line height for text of a particular family
//   right now, the line height changes based on if it has a glyph with
//   a descender on it or not

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 
// OPTIMIZATION CANDIDATES:
//
// - widgets currently contain their entire style, layout, and input 
//   descriptors. Creating pointers to these will make the structure
//   size of each widget much smaller (especially as style and layout 
//   continue to grow). 
//   - One downside is that you'd want to force users to register 
//     these descriptors so that they have known memory lifetimes. 
//     On the other hand, this would also reduce duplicate memory
//     storage of the same descriptor.
//   - One thing that could work is to hash the descriptors when they
//     are provided, and either point to an existing one thats already
//     stored, or store the new one. 
//     This would require some eviction strategy.
//   - This might make things like style composability an option
//     ie. The default for fields becomes (INHERIT OR DEFAULT)
//     On the other hand, having to loop looking upwards in a hierarchy
//     isn't great. Maybe at registration time, the actual values for
//     every field are computed (a styles parent would have to be known
//     at registration time)
// 
// - Turn Glyph_Layout's into a discriminated union that serves one of
//   several purposes:
//   - Describe a glyph
//   - Change active font properties like typeface, size, bold, etc.
//     (effectively, start a span)
// 
// - Long Text - odds are, long blocks of text will not all be visible
//   at once. 

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//  SAMPLE PROGRAM
//

#if 0

TODO

#endif


#ifndef STB_SPRINTF_H_INCLUDE
#  error "sloth.h relies on stb_sprintf.h. Include it first."
#endif

//////// TYPE DEFINITIONS ////////
// NOTE: These allow for the standard
// types to be overridden by outer code
// simply by defining the macro first

#ifndef Sloth_Char
#  define Sloth_Char char
#endif

#ifndef Sloth_U8
#  define Sloth_U8 unsigned char
#endif

#ifndef Sloth_U32
#  define Sloth_U32 unsigned int
#endif

#ifndef Sloth_U64
#  define Sloth_U64 unsigned long long int
#endif

#ifndef Sloth_S32
#  define Sloth_S32 int
#endif

#ifndef Sloth_R32
#  define Sloth_R32 float
#endif
#define Sloth_R32_Max  3.402823466e+38f
#define Sloth_R32_Min -3.402823466e+38f

#ifndef Sloth_Bool
#  define Sloth_Bool bool
#endif

#ifndef Sloth_Function
#  define Sloth_Function
#endif

#ifndef Sloth_Temp_String_Memory_Size
#  define Sloth_Temp_String_Memory_Size 512
#endif

#ifndef sloth_realloc
Sloth_Function Sloth_U8*
sloth_realloc_wrapper(Sloth_U8* base, Sloth_U32 old_size, Sloth_U32 new_size)
{
  if (new_size == 0) {
    free(base);
    return 0;
  } else {
    return (Sloth_U8*)realloc(base, new_size);
  }
}
#  define sloth_realloc(base, old_size, new_size) sloth_realloc_wrapper((Sloth_U8*)(base), (old_size), (new_size))
#endif

#ifndef sloth_free
#  define sloth_free(base, size) free(base);
#endif

#ifndef sloth_assert
#  ifdef DEBUG
#    define sloth_assert(c) if (!(c)) { do{ *((volatile int*)0) = 0xFFFF; }while(0); }
#  else
#    define sloth_assert(c)
#  endif
#endif

#define sloth_invalid_code_path sloth_assert(false)
#ifdef DEBUG
#  define sloth_invalid_default_case default: { sloth_invalid_code_path; } break;
#else
#  define sloth_invalid_default_case default: {} break;
#endif

#ifndef SLOTH_PROFILE_BEGIN
#  define SLOTH_PROFILE_BEGIN
#endif

//////// DATA TYPES  ////////

typedef struct Sloth_Ctx Sloth_Ctx;

typedef struct Sloth_ID Sloth_ID;
struct Sloth_ID
{
  Sloth_U32 value;
};

typedef struct Sloth_ID_Result Sloth_ID_Result;
struct Sloth_ID_Result
{
  Sloth_ID id;
  Sloth_U32 display_len;
  Sloth_Char* formatted;
};

typedef union Sloth_V2 Sloth_V2;
union Sloth_V2
{
  struct {
    Sloth_R32 x;
    Sloth_R32 y;
  };
  Sloth_R32 E[2];
};

typedef union Sloth_V4 Sloth_V4;
union Sloth_V4
{
  struct {
    Sloth_R32 x;
    Sloth_R32 y;
    Sloth_R32 z;
    Sloth_R32 w;
  };
  struct {
    Sloth_R32 r;
    Sloth_R32 g;
    Sloth_R32 b;
    Sloth_R32 a;
  };
  Sloth_R32 E[4];
};

typedef union Sloth_Rect Sloth_Rect;
union Sloth_Rect
{
  struct {
    Sloth_V2 value_min;
    Sloth_V2 value_max;
  };
};

enum {
  Sloth_Axis_X = 0,
  Sloth_Axis_Y = 1,
};

// HASHTABLE
// Implementation Details
// - Open Addressing - on collision, the table probes outward looking for
//   a suitable empty slot
// - Robin Hood Hashing - when probing, keys that will probe more times
//   are stored first, increasing lookup speed.
typedef struct Sloth_Hashtable Sloth_Hashtable;
struct Sloth_Hashtable
{
  Sloth_U32* keys;
  Sloth_U8**  values;

  // The allocated count of keys and values
  // ie. the size of keys is sizeof(U32) * cap;
  Sloth_U32  cap;

  // The total number of registered values
  Sloth_U32  used;
};

// ARENA
// A push buffer arena - only supports push/pop memory and clearing
typedef struct Sloth_Arena Sloth_Arena;
struct Sloth_Arena
{
  char* name;
  Sloth_U8** buckets;
  Sloth_U32 buckets_cap;
  Sloth_U32 buckets_len;
  Sloth_U32 bucket_cap;
  Sloth_U32 curr_bucket_len;
};

typedef struct Sloth_Arena_Loc Sloth_Arena_Loc;
struct Sloth_Arena_Loc
{
  Sloth_U32 bucket_index;
  Sloth_U32 bucket_at; // pos in bucket
};

typedef union Sloth_Glyph_ID Sloth_Glyph_ID;
union Sloth_Glyph_ID
{
  Sloth_U32 value;
  struct {
    Sloth_U8 id[3];
    Sloth_U8 family;
  };
};

typedef struct Sloth_Font_Weight_Family Sloth_Font_Weight_Family;
struct Sloth_Font_Weight_Family
{
  Sloth_U32 weight;
  Sloth_U32 glyph_family;
};

#define SLOTH_FONT_WEIGHTS_CAP 8

typedef struct Sloth_Font_ID Sloth_Font_ID;
struct Sloth_Font_ID
{
  Sloth_U32 value;
  Sloth_U32 weight_index;
};

typedef struct Sloth_Font_Metrics Sloth_Font_Metrics;
struct Sloth_Font_Metrics
{
  Sloth_R32 pixel_height;
  Sloth_R32 line_height;
};

typedef struct Sloth_Font Sloth_Font;
struct Sloth_Font
{
  char name[32];
  Sloth_U8* renderer_data;
  Sloth_Font_Metrics metrics;
  Sloth_Font_Weight_Family weights[SLOTH_FONT_WEIGHTS_CAP];
  Sloth_U32 weights_len;
};

typedef Sloth_U8*      Sloth_Font_Renderer_Load_Font(Sloth_Ctx* sloth, Sloth_Font* font, Sloth_U8* font_data, Sloth_U32 font_data_size, Sloth_U32 font_index, Sloth_R32 pixel_height);
typedef Sloth_Glyph_ID Sloth_Font_Renderer_Register_Glyph(Sloth_Ctx* sloth, Sloth_Font_ID font_id, Sloth_U32 codepoint);

typedef Sloth_U32 Sloth_Glyph_Data_Format;
enum 
{
  Sloth_GlyphData_RGBA8,
  Sloth_GlyphData_RGB8,
  Sloth_GlyphData_Alpha8,
};

static Sloth_U32 sloth_glyph_data_format_strides[] = {
  4, // Sloth_GlyphData_RGBA8
  3, // Sloth_GlyphData_RGB8
  1, // Sloth_GlyphData_Alpha8
};

typedef struct Sloth_Glyph_Desc Sloth_Glyph_Desc;
struct Sloth_Glyph_Desc
{
  Sloth_U32 family;
  Sloth_U32 id;
  Sloth_U8* data;
  Sloth_U32 width;
  Sloth_U32 height;
  Sloth_U32 stride;
  Sloth_Glyph_Data_Format format;
  
  Sloth_R32 cursor_to_glyph_start_xoff;
  Sloth_R32 cursor_to_next_glyph;
  Sloth_R32 baseline_offset_y;
  
  Sloth_R32 copy_gamma;
};

typedef struct Sloth_Glyph Sloth_Glyph;
struct Sloth_Glyph
{
  Sloth_U32 width;
  Sloth_U32 height;
  Sloth_U32 offset_x;
  Sloth_U32 offset_y;
  
  Sloth_R32 lsb;
  Sloth_R32 x_advance;
  Sloth_R32 baseline_offset_y;
};

typedef struct Sloth_Glyph_Atlas Sloth_Glyph_Atlas;
struct Sloth_Glyph_Atlas
{
  // TODO: Probably want a way to have more than one texture.
  Sloth_U8* data;
  Sloth_U32 dim; // the texture will always be a power of 2
  Sloth_U32 last_glyph;
  Sloth_U32 last_row_first_glyph;

  Sloth_Glyph* glyphs;
  Sloth_U32 glyphs_cap;
  Sloth_Hashtable glyphs_table;
  
  Sloth_U8 is_dirty;
};

typedef struct Sloth_Glyph_Info Sloth_Glyph_Info;
struct Sloth_Glyph_Info
{
  Sloth_Glyph glyph;
  
  Sloth_Rect uv;
};

typedef void Sloth_Renderer_Atlas_Updated(Sloth_Ctx* sloth);

typedef Sloth_U8 Sloth_Size_Kind;
enum {
  Sloth_SizeKind_None,
  Sloth_SizeKind_Pixels,
  Sloth_SizeKind_TextContent,
  Sloth_SizeKind_PercentOfSelf, // NOTE: not valid for widget width/height fields
  Sloth_SizeKind_PercentOfParent,
  Sloth_SizeKind_ChildrenSum,
  Sloth_SizeKind_Count,
};

typedef struct Sloth_Size Sloth_Size;
struct Sloth_Size
{
  Sloth_R32 value;
  Sloth_Size_Kind kind;
};

typedef union Sloth_Size_Range Sloth_Size_Range;
union Sloth_Size_Range
{
  struct {
    Sloth_Size min;
    Sloth_Size max;
  };
  Sloth_Size E[2];
};

typedef union Sloth_Size_Box Sloth_Size_Box;
union Sloth_Size_Box
{
  struct {
    // @Maintenance
    // left & right correspond to E[Sloth_Axis_X].min and E[Sloth_Axis_X].max
    // top & bottom correspond to E[Sloth_Axis_Y].min and E[Sloth_Axis_Y].max
    Sloth_Size left;
    Sloth_Size right;
    
    Sloth_Size top;    
    Sloth_Size bottom;
  };
  Sloth_Size_Range E[2];
};

typedef Sloth_U8 Sloth_Layout_Direction;
enum {
  Sloth_LayoutDirection_TopDown,
  Sloth_LayoutDirection_BottomUp,
  Sloth_LayoutDirection_LeftToRight,
  Sloth_LayoutDirection_RightToLeft,
};

typedef Sloth_U8 Sloth_Layout_Position_Kind;
enum {    
  // The parent elements Sloth_Layout_Direction value
  // dictates how this child gets layed out. Subsequent
  // siblings will reference this widget's position to
  // determine their position
  Sloth_LayoutPosition_ParentDecides,
  
  // The widget's margin.top and margin.left are used to 
  // offset this widget from its parents top and left.
  // Subsequent siblings will ignore this widget in their
  // layout passes
  Sloth_LayoutPosition_FixedInParent,
  
  // The widget's margin.top and margin.left are used to 
  // offset this widget from the top and left of the screen
  // Subsequent siblings will ignore this widget in their
  // layout passes
  Sloth_LayoutPosition_FixedOnScreen,
};

typedef struct Sloth_Layout_Position Sloth_Layout_Position;
struct Sloth_Layout_Position 
{
  Sloth_Layout_Position_Kind kind;
  union {
    Sloth_Size_Box at;
    struct {
      Sloth_Size left;
      Sloth_Size right;
      Sloth_Size top;
      Sloth_Size bottom;
    };
  };
  Sloth_R32 z;
};

typedef struct Sloth_Widget_Layout Sloth_Widget_Layout;
struct Sloth_Widget_Layout
{
  union {
    struct {
      Sloth_Size width;
      Sloth_Size height;
    };
    Sloth_Size size[2];
  };
  
  Sloth_Size_Box margin;

  Sloth_Layout_Direction direction;
  Sloth_Layout_Position position;
};

typedef Sloth_U32 Sloth_Text_Style_Flags;
enum 
{
  Sloth_TextStyle_Defaults      = 0,
  
  // Alignment
  Sloth_TextStyle_Align_Left    = 1,
  Sloth_TextStyle_Align_Center  = 2,
  Sloth_TextStyle_Align_Right   = 4,
  
  // Wrapping
  // default is to wrap
  Sloth_TextStyle_NoWrapText = 8,
};

typedef struct Sloth_Widget_Style Sloth_Widget_Style;
struct Sloth_Widget_Style
{
  Sloth_U32 color_bg;
  Sloth_U32 color_text;
  Sloth_U32 color_outline;
  
  Sloth_Glyph_ID bg_glyph;
  
  Sloth_R32 outline_thickness;
  
  // Top Left, Top Right, Bottom Right, Bottom Left
  Sloth_Size border_radius[4];
  
  Sloth_Text_Style_Flags text_style;
};

typedef Sloth_U32 Sloth_Widget_Input_Flags;
enum
{
  Sloth_WidgetInput_None = 0,
  Sloth_WidgetInput_Draggable = 1,
  Sloth_WidgetInput_TextSelectable = 2,
  Sloth_WidgetInput_TextCursor = 4,
  Sloth_WidgetInput_DoNotCaptureMouse = 8,
};

typedef struct Sloth_Widget_Input Sloth_Widget_Input;
struct Sloth_Widget_Input
{
  Sloth_Widget_Input_Flags flags;
};

typedef struct Sloth_Widget_Desc Sloth_Widget_Desc;
struct Sloth_Widget_Desc
{
  Sloth_Widget_Layout layout;
  Sloth_Widget_Style  style;
  Sloth_Widget_Input  input;
};

typedef struct Sloth_Glyph_Layout Sloth_Glyph_Layout;
struct Sloth_Glyph_Layout
{
  Sloth_Glyph_ID glyph_id;
  Sloth_Glyph_Info info;
  Sloth_Rect bounds;
  Sloth_U32 color;
  
  // TODO: Hanging bytes, could be much better used space
  // will take care of later
  Sloth_U8 draw; 
  Sloth_U8 is_line_start;
  Sloth_U8 selected;
};

typedef struct Sloth_Widget_Cached Sloth_Widget_Cached;
struct Sloth_Widget_Cached
{
  Sloth_U8 canary_start_;
  
  // Effective Desc
  // Fields that are used during interaction
  // These are cached from the previous frame
  Sloth_V2 offset; // offset from parent origin
  Sloth_V2 dim;    // pixel dimensions
  Sloth_Rect bounds;
  
  // only used if in free list
  Sloth_Widget_Cached* free_next;
  
  Sloth_U8 canary_end_;
};

typedef struct Sloth_Widget Sloth_Widget;
struct Sloth_Widget
{
  char* str; // TEMP

  // Tree Structuring
  Sloth_Widget* child_first;
  Sloth_Widget* child_last;
  Sloth_Widget* sibling_next;
  Sloth_Widget* sibling_prev;
  Sloth_Widget* parent;

  Sloth_ID id;
  Sloth_U32 touched_last_frame;
    
  Sloth_Widget_Cached* cached;
  
  // Primed Desc
  // Fields that describe how to render the 
  // widget, and which will be used for interaction
  // next frame
  Sloth_Widget_Layout layout;
  Sloth_Widget_Style  style;
  Sloth_Widget_Input  input;
  
  Sloth_Glyph_Layout* text;
  Sloth_U32           text_len;
  Sloth_V2            text_dim;
};

typedef struct Sloth_Widget_Pool Sloth_Widget_Pool;
struct Sloth_Widget_Pool
{
  Sloth_Widget* values;
  Sloth_U32 cap;
  Sloth_U32 len;
    
  Sloth_Widget* free_list;
};

typedef struct Sloth_Widget_Cached_Pool Sloth_Widget_Cached_Pool;
struct Sloth_Widget_Cached_Pool
{
  Sloth_Widget_Cached** buckets;
  Sloth_U32 buckets_cap;
  Sloth_U32 buckets_len;
  Sloth_U32 bucket_at;
  Sloth_U32 bucket_at_len;
  Sloth_U32 bucket_cap;
    
  Sloth_Widget_Cached* free_list;
};

typedef struct Sloth_Widget_Result Sloth_Widget_Result;
struct Sloth_Widget_Result
{
  Sloth_Widget* widget;
  
  // mouse input
  Sloth_U8 released;
  Sloth_U8 clicked;
  Sloth_U8 held;
  Sloth_V2 drag_offset_pixels;
  Sloth_V2 drag_offset_percent_parent;
  
  // text input
  Sloth_U32 selected_glyphs_first;
  Sloth_U32 selected_glyphs_one_past_last;
  Sloth_U32 glyph_cursor_pos;
};

#define SLOTH_VERTEX_STRIDE 9

typedef struct Sloth_VIBuffer Sloth_VIBuffer;
struct Sloth_VIBuffer
{
  Sloth_R32* verts;
  Sloth_U32  verts_cap;
  Sloth_U32  verts_len;

  Sloth_U32* indices;
  Sloth_U32  indices_cap;
  Sloth_U32  indices_len;
};

enum {
  SLOTH_DEBUG_DID_CALL_ADVANCE,
  SLOTH_DEBUG_DID_CALL_PREPARE,
};

typedef Sloth_U8 Sloth_Mouse_State;
enum {
  Sloth_MouseState_None = 0,
  Sloth_MouseState_IsDown = 1,
  Sloth_MouseState_WasDown = 2,
};

typedef void Sloth_Render_VIBuffer(Sloth_Ctx* sloth, Sloth_VIBuffer buffer);

struct Sloth_Ctx
{
  Sloth_Arena per_frame_memory;
  Sloth_Arena scratch;
  
  Sloth_Widget_Pool widgets;
  Sloth_Widget_Cached_Pool widget_caches;
  Sloth_Hashtable widget_cache_lut;
  
  // Fonts
  Sloth_U8* font_renderer_data;
  Sloth_Font_Renderer_Load_Font* font_renderer_load_font;
  Sloth_Font_Renderer_Register_Glyph* font_renderer_register_glyph;  
  Sloth_Font* fonts;
  Sloth_U32   fonts_cap;
  Sloth_U32   fonts_len;
  
  // the actual root of the current tree
  Sloth_Widget* widget_tree_root;

  Sloth_Widget* widget_tree_parent_cur;
  Sloth_Widget* widget_tree_last_addition; 

  // what the tree expects the next child will
  // be, based on the tree layout cached from the 
  // previous frame. 
  Sloth_Widget* widget_tree_next_child;
  Sloth_U32 widget_tree_depth_cur;
  Sloth_U32 widget_tree_depth_max;

  // Glyphs & Fonts
  Sloth_Glyph_Atlas glyph_atlas;
  Sloth_Font_ID active_text_glyph_family;
  Sloth_Renderer_Atlas_Updated* renderer_atlas_updated;

  // The geometry to render this frame
  Sloth_VIBuffer vibuf;
  Sloth_U8* render_data;
  
  // Input Tracking
  Sloth_V2 screen_dim;
  Sloth_V2 mouse_pos;
  Sloth_Mouse_State mouse_button_l;
  Sloth_Mouse_State mouse_button_r;
  Sloth_V2 mouse_down_pos;
  
  // 
  Sloth_ID last_active_widget;
  Sloth_ID hot_widget;
  Sloth_U8 hot_widget_age;
  Sloth_ID active_widget;
  Sloth_U8 active_widget_age;
  Sloth_U32 active_widget_selected_glyphs_first;
  Sloth_U32 active_widget_selected_glyphs_one_past_last;
  
  // Debug checks
  Sloth_U32 sentinel;
};

//////// INTERFACE  ////////

// Sloth Id Generation
// An Id will be constructed by hashing the string provided
// If ## appears in the input string, everything before the ##
// will be returned as a display string;
// If ### appears in the input string, the id will be constructed
// only of everything after the ###.
Sloth_Function Sloth_ID_Result sloth_make_id_v(Sloth_Char* fmt, va_list args);
Sloth_Function Sloth_ID_Result sloth_make_id_f(Sloth_Char* fmt, ...);
Sloth_Function Sloth_ID_Result sloth_make_id_len(Sloth_U32 len, Sloth_Char* str);
Sloth_Function Sloth_ID_Result sloth_make_id(Sloth_Char* str);
Sloth_Function Sloth_Bool      sloth_ids_equal(Sloth_ID a, Sloth_ID b);

// Sloth Vector and Rect
Sloth_Function Sloth_V2 sloth_make_v2(Sloth_R32 x, Sloth_R32 y);
Sloth_Function Sloth_V2 sloth_v2_add(Sloth_V2 a, Sloth_V2 b);
Sloth_Function Sloth_V2 sloth_v2_sub(Sloth_V2 a, Sloth_V2 b);
Sloth_Function Sloth_V2 sloth_v2_mulf(Sloth_V2 a, Sloth_R32 b);

Sloth_Function Sloth_Rect sloth_rect_union(Sloth_Rect a, Sloth_Rect b);
Sloth_Function void       sloth_rect_expand(Sloth_Rect* target, Sloth_R32 left, Sloth_R32 top, Sloth_R32 right, Sloth_R32 bottom);
Sloth_Function Sloth_V2   sloth_rect_dim(Sloth_Rect r);
Sloth_Function Sloth_Bool sloth_rect_contains(Sloth_Rect r, Sloth_V2 p);
Sloth_Function Sloth_V2   sloth_rect_get_closest_point(Sloth_Rect r, Sloth_V2 p);

// Size Functions
Sloth_Function Sloth_Size sloth_size(Sloth_Size_Kind k, Sloth_R32 v);
Sloth_Function Sloth_Size sloth_size_pixels(Sloth_R32 v);
Sloth_Function Sloth_Size sloth_size_text_content();
Sloth_Function Sloth_Size sloth_size_percent_parent(Sloth_R32 v);
Sloth_Function Sloth_Size sloth_size_children_sum();

Sloth_Function Sloth_Size_Box sloth_size_box_uniform(Sloth_Size_Kind k, Sloth_R32 v);
Sloth_Function Sloth_Size_Box sloth_size_box_uniform_pixels(Sloth_R32 v);

// Hashtable Functions
Sloth_Function void       sloth_hashtable_add(Sloth_Hashtable* table, Sloth_U32 key, Sloth_U8* value);
Sloth_Function Sloth_Bool sloth_hashtable_rem(Sloth_Hashtable* table, Sloth_U32 key);
Sloth_Function Sloth_U8*  sloth_hashtable_get(Sloth_Hashtable* table, Sloth_U32 key);
Sloth_Function void       sloth_hashtable_free(Sloth_Hashtable* table);

// Arena Functions
Sloth_Function void      sloth_arena_grow(Sloth_Arena* arena, Sloth_U32 min_size);

// convenience function that auto casts to a type size
#define sloth_arena_push_array(arena, type, count) (type*)sloth_arena_push((arena), sizeof(type) * (count))
Sloth_Function Sloth_U8* sloth_arena_push(Sloth_Arena* arena, Sloth_U32 size);

Sloth_Function void            sloth_arena_pop(Sloth_Arena* arena, Sloth_Arena_Loc to);
Sloth_Function Sloth_Arena_Loc sloth_arena_at(Sloth_Arena* arena);
Sloth_Function void            sloth_arena_clear(Sloth_Arena* arena);
Sloth_Function void            sloth_arena_free(Sloth_Arena* arena);

// Fonts
Sloth_Function Sloth_Font_ID  sloth_font_load_from_memory(Sloth_Ctx* sloth, char* font_name, Sloth_U32 font_name_len, Sloth_U8* data, Sloth_U32 data_size, Sloth_R32 pixel_height);
Sloth_Function Sloth_Font_ID  sloth_font_register_family(Sloth_Ctx* sloth, Sloth_Font_ID font_id, Sloth_U32 weight, Sloth_U32 family);
Sloth_Function Sloth_Glyph_ID sloth_font_register_codepoint(Sloth_Ctx* sloth, Sloth_Font_ID font_id, Sloth_U32 codepoint);
Sloth_Function Sloth_Font*    sloth_glyph_to_font(Sloth_Ctx* sloth, Sloth_Glyph_ID glyph_id);
Sloth_Function void           sloth_font_set_metrics(Sloth_Ctx* sloth, Sloth_Font_ID id, Sloth_Font_Metrics metrics);

// Atlas
Sloth_Function void             sloth_glyph_atlas_resize(Sloth_Glyph_Atlas* atlas, Sloth_U32 new_dim);
Sloth_Function Sloth_Glyph_ID   sloth_glyph_atlas_register(Sloth_Glyph_Atlas* atlas, Sloth_Glyph_Desc desc);
Sloth_Function void             sloth_glyph_atlas_unregister(Sloth_Glyph_Atlas* atlas, Sloth_Glyph_ID id);
Sloth_Function bool             sloth_glyph_atlas_contains(Sloth_Glyph_Atlas* atlas, Sloth_Glyph_ID id);
Sloth_Function Sloth_Glyph_Info sloth_glyph_atlas_lookup(Sloth_Glyph_Atlas* atlas, Sloth_Glyph_ID id);
Sloth_Function void             sloth_glyph_atlas_free(Sloth_Glyph_Atlas* atlas);
Sloth_Function Sloth_Glyph_ID   sloth_make_glyph_id(Sloth_U32 family, Sloth_U32 id);
Sloth_Function bool             sloth_glyph_id_matches_charcode(Sloth_Glyph_ID id, Sloth_U32 charcode);

// Input
Sloth_Function bool sloth_mouse_button_transitioned_down(Sloth_Mouse_State btn);
Sloth_Function bool sloth_mouse_button_transitioned_up  (Sloth_Mouse_State btn);
Sloth_Function bool sloth_mouse_button_held_down (Sloth_Mouse_State btn);
Sloth_Function bool sloth_mouse_button_held_up   (Sloth_Mouse_State btn);

// Widget Functions

// Widget Pool Functions
Sloth_Function Sloth_Widget* sloth_widget_pool_take(Sloth_Ctx* sloth);
Sloth_Function void          sloth_widget_pool_give(Sloth_Ctx* sloth, Sloth_Widget* widget);
Sloth_Function void          sloth_widget_pool_grow(Sloth_Widget_Pool* pool);
Sloth_Function void          sloth_widget_pool_free(Sloth_Widget_Pool* pool);

// Widget Cached Pool
Sloth_Function Sloth_Widget_Cached* sloth_widget_cached_pool_take(Sloth_Ctx* sloth);
Sloth_Function void                 sloth_widget_cached_pool_give(Sloth_Ctx* sloth, Sloth_Widget_Cached* widget);
Sloth_Function void                 sloth_widget_cached_pool_grow(Sloth_Widget_Cached_Pool* pool);
Sloth_Function void                 sloth_widget_cached_pool_free(Sloth_Widget_Cached_Pool* pool);


// Widget Tree Functions
// Construction
Sloth_Function Sloth_Widget* sloth_push_widget_on_tree(Sloth_Ctx* sloth, Sloth_ID id);
Sloth_Function Sloth_Widget* sloth_pop_widget_off_tree(Sloth_Ctx* sloth);
// Walking
typedef void Sloth_Tree_Walk_Cb(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_U8* user_data);
Sloth_Function void sloth_tree_walk_inorder_(Sloth_Ctx* sloth, Sloth_Widget* start, Sloth_Tree_Walk_Cb* cb, Sloth_U8* user_data);
Sloth_Function void sloth_tree_walk_preorder_(Sloth_Ctx* sloth, Sloth_Widget* start, Sloth_Tree_Walk_Cb* cb, Sloth_U8* user_data);
Sloth_Function void sloth_tree_walk_postorder_(Sloth_Ctx* sloth, Sloth_Widget* start, Sloth_Tree_Walk_Cb* cb, Sloth_U8* user_data);
Sloth_Function void sloth_tree_walk_inorder(Sloth_Ctx* sloth, Sloth_Tree_Walk_Cb* cb, Sloth_U8* user_data);
Sloth_Function void sloth_tree_walk_preorder(Sloth_Ctx* sloth, Sloth_Tree_Walk_Cb* cb, Sloth_U8* user_data);
Sloth_Function void sloth_tree_walk_postorder(Sloth_Ctx* sloth, Sloth_Tree_Walk_Cb* cb, Sloth_U8* user_data);

// Widget Operations
Sloth_Function Sloth_Widget_Result sloth_push_widget_v(Sloth_Ctx* sloth, Sloth_Widget_Desc desc, char* fmt, va_list args);
Sloth_Function Sloth_Widget_Result sloth_push_widget_f(Sloth_Ctx* sloth, Sloth_Widget_Desc desc, char* fmt, ...);
Sloth_Function Sloth_Widget_Result sloth_push_widget(Sloth_Ctx* sloth, Sloth_Widget_Desc desc, char* text);

// VIBuffer Operations
Sloth_Function void      sloth_vibuffer_set_vert(Sloth_VIBuffer* buf, Sloth_U32 vert_index, Sloth_R32 x, Sloth_R32 y, Sloth_R32 z, Sloth_R32 u, Sloth_R32 v, Sloth_V4 c);
Sloth_Function Sloth_U32 sloth_vibuffer_push_vert(Sloth_VIBuffer* buf, Sloth_R32 x, Sloth_R32 y, Sloth_R32 z, Sloth_R32 u, Sloth_R32 v, Sloth_V4 c);
Sloth_Function Sloth_U32 sloth_vibuffer_push_tri(Sloth_VIBuffer* buf, Sloth_U32 a, Sloth_U32 b, Sloth_U32 c);
Sloth_Function void      sloth_vibuffer_push_quad(Sloth_VIBuffer* buf, Sloth_U32 a, Sloth_U32 b, Sloth_U32 c, Sloth_U32 d);
Sloth_Function void      sloth_vibuffer_free(Sloth_VIBuffer* buf);

// Ctx Operations
Sloth_Function void sloth_ctx_activate_glyph_family(Sloth_Ctx* sloth, Sloth_U32 family);
Sloth_Function void sloth_ctx_free(Sloth_Ctx* sloth);

// TODO
// - Convenience functions
//    - load_sprite
//    - load_font

//////// IMPLEMENTATION  ////////
#ifdef SLOTH_IMPLEMENTATION

#define sloth_array_grow(base, len, cap, min_cap, ele_type) (ele_type*)sloth_array_grow_((Sloth_U8*)(base), (len), (cap), (min_cap), sizeof(ele_type))
Sloth_Function Sloth_U8*
sloth_array_grow_(Sloth_U8* base, Sloth_U32 len, Sloth_U32* cap, Sloth_U32 min_cap, Sloth_U32 ele_size)
{
  SLOTH_PROFILE_BEGIN;
  if (len < *cap) return base;
  
  Sloth_U32 new_cap = *cap * 2;
  if (new_cap == 0) new_cap = min_cap;

  Sloth_U8* new_base = sloth_realloc(
    base,
    *cap * ele_size,
    new_cap * ele_size
  );
  *cap = new_cap;
  
  return new_base;
}


// Temporary string memory
// TODO: Store this in the Sloth_Ctx rather than as a global
static Sloth_U8 sloth_temp_string_memory[Sloth_Temp_String_Memory_Size];

#define Sloth_Max(a,b) ((a) > (b) ? (a) : (b))
#define Sloth_Min(a,b) ((a) < (b) ? (a) : (b))
#define Sloth_Clamp(lower, v, higher) Sloth_Max(lower, Sloth_Min(higher, v))

#define sloth_has_flag(value, flag) (((value) & (flag)) != 0)
#define sloth_floor_r32(v) (Sloth_R32)((int)(v))

#define sloth_copy_memory(dst, src, len) sloth_copy_memory_((Sloth_U8*)(dst), (Sloth_U8*)(src), (len))
Sloth_Function void
sloth_copy_memory_(Sloth_U8* dst, Sloth_U8* src, Sloth_U32 len)
{
  SLOTH_PROFILE_BEGIN;
  for (Sloth_U32 i = 0; i < len; i++) dst[i] = src[i];
}

#define sloth_is_pow2(a) (!((Sloth_U32)(a) & ((Sloth_U32)(a) - 1)))

Sloth_Function Sloth_ID_Result 
sloth_make_id_v(Sloth_Char* fmt, va_list args)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_U32 len = (Sloth_U32)stbsp_vsnprintf(
    (char*)sloth_temp_string_memory, 
    Sloth_Temp_String_Memory_Size,
    (char*)fmt,
    args
  );

  // Break up formatted string into its parts
  // (what to display, what to discard)
  Sloth_U32 discard_to = 0;
  Sloth_U32 display_before = len;
  for (Sloth_U32 i = 0; i < len; i++) 
  {
    if (sloth_temp_string_memory[i] == '#') {
      if (i + 1 < len && sloth_temp_string_memory[i + 1] == '#')
      {
        if (i + 2 < len && sloth_temp_string_memory[i + 2] == '#' && (i + 3) > discard_to)
        {
          discard_to = i + 3;
          display_before = i;
          i += 2;
        }
        else if (i < display_before)
        {
          display_before = i;
        }
      }
    }
  }

  // Hash the non-discarded formatted string
  // djb2 hash - http://www.cse.yorku.ca/~oz/hash.html
  Sloth_U32 hash = 5381;
  for (Sloth_U32 i = discard_to; i < len; i++)
  {
    hash = ((hash << 5) + hash) + (Sloth_U8)sloth_temp_string_memory[i];
  }

  Sloth_ID_Result result;
  result.id.value = hash;
  result.display_len = display_before;
  result.formatted = (Sloth_Char*)&sloth_temp_string_memory[0];
  return result;
}

Sloth_Function Sloth_ID_Result 
sloth_make_id_f(Sloth_Char* fmt, ...)
{
  SLOTH_PROFILE_BEGIN;
  va_list args;
  va_start(args, fmt);
  Sloth_ID_Result result = sloth_make_id_v(fmt, args);
  va_end(args);
  return result;
}

Sloth_Function Sloth_ID_Result 
sloth_make_id_len(Sloth_U32 len, Sloth_Char* str)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_ID_Result result = sloth_make_id_f((char*)"%.*s", len, str);
  return result;
}

Sloth_Function Sloth_ID_Result 
sloth_make_id(Sloth_Char* str)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_ID_Result result = sloth_make_id_f((char*)"%s", str);
  return result;
}

Sloth_Function Sloth_Bool
sloth_ids_equal(Sloth_ID a, Sloth_ID b)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Bool result = a.value == b.value;
  return result;
}

Sloth_Function Sloth_V2
sloth_make_v2(Sloth_R32 x, Sloth_R32 y)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_V2 result;
  result.x = x;
  result.y = y;
  return result;
}

Sloth_Function Sloth_V2 
sloth_v2_add(Sloth_V2 a, Sloth_V2 b)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_V2 result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  return result;
}

Sloth_Function Sloth_V2 
sloth_v2_sub(Sloth_V2 a, Sloth_V2 b)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_V2 result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  return result;
}

Sloth_Function Sloth_V2 
sloth_v2_mulf(Sloth_V2 a, Sloth_R32 b)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_V2 result;
  result.x = a.x * b;
  result.y = a.y * b;
  return result;
}

Sloth_Function Sloth_V2   
sloth_rect_dim(Sloth_Rect r)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_V2 result;
  result.x = r.value_max.x - r.value_min.x;
  result.y = r.value_max.y - r.value_min.y;
  return result;
}

Sloth_Function Sloth_Rect 
sloth_rect_union(Sloth_Rect a, Sloth_Rect b)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Rect result;
  result.value_min.x = Sloth_Max(a.value_min.x, b.value_min.x);
  result.value_min.y = Sloth_Max(a.value_min.y, b.value_min.y);
  result.value_max.x = Sloth_Min(a.value_max.x, b.value_max.x);
  result.value_max.y = Sloth_Min(a.value_max.y, b.value_max.y);
  Sloth_V2 result_dim = sloth_rect_dim(result);
  if (result_dim.x < 0 || result_dim.y < 0) {
    result.value_min.x = 0; result.value_min.y = 0;
    result.value_max.x = 0; result.value_max.y = 0;
  }
  return result;
}

Sloth_Function void
sloth_rect_expand(Sloth_Rect* target, Sloth_R32 left, Sloth_R32 top, Sloth_R32 right, Sloth_R32 bottom)
{
  SLOTH_PROFILE_BEGIN;
  target->value_min.x -= left;
  target->value_min.y -= top;
  target->value_max.x += right;
  target->value_max.y += bottom;
}

Sloth_Function Sloth_Bool 
sloth_rect_contains(Sloth_Rect r, Sloth_V2 p)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Bool result = (
    p.x >= r.value_min.x && p.x <= r.value_max.x &&
    p.y >= r.value_min.y && p.y <= r.value_max.y
  );
  return result;
}

Sloth_Function Sloth_V2 
sloth_rect_get_closest_point(Sloth_Rect r, Sloth_V2 p)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_V2 result = p;
  result.x = Sloth_Max(r.value_min.x, Sloth_Min(r.value_max.x, p.x));
  result.y = Sloth_Max(r.value_min.y, Sloth_Min(r.value_max.y, p.y));
  return result;
}

Sloth_Function Sloth_Size 
sloth_size(Sloth_Size_Kind k, Sloth_R32 v)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Size result;
  result.value = v;
  result.kind = k;
  return result;
}

Sloth_Function Sloth_Size 
sloth_size_pixels(Sloth_R32 v)
{
  SLOTH_PROFILE_BEGIN;
  return sloth_size(Sloth_SizeKind_Pixels, v);
}

Sloth_Function Sloth_Size 
sloth_size_text_content()
{
  SLOTH_PROFILE_BEGIN;
  return sloth_size(Sloth_SizeKind_TextContent, 0);
}

Sloth_Function Sloth_Size 
sloth_size_percent_parent(Sloth_R32 v)
{
  SLOTH_PROFILE_BEGIN;
  return sloth_size(Sloth_SizeKind_PercentOfParent, v);
}

Sloth_Function Sloth_Size 
sloth_size_children_sum()
{
  SLOTH_PROFILE_BEGIN;
  return sloth_size(Sloth_SizeKind_ChildrenSum, 0);
}

Sloth_Function Sloth_Size_Box 
sloth_size_box_uniform(Sloth_Size_Kind k, Sloth_R32 v)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Size_Box result;
  result.left = sloth_size(k, v);
  result.right = result.left;
  result.top = result.left;
  result.bottom = result.left;
  return result;
}

Sloth_Function Sloth_Size_Box 
sloth_size_box_uniform_pixels(Sloth_R32 v)
{
  SLOTH_PROFILE_BEGIN;
  return sloth_size_box_uniform(Sloth_SizeKind_Pixels, v);
}

#define sloth_zero_struct_(ptr) sloth_zero_size__(sizeof(*ptr), (Sloth_U8*)(ptr));
Sloth_Function void
sloth_zero_size__(Sloth_U32 size, Sloth_U8* base)
{
  for (Sloth_U32 i = 0; i < size; i++) base[i] = 0;
}

Sloth_Function void
sloth_hashtable_realloc(Sloth_Hashtable* table, Sloth_U32 old_cap, Sloth_U32 new_cap)
{
  table->keys = (Sloth_U32*)sloth_realloc(
    table->keys, old_cap, sizeof(Sloth_U32) * new_cap);
  table->values = (Sloth_U8**)sloth_realloc(
    table->values, old_cap, sizeof(Sloth_U8*) * new_cap
  );

  Sloth_U32 indices_to_zero = new_cap - old_cap;
  sloth_zero_size__(
    sizeof(Sloth_U32) * indices_to_zero, 
    (Sloth_U8*)(table->keys + old_cap)
  );
  sloth_zero_size__(
    sizeof(Sloth_U8*) * indices_to_zero, 
    (Sloth_U8*)(table->values + old_cap)
  );

  table->cap = new_cap;
}

#define SLOTH_HASHTABLE_TOMBSTONE (1 << 31)
// Since this hashtable takes any 32 bit integer as a key, this 
// macro simply masks off the tombstone bit if it was present
#define SLOTH_HASHTABLE_VALIDATE_KEY(key) ((key) & ~SLOTH_HASHTABLE_TOMBSTONE)

#define SLOTH_HASHTABLE_CAP_MASK(table) ((table)->cap - 1)

// effectively key % table->cap
// this will be true so long as cap is a power of two
Sloth_Function Sloth_U32
sloth_hashtable_desired_pos(Sloth_Hashtable* table, Sloth_U32 key)
{
  Sloth_U32 result = key & SLOTH_HASHTABLE_CAP_MASK(table); 
  if (result == 0) result = 1;
  return result;
}

#define SLOTH_HASHTABLE_PROBE_DISTANCE(table, key, pos) (pos) - (sloth_hashtable_desired_pos((table), (key)))

#define SLOTH_HASHTABLE_KEY_IS_DELETED(key) ((key) & SLOTH_HASHTABLE_TOMBSTONE)

Sloth_Function void
sloth_hashtable_insert_(Sloth_Hashtable* table, Sloth_U32 key, Sloth_U8* value, Sloth_U32 index)
{
  table->keys[index] = key;
  table->values[index] = value;
}

Sloth_Function void
sloth_hashtable_add(Sloth_Hashtable* table, Sloth_U32 key, Sloth_U8* value)
{
  if (table->cap == 0) sloth_hashtable_realloc(table, 0, 2048);

  Sloth_U32 active_key = SLOTH_HASHTABLE_VALIDATE_KEY(key);
  Sloth_U8* active_value = value;
  Sloth_U32 index = sloth_hashtable_desired_pos(table, active_key);
  Sloth_U32 dist = 0;
  for (;;) 
  {
    if (table->keys[index] == 0)
    {
      sloth_hashtable_insert_(table, active_key, active_value, index);
      break;
    }

    Sloth_U32 existing_key = table->keys[index];
    Sloth_U32 existing_dist = SLOTH_HASHTABLE_PROBE_DISTANCE(
      table, existing_key, index
    );
    if (existing_dist < dist) {
      if (SLOTH_HASHTABLE_KEY_IS_DELETED(table->keys[index]))
      {
        sloth_hashtable_insert_(table, active_key, active_value, index);
        break;
      }

      // swap existing with the insertion and keep probing
      Sloth_U8* existing_value = table->values[index];
      table->values[index] = active_value;
      table->keys[index] = active_key;
      active_key = existing_key;
      active_value = existing_value;
      dist = existing_dist;
    }

    index = (index + 1) & SLOTH_HASHTABLE_CAP_MASK(table);
    dist += 1;
  }

  table->used += 1;
}

Sloth_Function Sloth_U32
sloth_hashtable_lookup_index_(Sloth_Hashtable* table, Sloth_U32 key, bool* is_empty)
{
  if (!table->keys) {
    if (is_empty) *is_empty = true;
    return 0;
  }
  key = SLOTH_HASHTABLE_VALIDATE_KEY(key);
  Sloth_U32 index = sloth_hashtable_desired_pos(table, key);
  while (table->keys[index] != 0 && table->keys[index] != key) {
    index = (index + 1) & SLOTH_HASHTABLE_CAP_MASK(table);
  }
  Sloth_U32 fkey = table->keys[index];  
  if (is_empty) {
    if (fkey == 0) *is_empty = true;
    if (SLOTH_HASHTABLE_KEY_IS_DELETED(fkey)) *is_empty = true;
  } else {
    if (fkey == 0) return 0;
    if (SLOTH_HASHTABLE_KEY_IS_DELETED(fkey)) return 0;
  }
  return index;
}

Sloth_Function Sloth_Bool 
sloth_hashtable_rem(Sloth_Hashtable* table, Sloth_U32 key)
{
  bool unused = false;
  Sloth_U32 index = sloth_hashtable_lookup_index_(table, key, &unused);
  if (index == 0) return false;
  table->keys[index] = table->keys[index] | SLOTH_HASHTABLE_TOMBSTONE;
  table->values[index] = 0;
  table->used -= 1;
  return true;
}

Sloth_Function Sloth_U8*  
sloth_hashtable_get(Sloth_Hashtable* table, Sloth_U32 key)
{
  Sloth_U32 index = sloth_hashtable_lookup_index_(table, key, 0);
  if (index == 0) return 0;
  return table->values[index];
}

Sloth_Function void
sloth_hashtable_free(Sloth_Hashtable* table)
{
  Sloth_U8* unused;
  unused = sloth_realloc(table->keys, sizeof(Sloth_U32) * table->cap, 0);
  unused = sloth_realloc(table->values, table->cap, 0);
}

Sloth_Function void      
sloth_arena_grow(Sloth_Arena* arena, Sloth_U32 min_size)
{
  SLOTH_PROFILE_BEGIN;
  printf("Growing arena: %s\n", arena->name);
  if (!arena->buckets) {
    arena->buckets = sloth_array_grow(arena->buckets, 0, &arena->buckets_cap, 64, Sloth_U8*);
    sloth_zero_size__(sizeof(Sloth_U8*) * 64, (Sloth_U8*)arena->buckets);
    arena->buckets_len = 0;
    arena->bucket_cap = 1024 * 1024; // 1 MB
    arena->curr_bucket_len = 0;
  }
  if (arena->curr_bucket_len + min_size >= arena->bucket_cap)
  {
    arena->buckets_len += 1;
  }
  sloth_assert(arena->buckets_len < arena->buckets_cap);
  if (!arena->buckets[arena->buckets_len]) {
    Sloth_U32 unused = 0;
    arena->buckets[arena->buckets_len] = sloth_array_grow(arena->buckets[arena->buckets_len], 0, &unused, Sloth_Max(arena->bucket_cap, min_size), Sloth_U8);
    arena->curr_bucket_len = 0;
  }
}

Sloth_Function Sloth_U8* 
sloth_arena_push(Sloth_Arena* arena, Sloth_U32 size)
{
  SLOTH_PROFILE_BEGIN;
  if (arena->curr_bucket_len + size > arena->bucket_cap) sloth_arena_grow(arena, size);
  
  Sloth_U8* bucket = arena->buckets[arena->buckets_len];
  Sloth_U8* result = bucket + arena->curr_bucket_len;
  arena->curr_bucket_len += size;

  return result;
}

Sloth_Function void
sloth_arena_pop(Sloth_Arena* arena, Sloth_Arena_Loc to)
{
  SLOTH_PROFILE_BEGIN;
  if (to.bucket_index > arena->buckets_cap) return;
  if (to.bucket_index > arena->buckets_len) return;
  if (to.bucket_at > arena->bucket_cap) return;
  if (to.bucket_index == arena->buckets_len && to.bucket_at > arena->curr_bucket_len) return;
  
  Sloth_U32 bucket_before = arena->buckets_len;
  Sloth_U32 bucket_before_len = arena->curr_bucket_len;
  
  arena->buckets_len = to.bucket_index;
  arena->curr_bucket_len = to.bucket_at;
  
  if (to.bucket_index == bucket_before)
  {
    Sloth_U32 rewind_dist = bucket_before_len - to.bucket_at;
    Sloth_U8* old_at = arena->buckets[arena->buckets_len] + to.bucket_at;
    sloth_zero_size__(rewind_dist, old_at);
  }
  else if (to.bucket_index < bucket_before)
  {
    // clear all buckets back to the current one (excluding the current one
    for (Sloth_U32 i = bucket_before; i > to.bucket_index && i < bucket_before; i--)
    {    
      sloth_zero_size__(arena->bucket_cap, arena->buckets[i]);
    }
    // clear the current one back to the current point
    sloth_zero_size__(arena->bucket_cap - to.bucket_at, arena->buckets[arena->buckets_len] + to.bucket_at);
  }
  else
  {
    sloth_invalid_code_path;
  }
}

Sloth_Function Sloth_Arena_Loc
sloth_arena_at(Sloth_Arena* arena)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Arena_Loc result;
  result.bucket_index = arena->buckets_len;
  result.bucket_at = arena->curr_bucket_len;
  return result;;
}

Sloth_Function void      
sloth_arena_clear(Sloth_Arena* arena)
{
  SLOTH_PROFILE_BEGIN;
  arena->buckets_len = 0;
  arena->curr_bucket_len = 0;
  
  // @DebugClear
#ifdef DEBUG
  for (Sloth_U32 i = 0; i < arena->buckets_cap; i++)
  {
    Sloth_U8* bucket = arena->buckets[i];
    if (bucket) sloth_zero_size__(arena->bucket_cap, bucket);
  }
#endif
}

Sloth_Function void      
sloth_arena_free(Sloth_Arena* arena)
{
  SLOTH_PROFILE_BEGIN;
  for (Sloth_U32 i = 0; i < arena->buckets_cap; i++)
  {
    Sloth_U8* bucket = arena->buckets[i];
    if (bucket) {
      arena->buckets[i] = sloth_realloc(bucket, arena->bucket_cap, 0);
    }
  }
  arena->buckets = (Sloth_U8**)sloth_realloc(arena->buckets, sizeof(Sloth_U8*) * arena->buckets_cap, 0);
  arena->buckets_cap = 0;
  arena->buckets_len = 0;
  arena->bucket_cap = 0;
  arena->curr_bucket_len = 0;
}

Sloth_Function Sloth_Font_ID 
sloth_font_load_from_memory(Sloth_Ctx* sloth, char* font_name, Sloth_U32 font_name_len, Sloth_U8* data, Sloth_U32 data_size, Sloth_R32 pixel_height)
{
  SLOTH_PROFILE_BEGIN;
  sloth->fonts = sloth_array_grow(sloth->fonts, sloth->fonts_len, &sloth->fonts_cap, 8, Sloth_Font);
  
  Sloth_Font_ID result;
  result.value = sloth->fonts_len++;
  result.weight_index = 0;
  
  Sloth_Font* new_font = sloth->fonts + result.value;
  sloth_zero_struct_(new_font);
  sloth_copy_memory_((Sloth_U8*)new_font->name, (Sloth_U8*)font_name, font_name_len);
  new_font->renderer_data = sloth->font_renderer_load_font(sloth, new_font, data, data_size, 0, pixel_height);
  new_font->metrics.pixel_height = pixel_height;
  
  return result;
}

Sloth_Function Sloth_Font*
sloth_font_get_(Sloth_Ctx* sloth, Sloth_Font_ID font_id)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Font* font = sloth->fonts + font_id.value;
  return font;
}

Sloth_Function Sloth_Font_ID
sloth_font_register_family(Sloth_Ctx* sloth, Sloth_Font_ID font_id, Sloth_U32 weight, Sloth_U32 family)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Font* font = sloth_font_get_(sloth, font_id);
  sloth_assert(font->weights_len < SLOTH_FONT_WEIGHTS_CAP);
  
  Sloth_Font_ID result = font_id;
  result.weight_index = font->weights_len++;
  font->weights[result.weight_index].weight = weight;
  font->weights[result.weight_index].glyph_family = family;
  
  return result;
}

Sloth_Function Sloth_Glyph_ID
sloth_font_register_codepoint(Sloth_Ctx* sloth, Sloth_Font_ID font_id, Sloth_U32 codepoint)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Glyph_ID result;
  sloth_zero_struct_(&result);
  if (sloth->font_renderer_register_glyph) {
    sloth->font_renderer_register_glyph(sloth, font_id, codepoint);
  }
  return result;
}

Sloth_Function Sloth_Font*
sloth_glyph_to_font(Sloth_Ctx* sloth, Sloth_Glyph_ID glyph_id)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Font* result = 0;
  for (Sloth_U32 i = 0; i < sloth->fonts_len; i++)
  {
    Sloth_Font* at = sloth->fonts + i;
    for (Sloth_U32 w = 0; w < at->weights_len; w++)
    {
      if (at->weights[w].glyph_family == glyph_id.family)
      {
        result = at;
        break;
      }
    }
    if (result) break;
  }
  return result;
}

Sloth_Function Sloth_Font_Metrics
sloth_font_get_metrics(Sloth_Ctx* sloth, Sloth_Font_ID id)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Font_Metrics result; sloth_zero_struct_(&result);
  Sloth_Font* font = sloth_font_get_(sloth, id);
  if (!font) return result;
  return font->metrics;
}

Sloth_Function void
sloth_font_set_metrics(Sloth_Ctx* sloth, Sloth_Font_ID id, Sloth_Font_Metrics metrics)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Font* font = sloth_font_get_(sloth, id);
  if (!font) return;
  font->metrics = metrics;
}


Sloth_Function void             
sloth_glyph_atlas_resize(Sloth_Glyph_Atlas* atlas, Sloth_U32 new_dim)
{
  SLOTH_PROFILE_BEGIN;
  sloth_assert(sloth_is_pow2(new_dim));
  Sloth_U32 new_size = new_dim * new_dim * sizeof(Sloth_U32);
  Sloth_U32 old_size = atlas->dim * atlas->dim * sizeof(Sloth_U32);
  atlas->data = sloth_realloc(atlas->data, old_size, new_size);
  atlas->dim = new_dim;
}

Sloth_Function Sloth_U32
sloth_xy_to_texture_offset(Sloth_U32 x, Sloth_U32 y, Sloth_U32 dim, Sloth_U32 bytes_per_pixel)
{
  SLOTH_PROFILE_BEGIN;
  return (((y * dim) + x) * bytes_per_pixel);
}
Sloth_Function Sloth_U32
sloth_xy_to_texture_offset_u32(Sloth_U32 x, Sloth_U32 y, Sloth_U32 dim)
{
  SLOTH_PROFILE_BEGIN;
  return sloth_xy_to_texture_offset(x, y, dim, sizeof(Sloth_U32));
}
Sloth_Function Sloth_U32
sloth_xy_to_texture_offset_u8(Sloth_U32 x, Sloth_U32 y, Sloth_U32 dim)
{
  SLOTH_PROFILE_BEGIN;
  return sloth_xy_to_texture_offset(x, y, dim, sizeof(Sloth_U8));
}

Sloth_Function Sloth_U32
sloth_color_apply_gamma(Sloth_U32 color, Sloth_R32 power)
{
  SLOTH_PROFILE_BEGIN;
  if (power == 1) return color;
  
  Sloth_R32 r = (Sloth_R32)((color >> 24) & 0xFF) / 255.0f;
  Sloth_R32 g = (Sloth_R32)((color >> 16) & 0xFF) / 255.0f;
  Sloth_R32 b = (Sloth_R32)((color >>  8) & 0xFF) / 255.0f;
  Sloth_R32 a = (Sloth_R32)((color      ) & 0xFF) / 255.0f;
  
  Sloth_R32 gamma_r = powf(r, power);
  Sloth_R32 gamma_g = powf(g, power);
  Sloth_R32 gamma_b = powf(b, power);
  Sloth_R32 gamma_a = powf(a, power);
  
  Sloth_U32 result = (((Sloth_U32)(gamma_r * 255) << 24) |
                      ((Sloth_U32)(gamma_g * 255) << 16) |
                      ((Sloth_U32)(gamma_b * 255) <<  8) |
                      ((Sloth_U32)(gamma_a * 255)));
  return result; 
}

Sloth_Function Sloth_Glyph_ID   
sloth_glyph_atlas_register(Sloth_Glyph_Atlas* atlas, Sloth_Glyph_Desc desc)
{
  SLOTH_PROFILE_BEGIN;
  if (!atlas->glyphs || atlas->glyphs_table.used >= atlas->glyphs_cap) 
  {
    atlas->glyphs = sloth_array_grow(
      atlas->glyphs, atlas->glyphs_table.used, &atlas->glyphs_cap, 256, Sloth_Glyph
    );
  }

  Sloth_Glyph_ID new_glyph_id;
  new_glyph_id.value = desc.id & 0x00FFFFFF;
  new_glyph_id.family = desc.family & 0xFF;
  
  // check if this glyph has already been registered
  if (sloth_hashtable_lookup_index_(&atlas->glyphs_table, new_glyph_id.value, 0) != 0) {
    return new_glyph_id;
  }

  Sloth_U32 new_glyph_index = atlas->glyphs_table.used;
  Sloth_Glyph* new_glyph = atlas->glyphs + new_glyph_index;
  sloth_hashtable_add(&atlas->glyphs_table, new_glyph_id.value, (Sloth_U8*)new_glyph);
  
  // TODO(PS): glyphs_table.used > 1 is because we've already
  // added the new glyph. If its the first glyph, there is no
  // previous glyph, but the table has a glyph in it already.
  //
  Sloth_U32 dst_x = 0;
  Sloth_U32 dst_y = 1;
  if (atlas->glyphs_table.used > 1) {
    Sloth_Glyph last_glyph = atlas->glyphs[atlas->last_glyph];
    dst_x = last_glyph.offset_x + last_glyph.width + 1; // add room for apron
    dst_y = last_glyph.offset_y;
  }
  atlas->last_glyph = new_glyph_index;
  
  // See if we should create the atlas texture in the first place
  if (atlas->dim == 0)
  {
    sloth_glyph_atlas_resize(atlas, 2048);
  }
  
  // See if we need to move to a new row
  Sloth_U32 apron_dim = 2;
  if ((dst_x + desc.width + apron_dim) > atlas->dim)
  {
    Sloth_Glyph first_glyph_in_row = atlas->glyphs[atlas->last_row_first_glyph];
    dst_x = 0;
    dst_y = first_glyph_in_row.offset_y + first_glyph_in_row.height + (apron_dim / 2);
    atlas->last_row_first_glyph = new_glyph_index;
    
    if (dst_y + desc.height + (apron_dim / 2) >= atlas->dim)
    {
      // TODO: I'm punting this case till later. Current implementation
      // will auto create the texture with size 2048x2048 if its null, but
      // wont grow again beyond that.
      // The complexity I don't want to deal with right now is that wehn
      // the texture is resized, it either has to:
      //   a) reflow all existing glyphs (I don't like this)
      //   b) begin finding the empty space to the right of existing rows
      //      of glyphs.
      // b is the solution I want to go with but I don't want to do that
      // right now
      
      
      // NOTE: If this assert fires, you just need to grow your glyph dim
      
      // TODO: It would be nice if this would just do it for you, or better yet
      // do it if we're under some bound (Say 4096) but create a second texture 
      // if we're above. To do that, it'll also want to make sure things like
      // Glyph Families (ie. a font's sprites) are all on one texture
      sloth_assert(dst_y + desc.height < atlas->dim);
    }
  }

  Sloth_U32 apron_x = dst_x;
  Sloth_U32 apron_y = dst_y - 1;
  dst_x += 1;
  
  new_glyph->offset_x = dst_x;
  new_glyph->offset_y = dst_y;
  new_glyph->width = desc.width;
  new_glyph->height = desc.height;
  new_glyph->lsb = desc.cursor_to_glyph_start_xoff;
  new_glyph->x_advance = desc.cursor_to_next_glyph;
  new_glyph->baseline_offset_y = desc.baseline_offset_y;
  
  if (desc.data)
  {
    Sloth_R32 copy_gamma = 1;
    if (desc.copy_gamma != 0) copy_gamma = desc.copy_gamma;
    
    Sloth_U8* src_row_at = desc.data;
    Sloth_U8* dst_row_at = atlas->data + (((dst_y * atlas->dim) + dst_x) * sizeof(Sloth_U32));
    switch (desc.format) 
    {
      case Sloth_GlyphData_RGBA8:
      case Sloth_GlyphData_RGB8:
      {
        Sloth_U32 stride = sloth_glyph_data_format_strides[desc.format];
        for (Sloth_U32 y = dst_y; y < dst_y + desc.height; y++)
        {
          Sloth_U8* dst_at = dst_row_at;
          Sloth_U8* src_at = src_row_at;
          for (Sloth_U32 x = dst_x; x < dst_x + desc.width; x++)
          {
            dst_at[0] = sloth_color_apply_gamma(*src_at++, copy_gamma);
            dst_at[1] = sloth_color_apply_gamma(*src_at++, copy_gamma);
            dst_at[2] = sloth_color_apply_gamma(*src_at++, copy_gamma);
            if (desc.format == Sloth_GlyphData_RGBA8) {
              dst_at[3] = sloth_color_apply_gamma(*src_at++, copy_gamma);
            } else {
              dst_at[3] = 0xFF;
            }
            dst_at += 4;
          }
          dst_row_at += atlas->dim * sizeof(Sloth_U32);
          src_row_at += desc.stride * stride;
        }
      } break;
      
      case Sloth_GlyphData_Alpha8:
      {
        for (Sloth_U32 y = dst_y; y < dst_y + desc.height; y++)
        {
          Sloth_U8* dst_at = dst_row_at;
          Sloth_U8* src_at = src_row_at;
          for (Sloth_U32 x = dst_x; x < dst_x + desc.width; x++)
          {
            Sloth_U8 alpha = *src_at++;
            dst_at[0] = 0xFF;
            dst_at[1] = 0xFF;
            dst_at[2] = 0xFF;
            dst_at[3] = sloth_color_apply_gamma(alpha, copy_gamma);;
            dst_at += 4;
          }
          dst_row_at += atlas->dim * sizeof(Sloth_U32);
          src_row_at += desc.stride;
        }
      } break;
      
      sloth_invalid_default_case;
    }
  }
  
  // Draw the Apron
  Sloth_U32 apron_vertical0 = sloth_xy_to_texture_offset_u32(apron_x, apron_y, atlas->dim);
  Sloth_U32 apron_vertical1 = sloth_xy_to_texture_offset_u32(apron_x + desc.width + 1, apron_y, atlas->dim);
  for (Sloth_U32 apron_y_at = 0; apron_y_at < desc.height + apron_dim; apron_y_at++)
  {
    Sloth_U32* a0 = (Sloth_U32*)(atlas->data + apron_vertical0);
    Sloth_U32* a1 = (Sloth_U32*)(atlas->data + apron_vertical1);
    // get the pixel immediately to the inside of the apron and copy its value
    // out to the apron
    Sloth_U32* g0 = (Sloth_U32*)(atlas->data + apron_vertical0 + sizeof(Sloth_U32));
    Sloth_U32* g1 = (Sloth_U32*)(atlas->data + apron_vertical1 - sizeof(Sloth_U32));
    *a0 = *g0;
    *a1 = *g1;
    apron_vertical0 += atlas->dim * sizeof(Sloth_U32);
    apron_vertical1 += atlas->dim * sizeof(Sloth_U32);
  }
  
  Sloth_U32 apron_horiz0 = sloth_xy_to_texture_offset_u32(apron_x, apron_y, atlas->dim);
  Sloth_U32 apron_horiz1 = sloth_xy_to_texture_offset_u32(apron_x, apron_y + desc.height + 1, atlas->dim);
  for (Sloth_U32 apron_x_at = 0; apron_x_at < desc.width + apron_dim; apron_x_at++)
  {
    Sloth_U32* a0 = (Sloth_U32*)(atlas->data + apron_horiz0);
    Sloth_U32* a1 = (Sloth_U32*)(atlas->data + apron_horiz1);
    // get the pixel immediately to the inside of the apron and copy its value
    // out to the apron
    Sloth_U32* g0 = (Sloth_U32*)(atlas->data + apron_horiz0 + (atlas->dim * sizeof(Sloth_U32)));
    Sloth_U32* g1 = (Sloth_U32*)(atlas->data + apron_horiz1 - (atlas->dim * sizeof(Sloth_U32)));
    *a0 = *g0;
    *a1 = *g1;
    apron_horiz0 += sizeof(Sloth_U32);
    apron_horiz1 += sizeof(Sloth_U32);
  }

  atlas->is_dirty = true;
  return new_glyph_id;
}

Sloth_Function void             
sloth_glyph_atlas_unregister(Sloth_Glyph_Atlas* atlas, Sloth_Glyph_ID id)
{
  SLOTH_PROFILE_BEGIN;
  // TODO: 
  sloth_invalid_code_path;
}

Sloth_Function bool
sloth_glyph_atlas_contains(Sloth_Glyph_Atlas* atlas, Sloth_Glyph_ID id)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Glyph* glyph = (Sloth_Glyph*)sloth_hashtable_get(&atlas->glyphs_table, id.value);
  return (glyph != 0);
}

Sloth_Function Sloth_Glyph_Info 
sloth_glyph_atlas_lookup(Sloth_Glyph_Atlas* atlas, Sloth_Glyph_ID id)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Glyph_Info result;
  sloth_zero_struct_(&result);

  Sloth_Glyph* glyph = (Sloth_Glyph*)sloth_hashtable_get(&atlas->glyphs_table, id.value);
  if (!glyph) return result;

  result.glyph = *glyph;
  result.uv.value_min.x = (Sloth_R32)glyph->offset_x / (Sloth_R32)atlas->dim;
  result.uv.value_min.y = (Sloth_R32)glyph->offset_y / (Sloth_R32)atlas->dim;
  result.uv.value_max.x = (Sloth_R32)(glyph->offset_x + glyph->width) / (Sloth_R32)atlas->dim;
  result.uv.value_max.y = (Sloth_R32)(glyph->offset_y + glyph->height) / (Sloth_R32)atlas->dim;

  return result;
}

Sloth_Function void
sloth_glyph_atlas_free(Sloth_Glyph_Atlas* atlas)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_U8* unused;
  unused = sloth_realloc(atlas->data, atlas->dim * atlas->dim, 0);
  unused = sloth_realloc(atlas->glyphs, sizeof(Sloth_Glyph) * atlas->glyphs_cap, 0);
  sloth_hashtable_free(&atlas->glyphs_table);
}

Sloth_Function Sloth_Glyph_ID   
sloth_make_glyph_id(Sloth_U32 family, Sloth_U32 id)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Glyph_ID result;
  result.value = id & 0x00FFFFFF;
  result.family = family;
  return result;
}

Sloth_Function bool
sloth_glyph_id_matches_charcode(Sloth_Glyph_ID id, Sloth_U32 charcode)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_U32 id_code = (id.id[0]      |
                       id.id[1] << 8 |
                       id.id[2] << 16);
  return id_code == charcode;
}

Sloth_Function bool
sloth_mouse_button_is_down(Sloth_Mouse_State btn)
{
  SLOTH_PROFILE_BEGIN;
  return (btn & Sloth_MouseState_IsDown);
}
Sloth_Function bool
sloth_mouse_button_was_down(Sloth_Mouse_State btn)
{
  SLOTH_PROFILE_BEGIN;
  return (btn & Sloth_MouseState_WasDown);
}

Sloth_Function bool 
sloth_mouse_button_transitioned_down(Sloth_Mouse_State btn)
{
  SLOTH_PROFILE_BEGIN;
  return sloth_mouse_button_is_down(btn) && !sloth_mouse_button_was_down(btn);
}

Sloth_Function bool 
sloth_mouse_button_transitioned_up (Sloth_Mouse_State btn)
{
  SLOTH_PROFILE_BEGIN;
  return !sloth_mouse_button_is_down(btn) && sloth_mouse_button_was_down(btn);
}

Sloth_Function bool 
sloth_mouse_button_held_down (Sloth_Mouse_State btn)
{
  SLOTH_PROFILE_BEGIN;
  return sloth_mouse_button_is_down(btn) && sloth_mouse_button_was_down(btn);
}

Sloth_Function bool 
sloth_mouse_button_held_up (Sloth_Mouse_State btn)
{
  SLOTH_PROFILE_BEGIN;
  return !sloth_mouse_button_is_down(btn) && !sloth_mouse_button_was_down(btn);
}

Sloth_Function void          
sloth_widget_pool_grow(Sloth_Widget_Pool* pool)
{
  if (pool->len >= pool->cap) printf("growing widget pool\n");
  pool->values = sloth_array_grow(
    pool->values,
    pool->len,
    &pool->cap,
    2048,
    Sloth_Widget
  );
}

Sloth_Function Sloth_Widget* 
sloth_widget_pool_take(Sloth_Ctx* sloth)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Widget* result = 0;
  if (!sloth->widgets.free_list)
  {
    sloth_widget_pool_grow(&sloth->widgets);
    result = sloth->widgets.values + sloth->widgets.len++;
  }
  else
  {
    result = sloth->widgets.free_list;
    sloth->widgets.free_list = result->sibling_next;
  }
  sloth_zero_struct_(result);
  return result;
}

Sloth_Function void          
sloth_widget_pool_give(Sloth_Ctx* sloth, Sloth_Widget* widget)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Widget* child = widget->child_first; 
  while (child) {
    Sloth_Widget* next = child->sibling_next;
    sloth_widget_pool_give(sloth, child);
    child = next;
  }

  widget->sibling_next = 0;
  widget->sibling_next = sloth->widgets.free_list;
  sloth->widgets.free_list = widget;
}

Sloth_Function void
sloth_widget_pool_free(Sloth_Widget_Pool* pool)
{
  SLOTH_PROFILE_BEGIN;
  sloth_free((void*)pool->values, sizeof(Sloth_Widget) * pool->cap);
  sloth_zero_struct_(pool);
}

Sloth_Function void          
sloth_widget_cached_pool_grow(Sloth_Widget_Cached_Pool* pool)
{
  if (!pool->buckets) {
    pool->buckets_cap = 32;
    pool->buckets = (Sloth_Widget_Cached**)sloth_realloc((void*)pool->buckets, 0, pool->buckets_cap * sizeof(Sloth_Widget_Cached_Pool*));
    pool->buckets_len = 0;
    pool->bucket_at = 0;
    pool->bucket_at_len = 0;
    pool->bucket_cap = 1024;
    sloth_zero_size__(sizeof(Sloth_Widget_Cached_Pool*) * pool->buckets_cap, (Sloth_U8*)pool->buckets);
  }
  
  if (pool->bucket_at_len >= pool->bucket_cap)
  {
    pool->bucket_at += 1;
    pool->bucket_at_len = 0;
  }
  
  if (pool->buckets[pool->bucket_at] == 0) {
    Sloth_Widget_Cached** at = pool->buckets + pool->bucket_at_len;
    *at = (Sloth_Widget_Cached*)sloth_realloc((void*)*at, 0, pool->bucket_cap * sizeof(Sloth_Widget_Cached_Pool));
  }
}

Sloth_Function Sloth_Widget_Cached* 
sloth_widget_cached_pool_take(Sloth_Ctx* sloth)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Widget_Cached_Pool* p = &sloth->widget_caches;
  Sloth_Widget_Cached* result = 0;
  if (!p->free_list)
  {    
    sloth_widget_cached_pool_grow(p);
    result = p->buckets[p->bucket_at] + p->bucket_at_len++;
  }
  else
  {
    result = p->free_list;
    p->free_list = result->free_next;
  }
  sloth_zero_struct_(result);
  return result;
}

Sloth_Function void          
sloth_widget_cached_pool_give(Sloth_Ctx* sloth, Sloth_Widget_Cached* widget)
{
  SLOTH_PROFILE_BEGIN;
  widget->free_next = sloth->widget_caches.free_list;
  sloth->widget_caches.free_list = widget;
}

Sloth_Function void
sloth_widget_cached_pool_free(Sloth_Widget_Cached_Pool* pool)
{
  SLOTH_PROFILE_BEGIN;
  for (Sloth_U32 i = 0; i < pool->buckets_cap; i++) 
  {
    sloth_free((void*)pool->buckets[i], sizeof(Sloth_Widget_Cached) * pool->bucket_cap);    
  }
  sloth_free((void*)pool->buckets, sizeof(Sloth_Widget_Cached*) * pool->buckets_cap);
  sloth_zero_struct_(pool);
}

#ifdef DEBUG
#  define sloth_validate_widget_(w) sloth_validate_widget__(w)
#endif

Sloth_Function void
sloth_validate_widget__(Sloth_Widget* w)
{
  sloth_assert(w->cached);
  sloth_assert(w->cached->canary_start_ == 0 && w->cached->canary_end_ == 0);
  
  // Look for cycles in w->siblings
  for (Sloth_Widget* s = w->sibling_prev; s != 0; s = s->sibling_prev)
  {
    sloth_assert(s != w);
  }
  
  // Look for cycles in w->children
  for (Sloth_Widget* c = w->child_first; c != 0; c = c->child_first) 
  {
    sloth_assert(c != w);
    sloth_validate_widget__(c);
  }
  
}

Sloth_Function Sloth_Widget_Cached*
sloth_get_cached_data_for_id(Sloth_Ctx* sloth, Sloth_ID id)
{
  Sloth_Widget_Cached* result = 0;
  result = (Sloth_Widget_Cached*)sloth_hashtable_get(&sloth->widget_cache_lut, id.value);
  if (!result) {
    result = sloth_widget_cached_pool_take(sloth);
    sloth_hashtable_add(&sloth->widget_cache_lut, id.value, (Sloth_U8*)result);
  } 
  sloth_assert(result != 0);  
  return result;
}

Sloth_Function Sloth_Widget* 
sloth_push_widget_on_tree(Sloth_Ctx* sloth, Sloth_ID id)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Widget* widget = sloth_widget_pool_take(sloth);
  if (sloth->widget_tree_parent_cur) {
    Sloth_Widget* parent = sloth->widget_tree_parent_cur;
    if (parent->child_last)
    {
      widget->sibling_prev = parent->child_last;
      sloth_assert(widget->sibling_prev != widget->sibling_next);
      parent->child_last->sibling_next = widget;
      sloth_assert(parent->child_last->sibling_next != parent->child_last);
      parent->child_last = widget;
    }
    else
    {
      parent->child_first = widget;
      parent->child_last = widget;
      widget->sibling_prev = 0;
    }
    widget->parent = parent;
  }
  else
  {
    sloth->widget_tree_root = widget;
  }

  sloth->widget_tree_last_addition = widget;
  sloth->widget_tree_parent_cur = widget;
  sloth->widget_tree_next_child = widget->child_first;
  sloth->widget_tree_depth_cur += 1;
  if (sloth->widget_tree_depth_cur > sloth->widget_tree_depth_max) {
    sloth->widget_tree_depth_max = sloth->widget_tree_depth_cur;
  }
  widget->id = id;  
  widget->cached = sloth_get_cached_data_for_id(sloth, id);
  
  return widget;
}

Sloth_Function Sloth_Widget*        
sloth_pop_widget_off_tree(Sloth_Ctx* sloth)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Widget* parent_cur = sloth->widget_tree_parent_cur;
  
  if (parent_cur != sloth->widget_tree_root) {
    // parent_cur is either the root or its parent has a valid
    // SLL of children
    sloth_assert((parent_cur->parent != 0 && parent_cur->parent->child_first != 0));
    
    // parent_cur either must have a previous sibling, or
    // it's parent must think this is the first child
    // not both.
    sloth_assert((bool)(parent_cur->sibling_prev != 0) != (bool)(parent_cur->parent->child_first == parent_cur));
  }
  
  // if there's an expected next child, that means last
  // frame this widget had more children than it does now
  if (sloth->widget_tree_next_child) {
    if (parent_cur->child_last) {
      Sloth_Widget* last_widget = parent_cur->child_last;
      last_widget->sibling_next = 0;
    } else {
      parent_cur->child_first = 0;
    }
    Sloth_Widget* at = sloth->widget_tree_next_child;
    while (at != 0) {
      Sloth_Widget* next_at = at->sibling_next;
      sloth_widget_pool_give(sloth, sloth->widget_tree_next_child);
      at = next_at;
    }
  }
  
  if (sloth->widget_tree_parent_cur != sloth->widget_tree_root)
  {
    sloth->widget_tree_parent_cur = parent_cur->parent;
    sloth->widget_tree_next_child = parent_cur->sibling_next;
    sloth->widget_tree_depth_cur -= 1;
  }
  else
  {
    // we're at the root
    sloth->widget_tree_next_child = 0;
  }
  
  sloth_assert(sloth->widget_tree_parent_cur);
  sloth_validate_widget_(parent_cur);
  return parent_cur;
}

// Order: Left Root Right
Sloth_Function void 
sloth_tree_walk_inorder_(Sloth_Ctx* sloth, Sloth_Widget* start, Sloth_Tree_Walk_Cb* cb, Sloth_U8* user_data)
{
  SLOTH_PROFILE_BEGIN;
  if (sloth->widgets.len == 0) return;
  
  Sloth_Arena_Loc scratch_at = sloth_arena_at(&sloth->scratch);
  Sloth_Widget** stack = sloth_arena_push_array(&sloth->scratch, Sloth_Widget*, sloth->widgets.len);
  Sloth_U32     stack_len = 0;

  Sloth_Widget* at = start;
  while (true)
  {
    if (at != 0) 
    {
      stack[stack_len++] = at; // push
      at = at->child_first;
    }
    else if (stack_len > 0)
    {
      at = stack[--stack_len]; // pop
      cb(sloth, at, user_data);
      at = at->sibling_next;
    }
    else
    {
      break;
    }
  }

  sloth_arena_pop(&sloth->scratch, scratch_at);
}

// Order: root, children, siblings
Sloth_Function void 
sloth_tree_walk_preorder_(Sloth_Ctx* sloth, Sloth_Widget* start, Sloth_Tree_Walk_Cb* cb, Sloth_U8* user_data)
{
  SLOTH_PROFILE_BEGIN;
  if (sloth->widgets.len == 0) return;
  
  Sloth_Arena_Loc scratch_at = sloth_arena_at(&sloth->scratch);
  Sloth_Widget** stack = sloth_arena_push_array(&sloth->scratch, Sloth_Widget*, sloth->widgets.len);
  Sloth_U32     stack_len = 0;

  Sloth_Widget* at = start;
  stack[stack_len++] = at;

  while (stack_len > 0)
  {
    at = stack[--stack_len]; // pop
    if (at != 0) // if the requested node exists
    {
      cb(sloth, at, user_data); // visit
      stack[stack_len++] = at->sibling_next; // right node
      stack[stack_len++] = at->child_first; // left node
    }
  }

  sloth_arena_pop(&sloth->scratch, scratch_at);
}

typedef struct Sloth_Postorder_Widget Sloth_Postorder_Widget;
struct Sloth_Postorder_Widget
{
  Sloth_Widget* widget;
  Sloth_U8 is_right_child;
};

// Order: children, siblings, root
// NOTE: I believe this is a bit of a modification on 
// postorder traversal, since we still want children
// to be visited from left to right. 
Sloth_Function void 
sloth_tree_walk_postorder_(Sloth_Ctx* sloth, Sloth_Widget* start, Sloth_Tree_Walk_Cb* cb, Sloth_U8* user_data)
{
  SLOTH_PROFILE_BEGIN;
  if (sloth->widgets.len == 0) return;
  
  Sloth_Arena_Loc scratch_at = sloth_arena_at(&sloth->scratch);
  Sloth_Postorder_Widget* stack = sloth_arena_push_array(&sloth->scratch, Sloth_Postorder_Widget, sloth->widgets.len);
  Sloth_U32     stack_len = 0;

  Sloth_Widget* at = start;
  do {
    while (at != 0) // while at exists
    {
      if (at->sibling_next != 0) // if right exists
      {
        Sloth_U32 index_r = stack_len++; // push right
        stack[index_r].widget = at->sibling_next; // push right
        stack[index_r].is_right_child = true;
      }
      // push
      Sloth_U32 index_l = stack_len++;
      stack[index_l].widget = at; 
      stack[index_l].is_right_child = false;

      at = at->child_first; // go to left
    }

    Sloth_Postorder_Widget* wat = &stack[--stack_len];
    at = wat->widget;
    while (at != 0 && !wat->is_right_child) // while at exists and is not the right child of a node
    {
      cb(sloth, at, user_data); // visit
      if (stack_len == 0) {
        wat = 0;
        break;
      }
      wat = &stack[--stack_len];
      at = wat->widget;
    }

    if (wat && wat->is_right_child) 
    {
      wat->is_right_child = false;
      at = wat->widget;
    }
  } while (stack_len > 0);

  sloth_arena_pop(&sloth->scratch, scratch_at);
}

Sloth_Function void 
sloth_tree_walk_inorder(Sloth_Ctx* sloth, Sloth_Tree_Walk_Cb* cb, Sloth_U8* user_data)
{
  SLOTH_PROFILE_BEGIN;
  sloth_tree_walk_inorder_(sloth, sloth->widget_tree_root, cb, user_data);
}

Sloth_Function void 
sloth_tree_walk_preorder(Sloth_Ctx* sloth, Sloth_Tree_Walk_Cb* cb, Sloth_U8* user_data)
{
  SLOTH_PROFILE_BEGIN;
  sloth_tree_walk_preorder_(sloth, sloth->widget_tree_root, cb, user_data);
}

Sloth_Function void 
sloth_tree_walk_postorder(Sloth_Ctx* sloth, Sloth_Tree_Walk_Cb* cb, Sloth_U8* user_data)
{
  SLOTH_PROFILE_BEGIN;
  sloth_tree_walk_postorder_(sloth, sloth->widget_tree_root, cb, user_data);
}


#ifdef DEBUG
#  define sloth_widget_validate_layout_(w) sloth_widget_validate_layout__(w)
#else
#  define sloth_widget_validate_layout_(w)
#endif
Sloth_Function void
sloth_widget_validate_layout__(Sloth_Widget* widget)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Widget_Layout layout = widget->layout;
  sloth_assert(layout.margin.left.kind != Sloth_SizeKind_TextContent && layout.margin.left.kind != Sloth_SizeKind_ChildrenSum);
  sloth_assert(layout.margin.right.kind != Sloth_SizeKind_TextContent && layout.margin.right.kind != Sloth_SizeKind_ChildrenSum);
  sloth_assert(layout.margin.top.kind != Sloth_SizeKind_TextContent && layout.margin.top.kind != Sloth_SizeKind_ChildrenSum);
  sloth_assert(layout.margin.bottom.kind != Sloth_SizeKind_TextContent && layout.margin.bottom.kind != Sloth_SizeKind_ChildrenSum);

  sloth_assert(layout.size[0].kind != Sloth_SizeKind_PercentOfSelf);
  sloth_assert(layout.size[1].kind != Sloth_SizeKind_PercentOfSelf);
}

Sloth_Function void
sloth_widget_handle_input_drag(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_Widget_Result* result)
{  
  if (!sloth_has_flag(widget->input.flags, Sloth_WidgetInput_Draggable)) return;
  result->drag_offset_pixels.x = sloth->mouse_pos.x - sloth->mouse_down_pos.x;
  result->drag_offset_pixels.y = sloth->mouse_pos.y - sloth->mouse_down_pos.y;
  
  Sloth_Widget* parent = widget->parent;
  if (parent) {
    Sloth_Widget_Cached parent_cached = *widget->parent->cached;
    result->drag_offset_percent_parent.x = result->drag_offset_pixels.x / parent_cached.dim.x;
    result->drag_offset_percent_parent.y = result->drag_offset_pixels.y / parent_cached.dim.y;
  }
}

Sloth_Function Sloth_Widget_Result
sloth_widget_handle_input(Sloth_Ctx* sloth, Sloth_Widget* widget)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Widget_Result result; sloth_zero_struct_(&result);
  result.widget = widget;
  
  Sloth_Widget_Input input = widget->input;
  Sloth_Widget_Cached cached = *widget->cached;
  if (sloth_ids_equal(sloth->active_widget, widget->id)) 
  {
    result.clicked = sloth_mouse_button_transitioned_down(sloth->mouse_button_l);
    result.held = sloth_mouse_button_held_down(sloth->mouse_button_l);  
    sloth_widget_handle_input_drag(sloth, widget, &result);
    
    if (sloth_has_flag(input.flags, Sloth_WidgetInput_TextSelectable))
    {
      result.selected_glyphs_first = sloth->active_widget_selected_glyphs_first;
      result.selected_glyphs_one_past_last = sloth->active_widget_selected_glyphs_one_past_last;
    }
  }
  
  if (sloth_ids_equal(sloth->last_active_widget, widget->id))
  {
    if (sloth_mouse_button_transitioned_up(sloth->mouse_button_l))
    {
      result.released = true;
      sloth_widget_handle_input_drag(sloth, widget, &result);
    }
    
    if (sloth_has_flag(input.flags, Sloth_WidgetInput_TextSelectable))
    {
      result.selected_glyphs_first = sloth->active_widget_selected_glyphs_first;
      result.selected_glyphs_one_past_last = sloth->active_widget_selected_glyphs_one_past_last;
    }
  }
  
  return result;
}

Sloth_Function Sloth_Widget_Result
sloth_push_widget_v(Sloth_Ctx* sloth, Sloth_Widget_Desc desc, char* fmt, va_list args)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_ID_Result idr = sloth_make_id_v(fmt, args);
  
  Sloth_Widget* widget = sloth_push_widget_on_tree(sloth, idr.id);
  sloth_assert(widget->parent || widget == sloth->widget_tree_root);
  widget->layout = desc.layout;
  widget->style  = desc.style;
  widget->input  = desc.input;
  widget->str = fmt;
  
  Sloth_Widget_Result result = sloth_widget_handle_input(sloth, widget);
  
  // TODO: convoluted - probably a more consise, reusable way to
  // access this information
  Sloth_Font_ID text_font = sloth->active_text_glyph_family;
  Sloth_U32 text_family = 0;
  if (sloth->fonts) {
    text_family = sloth->fonts[text_font.value].weights[text_font.weight_index].glyph_family;
  }
  
  bool show_selected = sloth_has_flag(desc.input.flags, Sloth_WidgetInput_TextSelectable);
  show_selected &= sloth_ids_equal(sloth->last_active_widget, widget->id);
  
  widget->text = sloth_arena_push_array(&sloth->per_frame_memory, Sloth_Glyph_Layout, idr.display_len + 1);
  for (Sloth_U32 glyph_i = 0; glyph_i < idr.display_len; glyph_i++)
  {
    Sloth_U32 char_code = (Sloth_U32)idr.formatted[glyph_i];
    Sloth_Glyph_ID g = sloth_make_glyph_id(text_family, char_code);
    
    
    bool after_first = glyph_i >= result.selected_glyphs_first;
    bool before_last = glyph_i < result.selected_glyphs_one_past_last;
    bool is_selected = (show_selected && after_first && before_last);
    widget->text[glyph_i].selected = is_selected;
    widget->text[glyph_i].glyph_id = g;
    
    if (!sloth_glyph_atlas_contains(&sloth->glyph_atlas, g))
    {
      sloth_font_register_codepoint(sloth, text_font, char_code);
    }
    
  }
  widget->text_len = idr.display_len;
  
  sloth_widget_validate_layout__(widget);

  return result;
}

Sloth_Function Sloth_Widget_Result 
sloth_push_widget_f(Sloth_Ctx* sloth, Sloth_Widget_Desc desc, char* fmt, ...)
{
  SLOTH_PROFILE_BEGIN;
  va_list args; va_start(args, fmt);
  Sloth_Widget_Result result = sloth_push_widget_v(sloth, desc, fmt, args);
  va_end(args);
  return result;
}

Sloth_Function Sloth_Widget_Result 
sloth_push_widget(Sloth_Ctx* sloth, Sloth_Widget_Desc desc, char* text)
{
  SLOTH_PROFILE_BEGIN;
  return sloth_push_widget_f(sloth, desc, text);
}

Sloth_Function void
sloth_pop_widget(Sloth_Ctx* sloth)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Widget* last_widget = sloth_pop_widget_off_tree(sloth);
  sloth_assert(sloth->widget_tree_parent_cur);
}

typedef struct Sloth_Layout_Cache Sloth_Layout_Cache;
struct Sloth_Layout_Cache
{
  Sloth_U8 axis;
  Sloth_R32 last_sibling_end;
};

Sloth_Function Sloth_R32
sloth_size_evaluate_margin(Sloth_Widget* widget, Sloth_Size margin_size, Sloth_U8 axis)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_R32 result = 0;
  if (margin_size.kind != Sloth_SizeKind_None)
  {
    if (margin_size.kind == Sloth_SizeKind_Pixels) {
      result = margin_size.value;
    } else {
      Sloth_Rect bounds;
      if (margin_size.kind == Sloth_SizeKind_PercentOfParent) {
        bounds = widget->parent->cached->bounds;
      } else if (margin_size.kind == Sloth_SizeKind_PercentOfSelf) {
        bounds = widget->cached->bounds;
      } else {
        sloth_invalid_code_path;
      }
      result = (bounds.value_max.E[axis] - bounds.value_min.E[axis]) * margin_size.value;
    }
  }
  return result;
}

Sloth_Function Sloth_R32
sloth_size_box_evaluate(Sloth_Widget* widget, Sloth_Size_Box margin, Sloth_U8 axis)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_R32 result = 0;
  result += sloth_size_evaluate_margin(widget, margin.E[axis].min, axis);
  result += sloth_size_evaluate_margin(widget, margin.E[axis].max, axis);
  return result;
}

Sloth_Function void
sloth_size_fixup_fixed_size_apply(Sloth_Widget* widget, Sloth_U8 axis)
{
  Sloth_R32 margin = sloth_size_evaluate_margin(widget, widget->layout.margin.E[axis].min, axis);
  margin += sloth_size_evaluate_margin(widget, widget->layout.margin.E[axis].max, axis);
  widget->cached->dim.E[axis] = widget->text_dim.E[axis] + margin;
}

// @PerAxisTreeWalkCB
Sloth_Function void
sloth_size_fixup_cb_fixed_size(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_U8* user_data)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Layout_Cache* lc = (Sloth_Layout_Cache*)user_data;
  Sloth_U8 axis = lc->axis;
  switch (widget->layout.size[axis].kind)
  {
    case Sloth_SizeKind_Pixels:
    {
      widget->cached->dim.E[axis] = widget->layout.size[axis].value;
    } break;
    case Sloth_SizeKind_TextContent:
    {
      sloth_size_fixup_fixed_size_apply(widget, axis);
    } break;
    default: {} break; // do nothing
  }
}

// @PerAxisTreeWalkCB
Sloth_Function void
sloth_size_fixup_cb_percent_parent(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_U8* user_data)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Layout_Cache* lc = (Sloth_Layout_Cache*)user_data;
  Sloth_U8 axis = lc->axis;
  switch (widget->layout.size[axis].kind)
  {
    case Sloth_SizeKind_PercentOfParent:
    {
      Sloth_Widget* parent = widget->parent;
      sloth_assert(parent);
      
      // NOTE: this violation rises from the fact that that
      // this child relies on its parent for size, and the parent
      // relies on its children for size. This will be solved in 
      // the violation fixup step
      bool unsolved_violation = parent->layout.size[axis].kind == Sloth_SizeKind_ChildrenSum;
      if (!unsolved_violation)
      {
        Sloth_R32 parent_margin = sloth_size_box_evaluate(parent, parent->layout.margin, axis);
        widget->cached->dim.E[axis] = (parent->cached->dim.E[axis] - parent_margin) * widget->layout.size[axis].value;
      }
    } break;
    
    case Sloth_SizeKind_TextContent:
    {
      if (axis == Sloth_Axis_Y) {
        sloth_size_fixup_fixed_size_apply(widget, axis);
      }
      sloth_assert(widget->cached->dim.E[axis] >= widget->text_dim.E[axis]);
    } break;
    default: {} break; // do nothing
  }
}

// @PerAxisTreeWalkCB
Sloth_Function void
sloth_size_fixup_cb_children_sum(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_U8* user_data)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Layout_Cache* lc = (Sloth_Layout_Cache*)user_data;
  Sloth_U8 axis = lc->axis;
  switch (widget->layout.size[axis].kind)
  {
    case Sloth_SizeKind_ChildrenSum:
    {
      Sloth_R32 dim = 0;
      Sloth_R32 max = 0;
      if (widget->child_first) {
        // Because no widgets have been laid out yet, we have to iterate
        // over all its children
        for (Sloth_Widget* child = widget->child_first; 
             child != 0; 
             child = child->sibling_next
        ){
          // TODO: Account for any child gap layout properties
          dim += child->cached->dim.E[axis];
          max = Sloth_Max(child->cached->dim.E[axis], max);
        }
      }
      
      if (widget->layout.direction == Sloth_LayoutDirection_LeftToRight ||
          widget->layout.direction == Sloth_LayoutDirection_RightToLeft)
      {
        if (axis == Sloth_Axis_X) {
          widget->cached->dim.x = dim;
        } else {
          widget->cached->dim.y = max;
        }
      }
      else if (widget->layout.direction == Sloth_LayoutDirection_TopDown ||
               widget->layout.direction == Sloth_LayoutDirection_BottomUp)
      {
        if (axis == Sloth_Axis_X) {
          widget->cached->dim.x = max;
        } else {
          widget->cached->dim.y = dim;
        }
      }
      else
      {
        sloth_invalid_code_path;
      }
      
      // TODO: This is a place, where if we wanted to, we could implement
      // wrapping a-la flexbox 
      // ie. All the children should know their dimensions by now, so we 
      // can just reposition them if laying them out in a row would overflow
      // this widgets parent
    } break;
    default: {} break; // do nothing
  }
}

// @PerAxisTreeWalkCB
Sloth_Function void
sloth_size_fixup_cb_violations(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_U8* user_data)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Layout_Cache* lc = (Sloth_Layout_Cache*)user_data;
  Sloth_U8 axis = lc->axis;
  switch (widget->layout.size[axis].kind)
  {
    case Sloth_SizeKind_PercentOfParent:
    {
      Sloth_Widget* parent = widget->parent;
      Sloth_Widget_Layout pl = parent->layout;
      bool unsolved_violation = pl.size[axis].kind == Sloth_SizeKind_ChildrenSum;
      // TODO:
    } break;
    default: {} break; // do nothing
  }
}

Sloth_Function Sloth_Rect
sloth_widget_calc_inner_bounds(Sloth_Widget* widget)
{
  SLOTH_PROFILE_BEGIN;
  SLOTH_PROFILE_BEGIN;
  sloth_widget_validate_layout_(widget);
  Sloth_Rect result = widget->cached->bounds;
  result.value_min.x += sloth_size_evaluate_margin(widget, widget->layout.margin.left,    Sloth_Axis_X);
  result.value_min.y += sloth_size_evaluate_margin(widget, widget->layout.margin.top,     Sloth_Axis_Y);
  result.value_max.x -= sloth_size_evaluate_margin(widget, widget->layout.margin.right,   Sloth_Axis_X);
  result.value_max.y -= sloth_size_evaluate_margin(widget, widget->layout.margin.bottom,  Sloth_Axis_Y);
  if (result.value_max.x < result.value_min.x) {
    Sloth_R32 avg = (result.value_max.x + result.value_min.x) / 2;
    result.value_max.x = avg;
    result.value_min.x = avg;
  }
  if (result.value_max.y < result.value_min.y) {
    Sloth_R32 avg = (result.value_max.y + result.value_min.y) / 2;
    result.value_max.y = avg;
    result.value_min.y = avg;
  }
  return result;
}

// This is used to figure out which extents of a child are relevant
// to its next sibling attempting to draw itself. ie. based on this
// widgets dimensions, how far should the next widget offset itself
// based on how the parent wants to lay its children out
Sloth_Function Sloth_V2
sloth_layout_get_child_relevant_extents(Sloth_Widget* parent, Sloth_Rect widget_bounds)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_V2 relevant_extents;
  sloth_zero_struct_(&relevant_extents);
  switch (parent->layout.direction)
  {
    case Sloth_LayoutDirection_TopDown:
    {
      relevant_extents.E[Sloth_Axis_X] = widget_bounds.value_min.x;
      relevant_extents.E[Sloth_Axis_Y] = widget_bounds.value_max.y;
    } break;
    case Sloth_LayoutDirection_BottomUp:
    {
      relevant_extents.E[Sloth_Axis_X] = widget_bounds.value_min.x;
      relevant_extents.E[Sloth_Axis_Y] = widget_bounds.value_min.y;
    } break;
    case Sloth_LayoutDirection_LeftToRight:
    {
      relevant_extents.E[Sloth_Axis_X] = widget_bounds.value_max.x;
      relevant_extents.E[Sloth_Axis_Y] = widget_bounds.value_min.y;
    } break;
    case Sloth_LayoutDirection_RightToLeft:
    {
      relevant_extents.E[Sloth_Axis_X] = widget_bounds.value_min.x;
      relevant_extents.E[Sloth_Axis_Y] = widget_bounds.value_min.y;
    } break;
    default: break;
  }
  return relevant_extents;
}

Sloth_Function Sloth_V2
sloth_layout_clip_bounds_to_start_pos(Sloth_Widget* widget, Sloth_Rect clip_bounds)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_V2 start;
  switch (widget->layout.direction)
  {
    case Sloth_LayoutDirection_TopDown:
    case Sloth_LayoutDirection_LeftToRight: { 
      start = clip_bounds.value_min;
    } break;
    case Sloth_LayoutDirection_RightToLeft: {
      start.x = clip_bounds.value_max.x;
      start.y = clip_bounds.value_min.y;
    } break;
    case Sloth_LayoutDirection_BottomUp: {
      start.x = clip_bounds.value_min.x;
      start.y = clip_bounds.value_max.y;
    } break;
  }
  return start;
}

Sloth_Function void
sloth_layout_position_parent_decides(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_Layout_Cache* lc)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_U8 axis = lc->axis;
  Sloth_V2 offset = widget->cached->offset;
  Sloth_Rect bounds = widget->cached->bounds;

  Sloth_Rect clip_bounds;
  if (widget->parent) 
  {
    clip_bounds = sloth_widget_calc_inner_bounds(widget->parent);
    Sloth_V2 start = sloth_layout_clip_bounds_to_start_pos(widget->parent, clip_bounds);
    if (widget == widget->parent->child_first)
    {
      offset.E[axis] = start.E[axis];
    }
    else 
    {
      sloth_assert(widget->sibling_prev != 0);
      Sloth_Widget* last_relevant_sibling = widget->sibling_prev;
      while(last_relevant_sibling && last_relevant_sibling->layout.position.kind != Sloth_LayoutPosition_ParentDecides)
      {
        last_relevant_sibling = last_relevant_sibling->sibling_prev;
      }
      if (last_relevant_sibling) {
        Sloth_V2 extents = sloth_layout_get_child_relevant_extents(widget->parent, last_relevant_sibling->cached->bounds);
        offset.E[axis] = extents.E[axis];
      } else {
        offset.E[axis] = start.E[axis]; // act as first sibling
      }
    }
  } 
  else 
  {
    // Root
    lc->last_sibling_end = 0;
    offset.E[axis] = 0;

    // effectively, do not clip
    clip_bounds.value_min.x = Sloth_R32_Min;
    clip_bounds.value_min.y = Sloth_R32_Min;
    clip_bounds.value_max.x = Sloth_R32_Max;
    clip_bounds.value_max.y = Sloth_R32_Max;
  }

  // Clip bounds to parent
  if (widget->parent) {
    switch (widget->parent->layout.direction)
    {
      case Sloth_LayoutDirection_RightToLeft:
      {
        if (axis == Sloth_Axis_X) {
          offset.E[axis] -= widget->cached->dim.E[axis];
        }
      } break;
      case Sloth_LayoutDirection_BottomUp:
      {
        if (axis == Sloth_Axis_Y) {
          offset.E[axis] -= widget->cached->dim.E[axis];
        }
      } break;
    }
  }
  bounds.value_min.E[axis] = Sloth_Max(
    offset.E[axis],
    clip_bounds.value_min.E[axis]
  );
  bounds.value_max.E[axis] = Sloth_Min(
    offset.E[axis] + widget->cached->dim.E[axis],
    clip_bounds.value_max.E[axis]
  );
  widget->cached->dim.E[axis] = sloth_rect_dim(bounds).E[axis];

  widget->cached->offset = offset;
  widget->cached->bounds = bounds;
}

// At this point, everything has had its dimensions figured out
Sloth_Function void
sloth_layout_position_fixed(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_Layout_Cache* lc)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_U32 axis = lc->axis;
  
  Sloth_Layout_Position pos = widget->layout.position;
  Sloth_R32 desired_offset_from_min = sloth_size_evaluate_margin(widget, pos.at.E[axis].min, axis);
  Sloth_R32 desired_offset_from_max = sloth_size_evaluate_margin(widget, pos.at.E[axis].max, axis);
  desired_offset_from_max += widget->cached->dim.E[axis];
  if (pos.at.E[axis].min.kind != Sloth_SizeKind_None &&
      pos.at.E[axis].max.kind != Sloth_SizeKind_None) 
  {
    // NOTE(PS): this isn't invalid. What we want to do is recalculate this 
    // widget's width, and reflow its text/children based on the implied offsets
    sloth_invalid_code_path;
  }
  
  Sloth_Widget* parent = widget->parent;
  Sloth_R32 desired_offset = 0;
  switch (widget->layout.position.kind)
  {
    case Sloth_LayoutPosition_FixedInParent:
    {
      desired_offset = parent->cached->offset.E[axis] + desired_offset_from_min;
      if (pos.at.E[axis].max.kind != Sloth_SizeKind_None) {
        desired_offset = parent->cached->offset.E[axis] + parent->cached->dim.E[axis] - desired_offset_from_max;
      }
    } break;
    
    case Sloth_LayoutPosition_FixedOnScreen:
    {
      desired_offset = desired_offset_from_min;
      if (pos.at.E[axis].max.kind != Sloth_SizeKind_None) {
        desired_offset = sloth->screen_dim.E[axis] - desired_offset_from_max;
      }
    } break;
    
    sloth_invalid_default_case;
  }
  
  widget->cached->offset.E[axis] = desired_offset;
  widget->cached->bounds.value_min.E[axis] = desired_offset;
  widget->cached->bounds.value_max.E[axis] = desired_offset + widget->cached->dim.E[axis];
}

// @PerAxisTreeWalkCB
// This function produces the final bounding boxes of each widget
Sloth_Function void
sloth_layout_cb(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_U8* user_data)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Layout_Cache* lc = (Sloth_Layout_Cache*)user_data;
  
  switch (widget->layout.position.kind)
  {
    case Sloth_LayoutPosition_ParentDecides:
    {
      sloth_layout_position_parent_decides(sloth, widget, lc);
    } break;
    
    case Sloth_LayoutPosition_FixedInParent:
    case Sloth_LayoutPosition_FixedOnScreen:
    {
      sloth_layout_position_fixed(sloth, widget, lc);
    } break;
    
    sloth_invalid_default_case;
  }
}

Sloth_Function Sloth_U32
sloth_render_quad_ptc(Sloth_VIBuffer* vibuf, Sloth_Rect bounds, Sloth_R32 z, Sloth_V2 uv_min, Sloth_V2 uv_max, Sloth_U32 color)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_V4 c4;
  c4.r = (Sloth_R32)((color >> 24) & 0xFF) / 255.0f;
  c4.g = (Sloth_R32)((color >> 16) & 0xFF) / 255.0f;
  c4.b = (Sloth_R32)((color >>  8) & 0xFF) / 255.0f;
  c4.a = (Sloth_R32)((color >>  0) & 0xFF) / 255.0f;
  
  Sloth_U32 v0 = sloth_vibuffer_push_vert(
    vibuf, bounds.value_min.x, bounds.value_min.y, z, uv_min.x, uv_min.y, c4
  );
  Sloth_U32 v1 = sloth_vibuffer_push_vert(
    vibuf, bounds.value_max.x, bounds.value_min.y, z, uv_max.x, uv_min.y, c4
  );
  Sloth_U32 v2 = sloth_vibuffer_push_vert(
    vibuf, bounds.value_max.x, bounds.value_max.y, z, uv_max.x, uv_max.y, c4
  );
  Sloth_U32 v3 = sloth_vibuffer_push_vert(
    vibuf, bounds.value_min.x, bounds.value_max.y, z, uv_min.x, uv_max.y, c4
  );
  sloth_vibuffer_push_quad(vibuf, v0, v1, v2, v3);
  
  return v0;
}

Sloth_Function void
sloth_render_update_quad_ptc(Sloth_VIBuffer* vibuf, Sloth_U32 quad_v0_index, Sloth_Rect bounds, Sloth_R32 z, Sloth_V2 uv_min, Sloth_V2 uv_max, Sloth_U32 color)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_V4 c4;
  c4.r = (Sloth_R32)((color >> 24) & 0xFF) / 255.0f;
  c4.g = (Sloth_R32)((color >> 16) & 0xFF) / 255.0f;
  c4.b = (Sloth_R32)((color >>  8) & 0xFF) / 255.0f;
  c4.a = (Sloth_R32)((color >>  0) & 0xFF) / 255.0f;
  
  sloth_assert(vibuf->verts_len > quad_v0_index + 3);
  
  sloth_vibuffer_set_vert(
    vibuf, quad_v0_index, bounds.value_min.x, bounds.value_min.y, z, uv_min.x, uv_min.y, c4
  );
  sloth_vibuffer_set_vert(
    vibuf, quad_v0_index + 1, bounds.value_max.x, bounds.value_min.y, z, uv_max.x, uv_min.y, c4
  );
  sloth_vibuffer_set_vert(
    vibuf, quad_v0_index + 2, bounds.value_max.x, bounds.value_max.y, z, uv_max.x, uv_max.y, c4
  );
  sloth_vibuffer_set_vert(
    vibuf, quad_v0_index + 3, bounds.value_min.x, bounds.value_max.y, z, uv_min.x, uv_max.y, c4
  );
}

Sloth_Function void
sloth_render_outline_ptc(Sloth_VIBuffer* vibuf, Sloth_Rect bounds, Sloth_R32 z, Sloth_R32 thickness, Sloth_V2 uv_min, Sloth_V2 uv_max, Sloth_U32 color)
{
  SLOTH_PROFILE_BEGIN;
  // Outline is rendered like
  // 
  // t t t t t t << notice the overhangs
  // l         r
  // l         r
  // l         r
  // b b b b b b <<
  
  // TODO(rjf): I'm not sure I want the outline to 
  // extend beyond the bounds requested. Better to have
  // it an inset outline
  Sloth_V2 top_min, top_max;
  top_min.x = bounds.value_min.x;
  top_min.y = bounds.value_min.y;
  top_max.x = bounds.value_max.x;
  top_max.y = bounds.value_min.y + thickness;
  Sloth_Rect top;
  top.value_min = top_min;
  top.value_max = top_max;
  
  Sloth_V2 bot_min, bot_max;
  bot_min.x = bounds.value_min.x;
  bot_min.y = bounds.value_max.y - thickness;
  bot_max.x = bounds.value_max.x;
  bot_max.y = bounds.value_max.y;
  Sloth_Rect bot;
  bot.value_min = bot_min;
  bot.value_max = bot_max;
  
  Sloth_V2 rig_min, rig_max;
  rig_min.x = bounds.value_min.x;
  rig_min.y = bounds.value_min.y + thickness;
  rig_max.x = bounds.value_min.x + thickness;
  rig_max.y = bounds.value_max.y - thickness;
  Sloth_Rect rig;
  rig.value_min = rig_min;
  rig.value_max = rig_max;
  
  Sloth_V2 lef_min, lef_max;
  lef_min.x = bounds.value_max.x - thickness;
  lef_min.y = bounds.value_min.y + thickness;
  lef_max.x = bounds.value_max.x;
  lef_max.y = bounds.value_max.y - thickness;
  Sloth_Rect lef;
  lef.value_min = lef_min;
  lef.value_max = lef_max;
  
  // TODO(PS): uv_min and uv_max here are not correct
  // they need to be projected onto the outline
  sloth_render_quad_ptc(vibuf, top, z, uv_min, uv_max, color);
  sloth_render_quad_ptc(vibuf, bot, z, uv_min, uv_max, color);
  sloth_render_quad_ptc(vibuf, lef, z, uv_min, uv_max, color);
  sloth_render_quad_ptc(vibuf, rig, z, uv_min, uv_max, color);
}

Sloth_Function Sloth_Rect
sloth_render_get_glyph_bounds(Sloth_Glyph_Info glyph, Sloth_V2 at, Sloth_V2* new_at)
{
  SLOTH_PROFILE_BEGIN;
  
  Sloth_Rect bounds;
  bounds.value_min.x = at.x + glyph.glyph.lsb;
  bounds.value_min.y = at.y + glyph.glyph.baseline_offset_y;
  bounds.value_max = sloth_v2_add(
    bounds.value_min, sloth_make_v2((Sloth_R32)glyph.glyph.width, (Sloth_R32)glyph.glyph.height)
  );
  if (new_at) {
    *new_at = at;
    new_at->x = sloth_floor_r32(at.x + glyph.glyph.x_advance);
  }
  return bounds;
}

Sloth_Function bool
sloth_render_clip_glyph_layout(Sloth_Ctx* sloth, Sloth_Glyph_Layout* glyph, Sloth_Rect text_bounds)
{
  SLOTH_PROFILE_BEGIN;
  
  Sloth_Rect bounds = glyph->bounds;
  
  // find nearest valid point for each extent that is within text_bounds
  Sloth_Rect clipped_bounds;
  clipped_bounds.value_min = sloth_rect_get_closest_point(text_bounds, bounds.value_min);
  clipped_bounds.value_max = sloth_rect_get_closest_point(text_bounds, bounds.value_max);
  
  // if those points are teh same, zero everything out return false (don't draw)
  Sloth_V2 clipped_dim = sloth_rect_dim(clipped_bounds);
  if (clipped_dim.x == 0 || clipped_dim.y == 0)
  {
    return false;
  }
  
  // get insets as percentage of overall width
  Sloth_V2 bounds_dim = sloth_rect_dim(glyph->bounds);
  Sloth_R32 inset_l_pct = (clipped_bounds.value_min.x - bounds.value_min.x) / bounds_dim.x;
  Sloth_R32 inset_r_pct = (clipped_bounds.value_max.x - bounds.value_max.x) / bounds_dim.x;
  Sloth_R32 inset_t_pct = (clipped_bounds.value_min.y - bounds.value_min.y) / bounds_dim.y;
  Sloth_R32 inset_b_pct = (clipped_bounds.value_max.y - bounds.value_max.y) / bounds_dim.y;
  
  // convert pct insets to be in terms of uv coordinates
  Sloth_V2 uv_dim = sloth_rect_dim(glyph->info.uv);
  Sloth_R32 inset_l_uv = inset_l_pct * uv_dim.x;
  Sloth_R32 inset_r_uv = inset_r_pct * uv_dim.x;
  Sloth_R32 inset_t_uv = inset_t_pct * uv_dim.y;
  Sloth_R32 inset_b_uv = inset_b_pct * uv_dim.y;
  
  // NOTE: I checked when I wrote this that this body
  // of operations doesn't modify the dimensions of 
  // clipped_bounds, meaning we don't also need to apply
  // some similar operation to the uvs
  clipped_bounds.value_min.x = sloth_floor_r32(clipped_bounds.value_min.x);
  clipped_bounds.value_min.y = sloth_floor_r32(clipped_bounds.value_min.y);
  clipped_bounds.value_max.x = sloth_floor_r32(clipped_bounds.value_max.x);
  clipped_bounds.value_max.y = sloth_floor_r32(clipped_bounds.value_max.y);
  
  glyph->bounds = clipped_bounds;
  
  glyph->info.uv.value_min.x += inset_l_uv;
  glyph->info.uv.value_min.y += inset_t_uv;
  glyph->info.uv.value_max.x += inset_r_uv;
  glyph->info.uv.value_max.y += inset_b_uv;
  
  // safety checks:
  // we should always be able to assume that the result of this operation
  // produces two rects that are smaller than the original
  Sloth_R32 epsilon = 0.01f;
  sloth_assert((sloth_rect_dim(glyph->bounds).x - bounds_dim.x) < epsilon);
  sloth_assert((sloth_rect_dim(glyph->bounds).y - bounds_dim.y) < epsilon);
  sloth_assert((sloth_rect_dim(glyph->info.uv).x - uv_dim.x) < epsilon);
  sloth_assert((sloth_rect_dim(glyph->info.uv).y - uv_dim.y) < epsilon);
  
  return true;
}

Sloth_Function void
sloth_render_shift_glyphs(Sloth_Glyph_Layout* glyphs, Sloth_U32 first, Sloth_U32 one_past_last, Sloth_U32 axis, Sloth_R32 shift)
{
  SLOTH_PROFILE_BEGIN;
  
  for (Sloth_U32 i = first; i < one_past_last; i++)
  {
    Sloth_Glyph_Layout* at = glyphs + i;
    at->bounds.value_min.E[axis] += shift;
    at->bounds.value_max.E[axis] += shift;
  }
}

Sloth_Function void
sloth_render_distribute_glyphs(Sloth_Glyph_Layout* glyphs, Sloth_U32 first, Sloth_U32 one_past_last, Sloth_U32 axis, Sloth_R32 space_between_glyphs)
{
  SLOTH_PROFILE_BEGIN;
  
  Sloth_Glyph_Layout* prev = glyphs + first;
  for (Sloth_U32 i = first + 1; i < one_past_last; i++)
  {
    Sloth_Glyph_Layout* at = glyphs + i;
    at->bounds.value_min.E[axis] = prev->bounds.value_max.E[axis] + space_between_glyphs;
    at->bounds.value_max.E[axis] = prev->bounds.value_max.E[axis] + space_between_glyphs;
    prev = at;
  }
}

Sloth_Function void
sloth_render_text_apply_align_center(Sloth_Widget* widget, Sloth_Glyph_Layout* glyphs, Sloth_U32 glyphs_cap, Sloth_Rect text_bounds)
{
  SLOTH_PROFILE_BEGIN;
    
  Sloth_R32 line_max_width = sloth_rect_dim(text_bounds).x;
  Sloth_U32 last_line_start_i = 0;
  Sloth_R32 last_line_start_x = glyphs[0].bounds.value_min.x;
  for (Sloth_U32 glyph_i = 1; glyph_i < glyphs_cap; glyph_i++)
  {
    if (!glyphs[glyph_i].is_line_start) continue;

    // process the line just completed
    Sloth_R32 last_line_end_x = glyphs[glyph_i - 1].bounds.value_max.x;
    Sloth_R32 last_line_w = last_line_end_x - last_line_start_x;
    Sloth_R32 last_line_room = line_max_width - last_line_w;
    //sloth_assert(last_line_room >= 0);
    Sloth_R32 last_line_offset = last_line_room / 2;
    sloth_render_shift_glyphs(glyphs, last_line_start_i, glyph_i, Sloth_Axis_X, last_line_offset);
    
    // start the new line
    last_line_start_i = glyph_i;
    last_line_start_x = glyphs[glyph_i].bounds.value_min.x;
  }
}

Sloth_Function void
sloth_render_text_apply_align_right(Sloth_Widget* widget, Sloth_Glyph_Layout* glyphs, Sloth_U32 glyphs_cap, Sloth_Rect text_bounds)
{
  SLOTH_PROFILE_BEGIN;
  
  Sloth_R32 line_max_width = sloth_rect_dim(text_bounds).x;
  Sloth_U32 last_line_start_i = 0;
  Sloth_R32 last_line_start_x = glyphs[0].bounds.value_min.x;
  for (Sloth_U32 glyph_i = 1; glyph_i < glyphs_cap; glyph_i++)
  {
    if (!glyphs[glyph_i].is_line_start) continue;

    // process the line just completed
    Sloth_R32 last_line_end_x = glyphs[glyph_i - 1].bounds.value_max.x;
    Sloth_R32 last_line_w = last_line_end_x - last_line_start_x;
    Sloth_R32 last_line_room = line_max_width - last_line_w;
    //sloth_assert(last_line_room >= 0);
    sloth_render_shift_glyphs(glyphs, last_line_start_i, glyph_i, Sloth_Axis_X, last_line_room);
    
    // start the new line
    last_line_start_i = glyph_i;
    last_line_start_x = glyphs[glyph_i].bounds.value_min.x;
  }
}

Sloth_Function Sloth_V2
sloth_layout_text_in_widget(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_Rect text_bounds)
{
  SLOTH_PROFILE_BEGIN;
  
  Sloth_V2 text_dim; sloth_zero_struct_(&text_dim);
  
  Sloth_U32 last_line_break = 0;
  Sloth_Glyph_Layout* text = widget->text;
  
  Sloth_Font* active_font = 0;
  Sloth_R32 line_advance = 0;
  Sloth_V2 at; at.x = 0; at.y = 0;
  for (Sloth_U32 glyph_i = 0; glyph_i < widget->text_len; glyph_i++)
  {
    Sloth_Glyph_Layout* text_at = text + glyph_i;
        
    Sloth_V2 next_at;
    Sloth_Rect glyph_bounds;
    bool is_newline = sloth_glyph_id_matches_charcode(text_at->glyph_id, '\n');
    if (!is_newline)
    {
      text_at->info = sloth_glyph_atlas_lookup(&sloth->glyph_atlas, text_at->glyph_id);
      text_at->color = widget->style.color_text;
      
      if (glyph_i == 0) 
      {
        text_at->is_line_start = true;      
        active_font = sloth_glyph_to_font(sloth, text_at->glyph_id);
        if (active_font) {
          line_advance = active_font->metrics.line_height;
        } else {
          line_advance = text_at->info.glyph.height;
        }
        at.y += line_advance;
      }
      
      next_at = at;
      glyph_bounds = sloth_render_get_glyph_bounds(text_at->info, at, &next_at);
      text_at->bounds = glyph_bounds;
    }
    
    if(is_newline) 
    {
      next_at.x = 0;
      next_at.y += line_advance;
    }
    else if(glyph_bounds.value_max.x > text_bounds.value_max.x)
    {
      // look backwards to the last line-break glyph and move everything since to a new line
      Sloth_U32 line_break = glyph_i;
      for (Sloth_U32 lb_i = glyph_i; lb_i > last_line_break && lb_i < widget->text_len; lb_i--)
      {
        bool is_space = sloth_glyph_id_matches_charcode(widget->text[lb_i].glyph_id, ' ');
        bool is_newline = sloth_glyph_id_matches_charcode(widget->text[lb_i].glyph_id, '\n');
        if (is_space || is_newline)
        {
          line_break = lb_i + 1;
          break;
        }
      }
      
      if (!sloth_has_flag(widget->style.text_style, Sloth_TextStyle_NoWrapText))
      {
        at.x = 0;
        at.y += line_advance;
        
        if (line_break == last_line_break)
        {
          // since there was nowhere on this line acceptable for
          // a line break, advance the current glyph to the next
          // line since its the first glyph that will overflow
          line_break = glyph_i;
        }
        
        text[line_break].is_line_start = true;
        if (text[line_break].glyph_id.family != text[last_line_break].glyph_id.family)
        {
          active_font = sloth_glyph_to_font(sloth, text_at->glyph_id);
          if (active_font) {
            line_advance = active_font->metrics.line_height;
          } else {
            line_advance = text_at->info.glyph.height;
          }
        }
        
        for (Sloth_U32 new_line_i = line_break; new_line_i <= glyph_i; new_line_i++)
        {
          Sloth_Glyph_Layout new_line_glyph = widget->text[new_line_i];
          Sloth_Rect new_bounds = sloth_render_get_glyph_bounds(new_line_glyph.info, at, &next_at);
          widget->text[new_line_i].bounds = new_bounds;
          at = next_at;         
        }
        last_line_break = line_break;
      }
      
      // TODO: stop creating glyphs if we're at a point where they won't
      // appear within text_bounds at all (ie you've moved to the next
      // line down, and the new glyph will get completely clipped).
      // NOTE: This will involve truncating the later iterations to layout
      // glyphs
    }
        
    at = next_at;
  }
  
  // Handle text alignment
  // This just adjusts the existing positions given
  // during the default layout step above 
  Sloth_Text_Style_Flags text_style = widget->style.text_style;
  // no action necessary for Align_Left
  if (sloth_has_flag(text_style, Sloth_TextStyle_Align_Center))
  {
    sloth_render_text_apply_align_center(widget, widget->text, widget->text_len, text_bounds);
  }
  else if (sloth_has_flag(text_style, Sloth_TextStyle_Align_Right))
  {
    sloth_render_text_apply_align_right(widget, widget->text, widget->text_len, text_bounds);
  }
  
  Sloth_V2 text_max; sloth_zero_struct_(&text_max);
  for (Sloth_U32 glyph_i = 0; glyph_i < widget->text_len; glyph_i++)
  {
    text_max.x = Sloth_Max(text_max.x, text[glyph_i].bounds.value_max.x);
    text_max.y = Sloth_Max(text_max.y, text[glyph_i].bounds.value_max.y);
  }
  text_dim.x = text_max.x - text_bounds.value_min.x;
  text_dim.y = text_max.y - text_bounds.value_min.y;
  
  return text_dim;
}

Sloth_Function void
sloth_size_kind_text_contents_layout_text(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_U8* user_data)
{
  SLOTH_PROFILE_BEGIN;
  
  Sloth_Rect text_bounds;
  sloth_zero_struct_(&text_bounds);
  Sloth_Widget_Layout l = widget->layout;
  if (l.width.kind == Sloth_SizeKind_TextContent) 
  {
    text_bounds.value_max.x = Sloth_R32_Max;
    if (l.height.kind == Sloth_SizeKind_Pixels) {
      Sloth_R32 margin_t = sloth_size_evaluate_margin(widget, widget->layout.margin.top, 0);
      Sloth_R32 margin_b = sloth_size_evaluate_margin(widget, widget->layout.margin.bottom, 0);
      text_bounds.value_max.y = Sloth_Max(0, l.height.value - (margin_t + margin_b));
    } else {
      text_bounds.value_max.y = Sloth_R32_Max;
    }
  }
  else if (l.width.kind == Sloth_SizeKind_Pixels &&
           l.height.kind == Sloth_SizeKind_TextContent)
  {
    Sloth_R32 margin_l = sloth_size_evaluate_margin(widget, widget->layout.margin.left, 0);
    Sloth_R32 margin_r = sloth_size_evaluate_margin(widget, widget->layout.margin.right, 0);
    text_bounds.value_max.x = Sloth_Max(0, l.width.value - (margin_l + margin_r));
    text_bounds.value_max.y = Sloth_R32_Max;
  }
  else
  {
    return; // will be handled later
  }
  
  widget->text_dim = sloth_layout_text_in_widget(sloth, widget, text_bounds);
}

Sloth_Function void
sloth_percent_parent_width_layout_text(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_U8* user_data)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Widget_Layout l = widget->layout;
  if (l.width.kind != Sloth_SizeKind_PercentOfParent) return;
  if (l.height.kind != Sloth_SizeKind_TextContent) return;
  
  Sloth_R32 margin = sloth_size_evaluate_margin(widget, widget->layout.margin.left, Sloth_Axis_X);
  margin += sloth_size_evaluate_margin(widget, widget->layout.margin.right, Sloth_Axis_X);
  
  Sloth_Rect text_bounds; sloth_zero_struct_(&text_bounds);
  text_bounds.value_max.x = Sloth_Max(0, widget->cached->dim.x - margin);
  text_bounds.value_max.y = Sloth_R32_Max;
  widget->text_dim = sloth_layout_text_in_widget(sloth, widget, text_bounds);
  
}

Sloth_Function void
sloth_child_sum_width_layout_text(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_U8* user_data)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Widget_Layout l = widget->layout;
  if (l.width.kind != Sloth_SizeKind_ChildrenSum) return;
  if (l.height.kind != Sloth_SizeKind_TextContent) return;
  
  Sloth_R32 margin = sloth_size_evaluate_margin(widget, widget->layout.margin.left, Sloth_Axis_X);
  margin += sloth_size_evaluate_margin(widget, widget->layout.margin.right, Sloth_Axis_X);
  
  Sloth_Rect text_bounds; sloth_zero_struct_(&text_bounds);
  text_bounds.value_max.x = Sloth_Max(0, widget->cached->dim.x - margin);
  text_bounds.value_max.y = Sloth_R32_Max;
  widget->text_dim = sloth_layout_text_in_widget(sloth, widget, text_bounds);
}

Sloth_Function void
sloth_known_size_layout_text(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_U8* user_data)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Widget_Layout l = widget->layout;
  Sloth_Size_Kind wk = l.width.kind;
  Sloth_Size_Kind hk = l.height.kind;
  if (wk == Sloth_SizeKind_TextContent || hk == Sloth_SizeKind_TextContent) return;
  
  Sloth_R32 margin_x = sloth_size_evaluate_margin(widget, widget->layout.margin.left, Sloth_Axis_X);
  margin_x += sloth_size_evaluate_margin(widget, widget->layout.margin.right, Sloth_Axis_X);
  
  Sloth_R32 margin_y = sloth_size_evaluate_margin(widget, widget->layout.margin.top, Sloth_Axis_Y);
  margin_y += sloth_size_evaluate_margin(widget, widget->layout.margin.bottom, Sloth_Axis_Y);
  
  Sloth_Rect text_bounds; sloth_zero_struct_(&text_bounds);
  text_bounds.value_max.x = Sloth_Max(0, widget->cached->dim.x - margin_x);
  text_bounds.value_max.y = Sloth_Max(0, widget->cached->dim.y - margin_y);
  
  widget->text_dim = sloth_layout_text_in_widget(sloth, widget, text_bounds);
}

Sloth_Function void
sloth_offset_and_clip_text(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_U8* user_data)
{  
  Sloth_V2 offset = widget->cached->offset;
  offset.x += sloth_size_evaluate_margin(widget, widget->layout.margin.left, Sloth_Axis_X);
  offset.y += sloth_size_evaluate_margin(widget, widget->layout.margin.top,  Sloth_Axis_Y);
  for (Sloth_U32 i = 0; i < widget->text_len; i++)
  {
    // offset
    widget->text[i].bounds.value_min.x += offset.x;
    widget->text[i].bounds.value_min.y += offset.y;
    widget->text[i].bounds.value_max.x += offset.x;
    widget->text[i].bounds.value_max.y += offset.y;
    
    // clip
    widget->text[i].draw = sloth_render_clip_glyph_layout(sloth, widget->text + i, widget->cached->bounds);
  }
}

Sloth_Function void
sloth_render_text_in_widget(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_Rect text_bounds, Sloth_R32 z)
{
  Sloth_Glyph_ID id;
  id.value = 0;
  id.id[0] = 1;
  Sloth_Glyph_Info white = sloth_glyph_atlas_lookup(&sloth->glyph_atlas, id);
  
  Sloth_U32 selection_line_start = 0;
  Sloth_Rect line_dim;
  Sloth_S32 selection_quad_v0 = -1;
  for (Sloth_U32 i = 0; i < widget->text_len; i++)
  {
    if (!widget->text[i].draw) continue;
    
    Sloth_Glyph_Layout gl = widget->text[i];
    if (gl.selected) 
    {
      if (selection_quad_v0 == -1 || gl.is_line_start) 
      {
        if (selection_quad_v0 >= 0) {
          sloth_rect_expand(&line_dim, 4, 4, 4, 4);
          sloth_render_update_quad_ptc(&sloth->vibuf, selection_quad_v0, line_dim, z, white.uv.value_min, white.uv.value_max, 0x0088FFFF);
        }
        selection_line_start = i;
        line_dim = gl.bounds;
        selection_quad_v0 = (Sloth_S32)sloth_render_quad_ptc(&sloth->vibuf, gl.bounds, z, white.uv.value_min, white.uv.value_max, 0x0088FFFF);
      }
      else
      {
        line_dim.value_min.x = Sloth_Min(gl.bounds.value_min.x, line_dim.value_min.x);
        line_dim.value_min.y = Sloth_Min(gl.bounds.value_min.y, line_dim.value_min.y);
        line_dim.value_max.x = Sloth_Max(gl.bounds.value_max.x, line_dim.value_max.x);
        line_dim.value_max.y = Sloth_Max(gl.bounds.value_max.y, line_dim.value_max.y);
      }
    }
    else if (selection_quad_v0 != -1)
    {
      sloth_rect_expand(&line_dim, 4, 4, 4, 4);
      sloth_render_update_quad_ptc(&sloth->vibuf, selection_quad_v0, line_dim, z, white.uv.value_min, white.uv.value_max, 0x0088FFFF);
      selection_quad_v0 = -1;
    }
    
    sloth_render_quad_ptc(&sloth->vibuf, gl.bounds, z, gl.info.uv.value_min, gl.info.uv.value_max, gl.color);
  }
  
  if (selection_quad_v0 != -1)
  {
    sloth_rect_expand(&line_dim, 4, 4, 4, 4);
    sloth_render_update_quad_ptc(&sloth->vibuf, selection_quad_v0, line_dim, z, white.uv.value_min, white.uv.value_max, 0x0088FFFF);
    selection_quad_v0 = -1;
  }
}

Sloth_Function void
sloth_render_cb(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_U8* user_data)
{
  // TODO: texture filtering - the calling code should pass in a glyph family via user_Data
  // and this function shouldn't render anything that isn't in that family.
  // That being said, it should compute layout for those things if they 
  // influence the layout of other things
  // ie. a sprite glyph in the middle of text should still have layout computed here
  // due to the fact that it will offset the layout of later characters
  // TODO: Figure out if there's a way to cache that info so we don't have to compute
  // it multiple times per frame.

  SLOTH_PROFILE_BEGIN;
  Sloth_Rect bounds = widget->cached->bounds;
  
  // TODO: UV Coordinates need to be corrected in the case where the widget
  // uses an image or sprite as a background
  Sloth_Glyph_ID bg_id = widget->style.bg_glyph;
  if (bg_id.value == 0) {
    sloth_zero_struct_(&bg_id);
    bg_id.id[0] = 1;
  }
  
  Sloth_Glyph_Info bg_glyph = sloth_glyph_atlas_lookup(
    &sloth->glyph_atlas, bg_id
  );
  Sloth_V2 bg_uv_min = bg_glyph.uv.value_min;
  Sloth_V2 bg_uv_max = bg_glyph.uv.value_max;  
  Sloth_R32 z = widget->layout.position.z;
  sloth_render_quad_ptc(&sloth->vibuf, bounds, z, bg_uv_min, bg_uv_max, widget->style.color_bg);
  
  sloth_render_text_in_widget(sloth, widget, widget->cached->bounds, z);
  
  if (widget->style.outline_thickness > 0)
  {
    Sloth_R32 t = widget->style.outline_thickness;
    Sloth_U32 c = widget->style.color_outline;
    sloth_render_outline_ptc(&sloth->vibuf, widget->cached->bounds, z, t, bg_uv_min, bg_uv_max, c);
  }
  
  if (sloth_ids_equal(sloth->hot_widget, widget->id) || sloth_ids_equal(sloth->active_widget, widget->id))
  {
    Sloth_Glyph_ID white_id; sloth_zero_struct_(&white_id);
    white_id.id[0] = 1;
    Sloth_Glyph_Info white_glyph = sloth_glyph_atlas_lookup(
      &sloth->glyph_atlas, white_id
    );
    Sloth_U32 color = 0xFF00FFFF;
    if (sloth_ids_equal(sloth->active_widget, widget->id)) color = 0x00FFFFFF;
    sloth_render_outline_ptc(&sloth->vibuf, widget->cached->bounds, z, 2, white_glyph.uv.value_min, white_glyph.uv.value_max, color);
  }
  
}

Sloth_Function void
sloth_find_hot_and_active(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_U8* ud)
{
  if (sloth_has_flag(widget->input.flags, Sloth_WidgetInput_DoNotCaptureMouse)) return;
  
  // Active
  if (sloth_mouse_button_is_down(sloth->mouse_button_l) &&
      sloth_rect_contains(widget->cached->bounds, sloth->mouse_down_pos))
  {
    sloth->active_widget = widget->id;
    printf("Active: %s\n", widget->str);
    for (Sloth_U32 glyph_i = 0; glyph_i < widget->text_len; glyph_i++)
    {
      Sloth_Glyph_Layout g = widget->text[glyph_i];
      if (sloth_rect_contains(g.bounds, sloth->mouse_down_pos)) {
        sloth->active_widget_selected_glyphs_first = glyph_i;
      }
      if (sloth_rect_contains(g.bounds, sloth->mouse_pos))
      {
        sloth->active_widget_selected_glyphs_one_past_last = glyph_i + 1;
      }
      
      // TODO(PS): what if they drag away from the widget?
    }
  }
  
  // Hot
  if (sloth_rect_contains(widget->cached->bounds, sloth->mouse_pos))
  {
    sloth->hot_widget = widget->id;
  }
}

Sloth_Function void
sloth_frame_prepare(Sloth_Ctx* sloth)
{
  SLOTH_PROFILE_BEGIN;
  sloth_assert(sloth->sentinel == SLOTH_DEBUG_DID_CALL_ADVANCE);
  
  sloth->vibuf.verts_len = 0;
  sloth->vibuf.indices_len = 0;
  
  // Handle this frames input on the visuals rendered at the end 
  // of last frame
  if (sloth->active_widget.value != 0) 
  {
    if (!sloth_ids_equal(sloth->active_widget, sloth->last_active_widget))
    {
      sloth->active_widget_selected_glyphs_first = 0;
      sloth->active_widget_selected_glyphs_one_past_last = 0;
    }
    sloth->last_active_widget = sloth->active_widget;
  }
  sloth->hot_widget.value = 0;
  sloth->active_widget.value = 0;
  sloth_tree_walk_preorder(sloth, sloth_find_hot_and_active, 0);
  
  // Fixup 
  sloth->widget_tree_next_child = sloth->widget_tree_root;
  sloth->widget_tree_root = 0;
  sloth->widget_tree_last_addition = 0;
  sloth->widget_tree_parent_cur = 0;
  
  // TEMP
  sloth->widgets.len = 0;
  sloth->widgets.free_list = 0;

  sloth_arena_clear(&sloth->per_frame_memory);
  sloth_arena_clear(&sloth->scratch);
  
  sloth->sentinel = SLOTH_DEBUG_DID_CALL_PREPARE;
}

// NOTE: I suspect that this will become a pretty hefty function
// It is responsible for:
// - updating each widget's cached sizes
// - laying out the next frames data
// - outputting the vertex and index buffers needed for the 
//   frame to be rendered
Sloth_Function void
sloth_frame_advance(Sloth_Ctx* sloth)
{
  SLOTH_PROFILE_BEGIN;
  sloth_assert(sloth->sentinel == SLOTH_DEBUG_DID_CALL_PREPARE);
  Sloth_Layout_Cache lc;
  
  // Update the atlas_texture if necessary
  if (sloth->glyph_atlas.is_dirty && sloth->renderer_atlas_updated) {
    sloth->renderer_atlas_updated(sloth);
  }
  sloth->glyph_atlas.is_dirty = false;
  
  // Layout text for widgets that will rely on the size of their
  // text contents.
  sloth_tree_walk_preorder(sloth, sloth_size_kind_text_contents_layout_text, 0);
  
  // Update widget's cached sizes in the following ways:
  // 1. Preorder - Sloth_SizeKind_Pixels & Sloth_SizeKind_TextContent
  //    can be set outright, don't rely on other information
  lc.axis = 0; sloth_tree_walk_preorder(sloth, sloth_size_fixup_cb_fixed_size, (Sloth_U8*)&lc);
  lc.axis = 1; sloth_tree_walk_preorder(sloth, sloth_size_fixup_cb_fixed_size, (Sloth_U8*)&lc);
  
  // 2. Preorder - Calculate sizes that rely on parents
  lc.axis = 0; sloth_tree_walk_preorder(sloth, sloth_size_fixup_cb_percent_parent, (Sloth_U8*)&lc);
  sloth_tree_walk_preorder(sloth, sloth_percent_parent_width_layout_text, 0);
  lc.axis = 1; sloth_tree_walk_preorder(sloth, sloth_size_fixup_cb_percent_parent, (Sloth_U8*)&lc);
  
  // 3. Postorder - Calculate sizes that rely on size of children
  lc.axis = 0; sloth_tree_walk_postorder(sloth, sloth_size_fixup_cb_children_sum, (Sloth_U8*)&lc);
  sloth_tree_walk_preorder(sloth, sloth_child_sum_width_layout_text, 0);
  lc.axis = 1; sloth_tree_walk_postorder(sloth, sloth_size_fixup_cb_children_sum, (Sloth_U8*)&lc);
  
  // 4. Preorder - Handle any unhandled cases, including ones that
  //    might not have a neat solution. 
  lc.axis = 0; sloth_tree_walk_preorder(sloth, sloth_size_fixup_cb_violations, (Sloth_U8*)&lc);
  lc.axis = 1; sloth_tree_walk_preorder(sloth, sloth_size_fixup_cb_violations, (Sloth_U8*)&lc);
  
  // Layout text for widgets didn't previously layout their text
  // for sizing purposes. This procedure also clips text for all widgets
  sloth_tree_walk_preorder(sloth, sloth_known_size_layout_text, 0);
  
  // Set final bounding boxes for all widgets (preorder)
  lc.axis = 0; 
  lc.last_sibling_end = 0;
  sloth_tree_walk_preorder(sloth, sloth_layout_cb, (Sloth_U8*)&lc);
  lc.axis = 1; 
  lc.last_sibling_end = 0;
  sloth_tree_walk_preorder(sloth, sloth_layout_cb, (Sloth_U8*)&lc);
  
  sloth_tree_walk_preorder(sloth, sloth_offset_and_clip_text, 0);
  
  // Begin Render Passes
  // Each pass corresponds to a texture lookup.
  // First will be a pass with no texture, then passes for each needed texture atlas
  sloth_tree_walk_preorder(sloth, sloth_render_cb, 0);

  sloth->sentinel = SLOTH_DEBUG_DID_CALL_ADVANCE;
}

Sloth_Function void
sloth_vibuffer_set_vert(Sloth_VIBuffer* buf, Sloth_U32 vert_index, Sloth_R32 x, Sloth_R32 y, Sloth_R32 z, Sloth_R32 u, Sloth_R32 v, Sloth_V4 c)
{
  Sloth_U32 vi = vert_index * SLOTH_VERTEX_STRIDE;
  buf->verts[vi++] = x;
  buf->verts[vi++] = y;
  buf->verts[vi++] = z;
  buf->verts[vi++] = u;
  buf->verts[vi++] = v;
  buf->verts[vi++] = c.x;
  buf->verts[vi++] = c.y;
  buf->verts[vi++] = c.z;
  buf->verts[vi++] = c.w;
}

Sloth_Function Sloth_U32 
sloth_vibuffer_push_vert(Sloth_VIBuffer* buf, Sloth_R32 x, Sloth_R32 y, Sloth_R32 z, Sloth_R32 u, Sloth_R32 v, Sloth_V4 c)
{
  SLOTH_PROFILE_BEGIN;
  buf->verts = sloth_array_grow(
    buf->verts,
    buf->verts_len,
    &buf->verts_cap,
    SLOTH_VERTEX_STRIDE * 256,
    Sloth_R32
  );
  sloth_assert((buf->verts_len % SLOTH_VERTEX_STRIDE) == 0);

  Sloth_U32 vert_index = buf->verts_len / SLOTH_VERTEX_STRIDE;
  sloth_vibuffer_set_vert(buf, vert_index, x, y, z, u, v, c);
  buf->verts_len += SLOTH_VERTEX_STRIDE;
  sloth_assert((buf->verts_len % SLOTH_VERTEX_STRIDE) == 0);

  return vert_index;
}

Sloth_Function Sloth_U32 
sloth_vibuffer_push_tri(Sloth_VIBuffer* buf, Sloth_U32 a, Sloth_U32 b, Sloth_U32 c)
{
  SLOTH_PROFILE_BEGIN;
  buf->indices = sloth_array_grow(
    buf->indices,
    buf->indices_len,
    &buf->indices_cap,
    3 * (256 / 4),
    Sloth_U32
  );
  sloth_assert((buf->indices_len % 3) == 0);
  Sloth_U32 tri_index = buf->indices_len / 3;
  buf->indices[buf->indices_len++] = a;
  buf->indices[buf->indices_len++] = b;
  buf->indices[buf->indices_len++] = c;
  sloth_assert((buf->indices_len % 3) == 0);
  return tri_index;
}

Sloth_Function void
sloth_vibuffer_push_quad(Sloth_VIBuffer* buf, Sloth_U32 a, Sloth_U32 b, Sloth_U32 c, Sloth_U32 d)
{
  SLOTH_PROFILE_BEGIN;
  sloth_vibuffer_push_tri(buf, a, b, c);
  sloth_vibuffer_push_tri(buf, a, c, d);
}

Sloth_Function void
sloth_vibuffer_free(Sloth_VIBuffer* buf)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_U8* unused;
  unused = sloth_realloc(buf->verts, buf->verts_cap * sizeof(Sloth_R32), 0);
  unused = sloth_realloc(buf->indices, buf->indices_cap * sizeof(Sloth_U32), 0);
}

Sloth_Function void
sloth_ctx_free(Sloth_Ctx* sloth)
{
  SLOTH_PROFILE_BEGIN;
  sloth_widget_pool_free(&sloth->widgets);
  sloth_arena_free(&sloth->per_frame_memory);
  sloth_arena_free(&sloth->scratch);
  sloth_vibuffer_free(&sloth->vibuf);
}

typedef struct Sloth_Tree_Print_Data Sloth_Tree_Print_Data;
struct Sloth_Tree_Print_Data
{
  Sloth_U32 indent;
  
  char* buffer;
  Sloth_U32 buffer_size;
  Sloth_U32 buffer_used;
};

Sloth_Function void
sloth_widget_tree_print_cb(Sloth_Ctx* sloth, Sloth_Widget* widget, Sloth_U8* user_data)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Tree_Print_Data* d = (Sloth_Tree_Print_Data*)user_data;
  
  Sloth_U32 needed = stbsp_snprintf(0, 0, "%.*s%s\n",
                                    d->indent * 2, "                                                ",
                                    widget->str);
  if (d->buffer_used + needed > d->buffer_size)
  {
    Sloth_U32 buffer_size = Sloth_Max(d->buffer_size * 2, 2048);
    d->buffer = (char*)sloth_realloc(d->buffer, d->buffer_size, buffer_size);
    d->buffer_size = buffer_size;
  }
  
  d->buffer_used += stbsp_snprintf(d->buffer + d->buffer_used, 
                                   d->buffer_size - d->buffer_used, 
                                   "%.*s%s\n",
                                    d->indent * 2, "                                                ",
                                   widget->str);
  
  if (widget->child_first) {
    d->indent += 1;
  } else if (!widget->sibling_next) {
    d->indent -= 1;
  }
}


Sloth_Function char*
sloth_widget_tree_print(Sloth_Ctx* sloth)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Tree_Print_Data d = {
    .indent = 0,
  };
  sloth_tree_walk_preorder(sloth, sloth_widget_tree_print_cb, (Sloth_U8*)&d);
  return d.buffer;
}


///////////////////////////////////////////////////////
// FONT ATLAS HELPERS
// TODO: Implement this with freetype as well

#ifdef SLOTH_STBTT_ATLAS

#ifndef __STB_INCLUDE_STB_TRUETYPE_H__
#  error "You must include stb_truetype.h (and link to it, if not implementing in this compilation unit).
#endif

typedef struct Sloth_Stbtt_Font Sloth_Stbtt_Font;
struct Sloth_Stbtt_Font
{
  stbtt_fontinfo font;
  Sloth_R32 scale;
};

Sloth_Function Sloth_U8*
sloth_stbtt_font_init(Sloth_Ctx* sloth, Sloth_Font* font, Sloth_U8* font_memory, Sloth_U32 font_memory_size, Sloth_U32 font_index, Sloth_R32 pixel_height)
{
  Sloth_Stbtt_Font* result = (Sloth_Stbtt_Font*)sloth_realloc(0, 0, sizeof(Sloth_Stbtt_Font));
  sloth_zero_struct_(result);
  
  stbtt_InitFont(&result->font, font_memory, stbtt_GetFontOffsetForIndex(font_memory, font_index));
  result->scale = stbtt_ScaleForPixelHeight(&result->font, pixel_height);
  
  Sloth_S32 ascent, descent, line_gap;
  stbtt_GetFontVMetrics(&result->font, &ascent, &descent, &line_gap);
  font->metrics.line_height = (ascent - descent + line_gap) * result->scale;
  
  return (Sloth_U8*)result;
}

Sloth_Function Sloth_Glyph_ID
sloth_stbtt_register_glyph(Sloth_Ctx* sloth, Sloth_Font_ID font_id, Sloth_U32 codepoint)
{
  Sloth_Font* font = sloth_font_get_(sloth, font_id);
  Sloth_Stbtt_Font* stb_font = (Sloth_Stbtt_Font*)font->renderer_data;
  
  Sloth_Glyph_Desc gd;
  sloth_zero_struct_(&gd);
  gd.family = font->weights[font_id.weight_index].glyph_family;
  gd.id = codepoint;
  gd.format = Sloth_GlyphData_Alpha8;

  Sloth_S32 advance, lsb, x0, y0, x1, y1;
  Sloth_U8* bitmap = stbtt_GetCodepointBitmapSubpixel(&stb_font->font, 0, stb_font->scale, 0, 0, codepoint, (Sloth_S32*)&gd.width, (Sloth_S32*)&gd.height, 0, 0);
  stbtt_GetCodepointHMetrics(&stb_font->font, codepoint, &advance, &lsb);
  stbtt_GetCodepointBitmapBoxSubpixel(&stb_font->font, codepoint, stb_font->scale, stb_font->scale, 0, 0, &x0, &y0, &x1, &y1);
  
  gd.id = codepoint;
  gd.stride = gd.width;
  gd.data = bitmap;
  gd.cursor_to_glyph_start_xoff = stb_font->scale * (Sloth_R32)lsb;
  gd.cursor_to_next_glyph = stb_font->scale * (Sloth_R32)advance;
  gd.baseline_offset_y = y0;
  
  Sloth_Glyph_ID result = sloth_glyph_atlas_register(&sloth->glyph_atlas, gd);
  
  // TODO(PS): It might be better if we don't free the bitmap each time
  // but use a persistent backbuffer that gets saved in something like
  // a Sloth_Stbtt_Ctx thing?
  stbtt_FreeBitmap(bitmap, 0);
  
  return result;
}

#endif // SLOTH_STBTT_ATLAS

#ifdef SLOTH_FREETYPE_ATLAS

#include "freetype/freetype2/ft2build.h"
#include FT_FREETYPE_H

typedef struct Sloth_FT2_Ctx Sloth_FT2_Ctx;
struct Sloth_FT2_Ctx
{
  FT_Library ft;
};

typedef struct Sloth_FT2_Font Sloth_FT2_Font;
struct Sloth_FT2_Font
{
  FT_Face ft_face;
};


Sloth_Function Sloth_U8*
sloth_ft2_font_init(Sloth_Ctx* sloth, Sloth_Font* font, Sloth_U8* font_memory, Sloth_U32 font_memory_size, Sloth_U32 font_index, Sloth_R32 pixel_height)
{
  FT_Error error = 0;
  Sloth_FT2_Ctx* ft_ctx = (Sloth_FT2_Ctx*)sloth->font_renderer_data;
  if (ft_ctx == 0)
  {
    ft_ctx = (Sloth_FT2_Ctx*)sloth_realloc(0, 0, sizeof(Sloth_FT2_Ctx));
    error = FT_Init_FreeType(&ft_ctx->ft);
    sloth_assert(!error);
    sloth->font_renderer_data = (Sloth_U8*)ft_ctx;
  }
  
  Sloth_FT2_Font* result = (Sloth_FT2_Font*)sloth_realloc(0, 0, sizeof(Sloth_FT2_Font));
  error = FT_New_Memory_Face(ft_ctx->ft, font_memory, font_memory_size, font_index, &result->ft_face);
  sloth_assert(!error);
  
  error = FT_Set_Pixel_Sizes(result->ft_face, 0, pixel_height);
  Sloth_U32 line_height = (result->ft_face->size->metrics.ascender - result->ft_face->size->metrics.descender);  
  line_height = line_height >> 6;
  font->metrics.line_height = (Sloth_R32)line_height;
  
  return (Sloth_U8*)result;
}

// TODO(PS): This currently doesn't do anything with different font weights
Sloth_Function Sloth_Glyph_ID
sloth_ft2_register_glyph(Sloth_Ctx* sloth, Sloth_Font_ID font_id, Sloth_U32 codepoint)
{
  Sloth_FT2_Ctx* ft_ctx = (Sloth_FT2_Ctx*)sloth->font_renderer_data;
  sloth_assert(ft_ctx != 0);
  
  Sloth_Font* font = sloth_font_get_(sloth, font_id);
  Sloth_FT2_Font* ft_font = (Sloth_FT2_Font*)font->renderer_data;
    
  FT_Error error = FT_Set_Pixel_Sizes(ft_font->ft_face, 0, font->metrics.pixel_height);
    
  FT_UInt glyph_index = FT_Get_Char_Index(ft_font->ft_face, codepoint);
  
  FT_Int32 load_flags = 0;
  error = FT_Load_Glyph(ft_font->ft_face, glyph_index, load_flags);
  sloth_assert(!error);
  
  // Ensure the glyph is rendered
  if (ft_font->ft_face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
  {
    error = FT_Render_Glyph(ft_font->ft_face->glyph, FT_RENDER_MODE_NORMAL);
    sloth_assert(!error);
  }
  
  FT_GlyphSlot slot = ft_font->ft_face->glyph;
  Sloth_Glyph_Desc gd;
  sloth_zero_struct_(&gd);
  gd.family = font->weights[font_id.weight_index].glyph_family;
  gd.id = codepoint;
  gd.format = Sloth_GlyphData_Alpha8;
  gd.width  = slot->bitmap.width;
  gd.height = slot->bitmap.rows;
  gd.stride = slot->bitmap.pitch;
  gd.data   = slot->bitmap.buffer;
  gd.cursor_to_glyph_start_xoff = slot->bitmap_left;
  gd.cursor_to_next_glyph = slot->advance.x >> 6; 
  gd.baseline_offset_y = - slot->bitmap_top;
    
  Sloth_Glyph_ID result = sloth_glyph_atlas_register(&sloth->glyph_atlas, gd);
  return result;
}

#endif

///////////////////////////////////////////////////////
// RENDERING IMPLEMENTATIONS

#ifdef SLOTH_SOKOL_RENDERER

#ifndef SOKOL_GFX_INCLUDED
#  error "The sloth.h sokol renderer backend requires sokol_gfx.h to be included first."
#endif

#include "./sloth_sokol_shader.glsl.h"

typedef struct Sloth_Sokol_Data Sloth_Sokol_Data;
struct Sloth_Sokol_Data
{
  // TODO(PS): supporting multiple textures
  sg_image atlas_texture;
  Sloth_U32 atlas_texture_dim;
  
  sg_pass_action pass_action;
  sg_pipeline pip;
  sg_bindings bind;
  Sloth_U32 bind_quad_cap;
};

Sloth_Function void
sloth_render_sokol_buffers_create(Sloth_Sokol_Data* sd, Sloth_U32 quads)
{
  if (sd->bind.vertex_buffers[0].id != 0) 
  {
    sg_destroy_buffer(sd->bind.vertex_buffers[0]);
    sd->bind.vertex_buffers[0].id = 0;
  }
  if (sd->bind.index_buffer.id != 0) 
  {
    sg_destroy_buffer(sd->bind.index_buffer);
    sd->bind.index_buffer.id = 0;
  }
  
  sg_buffer_desc vbd; sloth_zero_struct_(&vbd);
  vbd.usage = SG_USAGE_STREAM;
  vbd.data.size = quads * 4 * sizeof(Sloth_R32);
  vbd.label = "sloth sokol vertices";

  sg_buffer_desc ibd; sloth_zero_struct_(&ibd);
  ibd.usage = SG_USAGE_STREAM;
  ibd.type = SG_BUFFERTYPE_INDEXBUFFER;
  ibd.data.size = quads * 6 * sizeof(Sloth_U32);
  ibd.label = "sloth sokol indices";

  sd->bind.vertex_buffers[0] = sg_make_buffer(&vbd);
  sd->bind.index_buffer = sg_make_buffer(&ibd);
  sd->bind_quad_cap = quads;
}

Sloth_Function void
sloth_renderer_sokol_atlas_updated(Sloth_Ctx* sloth)
{
  Sloth_Sokol_Data* sd = (Sloth_Sokol_Data*)sloth->render_data;
  sloth_assert(sd != 0);
  
  Sloth_Glyph_Atlas atlas = sloth->glyph_atlas;
  if (sd->atlas_texture.id == 0 || sd->atlas_texture_dim != atlas.dim)
  {
    // create 
    
    sg_image_desc atlas_texture_desc = {
      .width  = atlas.dim,
      .height = atlas.dim,
      .usage = SG_USAGE_DYNAMIC,
      .min_filter = SG_FILTER_NEAREST, //SG_FILTER_LINEAR,
      .mag_filter = SG_FILTER_NEAREST, //SG_FILTER_LINEAR,
      .label = "sloth atlas texture",
    };
    sg_image new_atlas_texture = sg_make_image(&atlas_texture_desc);
    
    if (sd->atlas_texture.id != 0)
    {
      sg_destroy_image(sd->atlas_texture);
    }
    
    sd->atlas_texture = new_atlas_texture;
    sd->atlas_texture_dim = atlas.dim;
  }
  
  // just update the data of the existing texture since
  // the new data still fits
  sg_image_data data = {
    .subimage[0][0] = {
      .ptr = (const char*)atlas.data,
      .size = atlas.dim * atlas.dim * sizeof(Sloth_U32),
    },
  };
  sg_update_image(sd->atlas_texture, &data);
  
  sd->bind.fs_images[SLOT_tex] = sd->atlas_texture;
}

Sloth_Function void
sloth_render_sokol_init(Sloth_Ctx* sloth)
{
  SLOTH_PROFILE_BEGIN;
  sloth->render_data = sloth_realloc(
    sloth->render_data,
    0, sizeof(Sloth_Sokol_Data)
  );

  Sloth_Sokol_Data* sd = (Sloth_Sokol_Data*)sloth->render_data;
  sloth_zero_struct_(sd);
  
  sloth->renderer_atlas_updated = sloth_renderer_sokol_atlas_updated;
  
  sg_pass_action pass_action;
  sloth_zero_struct_(&pass_action);
  pass_action.colors[0].action = SG_ACTION_CLEAR;
  pass_action.colors[0].value.r = 0;
  pass_action.colors[0].value.g = 0;
  pass_action.colors[0].value.b = 0;
  pass_action.colors[0].value.a = 1;
  sd->pass_action = pass_action;

  sg_pipeline_desc pd;
  sloth_zero_struct_(&pd);
  pd.shader = sg_make_shader(sloth_viz_shader_desc(sg_query_backend()));
  pd.index_type = SG_INDEXTYPE_UINT32;
  pd.layout.attrs[ATTR_sloth_viz_vs_position].format = SG_VERTEXFORMAT_FLOAT3;
  pd.layout.attrs[ATTR_sloth_viz_vs_uv].format       = SG_VERTEXFORMAT_FLOAT2;
  pd.layout.attrs[ATTR_sloth_viz_vs_color].format    = SG_VERTEXFORMAT_FLOAT4;
  pd.label = "sloth sokol pipeline";
  pd.colors[0].blend.enabled = true;
  pd.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
  pd.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  pd.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA;
  pd.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  sd->pip = sg_make_pipeline(&pd);
  
  sloth_zero_struct_(&sd->bind);
  sloth_render_sokol_buffers_create(sd, 1024);
}

Sloth_Function void
sloth_render_sokol(Sloth_Ctx* sloth, Sloth_U32 width, Sloth_U32 height)
{
  SLOTH_PROFILE_BEGIN;
  Sloth_Sokol_Data* sd = (Sloth_Sokol_Data*)sloth->render_data;
  
  // Update the bindings
  if (sloth->vibuf.verts_len > sd->bind_quad_cap * 4) {
    Sloth_U32 new_cap = sd->bind_quad_cap * 2;
    while (new_cap * 4 < sloth->vibuf.verts_len) new_cap *= 2;
    sloth_render_sokol_buffers_create(sd, new_cap);
  }
  // TODO: Check if we are about to push a range bigger than the 
  // buffer's size. If we are, we need to create a new buffer and
  // free the old one
  sg_range vertex_range;
  vertex_range.ptr = (const void*)sloth->vibuf.verts;
  vertex_range.size = sloth->vibuf.verts_len * sizeof(Sloth_R32);
  sg_update_buffer(sd->bind.vertex_buffers[0], (const sg_range*)&vertex_range);

  sg_range index_range;
  index_range.ptr = (const void*)sloth->vibuf.indices;
  index_range.size = sloth->vibuf.indices_len * sizeof(Sloth_U32);
  sg_update_buffer(sd->bind.index_buffer, (const sg_range*)&index_range);

  // Draw the Frame
  sg_begin_default_pass(&sd->pass_action, width, height);
    if (sloth->vibuf.indices_len > 0)
    {
      sg_apply_pipeline(sd->pip);
      sg_apply_bindings(&sd->bind);

      sloth_viz_vs_params_t sloth_viz_vs_params = {
        .mvp = HMM_Orthographic(
          0, (Sloth_R32)width,
          (Sloth_R32)height, 0,
          -1, 100
        )
      };
      sg_apply_uniforms(
        SG_SHADERSTAGE_VS, 
        SLOT_sloth_viz_vs_params, 
        &SG_RANGE(sloth_viz_vs_params)
      );

      sg_draw(0, sloth->vibuf.indices_len, 1);
    }
  sg_end_pass();
}

#endif // SLOTH_SOKOL_RENDERER
#endif // SLOTH_IMPLEMENTATION
#endif // SLOTH_H


/*
Copyright 2022 Peter Slattery

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
