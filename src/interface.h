//
// File: interface.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef INTERFACE_H

enum string_alignment
{
    Align_Left,
    Align_Center,
    Align_Right,
};

internal void
DrawCharacter_ (render_quad_batch_constructor* BatchConstructor, r32 MinX, r32 MinY, codepoint_bitmap CodepointInfo, v4 Color)
{
    s32 AlignedMinX = (s32)(MinX);
    s32 AlignedMinY = (s32)(MinY);
    s32 AlignedMaxX = AlignedMinX + (CodepointInfo.Width);
    s32 AlignedMaxY = AlignedMinY + (CodepointInfo.Height);
    
    PushQuad2DOnBatch(BatchConstructor, 
                      v2{(r32)AlignedMinX, (r32)AlignedMinY}, v2{(r32)AlignedMaxX, (r32)AlignedMinY}, 
                      v2{(r32)AlignedMaxX, (r32)AlignedMaxY}, v2{(r32)AlignedMinX, (r32)AlignedMaxY},  
                      CodepointInfo.UVMin, CodepointInfo.UVMax, 
                      Color);
}

internal v2
DrawCharacterLeftAligned (render_quad_batch_constructor* BatchConstructor, char C, bitmap_font Font, v2 Position, v4 Color)
{
    s32 GlyphDataIndex = GetIndexForCodepoint(Font, C);
    codepoint_bitmap CodepointInfo = Font.CodepointValues[GlyphDataIndex];
    
    // NOTE(Peter): 
    r32 MinX = Position.x + CodepointInfo.XOffset;
    r32 MinY = Position.y + CodepointInfo.YOffset;
    DrawCharacter_(BatchConstructor, MinX, MinY, CodepointInfo, Color);
    
    // NOTE(Peter): 
    v2 PointAfterCharacter = v2{Position.x + CodepointInfo.Width, Position.y};
    return PointAfterCharacter;
}

internal v2
DrawCharacterRightAligned (render_quad_batch_constructor* BatchConstructor, char C, bitmap_font Font, v2 Position, v4 Color)
{
    s32 GlyphDataIndex = GetIndexForCodepoint(Font, C);
    codepoint_bitmap CodepointInfo = Font.CodepointValues[GlyphDataIndex];
    
    // NOTE(Peter): 
    r32 MinX = Position.x - (CodepointInfo.XOffset + CodepointInfo.Width);
    r32 MinY = Position.y + CodepointInfo.YOffset + CodepointInfo.YOffset;
    DrawCharacter_(BatchConstructor, MinX, MinY, CodepointInfo, Color);
    
    // NOTE(Peter): 
    v2 PointAfterCharacter = v2{Position.x-(CodepointInfo.Width + CodepointInfo.XOffset), Position.y};
    return PointAfterCharacter;
}

internal v2
DrawStringLeftAligned (render_quad_batch_constructor* BatchConstructor, s32 Length, char* String, v2 InitialRegisterPosition, bitmap_font* Font, v4 Color)
{
    v2 RegisterPosition = InitialRegisterPosition;
    char* C = String;
    for (s32 i = 0; i < Length; i++)
    {
        v2 PositionAfterCharacter = DrawCharacterLeftAligned(BatchConstructor, *C, *Font, RegisterPosition, Color);
        RegisterPosition.x = PositionAfterCharacter.x;
        C++;
    }
    return RegisterPosition;
}

internal v2
DrawStringRightAligned (render_quad_batch_constructor* BatchConstructor, s32 Length, char* String, v2 InitialRegisterPosition, bitmap_font* Font, v4 Color)
{
    v2 RegisterPosition = InitialRegisterPosition;
    char* C = String + Length - 1;
    for (s32 i = Length - 1; i >= 0; i--)
    {
        v2 PositionAfterCharacter = DrawCharacterRightAligned(BatchConstructor, *C, *Font, RegisterPosition, Color);
        RegisterPosition.x = PositionAfterCharacter.x;
        C--;
    }
    return RegisterPosition;
}

internal v2
DrawString (render_command_buffer* RenderBuffer, string String, bitmap_font* Font, v2 Position, v4 Color, string_alignment Alignment = Align_Left)
{
    DEBUG_TRACK_FUNCTION;
    v2 LowerRight = Position;
    
    render_quad_batch_constructor BatchConstructor = PushRenderTexture2DBatch(RenderBuffer, String.Length,
                                                                              Font->BitmapMemory,
                                                                              Font->BitmapTextureHandle,
                                                                              Font->BitmapWidth,
                                                                              Font->BitmapHeight,
                                                                              Font->BitmapBytesPerPixel,
                                                                              Font->BitmapStride);
    
    v2 RegisterPosition = Position;
    if (Alignment == Align_Left)
    {
        RegisterPosition = DrawStringLeftAligned(&BatchConstructor, StringExpand(String), RegisterPosition, Font, Color);
    }
    else if (Alignment == Align_Right)
    {
        RegisterPosition = DrawStringRightAligned(&BatchConstructor, StringExpand(String), RegisterPosition, Font, Color);
    }
    else
    {
        InvalidCodePath;
    }
    
    LowerRight.x = RegisterPosition.x;
    
    return LowerRight;
}

internal void
DrawCursor (render_quad_batch_constructor* BatchConstructor, v2 Position, v4 Color, bitmap_font Font)
{
    v2 Min = Position;
    v2 Max = Position + v2{(r32)Font.MaxCharWidth, (r32)(Font.Ascent + Font.Descent)};
    PushQuad2DOnBatch(BatchConstructor, Min, Max, Color);
}

internal v2
DrawStringWithCursor (render_command_buffer* RenderBuffer, string String, s32 CursorPosition, bitmap_font* Font, v2 Position, v4 Color, v4 CursorColor, string_alignment Alignment = Align_Left)
{
    DEBUG_TRACK_FUNCTION;
    v2 LowerRight = Position;
    
    // NOTE(Peter): We push this on first so that the cursor will be drawn underneath any character it may overlap with
    render_quad_batch_constructor CursorBatch = PushRenderQuad2DBatch(RenderBuffer, 1);
    render_quad_batch_constructor BatchConstructor = PushRenderTexture2DBatch(RenderBuffer, String.Length,
                                                                              Font->BitmapMemory,
                                                                              Font->BitmapTextureHandle,
                                                                              Font->BitmapWidth,
                                                                              Font->BitmapHeight,
                                                                              Font->BitmapBytesPerPixel,
                                                                              Font->BitmapStride);
    
    v2 RegisterPosition = Position;
    if (Alignment == Align_Left)
    {
        RegisterPosition = DrawStringLeftAligned(&BatchConstructor, StringExpand(String), RegisterPosition, Font, Color);
        DrawCursor(&CursorBatch, RegisterPosition, GreenV4, *Font);
        if (String.Length - CursorPosition > 0)
        {
            RegisterPosition = DrawStringLeftAligned(&BatchConstructor,
                                                     String.Length - CursorPosition,
                                                     String.Memory + CursorPosition, 
                                                     RegisterPosition, Font, Color);
        }
    }
    else if (Alignment == Align_Right)
    {
        RegisterPosition = DrawStringRightAligned(&BatchConstructor,
                                                  CursorPosition, String.Memory,
                                                  RegisterPosition, Font, Color);
        DrawCursor(&CursorBatch, RegisterPosition, GreenV4, *Font);
        if (String.Length - CursorPosition > 0)
        {
            RegisterPosition = DrawStringRightAligned(&BatchConstructor,
                                                      String.Length - CursorPosition,
                                                      String.Memory + CursorPosition, 
                                                      RegisterPosition, Font, Color);
        }
    }
    else
    {
        InvalidCodePath;
    }
    
    LowerRight.x = RegisterPosition.x;
    return LowerRight;
}

struct interface_config
{
    v4 PanelBGColors[4];
    
    v4 ButtonColor_Inactive, ButtonColor_Active, ButtonColor_Selected;
    
    v4 TextColor;
    
#define LIST_BG_COLORS_COUNT 2
    v4 ListBGColors[LIST_BG_COLORS_COUNT];
    v4 ListBGHover;
    v4 ListBGSelected;
    
    bitmap_font* Font;
    r32 FontSize;
    v2 Margin;
    r32 RowHeight;
};

struct ui_layout
{
    rect Bounds;
    v2 Margin;
    r32 RowHeight;
    r32 RowYAt;
    
    b32 DrawHorizontal;
    u32 RowDivisions;
    u32 RowElementsCount;
};

struct ui_interface
{
    interface_config Style;
    mouse_state Mouse;
    render_command_buffer* RenderBuffer;
};

static ui_layout
ui_CreateLayout(ui_interface Interface, rect Bounds)
{
    ui_layout Result = {0};
    Result.Bounds = Bounds;
    Result.Margin = Interface.Style.Margin;
    Result.RowHeight = Interface.Style.RowHeight;
    Result.RowYAt = Bounds.Max.y - Result.RowHeight;
    return Result;
}

static void
ui_StartRow(ui_layout* Layout, u32 RowDivisions)
{
    Layout->DrawHorizontal = true;
    Layout->RowDivisions = RowDivisions;
    Layout->RowElementsCount = 0;
}

static void
ui_StartRow(ui_layout* Layout)
{
    ui_StartRow(Layout, 0);
}

static void
ui_EndRow(ui_layout* Layout)
{
    Layout->DrawHorizontal = false;
}

static b32
ui_TryReserveElementBounds(ui_layout* Layout, rect* Bounds)
{
    b32 Result = true;
    if (!Layout->DrawHorizontal)
    {
        Bounds->Min = { Layout->Bounds.Min.x, Layout->RowYAt };
        Bounds->Max = { Layout->Bounds.Max.x, Bounds->Min.y + Layout->RowHeight };
        Layout->RowYAt -= Layout->RowHeight;
    }
    else
    {
        if (Layout->RowDivisions > 0)
        {
            Assert(Layout->RowElementsCount < Layout->RowDivisions);
            r32 ElementWidth = Width(Layout->Bounds) / Layout->RowDivisions;
            Bounds->Min = { 
                Layout->Bounds.Min.x + (ElementWidth * Layout->RowElementsCount) + Layout->Margin.x,
                Layout->RowYAt
            };
            Bounds->Max = { 
                Bounds->Min.x + ElementWidth - Layout->Margin.x, 
                Bounds->Min.y + Layout->RowHeight
            };
            Layout->RowElementsCount++;
        }
        else
        {
            Result = false;
        }
    }
    return Result;
}

static rect
ui_ReserveTextLineBounds(ui_interface Interface, string Text, ui_layout* Layout)
{
    rect Bounds = {0};
    
    return Bounds;
}

//
// Drawing Functions
//

static void
ui_FillRect(ui_interface* Interface, rect Bounds, v4 Color)
{
    PushRenderQuad2D(Interface->RenderBuffer, RectExpand(Bounds), Color);
}

static void
ui_OutlineRect(ui_interface* Interface, rect Bounds, r32 Thickness, v4 Color)
{
    PushRenderBoundingBox2D(Interface->RenderBuffer, Bounds.Min, Bounds.Max, Thickness, Color);
}

internal void
ui_DrawString(ui_interface* Interface, string String, rect Bounds, v4 Color, string_alignment Alignment = Align_Left)
{
    DEBUG_TRACK_FUNCTION;
    render_quad_batch_constructor BatchConstructor = PushRenderTexture2DBatch(Interface->RenderBuffer, 
                                                                              String.Length,
                                                                              Interface->Style.Font->BitmapMemory,
                                                                              Interface->Style.Font->BitmapTextureHandle,
                                                                              Interface->Style.Font->BitmapWidth,
                                                                              Interface->Style.Font->BitmapHeight,
                                                                              Interface->Style.Font->BitmapBytesPerPixel,
                                                                              Interface->Style.Font->BitmapStride);
    
    v2 RegisterPosition = Bounds.Min + Interface->Style.Margin;
    if (Alignment == Align_Left)
    {
        RegisterPosition = DrawStringLeftAligned(&BatchConstructor, StringExpand(String), RegisterPosition, Interface->Style.Font, Color);
    }
    else if (Alignment == Align_Right)
    {
        RegisterPosition = DrawStringRightAligned(&BatchConstructor, StringExpand(String), RegisterPosition, Interface->Style.Font, Color);
    }
    else
    {
        InvalidCodePath;
    }
}

static void
ui_TextBox(ui_interface* Interface, rect Bounds, string Text, v4 BGColor, v4 TextColor)
{
    ui_FillRect(Interface, Bounds, BGColor);
    ui_DrawString(Interface, Text, Bounds, TextColor);
}

static b32
ui_Button(ui_interface* Interface, string Text, rect Bounds, v4 InactiveColor, v4 HoverColor, v4 ClickedColor)
{
    b32 Pressed = false;
    v4 ButtonBG = InactiveColor;
    if (PointIsInRect(Interface->Mouse.Pos, Bounds))
    {
        ButtonBG = HoverColor;
        if (MouseButtonTransitionedDown(Interface->Mouse.LeftButtonState))
        {
            ButtonBG = ClickedColor;
            Pressed = true;
        }
    }
    ui_TextBox(Interface, Bounds, Text, ButtonBG, Interface->Style.TextColor);
    return Pressed;
}

static b32
ui_Button(ui_interface* Interface, string Text, rect Bounds)
{
    v4 BGColor = Interface->Style.ButtonColor_Inactive;
    v4 HoverColor = Interface->Style.ButtonColor_Active;
    v4 SelectedColor = Interface->Style.ButtonColor_Selected;
    return ui_Button(Interface, Text, Bounds, BGColor, HoverColor, SelectedColor);
}

static b32
ui_LayoutButton(ui_interface* Interface, string Text, ui_layout* Layout)
{
    rect ButtonBounds = {0};
    if (!ui_TryReserveElementBounds(Layout, &ButtonBounds))
    {
        ButtonBounds = ui_ReserveTextLineBounds(*Interface, Text, Layout);
    }
    
    v4 BGColor = Interface->Style.ButtonColor_Inactive;
    v4 HoverColor = Interface->Style.ButtonColor_Active;
    v4 SelectedColor = Interface->Style.ButtonColor_Selected;
    return ui_Button(Interface, Text, ButtonBounds, BGColor, HoverColor, SelectedColor);
}

static b32
ui_LayoutListEntry(ui_interface* Interface, ui_layout* Layout, string Text, u32 Index)
{
    rect Bounds = {0};
    if (!ui_TryReserveElementBounds(Layout, &Bounds))
    {
        // TODO(Peter): this isn't really invalid, but I don't have a concrete use case
        // for it yet. This should only fire if the Layout component is drawing a row,
        // but if you're in row mode during a list, what should happen?
        // Punting this till I have a use case
        InvalidCodePath;
    }
    v4 BGColor = Interface->Style.ListBGColors[Index % LIST_BG_COLORS_COUNT];
    v4 HoverColor = Interface->Style.ListBGHover;
    v4 SelectedColor = Interface->Style.ListBGSelected;
    return ui_Button(Interface, Text, Bounds, BGColor, HoverColor, SelectedColor);
}

//
// OLD
//

struct slider_result
{
    r32 Percent;
    r32 Advance;
};

internal slider_result
EvaluateSlider (render_command_buffer* RenderBuffer, v2 Min, v2 Max, string Label, r32 Percent, interface_config Config, mouse_state Mouse)
{
    slider_result Result = {};
    
    v4 BGColor = Config.ButtonColor_Inactive;
    v4 FillColor = Config.ButtonColor_Selected;
    
    r32 DisplayPercent = Percent;
    
    if (PointIsInRange(Mouse.Pos, Min, Max))
    {
        BGColor = Config.ButtonColor_Active;
    }
    
    if (MouseButtonTransitionedDown(Mouse.LeftButtonState))
    {
        if (PointIsInRange(Mouse.DownPos, Min, Max))
        {
            r32 TempFillPercent = (Mouse.Pos.y - Min.y) / (Max.y - Min.y);
            
            DisplayPercent = GSClamp(0.0f, TempFillPercent, 1.0f);
        }
    }
    
    r32 FillHeight = ((Max.y - Min.y) - 4) * DisplayPercent;
    
    PushRenderQuad2D(RenderBuffer, Min, Max, BGColor);
    PushRenderQuad2D(RenderBuffer, Min + v2{2, 2}, v2{Max.x - 2, Min.y + 2 + FillHeight}, FillColor);
    
    // TODO(Peter): display the actual value of the slider
    
    DrawString(RenderBuffer, Label, Config.Font, Min, Config.TextColor);
    
    Result.Percent = DisplayPercent;
    Result.Advance = (Max.y - Min.y) + Config.Margin.y; 
    
    return Result;
}

enum selection_state
{
    Selection_None,
    Selection_Selected,
    Selection_Deselected,
};

struct interface_list
{
    rect ListBounds;
    
    v2 ListElementDimensions;
    v2 ElementLabelIndent;
    
    v4 TextColor;
    v4* LineBGColors;
    s32 LineBGColorsCount;
    v4 LineBGHoverColor;
    
    s32 ListElementsCount;
};

internal rect
DrawListElementBackground(interface_list* List, mouse_state Mouse, render_command_buffer* RenderBuffer)
{
    rect LineBounds = {};
    LineBounds.Min = v2{
        List->ListBounds.Min.x,
        List->ListBounds.Max.y - (List->ListElementDimensions.y * (List->ListElementsCount + 1))
    };
    LineBounds.Max = LineBounds.Min + List->ListElementDimensions;
    
    v4 Color = List->LineBGColors[List->ListElementsCount % List->LineBGColorsCount];
    if (PointIsInRange(Mouse.Pos, LineBounds.Min, LineBounds.Max))
    {
        Color = List->LineBGHoverColor;
    }
    
    PushRenderQuad2D(RenderBuffer, LineBounds.Min, LineBounds.Max, Color);
    return LineBounds;
}

internal rect
DrawListElement(string Label, interface_list* List, mouse_state Mouse, render_command_buffer* RenderBuffer, interface_config Interface)
{
    rect Bounds = DrawListElementBackground(List, Mouse, RenderBuffer);
    
    v2 LabelPosition = Bounds.Min + List->ElementLabelIndent;
    DrawString(RenderBuffer, Label, Interface.Font, LabelPosition, List->TextColor);
    
    List->ListElementsCount++;
    return Bounds;
}


internal r32
EvaluateColorChannelSlider (render_command_buffer* RenderBuffer, v4 ChannelMask, v2 Min, v2 Max, r32 Current, mouse_state Mouse)
{
    r32 Result = Current;
    
    render_quad_batch_constructor Batch = PushRenderQuad2DBatch(RenderBuffer, 2);
    
    v4 LeftColor = ChannelMask * 0;
    LeftColor.a = 1.f;
    v4 RightColor = ChannelMask;
    PushQuad2DOnBatch(&Batch, 
                      Min, v2{Max.x, Min.y}, Max, v2{Min.x, Max.y},
                      v2{0, 0}, v2{1, 0}, v2{1, 1}, v2{0, 1},
                      LeftColor, RightColor, RightColor, LeftColor);
    
    if (MouseButtonTransitionedDown(Mouse.LeftButtonState))
    {
        if (PointIsInRange(Mouse.DownPos, Min, Max))
        {
            Result = ((r32)Mouse.Pos.x - Min.x) / (Max.x - Min.x);
            Result = GSClamp01(Result);
        }
    }
    
    r32 DragBarWidth = 8;
    v2 DragBarMin = v2{GSLerp(Min.x, Max.x, Result) - (DragBarWidth / 2), Min.y - 2};
    v2 DragBarMax = DragBarMin + v2{DragBarWidth, (Max.y - Min.y) + 4};
    
    PushQuad2DOnBatch(&Batch, DragBarMin, DragBarMax, v4{.3f, .3f, .3f, 1.f});
    
    return Result;
}

internal b32
EvaluateColorPicker (render_command_buffer* RenderBuffer, v4* Value, v2 PanelMin, interface_config Config, mouse_state Mouse)
{
    b32 ShouldClose = false;
    
    v2 PanelMax = v2{400, 500};
    if (MouseButtonTransitionedDown(Mouse.LeftButtonState) && !PointIsInRange(Mouse.Pos, PanelMin, PanelMax))
    {
        ShouldClose = true;
    }
    else
    {
        PushRenderQuad2D(RenderBuffer, PanelMin, PanelMax, v4{.5f, .5f, .5f, 1.f});
        
        v2 TitleMin = v2{PanelMin.x + 5, PanelMax.y - (Config.Font->PixelHeight + 5)};
        DrawString(RenderBuffer, MakeStringLiteral("Color Picker"), Config.Font, 
                   TitleMin, WhiteV4);
        
        v2 SliderDim = v2{(PanelMax.x - PanelMin.x) - 20, 32};
        // channel sliders
        v2 SliderMin = TitleMin - v2{0, SliderDim.y + 10};
        Value->r = EvaluateColorChannelSlider(RenderBuffer, RedV4, SliderMin, SliderMin + SliderDim, Value->r, Mouse);
        SliderMin.y -= SliderDim.y + 10;
        Value->g = EvaluateColorChannelSlider(RenderBuffer, GreenV4, SliderMin, SliderMin + SliderDim, Value->g, Mouse);
        SliderMin.y -= SliderDim.y + 10;
        Value->b = EvaluateColorChannelSlider(RenderBuffer, BlueV4, SliderMin, SliderMin + SliderDim, Value->b, Mouse);
        SliderMin.y -= SliderDim.y + 10;
        Value->a = EvaluateColorChannelSlider(RenderBuffer, WhiteV4, SliderMin, SliderMin + SliderDim, Value->a, Mouse);
        
        // Output Color Display
        SliderMin.y -= 100;
        PushRenderQuad2D(RenderBuffer, SliderMin, SliderMin + v2{75, 75}, *Value);
    }
    
    return ShouldClose;
}

struct search_lister_result
{
    s32 HotItem;
    s32 SelectedItem;
    b32 ShouldRemainOpen;
};

typedef string search_lister_get_list_item_at_offset(u8* ListMemory, s32 ListLength, string SearchString, s32 Offset);

internal search_lister_result
EvaluateSearchLister (ui_interface* Interface, v2 TopLeft, v2 Dimension, string Title, 
                      string* ItemList, s32* ListLUT, s32 ListLength,
                      s32 HotItem,
                      string* SearchString, s32 SearchStringCursorPosition)
{
    search_lister_result Result = {};
    Result.ShouldRemainOpen = true;
    Result.HotItem = HotItem;
    
    // TODO(Peter): Was tired. Nothing wrong with the code below
    InvalidCodePath;
#if 0
    // Title Bar
    rect TitleBarBounds = rect{v2{TopLeft.x, TopLeft.y - 30}, v2{TopLeft.x + 300, TopLeft.y}};
    ui_FillRect(Interface, TitleBarBounds, v4{.3f, .3f, .3f, 1.f});
    ui_DrawString(Interface, Title, TitleBarBounds, Interface->Style.TextColor);
    
    MakeStringBuffer(DebugString, 256);
    PrintF(&DebugString, "Hot Item: %d  |  Filtered Items: %d", HotItem, ListLength);
    rect DebugBounds = MakeRectMinWidth(v2{ TopLeft.x + 256, TopLeft.y - 25}, v2{256, Interface->Style.LineHeight});
    ui_DrawString(Interface, DebugString, DebugBounds, Interface->Style.TextColor);
    
    // Search Bar
    PushRenderQuad2D(RenderBuffer, v2{TopLeft.x, TopLeft.y - 30}, v2{TopLeft.x + 300, TopLeft.y}, v4{.3f, .3f, .3f, 1.f});
    DrawStringWithCursor(RenderBuffer, *SearchString, SearchStringCursorPosition, Font, v2{TopLeft.x, TopLeft.y - 25}, WhiteV4, GreenV4);
    TopLeft.y -= 30;
    
    for (s32 i = 0; i < ListLength; i++)
    {
        s32 FilteredIndex = ListLUT[i];
        string ListItemString = ItemList[FilteredIndex];
        
        v2 Min = v2{TopLeft.x, TopLeft.y - 30};
        v2 Max = Min + Dimension - v2{0, Config.Margin.y};
        
        v4 ButtonColor = Config.ButtonColor_Inactive;
        if (i == HotItem)
        {
            ButtonColor = Config.ButtonColor_Active;
        }
        
        if (ui_Button(Interface, ListItemString, rect{Min, Max}))
        {
            Result.SelectedItem = i;
        }
        
        TopLeft.y -= 30;
    }
#endif
    
    return Result;
}

#define INTERFACE_H
#endif // INTERFACE_H