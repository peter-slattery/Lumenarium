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