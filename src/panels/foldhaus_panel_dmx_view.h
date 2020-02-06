//
// File: foldhaus_panel_dmx_view.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_PANEL_DMX_VIEW_H

struct universe_view_operation_state
{
    b32 MouseDown;
    v2 DisplayOffset;
    r32 Zoom;
};

GSMetaTag(panel_init);
internal void
DMXView_Init(panel* Panel, app_state* State)
{
    
}

GSMetaTag(panel_cleanup);
internal void
DMXView_Cleanup(panel* Panel, app_state* State)
{
    
}

internal void
DrawSACNUniversePixels (render_command_buffer* RenderBuffer, sacn_universe* ToDraw, 
                        v2 TopLeft, v2 Dimension)
{
    Assert(ToDraw);
    
    s32 PixelsPerRow = 21;
    r32 PixelDim = Dimension.x / PixelsPerRow;
    v2 PixelSize = v2{PixelDim, PixelDim};
    
    v2 PixelRegister = TopLeft;
    v4 DisplayColor = {0, 0, 0, 1};
    
    s32 PixelsToDraw = ToDraw->SizeInSendBuffer - STREAM_HEADER_SIZE;
    render_quad_batch_constructor BatchConstructor = PushRenderQuad2DBatch(RenderBuffer, PixelsToDraw);
    
    u8* ColorCursor = (u8*)ToDraw->StartPositionInSendBuffer + STREAM_HEADER_SIZE;
    s32 PixelsDrawn = 0;
    for (s32 i = 0; i < PixelsToDraw; i++)
    {
        PixelRegister.x = TopLeft.x + (PixelsDrawn % PixelsPerRow) * PixelDim;
        PixelRegister.y = TopLeft.y - (PixelsDrawn / PixelsPerRow) * PixelDim;
        
        r32 Value = *ColorCursor++ / 255.f;
        DisplayColor.r = Value;
        DisplayColor.g = Value;
        DisplayColor.b = Value;
        
        PushQuad2DOnBatch(&BatchConstructor, PixelRegister, PixelRegister + PixelSize, DisplayColor);
        
        ++PixelsDrawn;
    }
}

GSMetaTag(panel_render);
internal void
DMXView_Render(panel Panel, rect PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context, mouse_state Mouse)
{
#if 0
    DEBUG_TRACK_SCOPE(DrawUniverseOutputDisplay);
    
    universe_view_operation_state* OpState = (universe_view_operation_state*)Operation.OpStateMemory;
    
    string TitleBarString = InitializeEmptyString(PushArray(State->Transient, char, 64), 64);
    
    v2 DisplayArea_Dimension = v2{600, 600};
    
    v2 DisplayContents_Offset = OpState->DisplayOffset;
    
    //
    // TODO(Peter): I don't like this. Dragging the Universe view should be an operation mode, just
    // like rotating the 3D view, but modes don't have access to the state of modes above them in the stack
    // (and attempting to cast those states to the appropriate type seems risky)
    //
    // :NeedToPassStateDownModeChain
    //
    if (OpState->MouseDown)
    {
        DisplayContents_Offset += (Mouse.Pos - Mouse.DownPos);
    }
    
    v2 DisplayArea_TopLeft = v2{300, (r32)RenderBuffer->ViewHeight - 50} + DisplayContents_Offset;
    v2 UniverseDisplayDimension = v2{100, 100} * OpState->Zoom;
    v2 Padding = v2{25, 50} * OpState->Zoom;
    
    v2 UniverseDisplayTopLeft = DisplayArea_TopLeft;
    
    sacn_universe_buffer* UniverseList = State->SACN.UniverseBuffer;
    while(UniverseList)
    {
        for (s32 UniverseIdx = 0;
             UniverseIdx < UniverseList->Used;
             UniverseIdx++)
        {
            sacn_universe* Universe = UniverseList->Universes + UniverseIdx;
            DrawSACNUniversePixels(RenderBuffer, Universe, UniverseDisplayTopLeft, UniverseDisplayDimension);
            
            
            if (OpState->Zoom > .5f)
            {
                v2 TitleDisplayStart = UniverseDisplayTopLeft + v2{0, 12};
                PrintF(&TitleBarString, "Universe %d", Universe->Universe);
                DrawString(RenderBuffer, TitleBarString, State->Interface.Font, 
                           TitleDisplayStart, WhiteV4);
            }
            
            UniverseDisplayTopLeft.x += UniverseDisplayDimension.x + Padding.x;
            if (UniverseDisplayTopLeft.x > DisplayArea_TopLeft.x + DisplayArea_Dimension.x)
            {
                UniverseDisplayTopLeft.x = DisplayArea_TopLeft.x;
                UniverseDisplayTopLeft.y -= UniverseDisplayDimension.y + Padding.y;
            }
            
            if (UniverseDisplayTopLeft.y < DisplayArea_TopLeft.y - DisplayArea_Dimension.y)
            {
                break;
            }
            
        }
        UniverseList = UniverseList->Next;
    }
#endif
}


#define FOLDHAUS_PANEL_DMX_VIEW_H
#endif // FOLDHAUS_PANEL_DMX_VIEW_H