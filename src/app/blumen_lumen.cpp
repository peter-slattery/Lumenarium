//
// File: blumen_lumen.cpp
// Author: Peter Slattery
// Creation Date: 2021-01-23
//
#ifndef BLUMEN_LUMEN_CPP

struct foo_
{
    u32 Heyo;
};

internal gs_data
BlumenLumen_CustomInit(app_state* State, context Context)
{
    // This is memory for any custom data that we want to use
    // as a part of a particular sculpture.
    // By returning it from here, it will be sent as an argument to
    // the sculpture's CustomUpdate function;
    gs_data Result = {};
    
    Result = PushSizeToData(&State->Permanent, sizeof(foo_));
    foo_* MyFoo = (foo_*)Result.Memory;
    MyFoo->Heyo = 5;
    
    { // Animation PLAYGROUND
        
        animation Anim = {0};
        Anim.Name = PushStringF(&State->Permanent, 256, "test_anim_one");
        Anim.Layers = AnimLayerArray_Create(State->AnimationSystem.Storage, 8);
        Anim.Blocks_ = AnimBlockArray_Create(State->AnimationSystem.Storage, 8);
        Anim.PlayableRange.Min = 0;
        Anim.PlayableRange.Max = SecondsToFrames(15, State->AnimationSystem);
        Animation_AddLayer(&Anim, MakeString("Base Layer"), BlendMode_Overwrite, &State->AnimationSystem);
        Animation_AddLayer(&Anim, MakeString("Color Layer"), BlendMode_Multiply, &State->AnimationSystem);
        Animation_AddLayer(&Anim, MakeString("Sparkles"), BlendMode_Add, &State->AnimationSystem);
        
        Animation_AddBlock(&Anim, 0, Anim.PlayableRange.Max, Patterns_IndexToHandle(5), 0);
        
        AnimationArray_Push(&State->AnimationSystem.Animations, Anim);
        
        State->AnimationSystem.TimelineShouldAdvance = true;
    } // End Animation Playground
    
    return Result;
}

internal void
BlumenLumen_CustomUpdate(gs_data UserData, app_state* State, context* Context)
{
    foo_* MyFoo = (foo_*)UserData.Memory;
    Assert(MyFoo->Heyo == 5);
}


#define BLUMEN_LUMEN_CPP
#endif // BLUMEN_LUMEN_CPP