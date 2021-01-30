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

typedef struct user_space_desc
{
    us_load_patterns_proc* LoadPatterns;
    us_custom_init_proc* CustomInit;
    us_custom_update_proc* CustomUpdate;
    
    gs_data UserData;
} user_space_desc;

#define USERSPACE_H
#endif // USERSPACE_H