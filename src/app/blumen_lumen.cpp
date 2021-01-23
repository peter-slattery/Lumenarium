//
// File: blumen_lumen.cpp
// Author: Peter Slattery
// Creation Date: 2021-01-23
//
#ifndef BLUMEN_LUMEN_CPP

// TODO(pjs):
// - need to request opening a network port and getting messages from it pumped into CustomUpdate
//

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
    
#if 1
    gs_const_string SculpturePath = ConstString("data/test_blumen.fold");
    LoadAssembly(&State->Assemblies, &State->LedSystem, State->Transient, Context, SculpturePath, State->GlobalLog);
#endif
    
    { // Animation PLAYGROUND
        animation Anim0 = {0};
        Anim0.Name = PushStringF(&State->Permanent, 256, "test_anim_zero");
        Anim0.Layers = AnimLayerArray_Create(State->AnimationSystem.Storage, 8);
        Anim0.Blocks_ = AnimBlockArray_Create(State->AnimationSystem.Storage, 8);
        Anim0.PlayableRange.Min = 0;
        Anim0.PlayableRange.Max = SecondsToFrames(15, State->AnimationSystem);
        Animation_AddLayer(&Anim0, MakeString("Base Layer"), BlendMode_Overwrite, &State->AnimationSystem);
        
        Animation_AddBlock(&Anim0, 0, Anim0.PlayableRange.Max, Patterns_IndexToHandle(5), 0);
        
        AnimationArray_Push(&State->AnimationSystem.Animations, Anim0);
        
        animation Anim1 = {0};
        Anim1.Name = PushStringF(&State->Permanent, 256, "test_anim_one");
        Anim1.Layers = AnimLayerArray_Create(State->AnimationSystem.Storage, 8);
        Anim1.Blocks_ = AnimBlockArray_Create(State->AnimationSystem.Storage, 8);
        Anim1.PlayableRange.Min = 0;
        Anim1.PlayableRange.Max = SecondsToFrames(15, State->AnimationSystem);
        Animation_AddLayer(&Anim1, MakeString("Base Layer"), BlendMode_Overwrite, &State->AnimationSystem);
        
        Animation_AddBlock(&Anim1, 0, Anim0.PlayableRange.Max, Patterns_IndexToHandle(5), 0);
        
        AnimationArray_Push(&State->AnimationSystem.Animations, Anim1);
        
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