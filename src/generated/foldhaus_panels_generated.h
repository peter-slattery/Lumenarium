global_variable s32 GlobalPanelDefsCount = 6;
global_variable panel_definition GlobalPanelDefs[] = {
    { "Sculpture View", 14, SculptureView_Init, SculptureView_Cleanup, SculptureView_Render, SculptureView_Commands},
    { "Animation Timeline", 18, AnimationTimeline_Init, AnimationTimeline_Cleanup, AnimationTimeline_Render, AnimationTimeline_Commands },
    { "DMX View", 8, DMXView_Init, DMXView_Cleanup, DMXView_Render, 0 },
    { "Profiler", 8, ProfilerView_Init, ProfilerView_Cleanup, ProfilerView_Render, 0 },
    { "Hierarchy", 9, HierarchyView_Init, HierarchyView_Cleanup, HierarchyView_Render, 0 },
    { "Node Graph", 10, NodeGraph_Init, NodeGraph_Cleanup, NodeGraph_Render, NodeGraph_Commands },
};