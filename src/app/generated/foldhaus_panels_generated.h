enum panel_type {
    PanelType_FileView,
    PanelType_SculptureView,
    PanelType_AnimationTimeline,
    PanelType_DMXView,
    PanelType_HierarchyView,
    PanelType_NodeGraph,
    PanelType_ProfilerView,
};
global s32 GlobalPanelDefsCount = 6;
global panel_definition GlobalPanelDefs[] = {
    { "File View", 9, FileView_Init, FileView_Cleanup, FileView_Render, FileView_Commands, FileView_CommandsCount },
    { "Sculpture View", 14, SculptureView_Init, SculptureView_Cleanup, SculptureView_Render, SculptureView_Commands, SculptureView_CommandsCount },
    { "Animation Timeline", 18, AnimationTimeline_Init, AnimationTimeline_Cleanup, AnimationTimeline_Render, AnimationTimeline_Commands, AnimationTimeline_CommandsCount },
    { "Dmx View", 8, DMXView_Init, DMXView_Cleanup, DMXView_Render, DMXView_Commands, DMXView_CommandsCount },
    { "Hierarchy", 9, HierarchyView_Init, HierarchyView_Cleanup, HierarchyView_Render, HierarchyView_Commands, HierarchyView_CommandsCount },
    { "Profiler", 8, ProfilerView_Init, ProfilerView_Cleanup, ProfilerView_Render, ProfilerView_Commands, ProfilerView_CommandsCount },
};
