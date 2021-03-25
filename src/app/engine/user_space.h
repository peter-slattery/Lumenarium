//
// File: userspace.h
// Author: Peter Slattery
// Creation Date: 2021-01-30
//
#ifndef USERSPACE_H

#define US_LOAD_PATTERNS(name) void name(app_state* State)
typedef US_LOAD_PATTERNS(us_load_patterns_proc);

#define US_CUSTOM_INIT(name) gs_data name (app_state* State, context Context)
typedef US_CUSTOM_INIT(us_custom_init_proc);

#define US_CUSTOM_UPDATE(name) void name(gs_data UserData, app_state* State, context* Context)
typedef US_CUSTOM_UPDATE(us_custom_update_proc);

#define US_CUSTOM_DEBUG_UI(name) void name(gs_data UserData, panel* Panel, rect2 PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
typedef US_CUSTOM_DEBUG_UI(us_custom_debug_ui);

#define US_CUSTOM_CLEANUP(name) void name(gs_data UserData, app_state* State, context Context)
typedef US_CUSTOM_CLEANUP(us_custom_cleanup_proc);

typedef struct user_space_desc
{
    us_load_patterns_proc* LoadPatterns;
    us_custom_init_proc* CustomInit;
    us_custom_update_proc* CustomUpdate;
    us_custom_debug_ui* CustomDebugUI;
    us_custom_cleanup_proc* CustomCleanup;
    
    gs_data UserData;
} user_space_desc;

#define USERSPACE_H
#endif // USERSPACE_H