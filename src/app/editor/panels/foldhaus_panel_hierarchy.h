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
    
    LoadAssembly(&State->Assemblies, &State->LedSystem, State->Transient, Context, FileInfo.Path, State->GlobalLog);
}

GSMetaTag(panel_render);
GSMetaTag(panel_type_hierarchy);
internal void
HierarchyView_Render(panel* Panel, rect2 PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    ui_PushLayout(&State->Interface, PanelBounds, LayoutDirection_TopDown, MakeString("Hierarchy Layout"));
    
    // TODO(pjs): Come back to this after the layout stuff is handled.
    // Ideally it handles the visuals of the hierarchy itself.
    
#if 0
    gs_string TempString = PushString(State->Transient, 256);
    u32 LineCount = (u32)(Rect2Height(PanelBounds) / Layout->RowHeight) + 1;
    u32 AssembliesToDraw = Min(LineCount, State->Assemblies.Count);
    rect2* LineBounds = PushArray(State->Transient, rect2, LineCount);
    
    // Fill in alternating color rows for the backgrounds
    for (u32 Line = 0; Line < LineCount; Line++)
    {
        v4 ListItemBGColor = ui_GetListItemBGColor(State->Interface.Style, Line);
        ui_FillRect(&State->Interface, LineBounds[Line], ListItemBGColor);
    }
    
    for (u32 AssemblyIndex = 0; AssemblyIndex < AssembliesToDraw; AssemblyIndex++)
    {
        assembly Assembly = State->Assemblies.Values[AssemblyIndex];
        PrintF(&TempString, "%S", Assembly.Name);
        
        ui_StartRow(&State->Interface, 2);
        {
            ui_DrawString(&State->Interface, TempString);
            if (ui_LayoutListButton(&State->Interface, MakeString("X"), AssemblyIndex))
            {
                UnloadAssembly(AssemblyIndex, State, Context);
            }
        }
        ui_EndRow(&State->Interface);
    }
    
    if (AssembliesToDraw < LineCount)
    {
        // NOTE(Peter): Add assembly button
        PrintF(&TempString, "+ Add Assembly");
        if (ui_ListButton(&State->Interface, TempString, LineBounds[AssembliesToDraw], AssembliesToDraw))
        {
            panel* FileBrowser = PanelSystem_PushPanel(&State->PanelSystem, PanelType_FileView, State, Context);
            Panel_PushModalOverride(Panel, FileBrowser, LoadAssemblyCallback);
        }
    }
#endif
    
    ui_PopLayout(&State->Interface);
}


#define FOLDHAUS_PANEL_HIERARCHY_H
#endif // FOLDHAUS_PANEL_HIERARCHY_H
