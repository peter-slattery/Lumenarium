//
// File: userspace.cpp
// Author: Peter Slattery
// Creation Date: 2021-01-30
//
#ifndef USERSPACE_CPP

internal void
US_LoadPatterns(user_space_desc* Desc, app_state* State, context Context)
{
    if (Desc->LoadPatterns)
    {
        Desc->LoadPatterns(State);
    }
}

internal void
US_CustomInit(user_space_desc* Desc, app_state* State, context Context)
{
    if (Desc->CustomInit)
    {
        Desc->UserData = Desc->CustomInit(State, Context);
    }
}

internal void
US_CustomUpdate(user_space_desc* Desc, app_state* State, context* Context)
{
    if (Desc->CustomUpdate)
    {
        Desc->CustomUpdate(Desc->UserData, State, Context);
    }
}

internal void
US_CustomCleanup(user_space_desc* Desc, app_state* State, context Context)
{
    if (Desc->CustomCleanup)
    {
        Desc->CustomCleanup(Desc->UserData, State, Context);
    }
}


#define USERSPACE_CPP
#endif // USERSPACE_CPP