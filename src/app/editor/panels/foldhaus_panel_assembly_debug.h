//
// File: foldhaus_panel_assembly_debug.h
// Author: Peter Slattery
// Creation Date: 2021-01-15
//
#ifndef FOLDHAUS_PANEL_ASSEMBLY_DEBUG_H

GSMetaTag(panel_init);
GSMetaTag(panel_type_file_view);
internal void
AssemblyDebug_Init(panel* Panel, app_state* State, context Context)
{
}

GSMetaTag(panel_cleanup);
GSMetaTag(panel_type_file_view);
internal void
AssemblyDebug_Cleanup(panel* Panel, app_state* State)
{
    
}

GSMetaTag(panel_render);
GSMetaTag(panel_type_file_view);
internal void
AssemblyDebug_Render(panel* Panel, rect2 PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    ui_interface* Interface = &State->Interface;
    ui_PushLayout(Interface, PanelBounds, LayoutDirection_TopDown, MakeString("Assembly Debug Layout"));
    
    State->AssemblyDebugState.TargetAssembly = ui_LabeledTextEntryU64(Interface, MakeString("Assembly"), State->AssemblyDebugState.TargetAssembly);
    
    State->AssemblyDebugState.TargetStrip = ui_LabeledTextEntryU64(Interface, MakeString("Strip"), State->AssemblyDebugState.TargetStrip);
    
    
    gs_string OverrideStr = MakeString(OverrideTypeStrings[State->AssemblyDebugState.Override]);
    if (ui_BeginLabeledDropdown(Interface, MakeString("Override"), OverrideStr))
    {
        for (u32 i = 0; i < ADS_Override_Count; i++)
        {
            if (ui_Button(Interface, MakeString(OverrideTypeStrings[i])))
            {
                State->AssemblyDebugState.Override = (override_type)i;
            }
        }
    }
    ui_EndLabeledDropdown(Interface);
}

#define FOLDHAUS_PANEL_ASSEMBLY_DEBUG_H
#endif // FOLDHAUS_PANEL_ASSEMBLY_DEBUG_H