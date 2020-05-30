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
HierarchyView_Init(panel* Panel, app_state* State)
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
HierarchyView_Render(panel Panel, rect PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context, mouse_state Mouse)
{
    ui_layout Layout = ui_CreateLayout(State->Interface_, PanelBounds);
    v4 ListItemHover = State->Interface_.Style.ListBGHover;
    v4 ListItemSelected = State->Interface_.Style.ListBGSelected;
    
    string TempString = PushString(&State->Transient, 256);
    
    u32 LineCount = (u32)(gs_Height(PanelBounds) / Layout.RowHeight) + 1;
    u32 LinesDrawn = 0;
    u32 AssembliesToDraw = GSMin(LineCount, State->ActiveAssemblyIndecies.Used);
    for (; LinesDrawn < AssembliesToDraw; LinesDrawn++)
    {
        rect Bounds = ui_ReserveElementBounds(&Layout);
        v4 ListItemBGColor = ui_GetListItemBGColor(State->Interface_.Style, LinesDrawn);
        ui_FillRect(&State->Interface_, Bounds, ListItemBGColor);
        gs_list_handle AssemblyHandle = *State->ActiveAssemblyIndecies.GetElementAtIndex(LinesDrawn);
        assembly Assembly = *State->AssemblyList.GetElementWithHandle(AssemblyHandle);
        PrintF(&TempString, "%S", Assembly.Name);
        
        ui_layout ItemLayout = ui_CreateLayout(State->Interface_, Bounds);
        ui_StartRow(&ItemLayout, 2);
        {
            ui_LayoutDrawString(&State->Interface_, &ItemLayout, TempString, State->Interface_.Style.TextColor);
            if (ui_LayoutButton(&State->Interface_, &ItemLayout, MakeStringLiteral("X"), ListItemBGColor, ListItemHover, ListItemSelected))
            {
                UnloadAssembly(AssemblyHandle.Index, State, Context);
            }
        }
        ui_EndRow(&ItemLayout);
    }
    
    if (LinesDrawn < LineCount)
    {
        v4 ListItemBGColor = ui_GetListItemBGColor(State->Interface_.Style, LinesDrawn++);
        PrintF(&TempString, "+ Add Assembly");
        if (ui_LayoutButton(&State->Interface_, &Layout, TempString, ListItemBGColor, ListItemHover, ListItemSelected))
        {
            string FilePath = PushString(&State->Transient, 256);
            b32 Success = GetFilePath(Context, &FilePath, "Foldhaus Files\0*.fold\0\0");
            if (Success)
            {
                LoadAssembly(State, Context, FilePath);
            }
        }
        
        for (; LinesDrawn < LineCount; LinesDrawn++)
        {
            rect Bounds = ui_ReserveElementBounds(&Layout);
            ListItemBGColor = ui_GetListItemBGColor(State->Interface_.Style, LinesDrawn);
            ui_FillRect(&State->Interface_, Bounds, ListItemBGColor);
        }
    }
}


#define FOLDHAUS_PANEL_HIERARCHY_H
#endif // FOLDHAUS_PANEL_HIERARCHY_H
