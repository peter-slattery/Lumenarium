#ifndef SLOTH_PROFILER_C
#define SLOTH_PROFILER_C

//////////////////////////////////////////////////////////////////////////////
// Context Cracking

#if defined(__clang__)
#  define SP_COMPILER_CLANG 1

#  if defined(__APPLE__) && defined(__MACH__)
#    define SP_OS_MAC 1
#  elif defined(__gnu_linux__)
#    define SP_OS_LINUX 1
#  elif defined(_WIN32)
#    define SP_OS_WINDOWS 1
#  else
#    error The compiler/platform combo is not supported
#  endif

#elif defined(__GNUC__) || defined(__GNUG__)
#  define SP_COMPILER_GCC 1

#  if defined(__gnu_linux__)
#    define SP_OS_LINUX 1
#  else
#    error The compiler/platform combo is not supported
#  endif

#endif

//////////////////////////////////////////////////////////////////////////////

#ifndef SLOTH_PROFILER_FUNCTION
#  define SLOTH_PROFILER_FUNCTION
#endif

#ifndef sp_assert
#  define sp_assert(v) 
#endif

#ifndef SP_U8
#  define SP_U8 unsigned char
#endif

#ifndef SP_S32
#  define SP_S32 int
#endif

#ifndef SP_U32
#  define SP_U32 unsigned int
#endif

#ifndef SP_U64
#  define SP_U64 unsigned long long int
#endif

#ifndef SP_R64
#  define SP_R64 double
#endif

#ifndef SP_MALLOC
#  define SP_MALLOC(type, count) (type*)malloc(sizeof(type) * (count))
#endif

#ifndef SP_REALLOC
#  define SP_REALLOC(base, type, old_count, new_count) (type*)realloc((void*)(base), (new_count) * sizeof(type))
#endif

//////////////////////////////////////////////////////////////////////////////
// Scope Instrumentation

#if !SP_COMPILER_CLANG && !SP_COMPILER_GCC 
#  error sloth_profiler.c currently relies on a defer implementation that utilizes clang/gcc extensions.
#  error The compiler you are using is not supported.
#endif

#ifndef defer
   static inline void defer_cleanup(void (^*b)(void)) { (*b)(); }
#  define defer_merge(a,b) a##b
#  define defer_varname(a) defer_merge(defer_scopevar_, a)
#  define defer __attribute__((cleanup(defer_cleanup))) void (^defer_varname(__COUNTER__))(void) =
#endif

// Macros for easy #undef if not in debug mode
// NOTE: These are the preferred way to call the sloth_profiler functions
#define SLOTH_PROFILER_FRAME_BEGIN() sp_frame_begin()
#define SLOTH_PROFILER_FRAME_END() sp_frame_end()
#define SLOTH_PROFILER_SCOPE sp_scope_begin((char*)__FUNCTION__, (char*)__FILE__, (SP_S32)__LINE__); defer ^{ sp_scope_end(); };

SLOTH_PROFILER_FUNCTION void sp_init(Sloth_Ctx* sloth, SP_U32 frames_cap);
SLOTH_PROFILER_FUNCTION void sp_frame_begin();
SLOTH_PROFILER_FUNCTION void sp_frame_end();
SLOTH_PROFILER_FUNCTION void sp_scope_begin(char* function, char* file, SP_S32 line);
SLOTH_PROFILER_FUNCTION void sp_scope_end();

SLOTH_PROFILER_FUNCTION void sp_draw();

// Platform Time Keeping Implementations

typedef struct Sloth_Profiler_Ticks Sloth_Profiler_Ticks;
struct Sloth_Profiler_Ticks
{
  SP_U64 value;
};
SLOTH_PROFILER_FUNCTION void sp_ticks_os_init();
SLOTH_PROFILER_FUNCTION Sloth_Profiler_Ticks sp_ticks_now();
SLOTH_PROFILER_FUNCTION Sloth_Profiler_Ticks sp_ticks_elapsed(Sloth_Profiler_Ticks start, Sloth_Profiler_Ticks end);
SLOTH_PROFILER_FUNCTION SP_R64 sp_ticks_to_seconds(Sloth_Profiler_Ticks ticks);
SLOTH_PROFILER_FUNCTION SP_R64 sp_ticks_to_milliseconds(Sloth_Profiler_Ticks ticks);

//////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION

#if defined(SLOTH_PROFILER_IMPLEMENTATION)

static const SP_U64 SP_NANOS_PER_SEC = 1000000000ULL;

typedef struct Sloth_Profiler_Scope_Id Sloth_Profiler_Scope_Id;
struct Sloth_Profiler_Scope_Id
{
  SP_U32 value;
};

// a record of a single time a scope was called
typedef struct Sloth_Profiler_Scope_Call Sloth_Profiler_Scope_Call;
struct Sloth_Profiler_Scope_Call
{
  Sloth_Profiler_Scope_Id id;
  char* name; // temp
  Sloth_Profiler_Ticks start;
  Sloth_Profiler_Ticks end;
  SP_U32 parent;
  SP_U32 first_child;
  SP_U32 last_child;
  SP_U32 next_sibling;
};

// A record of a particular scope, across all call so that scope
typedef struct Sloth_Profiler_Scope Sloth_Profiler_Scope;
struct Sloth_Profiler_Scope
{
  char* name;
  char* file;
  SP_U32 line;
  
  Sloth_Profiler_Ticks ticks_longest;
  Sloth_Profiler_Ticks ticks_shortest;
  Sloth_Profiler_Ticks ticks_average;
  SP_U64 total_count;
  SP_U64 count_last_frame;
};

typedef struct Sloth_Profiler_Frame Sloth_Profiler_Frame;
struct Sloth_Profiler_Frame
{
  Sloth_Profiler_Scope_Call* calls;
  SP_U32                     calls_len;
  SP_U32                     calls_cap;
  SP_U32                     parent_cur;
  
  Sloth_Profiler_Ticks start;
  Sloth_Profiler_Ticks end;
};

enum {
  SlothProfiler_Recording = 0,
  SlothProfiler_WaitingToBeginRecording = 1,
  SlothProfiler_Paused = 2
};

typedef struct Sloth_Profiler_Ctx Sloth_Profiler_Ctx;
struct Sloth_Profiler_Ctx
{
  // hashtable of scopes
  Sloth_Hashtable          scope_ids;
  Sloth_Profiler_Scope*    scopes;
  SP_U32                   scopes_cap;
  SP_U32                   scopes_len;
  
  // ring buffer of stored frames
  Sloth_Profiler_Frame* frames;
  SP_U32             frames_cap;
  SP_U32             frame_at;
  
  Sloth_U32 depth;
  
  // UI
  SP_U32 ui_frame;
  SP_U8 recording;
  SP_R64 visible_min;
  SP_R64 visible_max;
};

// a pointer to the applications Sloth_Ctx
// the profiler doesn't assume it owns this data
static Sloth_Ctx* sp_ctx_;
static Sloth_Profiler_Ctx* sp_pctx_;
static SP_R64 sp_ticks_per_second_;
static SP_R64 sp_ticks_per_millisecond_;

SLOTH_PROFILER_FUNCTION void
sp_init(Sloth_Ctx* sloth, SP_U32 frames_cap)
{
  sp_ctx_ = sloth;
  
  sp_pctx_ = SP_MALLOC(Sloth_Profiler_Ctx, 1);
  *sp_pctx_ = (Sloth_Profiler_Ctx){
    // Sloth_Hashtable has a default size of 2048
    .scope_ids = {},
    .scopes = SP_MALLOC(Sloth_Profiler_Scope, 2048),
    .scopes_cap = 2048,
    .scopes_len = 0,
    .frames = SP_MALLOC(Sloth_Profiler_Frame, frames_cap),
    .frames_cap = frames_cap,
    .frame_at = 0,
  };
  
  for (SP_U32 i = 0; i < sp_pctx_->frames_cap; i++)
  {
    sp_pctx_->frames[i] = (Sloth_Profiler_Frame){};
  }
  
  sp_ticks_os_init();
}

SLOTH_PROFILER_FUNCTION Sloth_Profiler_Frame*
sp_get_frame_cur()
{
  Sloth_Profiler_Frame* frame = sp_pctx_->frames + sp_pctx_->frame_at;
  return frame;
}

SLOTH_PROFILER_FUNCTION void
sp_begin_recording_force()
{
  sp_pctx_->recording = SlothProfiler_Recording;
}

SLOTH_PROFILER_FUNCTION void
sp_begin_recording()
{
  sp_pctx_->recording = SlothProfiler_WaitingToBeginRecording;
}

SLOTH_PROFILER_FUNCTION void
sp_pause_recording()
{
  sp_pctx_->recording = SlothProfiler_Paused;
}

SLOTH_PROFILER_FUNCTION Sloth_Profiler_Scope_Call*
sp_frame_push_call(Sloth_Profiler_Frame* frame, SP_U32* index_out)
{
  if (frame->calls_len >= frame->calls_cap) {
    SP_U32 calls_cap = frame->calls_cap * 2;
    if (calls_cap == 0) calls_cap = 1024;
    frame->calls = SP_REALLOC(frame->calls, Sloth_Profiler_Scope_Call, frame->calls_cap, calls_cap);
    frame->calls_cap = calls_cap;
  }
  
  SP_U32 index = frame->calls_len++;
  Sloth_Profiler_Scope_Call* call = frame->calls + index;
  if (index_out) *index_out = index;
  return call;
}

#define TEMP_MAX_DEPTH 8

SLOTH_PROFILER_FUNCTION void 
sp_frame_begin()
{
  if (sp_pctx_->recording == SlothProfiler_Paused) return;
  sp_pctx_->recording = SlothProfiler_Recording;
  
  Sloth_Profiler_Frame* frame = sp_get_frame_cur();
  frame->start = sp_ticks_now();
  frame->calls_len = 0;
  frame->parent_cur = 0;
  
  sp_pctx_->depth = 0;
  
  SP_U32 index = 1234; // garbage value
  Sloth_Profiler_Scope_Call* root = sp_frame_push_call(frame, &index);
  *root = (Sloth_Profiler_Scope_Call){
    .name = "root",
    .id.value = 0,
    .start = sp_ticks_now(),
    .parent = 0,
    .first_child = 0,
    .last_child = 0,
    .next_sibling = 0,
  };
  sloth_assert(index == 0);
}

SLOTH_PROFILER_FUNCTION void 
sp_frame_end()
{
  if (sp_pctx_->recording != SlothProfiler_Recording) return;
  
  Sloth_Profiler_Frame* frame = sp_pctx_->frames + sp_pctx_->frame_at;
  frame->end = sp_ticks_now();
  frame->calls[0].end = frame->end;
  // TODO: Frame Cleanup
  
  sp_pctx_->frame_at++;
  if (sp_pctx_->frame_at >= sp_pctx_->frames_cap) {
    sp_pctx_->frame_at = 0;
  }
}

SLOTH_PROFILER_FUNCTION void 
sp_scope_begin(char* function, char* file, SP_S32 line)
{  
  if (sp_pctx_->recording != SlothProfiler_Recording) return;
  
  sp_pctx_->depth += 1;
  if (sp_pctx_->depth > TEMP_MAX_DEPTH) return;
  
  
  // Hash the function name
  // djb2 hash - http://www.cse.yorku.ca/~oz/hash.html
  Sloth_Profiler_Scope_Id id = { .value = 5381 };
  for (char* at = function; *at != 0; at++)
  {
    id.value = ((id.value << 5) + id.value) + (Sloth_U8)at[0];
  }
  
  // Register the scope if needed
  if (!sloth_hashtable_get(&sp_pctx_->scope_ids, id.value)) {
    SP_U64 index = (Sloth_U64)sp_pctx_->scopes_len++;
    Sloth_Profiler_Scope* scope = sp_pctx_->scopes + index;
    *scope = (Sloth_Profiler_Scope){
      .name = function,
      .file = file,
      .line = line,
      .ticks_longest = 0,
      .ticks_shortest = 0,
      .ticks_average = 0,
      .total_count = 0,
      .count_last_frame = 0,
    };
    
    sloth_hashtable_add(&sp_pctx_->scope_ids, id.value, (Sloth_U8*)scope);
  }
  
  Sloth_Profiler_Frame* frame = sp_get_frame_cur();
  SP_U32 index = 0;
  Sloth_Profiler_Scope_Call* call = sp_frame_push_call(frame, &index);
  *call = (Sloth_Profiler_Scope_Call){
    .id = id,
    .name = function,
    .start = sp_ticks_now(),
    .parent = frame->parent_cur,
    .first_child = 0,
    .last_child = 0,
    .next_sibling = 0,
  };
  
  if (index > 0) {
    Sloth_Profiler_Scope_Call* parent = frame->calls + frame->parent_cur;    
    if (parent->first_child != 0) {
      Sloth_Profiler_Scope_Call* last_sib = frame->calls + parent->last_child;
      last_sib->next_sibling = index;
      sloth_assert(parent->last_child != index);
    } else {
      frame->calls[index - 1].first_child = index;
    }
    parent->last_child = index;
  }
  frame->parent_cur = index;
}

SLOTH_PROFILER_FUNCTION void 
sp_scope_end()
{
  if (sp_pctx_->recording != SlothProfiler_Recording) return;
  sp_pctx_->depth -= 1;
  if (sp_pctx_->depth > TEMP_MAX_DEPTH) return;
  
  Sloth_Profiler_Frame* frame = sp_get_frame_cur();
  Sloth_Profiler_Scope_Call* call = frame->calls + frame->parent_cur;
  call->end = sp_ticks_now();
  frame->parent_cur = call->parent;
}

SLOTH_PROFILER_FUNCTION Sloth_Profiler_Ticks
sp_ticks_elapsed(Sloth_Profiler_Ticks start, Sloth_Profiler_Ticks end)
{
  sloth_assert(end.value >= start.value);
  Sloth_Profiler_Ticks result = {
    .value = end.value - start.value
  };
  return result;
}

SLOTH_PROFILER_FUNCTION SP_R64
sp_ticks_to_seconds(Sloth_Profiler_Ticks ticks)
{
  SP_R64 result = (SP_R64)ticks.value / sp_ticks_per_second_;
  return result;
}

SLOTH_PROFILER_FUNCTION SP_R64
sp_ticks_to_milliseconds(Sloth_Profiler_Ticks ticks)
{
  SP_R64 result = (SP_R64)ticks.value / sp_ticks_per_millisecond_;
  return result;
}

#if defined(SP_OS_MAC)

static SP_U64 sp_osx_start_time_absolute_ = 0;
static mach_timebase_info_data_t sp_osx_mach_time_info_ = {};

SLOTH_PROFILER_FUNCTION void
sp_ticks_os_init()
{
  sp_osx_start_time_absolute_ = mach_absolute_time();
  mach_timebase_info(&sp_osx_mach_time_info_);
  
  SP_R64 numer = (SP_R64)sp_osx_mach_time_info_.numer;
  SP_R64 denom = (SP_R64)sp_osx_mach_time_info_.denom;
  SP_R64 nps =   (SP_R64)SP_NANOS_PER_SEC;
  sp_ticks_per_second_ = (numer / denom) * nps;
  sp_ticks_per_millisecond_ = (numer / denom) * 1000 * 1000;
}

SLOTH_PROFILER_FUNCTION Sloth_Profiler_Ticks 
sp_ticks_now()
{
  Sloth_Profiler_Ticks result = {
    .value = mach_absolute_time() - sp_osx_start_time_absolute_   
  };
  return result;
}

#elif defined(SP_OS_LINUX)

SLOTH_PROFILER_FUNCTION Sloth_Profiler_Ticks 
sp_ticks_now()
{
}

#elif defined(SP_OS_WINDOWS)

SLOTH_PROFILER_FUNCTION void
sp_ticks_os_init()
{
  LARGE_INTEGER freq;
  if (!QueryPerformanceFrequency(&freq))
  {
    SLOTH_PROFILER_ERROR("Unable to QueryPerformanceCounter");
  }
  sp_ticks_per_second = (SP_U64)freq.QuadPart;
}

SLOTH_PROFILER_FUNCTION Sloth_Profiler_Ticks 
sp_ticks_now()
{
  LARGE_INTEGER t;
  if (!QueryPerformanceCounter(&t)) 
  {
    SLOTH_PROFILER_ERROR("Unable to QueryPerformanceCounter");
  }
  Sloth_Profiler_Ticks result = {
    .value = (SP_U64)t.QuadPart,
  };
  return result;
}

#endif // SP_OS_*

////////////////////////////////////////////////////////
// Visualizer Interface

void
sp_popup_f(char* fmt, ...)
{
  Sloth_Widget_Desc desc = {
    .layout = {
      .width = sloth_size_text_content(),
      .height = sloth_size_text_content(),
      .margin = sloth_size_box_uniform_pixels(8),
      .position = {
        .kind = Sloth_LayoutPosition_FixedOnScreen,
        .left = sloth_size_pixels(sp_ctx_->mouse_pos.x),
        .top = sloth_size_pixels(sp_ctx_->mouse_pos.y),
        .z = -0.5f,
      },
    },
    .style = {
      .color_bg = 0x000000FF,
      .color_text = 0xFFFFFFFF,
    },
    .input = {
      .flags = Sloth_WidgetInput_DoNotCaptureMouse,
    },
  };
  
  va_list args; va_start(args, fmt);
  sloth_push_widget_v(sp_ctx_, desc, fmt, args);
  sloth_pop_widget(sp_ctx_);
  va_end(args);
}

bool
sp_button_f(char* fmt, ...)
{
  bool result = false;
  Sloth_Widget_Desc desc = {
    .layout = {
      .width = sloth_size_text_content(),
      .height = sloth_size_text_content(),
      .margin = sloth_size_box_uniform_pixels(8),
    },
    .style = {
      .color_bg = 0x000000FF,
      .color_text = 0xFFFFFFFF,
    },
  };
  
  va_list args; va_start(args, fmt);
  Sloth_Widget_Result r = sloth_push_widget_v(sp_ctx_, desc, fmt, args);
  if (sloth_ids_equal(r.widget->id, sp_ctx_->hot_widget)) {
    r.widget->style.color_bg = 0xFFFFFFFF;
    r.widget->style.color_text = 0x000000FF;
  }
  if (r.clicked) {
    result = true;
    r.widget->style.color_bg = 0x00FFFFFF;
    r.widget->style.color_text = 0x000000FF;
  }
  sloth_pop_widget(sp_ctx_);
  va_end(args);
  
  return result;
}

void
sp_h_scope_slider(char* name, SP_R64* cur_min, SP_R64* cur_max)
{
  Sloth_Widget_Desc desc = {
    .layout = {
      .width = sloth_size_percent_parent(1),
      .height = sloth_size_pixels(16),
    },
    .style = {
      .color_bg = 0x000000FF,
      .color_outline = 0xFFFFFFFF,
      .outline_thickness = 1,
    },
  };  
  
  Sloth_Widget_Desc handle_desc = {
    .layout = {
      .width = sloth_size_pixels(16),
      .height = sloth_size_pixels(16),
      .position.kind = Sloth_LayoutPosition_FixedInParent
    },
    .style = {
      .color_bg = 0x888888FF,
    },
    .input.flags = Sloth_WidgetInput_Draggable      
  };
    
  sloth_push_widget_f(sp_ctx_, desc, "###slider_%s", name);
  {
    handle_desc.layout.position.left = sloth_size_percent_parent(*cur_min);
    Sloth_Widget_Result r0 = sloth_push_widget_f(sp_ctx_, handle_desc, "###slider_%s_min", name);
    sloth_pop_widget(sp_ctx_);
    if (r0.held) {
      r0.widget->style.color_bg = 0x00FFFF00;
      SP_R64 new_min_pct = *cur_min + r0.drag_offset_percent_parent.x;
      new_min_pct = Sloth_Clamp(0, new_min_pct, 1);
      Sloth_Widget_Desc drag_desc = handle_desc;
      drag_desc.layout.position.left = sloth_size_percent_parent(new_min_pct);
      drag_desc.input.flags = Sloth_WidgetInput_DoNotCaptureMouse;
      sloth_push_widget_f(sp_ctx_, drag_desc, "###slider_moving_%s_min", name);
      sloth_pop_widget(sp_ctx_);      
    }
    if (r0.released) {
      Sloth_R32 new_min_pct = (*cur_min + r0.drag_offset_percent_parent.x);
      *cur_min = new_min_pct = Sloth_Clamp(0, new_min_pct, 1);
    }
    
    handle_desc.layout.position.left = (Sloth_Size){};
    handle_desc.layout.position.right = sloth_size_percent_parent(*cur_max);
    Sloth_Widget_Result r1 = sloth_push_widget_f(sp_ctx_, handle_desc, "###slider_%s_max", name);
    sloth_pop_widget(sp_ctx_);
    if (r1.held) {
      r1.widget->style.color_bg = 0x00FFFF00;
      SP_R64 new_max_pct = *cur_max + r1.drag_offset_percent_parent.x;
      new_max_pct = Sloth_Clamp(0, new_max_pct, 1);
      Sloth_Widget_Desc drag_desc = handle_desc;
      drag_desc.layout.position.right = sloth_size_percent_parent(new_max_pct);
      drag_desc.input.flags = Sloth_WidgetInput_DoNotCaptureMouse;
      sloth_push_widget_f(sp_ctx_, drag_desc, "###slider_moving_%s_max", name);
      sloth_pop_widget(sp_ctx_);      
    }
    if (r1.released) {
      Sloth_R32 new_max_pct = (*cur_max + r1.drag_offset_percent_parent.x);
      *cur_max = new_max_pct = Sloth_Clamp(0, new_max_pct, 1);
    }
    
  }  
  sloth_pop_widget(sp_ctx_);  
}

bool
sp_frame_bar(SP_U32 frame_i)
{
  bool pause_recording = false;
  
  Sloth_Widget_Desc border_desc = {
    .layout = {
      .width = sloth_size_percent_parent(1),
      .height = sloth_size_children_sum(),
      .margin = sloth_size_box_uniform_pixels(4),
      .direction = Sloth_LayoutDirection_LeftToRight,
      // TODO
      //.child_h_gap = sloth_size_pixels(4),
    },
    .style = {
      .color_outline = 0xFFFFFFFF,
      .outline_thickness = 4,
    },
  };
  Sloth_Widget_Desc frame_box_desc = {
    .layout = {
      .width = sloth_size_percent_parent(1.0f / sp_pctx_->frames_cap),
      .height = sloth_size_pixels(64),
    },
    .style = {
      .color_bg = 0xFFFFFFFF,
      .color_outline = 0x000000FF,
      .outline_thickness = 1,
    },
  };
  
  sloth_push_widget(sp_ctx_, border_desc, "###frame_bar");
  {
    Sloth_Widget_Result r;
    SP_U32 popup_frame = sp_pctx_->frames_cap;
    for (SP_U32 i = 0; i < sp_pctx_->frames_cap; i++) {
      r = sloth_push_widget_f(sp_ctx_, frame_box_desc, "###frame_bar_%d", i);
      if (i == frame_i) {
        r.widget->style.color_bg = 0x00FFFFFF;
      }
      if (sloth_ids_equal(r.widget->id, sp_ctx_->hot_widget)) {
        r.widget->style.color_bg = 0x00FF00FF;
        popup_frame = i;
        if (sloth_mouse_button_is_down(sp_ctx_->mouse_button_l))
        {
          pause_recording = true;
          sp_pctx_->ui_frame = i;
        }
      }
      sloth_pop_widget(sp_ctx_);
    }
    if (popup_frame < sp_pctx_->frames_cap)
    {
      sp_popup_f("Frame: %d##popup", popup_frame);
    }
  }
  sloth_pop_widget(sp_ctx_);
  
  border_desc.style.outline_thickness = 0;
  sloth_push_widget(sp_ctx_, border_desc, "###frame_bar_btns");
  {
    if (sp_button_f("Resume Recording")) {
      sp_begin_recording();
    }
    if (sp_button_f("Pause Recording")) {
      pause_recording = true;
      sp_pctx_->ui_frame = sp_pctx_->frame_at - 1;
    }
  }
  sloth_pop_widget(sp_ctx_);
  
  return pause_recording;
}

void
sp_frame_header(Sloth_Profiler_Frame* frame, SP_U32 frame_i)
{
  Sloth_Widget_Desc header_desc = {
    .layout = {
      .width = sloth_size_percent_parent(1),
      .height = sloth_size_text_content(),
      .margin = sloth_size_box_uniform_pixels(4),
    },
    .style = {
      .color_outline = 0xFFFFFFFF,
      .color_text = 0xFFFFFFFF,
      .outline_thickness = 1,
    },
  };
  
  bool mouse_down = sloth_mouse_button_is_down(sp_ctx_->mouse_button_l);
  Sloth_Profiler_Ticks ticks = sp_ticks_elapsed(frame->start, frame->end);
  SP_R64 ms = sp_ticks_to_milliseconds(ticks);
  sloth_push_widget_f(sp_ctx_, header_desc, "Frame: %d\nCalls: %d\nTicks: %lld\nTime: %fms###profiler_frame_header",
                      frame_i, frame->calls_len, ticks.value, ms);
  sloth_pop_widget(sp_ctx_);
}

Sloth_U32 sp_calls_visualized = 0;

void
sp_flame_graph_call(Sloth_Profiler_Frame* frame, SP_U32 call_index, Sloth_R32 visible_width_pixels,
                    Sloth_Profiler_Ticks visible_start, Sloth_Profiler_Ticks visible_duration, Sloth_U32 depth)
{
  
  Sloth_Widget_Desc scope_desc = {
    .layout = {
      .height = sloth_size_pixels(16),
      .position = {
        .kind = Sloth_LayoutPosition_FixedInParent,
        .top = sloth_size_pixels(16 * depth),
      },
    },
    .style = {
      .color_outline = 0x00FF00FF,
      .outline_thickness = 1,
    },
  };
  
  Sloth_Profiler_Scope_Id last_unused_id = {};
  Sloth_Profiler_Ticks unused_run_start = {};
  do {
    Sloth_Profiler_Scope_Call call = frame->calls[call_index];
    Sloth_Profiler_Scope* scope = (Sloth_Profiler_Scope*)sloth_hashtable_get(&sp_pctx_->scope_ids, call.id.value);
    char* name = "root";
    if (scope) name = scope->name;
    
    bool is_after_visible = call.start.value > (visible_start.value + visible_duration.value);
    bool is_before_visible = call.end.value < visible_start.value;
    if (is_after_visible || is_before_visible) {
      call_index = call.next_sibling;
      continue;
    }
    
    Sloth_Profiler_Ticks call_duration = sp_ticks_elapsed(call.start, call.end);
    SP_R64 call_pct_visible_duration = (SP_R64)call_duration.value / (SP_R64)visible_duration.value;
    
    // See if this block is next to more blocks of the same call
    // and collapse them if they're too small to see individually
    SP_R64 min_draw_pct_width = 0.05f;
    if (call_pct_visible_duration <= min_draw_pct_width)
    {
      bool is_in_run = (last_unused_id.value == call.id.value);
      bool is_end_of_run = false;
      if (call.next_sibling != 0) 
      {
        Sloth_Profiler_Scope_Call next_call = frame->calls[call.next_sibling];
        is_end_of_run = (next_call.id.value != last_unused_id.value);
      }
      if (is_in_run && is_end_of_run) 
      {
        call_duration = sp_ticks_elapsed(unused_run_start, call.end);
        call_pct_visible_duration = (SP_R64)call_duration.value / (SP_R64)visible_duration.value;        
      }
      if (is_end_of_run)
      {
        last_unused_id.value = 0;
      }
    }
    
    if (call_pct_visible_duration > min_draw_pct_width)
    {
      Sloth_Profiler_Ticks call_offset_start, call_offset_end;
      if (call.start.value > visible_start.value) {
        call_offset_start = sp_ticks_elapsed(visible_start, call.start);
      } else {
        call_offset_start.value = 0;
      }
      if (call.end.value < visible_start.value + visible_duration.value) {
        call_offset_end = sp_ticks_elapsed(visible_start, call.end);
      } else {
        call_offset_end.value = visible_duration.value;
      }
            
      SP_R64 call_pct_start = (SP_R64)call_offset_start.value / (SP_R64)visible_duration.value;
      SP_R64 call_pct_end   = (SP_R64)call_offset_end.value   / (SP_R64)visible_duration.value;      
      sloth_assert(call_pct_start >= 0 && call_pct_start <= 1 &&
                   call_pct_end   >= 0 && call_pct_end   <= 1);
      SP_R64 call_pct_duration = call_pct_end - call_pct_start;      
      
      
      
      scope_desc.layout.width = sloth_size_pixels(visible_width_pixels * call_pct_duration);
      scope_desc.layout.position.left = sloth_size_pixels(visible_width_pixels * call_pct_start);
    
      Sloth_Widget_Desc scope_bar_desc = {
        .layout = {
          .width = sloth_size_percent_parent(call_pct_duration),
          .height = sloth_size_pixels(16),
        },
        .style = {
          .color_outline = 0x00FF00FF,
          .outline_thickness = 1,
        },
      };
      
      Sloth_Widget_Result r = sloth_push_widget_f(sp_ctx_, scope_desc, "###%s_%d_fg_bar", name, sp_calls_visualized++);
      {
        if (sloth_ids_equal(r.widget->id, sp_ctx_->hot_widget))
        {
          Sloth_Profiler_Ticks t = sp_ticks_elapsed(call.start, call.end);      
          sp_popup_f("%s - %lld###fg_popup", name, t.value);
        }
      }
      sloth_pop_widget(sp_ctx_);
      
      if (call.first_child != 0) {
        sp_flame_graph_call(frame, call.first_child, visible_width_pixels, visible_start, visible_duration, depth + 1);
      }
    }
    else if (last_unused_id.value != call.id.value)
    {
      unused_run_start = call.start;
      last_unused_id = call.id;
    }
        
    sloth_assert(call.next_sibling == 0 || call.next_sibling > call_index);
    sloth_assert(call.next_sibling < frame->calls_len);
    call_index = call.next_sibling;
  } while (call_index != 0);
}

void
sp_flame_graph(Sloth_Profiler_Frame* frame, Sloth_R32 pixel_width)
{
  Sloth_Widget_Desc fg_desc = {
    .layout = {
      .width = sloth_size_percent_parent(1),
      .height = sloth_size_pixels(128),
      .margin = sloth_size_box_uniform_pixels(4),
    },
    .style = {
      .color_outline = 0xFFFFFFFF,
      .outline_thickness = 1,
    },
  };
  
  sp_calls_visualized = 0;
  Sloth_Profiler_Ticks frame_duration = sp_ticks_elapsed(frame->start, frame->end);
  Sloth_Profiler_Ticks frame_start = { 
    .value = frame->start.value + (frame_duration.value * sp_pctx_->visible_min),
  };
  frame_duration.value *= ((1.0f - sp_pctx_->visible_max) - sp_pctx_->visible_min);
  sloth_push_widget_f(sp_ctx_, fg_desc, "###profiler_flame_graph");
  if (frame->calls) 
  {
    sp_flame_graph_call(frame, 0, pixel_width, frame_start, frame_duration, 0);
  }
  sloth_pop_widget(sp_ctx_);
  
  sp_h_scope_slider("flame_graph_slider", &sp_pctx_->visible_min, &sp_pctx_->visible_max);
}

SLOTH_PROFILER_FUNCTION void 
sp_draw()
{
  if (!sp_pctx_) return;
  
  SP_U32 frame_i = sp_pctx_->frame_at - 1;
  if (sp_pctx_->recording == SlothProfiler_Paused) frame_i = sp_pctx_->ui_frame;
  if (frame_i > sp_pctx_->frames_cap) frame_i = sp_pctx_->frames_cap - 1;
  
  bool pause_recording = false;
  bool was_recording = sp_pctx_->recording == SlothProfiler_Recording;
  if (was_recording) sp_pause_recording();
  
  Sloth_Profiler_Frame* frame = sp_pctx_->frames + frame_i;
  
  Sloth_Widget_Desc bg_desc = {
    .layout = {
      .width = sloth_size_pixels(800),
      .height = sloth_size_pixels(600),
      .margin = sloth_size_box_uniform_pixels(16),
      .position = {
        .kind = Sloth_LayoutPosition_FixedOnScreen,
        .left = sloth_size_pixels(32),
        .top = sloth_size_pixels(48),
      },
    },
    .style = {
      .color_bg = 0x333333FF,
    },
  };
  sloth_push_widget(sp_ctx_, bg_desc, "###profiler_root");
  {
    pause_recording = sp_frame_bar(frame_i);    
    sp_frame_header(frame, frame_i);
    sp_flame_graph(frame, 800 - 32);
  }  
  sloth_pop_widget(sp_ctx_);
  
  if (pause_recording) { 
    sp_pause_recording(); 
  }
  else if (was_recording) 
  { 
    sp_begin_recording_force(); 
  }
}
  

#endif // SLOTH_PROFILER_IMPLEMENTATION

#endif // SLOTH_PROFILER_C