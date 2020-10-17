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

GSMetaTag(panel_render);
GSMetaTag(panel_type_hierarchy);
internal void
HierarchyView_Render(panel* Panel, rect2 PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context)
{
    ui_layout Layout = ui_CreateLayout(State->Interface, PanelBounds);
    gs_string TempString = PushString(State->Transient, 256);
    u32 LineCount = (u32)(Rect2Height(PanelBounds) / Layout.RowHeight) + 1;
    u32 AssembliesToDraw = Min(LineCount, State->Assemblies.Count);
    rect2* LineBounds = PushArray(State->Transient, rect2, LineCount);
    
    // Fill in alternating color rows for the backgrounds
    for (u32 Line = 0; Line < LineCount; Line++)
    {
        LineBounds[Line] = ui_ReserveElementBounds(&Layout);
        v4 ListItemBGColor = ui_GetListItemBGColor(State->Interface.Style, Line);
        ui_FillRect(&State->Interface, LineBounds[Line], ListItemBGColor);
    }
    
    for (u32 AssemblyIndex = 0; AssemblyIndex < AssembliesToDraw; AssemblyIndex++)
    {
        assembly Assembly = State->Assemblies.Values[AssemblyIndex];
        PrintF(&TempString, "%S", Assembly.Name);
        
        ui_layout ItemLayout = ui_CreateLayout(State->Interface, LineBounds[AssemblyIndex]);
        ui_StartRow(&ItemLayout, 2);
        {
            ui_LayoutDrawString(&State->Interface, &ItemLayout, TempString, State->Interface.Style.TextColor);
            if (ui_LayoutListButton(&State->Interface, &ItemLayout, MakeString("X"), AssemblyIndex))
            {
                UnloadAssembly(AssemblyIndex, State, Context);
            }
        }
        ui_EndRow(&ItemLayout);
    }
    
    if (AssembliesToDraw < LineCount)
    {
        // NOTE(Peter): Add assembly button
        PrintF(&TempString, "+ Add Assembly");
        if (ui_ListButton(&State->Interface, TempString, LineBounds[AssembliesToDraw], AssembliesToDraw))
        {
            gs_string FilePath = PushString(State->Transient, 256);
            
            // TODO(Peter): Took out file opening temporarily while I get things back up and running.
            // Ideally we can just write our own file lister using the new filehandler so that
            // execution doesn't suspend while we try and open a file
            InvalidCodePath;
#if 0
            b32 Success = GetFilePath(Context, &FilePath, "Foldhaus Files\0*.fold\0\0");
            if (Success)
            {
                LoadAssembly(&State->Assemblies, &State->LedSystem, &State->Transient, Context, FilePath.ConstString, State->GlobalLog);
            }
#endif
        }
    }
}


#define FOLDHAUS_PANEL_HIERARCHY_H
#endif // FOLDHAUS_PANEL_HIERARCHY_H
