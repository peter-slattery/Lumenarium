PANEL_INIT_PROC(HierarchyView_Init)
{
    
}

PANEL_CLEANUP_PROC(HierarchyView_Cleanup)
{
    
}

PANEL_RENDER_PROC(HierarchyView_Render)
{
    r32 PanelWidth = PanelMax.x - PanelMin.x;
    r32 PanelHeight = PanelMax.y - PanelMin.y;
    
    s32 LineBGColorsCount = 2;
v4 LineBGColors[] = {
        { .16f, .16f, .16f, 1.f },
        { .18f, .18f, .18f, 1.f },
    };
    v4 LineBGHoverColor = { .22f, .22f, .22f, 1.f };
    
    r32 LineHeight = State->Interface.Font->PixelHeight + 8;
    v2 LineMin = v2{PanelMin.x + State->Interface.Margin.x, PanelMax.y - LineHeight};
    v2 LineMax = LineMin + v2{PanelWidth, LineHeight};
    v2 TextOffset = v2{10, 4};
    string TempString = MakeString(PushArray(&State->Transient, char, 256), 256);
    
    s32 LineCount = (s32)(PanelHeight / LineHeight) + 1;
    for (s32 i = 0; i < LineCount; i++)
    {
        v4 Color = LineBGColors[i % LineBGColorsCount];
        if (PointIsInRange(Mouse.Pos, LineMin, LineMax))
        {
            Color = LineBGHoverColor;
        }
PushRenderQuad2D(RenderBuffer, LineMin, LineMax, Color);
        if (i < State->ActiveAssemblyIndecies.Used)
        {
array_entry_handle AssemblyHandle = *GetElementAtIndex(i, State->ActiveAssemblyIndecies);
        assembly Assembly = *GetElementWithHandle(AssemblyHandle, State->AssemblyList);
        PrintF(&TempString, "%S", Assembly.Name);
        
            DrawString(RenderBuffer, TempString, State->Interface.Font, LineMin + TextOffset, WhiteV4);
            
            PrintF(&TempString, "X");
            
            v2 XLowerRight = v2{LineMax.x - 25, LineMin.y + TextOffset.y};
            v2 XLowerLeft = DrawString(RenderBuffer, TempString, State->Interface.Font, XLowerRight, WhiteV4, Align_Right);
            
            if (MouseButtonTransitionedUp(Mouse.LeftButtonState) 
                && PointIsInRange(Mouse.Pos, XLowerLeft, LineMax))
            {
                UnloadAssembly(i, State, Context);
            }
        }
        else if (i == State->ActiveAssemblyIndecies.Used)
        {
            PrintF(&TempString, "+ Add Assembly");
            v2 TextMinX = LineMin + TextOffset;
            DrawString(RenderBuffer, TempString, State->Interface.Font, TextMinX, WhiteV4);
            
            if (MouseButtonTransitionedUp(Mouse.LeftButtonState)
                && PointIsInRange(Mouse.Pos, LineMin, LineMax))
            {
                char FilePath[256];
                b32 Success = Context.PlatformGetFilePath(FilePath, 256, "Foldhaus Files\0*.fold\0\0");
                if (Success)
                {
                    LoadAssembly(State, Context, FilePath);
                }
            }
        }

        LineMin.y = GSMax(PanelMin.y, LineMin.y - LineHeight);
        LineMax.y = GSMax(PanelMin.y, LineMax.y - LineHeight);
    }
}