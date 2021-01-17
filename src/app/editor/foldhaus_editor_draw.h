//
// File: foldhaus_editor_draw.h
// Author: Peter Slattery
// Creation Date: 2021-01-16
//
#ifndef FOLDHAUS_EDITOR_DRAW_H

internal void
Editor_DrawWidgetString(app_state* State, context* Context, render_command_buffer* RenderBuffer, ui_widget Widget, rect2 ClippingBox, v4 Color)
{
    gs_string Temp = PushString(State->Transient, 256);
    PrintF(&Temp, "%d", Widget.Id.Id);
    render_quad_batch_constructor BatchConstructor = PushRenderTexture2DBatch(RenderBuffer,
                                                                              Widget.String.Length,
                                                                              State->Interface.Style.Font->BitmapMemory,
                                                                              State->Interface.Style.Font->BitmapTextureHandle,
                                                                              State->Interface.Style.Font->BitmapWidth,
                                                                              State->Interface.Style.Font->BitmapHeight,
                                                                              State->Interface.Style.Font->BitmapBytesPerPixel,
                                                                              State->Interface.Style.Font->BitmapStride);
    
    v2 RegisterPosition = Widget.Bounds.Min + State->Interface.Style.Margin;
    
    switch (Widget.Alignment)
    {
        case Align_Left:
        {
            RegisterPosition = DrawStringLeftAligned(&BatchConstructor, StringExpand(Widget.String), RegisterPosition, State->Interface.Style.Font, ClippingBox, Color);
        }break;
        
        case Align_Right:
        {
            RegisterPosition = DrawStringRightAligned(&BatchConstructor, StringExpand(Widget.String), RegisterPosition, State->Interface.Style.Font, ClippingBox, Color);
        }break;
        
        InvalidDefaultCase;
    }
}

enum widget_fill_dir
{
    WidgetFill_Horizontal = 0,
    WidgetFill_Vertical = 1,
};

internal rect2
Editor_GetWidgetFillBounds(ui_widget Widget)
{
    Assert(ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawHorizontalFill) || ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawVerticalFill));
    
    rect2 Result = {};
    
    widget_fill_dir Dir = WidgetFill_Horizontal;
    if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawHorizontalFill)) { Dir = WidgetFill_Vertical; }
    widget_fill_dir OtherDir = (widget_fill_dir)(WidgetFill_Vertical - Dir);
    
    Result.Min.E[Dir] = Widget.Bounds.Min.E[Dir];
    Result.Max.E[Dir] = Widget.Bounds.Max.E[Dir];
    r32 FillToPoint = LerpR32(Widget.FillPercent, Widget.Bounds.Min.E[OtherDir], Widget.Bounds.Max.E[OtherDir]);
    if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawFillReversed))
    {
        Result.Min.E[OtherDir] = FillToPoint;
        Result.Max.E[OtherDir] = Widget.Bounds.Max.E[OtherDir];
    }
    else if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawFillAsHandle))
    {
        Result.Min.E[OtherDir] = FillToPoint - 5;
        Result.Max.E[OtherDir] = FillToPoint + 5;
    }
    else
    {
        Result.Min.E[OtherDir] = Widget.Bounds.Min.E[OtherDir];
        Result.Max.E[OtherDir] = FillToPoint;
    }
    
    return Result;
}

internal void
Editor_DrawWidget(app_state* State, context* Context, render_command_buffer* RenderBuffer, ui_widget Widget, rect2 ParentClipBounds)
{
    rect2 WidgetParentUnion = Widget.Bounds;
    WidgetParentUnion = Rect2Union(Widget.Bounds, ParentClipBounds);
    
    if (!Widget.Parent || (Rect2Area(WidgetParentUnion) > 0))
    {
        if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawBackground))
        {
            v4 Color = State->Interface.Style.ButtonColor_Inactive;
            if (ui_WidgetIdsEqual(Widget.Id, State->Interface.HotWidget))
            {
                Color = State->Interface.Style.ButtonColor_Active;
            }
            if (ui_WidgetIdsEqual(Widget.Id, State->Interface.ActiveWidget))
            {
                Color = State->Interface.Style.ButtonColor_Selected;
            }
            PushRenderQuad2DClipped(RenderBuffer, Widget.Bounds, WidgetParentUnion, Color);
        }
        
        if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawString) && Widget.String.Length > 0)
        {
            v4 Color = State->Interface.Style.TextColor;
            Editor_DrawWidgetString(State, Context, RenderBuffer, Widget, WidgetParentUnion, Color);
        }
        
        if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawHorizontalFill) ||
            ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawVerticalFill))
        {
            v4 Color = State->Interface.Style.ButtonColor_Selected;
            if (ui_WidgetIdsEqual(Widget.Id, State->Interface.HotWidget) ||
                ui_WidgetIdsEqual(Widget.Id, State->Interface.ActiveWidget))
            {
                Color = WhiteV4;
            }
            
            rect2 FillBounds = Editor_GetWidgetFillBounds(Widget);
            rect2 ClippedFillBounds = Rect2Union(FillBounds, WidgetParentUnion);
            PushRenderQuad2D(RenderBuffer, ClippedFillBounds, Color);
            
            if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawString) && Widget.String.Length > 0)
            {
                // TODO(pjs): add this color to the style
                v4 TextColor = BlackV4;
                Editor_DrawWidgetString(State, Context, RenderBuffer, Widget, ClippedFillBounds, TextColor);
            }
        }
        
        if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawOutline))
        {
            // TODO(pjs): replace these with values from the style
            r32 Thickness = 1.0f;
            v4 Color = WhiteV4;
            PushRenderBoundingBox2D(RenderBuffer, WidgetParentUnion.Min, WidgetParentUnion.Max, Thickness, Color);
        }
    }
    
    if (Widget.ChildrenRoot)
    {
        Editor_DrawWidget(State, Context, RenderBuffer, *Widget.ChildrenRoot, WidgetParentUnion);
    }
    
    if (Widget.Next)
    {
        Editor_DrawWidget(State, Context, RenderBuffer, *Widget.Next, ParentClipBounds);
    }
}


#define FOLDHAUS_EDITOR_DRAW_H
#endif // FOLDHAUS_EDITOR_DRAW_H