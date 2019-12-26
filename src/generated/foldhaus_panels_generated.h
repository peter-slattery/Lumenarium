struct panel_definition
{
    char* PanelName;
    s32 PanelNameLength;
    panel_init_proc* Init;
    panel_cleanup_proc* Cleanup;
    panel_render_proc* Render;
    input_command* InputCommands;
};

global_variable s32 GlobalPanelDefsCount = 5;
global_variable panel_definition GlobalPanelDefs[] = {
    { "Sculpture View", 14, SculptureView_Init, SculptureView_Cleanup, SculptureView_Render, SculptureView_Commands},
    { "Animation Timeline", 18, AnimationTimeline_Init, AnimationTimeline_Cleanup, AnimationTimeline_Render, 0 },
    { "DMX View", 8, DMXView_Init, DMXView_Cleanup, DMXView_Render, 0 },
    { "Profiler", 8, ProfilerView_Init, ProfilerView_Cleanup, ProfilerView_Render, 0 },
    { "Hierarchy", 9, HierarchyView_Init, HierarchyView_Cleanup, HierarchyView_Render, 0 },
};