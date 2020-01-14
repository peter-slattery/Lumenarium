//
// File: foldhaus_panel_hierarchy.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_PANEL_HIERARCHY_H

PANEL_INIT_PROC(HierarchyView_Init)
{
    
}

PANEL_CLEANUP_PROC(HierarchyView_Cleanup)
{
    
}

PANEL_RENDER_PROC(HierarchyView_Render)
{
    r32 PanelWidth = PanelBounds.Max.x - PanelBounds.Min.x;
    r32 PanelHeight = PanelBounds.Max.y - PanelBounds.Min.y;
    
    v4 LineBGColors[] = {
        { .16f, .16f, .16f, 1.f },
        { .18f, .18f, .18f, 1.f },
    };
    
    interface_list List = {};
    List.LineBGColors = LineBGColors;
    List.LineBGColorsCount = sizeof(LineBGColors) / sizeof(LineBGColors[0]);
    List.LineBGHoverColor = v4{ .22f, .22f, .22f, 1.f };
    List.ListBounds = PanelBounds;
    List.ListElementDimensions = v2{
        Width(PanelBounds), 
        (r32)(State->Interface.Font->PixelHeight + 8),
    };
    
    v2 TextOffset = v2{10, 4};
    string TempString = MakeString(PushArray(&State->Transient, char, 256), 0, 256);
    
    u32 LineCount = (u32)(PanelHeight / List.ListElementDimensions.y) + 1;
    for (u32 i = 0; i < LineCount; i++)
    {
        rect ElementBounds = DrawListElementBackground(&List, Mouse, RenderBuffer);
        
        if (i < State->ActiveAssemblyIndecies.Used)
        {
            gs_list_handle AssemblyHandle = *State->ActiveAssemblyIndecies.GetElementAtIndex(i);
            assembly Assembly = *State->AssemblyList.GetElementWithHandle(AssemblyHandle);
            PrintF(&TempString, "%S", Assembly.Name);
            
            DrawString(RenderBuffer, TempString, State->Interface.Font, ElementBounds.Min + TextOffset, WhiteV4);
            
            PrintF(&TempString, "X");
            
            v2 XLowerRight = v2{ElementBounds.Max.x - 25, ElementBounds.Min.y + TextOffset.y};
            v2 XLowerLeft = DrawString(RenderBuffer, TempString, State->Interface.Font, XLowerRight, WhiteV4, Align_Right);
            
            if (MouseButtonTransitionedUp(Mouse.LeftButtonState) 
                && PointIsInRange(Mouse.Pos, XLowerLeft, ElementBounds.Max))
            {
                UnloadAssembly(AssemblyHandle.Index, State, Context);
            }
        }
        else if (i == State->ActiveAssemblyIndecies.Used)
        {
            PrintF(&TempString, "+ Add Assembly");
            v2 TextMinX = ElementBounds.Min + TextOffset;
            DrawString(RenderBuffer, TempString, State->Interface.Font, TextMinX, WhiteV4);
            
            if (MouseButtonTransitionedUp(Mouse.LeftButtonState)
                && PointIsInRange(Mouse.Pos, ElementBounds.Min, ElementBounds.Max))
            {
                char FilePath[256];
                b32 Success = Context.PlatformGetFilePath(FilePath, 256, "Foldhaus Files\0*.fold\0\0");
                if (Success)
                {
                    LoadAssembly(State, Context, FilePath);
                }
            }
        }
        
        List.ListElementsCount++;
    }
}


#define FOLDHAUS_PANEL_HIERARCHY_H
#endif // FOLDHAUS_PANEL_HIERARCHY_H
