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
    string TempString = PushString(&State->Transient, 256);
    u32 LineCount = (u32)(gs_Height(PanelBounds) / Layout.RowHeight) + 1;
    u32 AssembliesToDraw = GSMin(LineCount, State->Assemblies.Count);
    rect* LineBounds = PushArray(&State->Transient, rect, LineCount);
    
    // Fill in alternating color rows for the backgrounds
    for (u32 Line = 0; Line < LineCount; Line++)
    {
        LineBounds[Line] = ui_ReserveElementBounds(&Layout);
        v4 ListItemBGColor = ui_GetListItemBGColor(State->Interface_.Style, Line);
        ui_FillRect(&State->Interface_, LineBounds[Line], ListItemBGColor);
    }
    
    for (u32 AssemblyIndex = 0; AssemblyIndex < AssembliesToDraw; AssemblyIndex++)
    {
        assembly Assembly = State->Assemblies.Values[AssemblyIndex];
        PrintF(&TempString, "%S", Assembly.Name);
        
        ui_layout ItemLayout = ui_CreateLayout(State->Interface_, LineBounds[AssemblyIndex]);
        ui_StartRow(&ItemLayout, 2);
        {
            ui_LayoutDrawString(&State->Interface_, &ItemLayout, TempString, State->Interface_.Style.TextColor);
            if (ui_LayoutListButton(&State->Interface_, &ItemLayout, MakeStringLiteral("X"), AssemblyIndex))
            {
                UnloadAssembly(AssemblyIndex, State, Context);
            }
        }
        ui_EndRow(&ItemLayout);
    }
    
    if (AssembliesToDraw < LineCount)
    {
        PrintF(&TempString, "+ Add Assembly");
        if (ui_ListButton(&State->Interface_, TempString, LineBounds[AssembliesToDraw], AssembliesToDraw))
        {
            string FilePath = PushString(&State->Transient, 256);
            b32 Success = GetFilePath(Context, &FilePath, "Foldhaus Files\0*.fold\0\0");
            if (Success)
            {
                LoadAssembly(&State->Assemblies, &State->LedSystem, &State->Transient, Context, FilePath, State->GlobalLog);
            }
        }
    }
}


#define FOLDHAUS_PANEL_HIERARCHY_H
#endif // FOLDHAUS_PANEL_HIERARCHY_H
