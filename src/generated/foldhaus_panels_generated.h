global_variable s32 GlobalPanelDefsCount = 4;
global_variable panel_definition GlobalPanelDefs[] = {
    { "Sculpture View", 14, SculptureView_Init, SculptureView_Cleanup, SculptureView_Render },
    { "Animation Timeline", 18, AnimationTimeline_Init, AnimationTimeline_Cleanup, AnimationTimeline_Render },
    { "DMX View", 8, DMXView_Init, DMXView_Cleanup, DMXView_Render },
    { "Profiler", 8, ProfilerView_Init, ProfilerView_Cleanup, ProfilerView_Render },
};