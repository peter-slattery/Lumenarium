//
// File: foldhaus_panel_hierarchy.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_PANEL_HIERARCHY_H

input_command HierarchyView_Commands[] = {{}};
s32 HierarchyView_CommandsCount = 0;

GSMetaTag(panel_init);
GSMetaTag(panel_type_hierarchy);
internal void
HierarchyView_Init(panel* Panel, app_state* State, context Context)
{
    
}

GSMetaTag(panel_cleanup);
GSMetaTag(panel_type_hierarchy);
internal void
HierarchyView_Cleanup(panel* Panel, app_state* State)
{
    
}

PANEL_MODAL_OVERRIDE_CALLBACK(LoadAssemblyCallback)
{
    Assert(ReturningFrom->TypeIndex == PanelType_FileView);
    file_view_state* FileViewState = Panel_GetStateStruct(ReturningFrom, file_view_state);
    gs_file_info FileInfo = FileViewState->SelectedFile;
    
    LoadAssembly(&State->Assemblies, &State->LedSystem, State->Transient, Context, FileInfo.Path, GlobalLogBuffer);
}

GSMetaTag(panel_render);
GSMetaTag(panel_type_hierarchy);
internal void
HierarchyView_Render(panel* Panel, rect2 PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    gs_string TempString = PushString(State->Transient, 256);
    
    ui_PushLayout(&State->Interface, PanelBounds, LayoutDirection_TopDown, MakeString("Hierarchy Layout"));
    ui_BeginList(&State->Interface, MakeString("Hierarchy List"), 10, State->Assemblies.Count + 1);
    {
        ui_column_spec Cols[2] = {
            ui_column_spec{ UIColumnSize_Fill, 0 },
            ui_column_spec{ UIColumnSize_MaxWidth, 128 }
        };
        for (u32 i = 0; i < State->Assemblies.Count; i++)
        {
            ui_BeginRow(&State->Interface, 2, &Cols[0]);
            
            assembly Assembly = State->Assemblies.Values[i];
            PrintF(&TempString, "%S", Assembly.Name);
            
            ui_Label(&State->Interface, TempString);
            if (ui_Button(&State->Interface, MakeString("X")))
            {
                UnloadAssembly(i, State, Context);
            }
            
            ui_EndRow(&State->Interface);
        }
        
        
        ui_BeginRow(&State->Interface, 2, &Cols[0]);
        ui_Label(&State->Interface, MakeString(" "));
        if (ui_Button(&State->Interface, MakeString("+ Add Assembly")))
        {
            panel* FileBrowser = PanelSystem_PushPanel(&State->PanelSystem, PanelType_FileView, State, Context);
            FileView_SetMode(FileBrowser, FileViewMode_Load);
            Panel_PushModalOverride(Panel, FileBrowser, LoadAssemblyCallback);
        }
        ui_EndRow(&State->Interface);
    }
    ui_EndList(&State->Interface);
    ui_PopLayout(&State->Interface, MakeString("Hierarchy Layout"));
}


#define FOLDHAUS_PANEL_HIERARCHY_H
#endif // FOLDHAUS_PANEL_HIERARCHY_H
