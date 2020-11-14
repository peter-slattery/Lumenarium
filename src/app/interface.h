//
// File: interface.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef INTERFACE_H

// Widget Capabilities
// - string
// - background
// - outline
// - active (mouse is interacting)
// - hot (mouse could be about to interact)
// - retained state - if a toggle is active, or a drop down is open

enum gs_string_alignment
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
DrawStringLeftAligned (render_quad_batch_constructor* BatchConstructor, s32 Length, char* gs_string, v2 InitialRegisterPosition, bitmap_font* Font, v4 Color)
{
    v2 RegisterPosition = InitialRegisterPosition;
    char* C = gs_string;
    for (s32 i = 0; i < Length; i++)
    {
        v2 PositionAfterCharacter = DrawCharacterLeftAligned(BatchConstructor, *C, *Font, RegisterPosition, Color);
        RegisterPosition.x = PositionAfterCharacter.x;
        C++;
    }
    return RegisterPosition;
}

internal v2
DrawStringRightAligned (render_quad_batch_constructor* BatchConstructor, s32 Length, char* gs_string, v2 InitialRegisterPosition, bitmap_font* Font, v4 Color)
{
    v2 RegisterPosition = InitialRegisterPosition;
    char* C = gs_string + Length - 1;
    for (s32 i = Length - 1; i >= 0; i--)
    {
        v2 PositionAfterCharacter = DrawCharacterRightAligned(BatchConstructor, *C, *Font, RegisterPosition, Color);
        RegisterPosition.x = PositionAfterCharacter.x;
        C--;
    }
    return RegisterPosition;
}

internal v2
DrawString(render_command_buffer* RenderBuffer, gs_string String, bitmap_font* Font, v2 Position, v4 Color, gs_string_alignment Alignment = Align_Left)
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
DrawStringWithCursor (render_command_buffer* RenderBuffer, gs_string String, s32 CursorPosition, bitmap_font* Font, v2 Position, v4 Color, v4 CursorColor, gs_string_alignment Alignment = Align_Left)
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
                                                     String.Str + CursorPosition,
                                                     RegisterPosition, Font, Color);
        }
    }
    else if (Alignment == Align_Right)
    {
        RegisterPosition = DrawStringRightAligned(&BatchConstructor,
                                                  CursorPosition, String.Str,
                                                  RegisterPosition, Font, Color);
        DrawCursor(&CursorBatch, RegisterPosition, GreenV4, *Font);
        if (String.Length - CursorPosition > 0)
        {
            RegisterPosition = DrawStringRightAligned(&BatchConstructor,
                                                      String.Length - CursorPosition,
                                                      String.Str + CursorPosition,
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

// TODO(pjs): remove the need for htis (go thru and remove code that's in the #else block of #ifdef EXTERNAL_RENDERER s
#define EXTERNAL_RENDERER

enum ui_widget_flag
{
    UIWidgetFlag_DrawBackground,
    UIWidgetFlag_DrawOutline,
    UIWidgetFlag_Clickable,
};

struct ui_widget_id
{
    u64 Index;
    u64 LayoutId;
};

struct ui_widget
{
    ui_widget_id Id;
    
    gs_string String;
    gs_string_alignment Alignment;
    
    rect2 Bounds;
    u64 Flags;
    bool RetainedState;
};

struct ui_eval_result
{
    bool Clicked;
};

struct interface_config
{
    v4 PanelBGColors[4];
    
    // TODO(pjs): Turn these into _Default, _Hot, _Active
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

enum ui_layout_direction
{
    LayoutDirection_TopDown,
    LayoutDirection_BottomUp,
};

struct ui_layout
{
    u64 Id;
    
    rect2 Bounds;
    v2 Margin;
    r32 RowHeight;
    r32 RowYAt;
    
    ui_layout_direction FillDirection;
    
    b32 DrawHorizontal;
    u32 ColumnsMax;
    r32* ColumnWidths;
    u32 ColumnsCount;
    
    // NOTE(pjs): I'm not sure this will stay but
    // its here so that when we end things like a dropdown,
    // we can check the retained state of that dropdown
    ui_widget_id WidgetReference;
};

struct ui_widget_retained_state
{
    ui_widget_id Id;
    bool Value;
};

struct ui_interface
{
    interface_config Style;
    mouse_state Mouse;
    render_command_buffer* RenderBuffer;
    
    ui_widget* Widgets;
    u64 WidgetsCount;
    u64 WidgetsCountMax;
    
    ui_widget_id HotWidget;
    ui_widget_id ActiveWidget;
    
#define LAYOUT_COUNT_MAX 8
    ui_layout LayoutStack[LAYOUT_COUNT_MAX];
    u64 LayoutStackCount;
    u64 LayoutIdAcc;
    
#define RETAINED_STATE_MAX 128
    ui_widget_retained_state RetainedState[RETAINED_STATE_MAX];
    u64 RetainedStateCount;
};

internal void
ui_InterfaceReset(ui_interface* Interface)
{
    Interface->WidgetsCount = 0;
    Interface->LayoutStackCount = 0;
    Interface->LayoutIdAcc = 0;
}

internal bool
ui_WidgetIdsEqual(ui_widget_id A, ui_widget_id B)
{
    bool Result = (A.Index == B.Index) && (A.LayoutId == B.LayoutId);
    return Result;
}

internal ui_widget_retained_state*
ui_GetRetainedState(ui_interface* Interface, ui_widget_id Id)
{
    ui_widget_retained_state* Result = 0;
    for (u64 i = 0; i < Interface->RetainedStateCount; i++)
    {
        if (ui_WidgetIdsEqual(Interface->RetainedState[i].Id, Id))
        {
            Result = Interface->RetainedState + i;
            break;
        }
    }
    return Result;
}

internal ui_widget_retained_state*
ui_CreateRetainedState(ui_interface* Interface, ui_widget_id Id)
{
    u64 Index = Interface->RetainedStateCount++;
    ui_widget_retained_state* Result = Interface->RetainedState + Index;
    Result->Id = Id;
    return Result;
}

//
// Interaction
//

internal b32
ui_MouseClickedRect(ui_interface Interface, rect2 Rect)
{
    b32 Result = MouseButtonTransitionedDown(Interface.Mouse.LeftButtonState);
    Result &= PointIsInRect(Rect, Interface.Mouse.Pos);
    return Result;
}

// Layout

static ui_layout
ui_CreateLayout(ui_interface* Interface, rect2 Bounds, ui_layout_direction FillDirection = LayoutDirection_TopDown)
{
    ui_layout Result = {0};
    Result.Bounds = Bounds;
    Result.Margin = Interface->Style.Margin;
    Result.RowHeight = Interface->Style.RowHeight;
    Result.FillDirection = FillDirection;
    switch(FillDirection)
    {
        case LayoutDirection_BottomUp:
        {
            Result.RowYAt = Bounds.Min.y;
        }break;
        
        case LayoutDirection_TopDown:
        {
            Result.RowYAt = Bounds.Max.y - Result.RowHeight;
        }break;
    }
    
    Result.Id = ++Interface->LayoutIdAcc;
    
    return Result;
}

static void
ui_PushLayout(ui_interface* Interface, ui_layout Layout)
{
    Assert(Interface->LayoutStackCount < LAYOUT_COUNT_MAX);
    Interface->LayoutStack[Interface->LayoutStackCount++] = Layout;
}

static void
ui_PopLayout(ui_interface* Interface)
{
    Assert(Interface->LayoutStackCount > 0);
    Interface->LayoutStackCount -= 1;
}

static void
ui_StartRow(ui_interface* Interface, u32 ColumnsMax = 0)
{
    u64 LayoutIdx = Interface->LayoutStackCount - 1;
    Interface->LayoutStack[LayoutIdx].DrawHorizontal = true;
    Interface->LayoutStack[LayoutIdx].ColumnsMax = ColumnsMax;
    Interface->LayoutStack[LayoutIdx].ColumnWidths = 0;
    Interface->LayoutStack[LayoutIdx].ColumnsCount = 0;
}

static void
ui_StartRow(ui_interface* Interface, u32 ColumnsMax, r32* ColumnWidths)
{
    u64 LayoutIdx = Interface->LayoutStackCount - 1;
    Interface->LayoutStack[LayoutIdx].DrawHorizontal = true;
    Interface->LayoutStack[LayoutIdx].ColumnsMax = ColumnsMax;
    Interface->LayoutStack[LayoutIdx].ColumnWidths = ColumnWidths;
    Interface->LayoutStack[LayoutIdx].ColumnsCount = 0;
}

static void
ui_EndRow(ui_interface* Interface)
{
    u64 LayoutIdx = Interface->LayoutStackCount - 1;
    Interface->LayoutStack[LayoutIdx].DrawHorizontal = false;
    Interface->LayoutStack[LayoutIdx].ColumnWidths = 0;
    Interface->LayoutStack[LayoutIdx].RowYAt -= Interface->LayoutStack[LayoutIdx].RowHeight;
}

static b32
ui_TryReserveElementBounds(ui_layout* Layout, rect2* Bounds)
{
    b32 Result = true;
    if (!Layout->DrawHorizontal)
    {
        Bounds->Min = { Layout->Bounds.Min.x, Layout->RowYAt };
        Bounds->Max = { Layout->Bounds.Max.x, Bounds->Min.y + Layout->RowHeight };
        
        switch (Layout->FillDirection)
        {
            case LayoutDirection_BottomUp:
            {
                Layout->RowYAt += Layout->RowHeight;
            }break;
            
            case LayoutDirection_TopDown:
            {
                Layout->RowYAt -= Layout->RowHeight;
            }break;
            
            InvalidDefaultCase;
        }
    }
    else
    {
        if (Layout->ColumnsMax > 0)
        {
            Assert(Layout->ColumnsCount < Layout->ColumnsMax);
            if (Layout->ColumnWidths != 0)
            {
                v2 Min = { Layout->Bounds.Min.x, Layout->RowYAt };
                for (u32 i = 0; i < Layout->ColumnsCount; i++)
                {
                    Min.x += Layout->ColumnWidths[i];
                }
                Bounds->Min = Min;
                Bounds->Max = Bounds->Min + v2{ Layout->ColumnWidths[Layout->ColumnsCount], Layout->RowHeight };
            }
            else
            {
                r32 ElementWidth = Rect2Width(Layout->Bounds) / Layout->ColumnsMax;
                Bounds->Min = {
                    Layout->Bounds.Min.x + (ElementWidth * Layout->ColumnsCount) + Layout->Margin.x,
                    Layout->RowYAt
                };
                Bounds->Max = {
                    Bounds->Min.x + ElementWidth - Layout->Margin.x,
                    Bounds->Min.y + Layout->RowHeight
                };
            }
            Layout->ColumnsCount++;
        }
        else
        {
            Result = false;
        }
    }
    return Result;
}

static rect2
ui_ReserveElementBounds(ui_layout* Layout)
{
    rect2 Bounds = {0};
    if (!ui_TryReserveElementBounds(Layout, &Bounds))
    {
        InvalidCodePath;
    }
    return Bounds;
}

static rect2
ui_LayoutRemaining(ui_layout Layout)
{
    rect2 Result = Layout.Bounds;
    Result.Max.y = Layout.RowYAt;
    if (Layout.DrawHorizontal)
    {
        Result.Max.y -= Layout.RowHeight;
    }
    return Result;
}

// Widgets

internal ui_widget
ui_CreateWidget(gs_string String)
{
    ui_widget Result = {};
    Result.String = String;
    Result.Alignment = Align_Left;
    return Result;
}

internal void
ui_WidgetSetFlag(ui_widget* Widget, u64 Flag)
{
    u64 Value = ((u64)1 << Flag);
    Widget->Flags = Widget->Flags | Value;
}

internal bool
ui_WidgetIsFlagSet(ui_widget Widget, u64 Flag)
{
    u64 Value = ((u64)1 << Flag);
    bool Result = (Widget.Flags & Value);
    return Result;
}

internal ui_eval_result
ui_EvaluateWidget(ui_interface* Interface, ui_widget* Widget, rect2 Bounds)
{
    ui_eval_result Result = {};
    
    Assert(Interface->WidgetsCount < Interface->WidgetsCountMax);
    Widget->Id.Index = Interface->WidgetsCount++;
    Widget->Id.LayoutId = Interface->LayoutStack[Interface->LayoutStackCount - 1].Id;
    
    Widget->Bounds = Bounds;
    Interface->Widgets[Widget->Id.Index] = *Widget;
    
    if (ui_WidgetIsFlagSet(*Widget, UIWidgetFlag_Clickable))
    {
        if (PointIsInRect(Widget->Bounds, Interface->Mouse.Pos))
        {
            if (ui_WidgetIdsEqual(Interface->HotWidget, Widget->Id) && MouseButtonTransitionedDown(Interface->Mouse.LeftButtonState))
            {
                Result.Clicked = true;
                Interface->ActiveWidget = Widget->Id;
            }
            
            if (ui_WidgetIdsEqual(Interface->ActiveWidget, Widget->Id) &&
                MouseButtonTransitionedUp(Interface->Mouse.LeftButtonState))
            {
                Interface->ActiveWidget = {};
            }
            
            Interface->HotWidget = Widget->Id;
        }
    }
    
    return Result;
}

internal ui_eval_result
ui_EvaluateWidget(ui_interface* Interface, ui_widget* Widget)
{
    rect2 Bounds = {0};
    ui_layout* Layout = Interface->LayoutStack + Interface->LayoutStackCount - 1;
    if (!ui_TryReserveElementBounds(Layout, &Bounds))
    {
        // TODO(pjs): This isn't invalid, but Idk when we'd hit this case yet
        InvalidCodePath;
    }
    
    return ui_EvaluateWidget(Interface, Widget, Bounds);
}

//
// Drawing Functions
//

static r32
ui_GetTextLineHeight(ui_interface Interface)
{
    r32 Result = Interface.Style.Font->PixelHeight + (2 * Interface.Style.Margin.y);
    return Result;
}

static void
ui_FillRect(ui_interface* Interface, rect2 Bounds, v4 Color)
{
    PushRenderQuad2D(Interface->RenderBuffer, Bounds.Min, Bounds.Max, Color);
}

static void
ui_OutlineRect(ui_interface* Interface, rect2 Bounds, r32 Thickness, v4 Color)
{
    PushRenderBoundingBox2D(Interface->RenderBuffer, Bounds.Min, Bounds.Max, Thickness, Color);
}

internal void
ui_DrawString(ui_interface* Interface, gs_string String, rect2 Bounds, gs_string_alignment Alignment = Align_Left)
{
    DEBUG_TRACK_FUNCTION;
    ui_widget Widget = ui_CreateWidget(String);
    Widget.Bounds = Bounds;
    ui_EvaluateWidget(Interface, &Widget);
}

internal void
ui_DrawString(ui_interface* Interface, gs_string String, gs_string_alignment Alignment = Align_Left)
{
    DEBUG_TRACK_FUNCTION;
    ui_widget Widget = ui_CreateWidget(String);
    ui_EvaluateWidget(Interface, &Widget);
}

static b32
ui_Button(ui_interface* Interface, gs_string Text)
{
    ui_widget Widget = ui_CreateWidget(Text);
    ui_WidgetSetFlag(&Widget, UIWidgetFlag_Clickable);
    ui_WidgetSetFlag(&Widget, UIWidgetFlag_DrawBackground);
    ui_eval_result Result = ui_EvaluateWidget(Interface, &Widget);
    return Result.Clicked;
}

static b32
ui_Button(ui_interface* Interface, gs_string Text, rect2 Bounds)
{
    ui_widget Widget = ui_CreateWidget(Text);
    ui_WidgetSetFlag(&Widget, UIWidgetFlag_Clickable);
    ui_WidgetSetFlag(&Widget, UIWidgetFlag_DrawBackground);
    ui_eval_result Result = ui_EvaluateWidget(Interface, &Widget, Bounds);
    return Result.Clicked;
}

struct list_item_colors
{
    v4 Hover;
    v4 Selected;
    v4 BGColor;
};

inline v4
ui_GetListItemBGColor(interface_config Style, u32 ElementIndex)
{
    v4 Result = Style.ListBGColors[ElementIndex % LIST_BG_COLORS_COUNT];
    return Result;
}

static list_item_colors
ui_GetListItemColors(ui_interface* Interface, u32 ListItemIndex)
{
    list_item_colors Result = {};
    Result.Hover = Interface->Style.ListBGHover;
    Result.Selected = Interface->Style.ListBGSelected;
    Result.BGColor = ui_GetListItemBGColor(Interface->Style, ListItemIndex);
    return Result;
}

static b32
ui_ListButton(ui_interface* Interface, gs_string Text, rect2 Bounds, u32 ListItemIndex)
{
    ui_widget Widget = ui_CreateWidget(Text);
    ui_WidgetSetFlag(&Widget, UIWidgetFlag_DrawBackground);
    ui_WidgetSetFlag(&Widget, UIWidgetFlag_Clickable);
    // TODO(pjs): Reimplement alternating color backgrounds
    Widget.Bounds = Bounds;
    ui_eval_result Result = ui_EvaluateWidget(Interface, &Widget);
    return Result.Clicked;
}

static b32
ui_LayoutListButton(ui_interface* Interface, ui_layout* Layout, gs_string Text, u32 ListItemIndex)
{
    return ui_Button(Interface, Text);
}

static b32
ui_LayoutListEntry(ui_interface* Interface, ui_layout* Layout, gs_string Text, u32 Index)
{
    rect2 Bounds = {0};
    if (!ui_TryReserveElementBounds(Layout, &Bounds))
    {
        // TODO(Peter): this isn't really invalid, but I don't have a concrete use case
        // for it yet. This should only fire if the Layout component is drawing a row,
        // but if you're in row mode during a list, what should happen?
        // Punting this till I have a use case
        InvalidCodePath;
    }
    return ui_Button(Interface, Text, Bounds);
}

internal bool
ui_EvaluateDropdown(ui_interface* Interface, ui_widget Widget, ui_eval_result EvalResult)
{
    ui_widget_retained_state* State = ui_GetRetainedState(Interface, Widget.Id);
    if (!State) {
        State = ui_CreateRetainedState(Interface, Widget.Id);
    }
    
    if (EvalResult.Clicked)
    {
        State->Value = !State->Value;
    }
    
    if (State->Value)
    {
        ui_layout ParentLayout = Interface->LayoutStack[Interface->LayoutStackCount - 1];
        
        r32 SpaceAbove = ParentLayout.Bounds.Max.y - Widget.Bounds.Max.y;
        r32 SpaceBelow = Widget.Bounds.Min.y - ParentLayout.Bounds.Min.y;
        ui_layout_direction Direction = LayoutDirection_TopDown;
        rect2 MenuBounds = {};
        
        if (SpaceAbove > SpaceBelow)
        {
            r32 ParentLayoutMaxY = ParentLayout.Bounds.Max.y;
            Direction = LayoutDirection_BottomUp;
            MenuBounds = rect2{
                v2{ Widget.Bounds.Min.x, Widget.Bounds.Max.y },
                v2{ Widget.Bounds.Max.x, ParentLayoutMaxY }
            };
        }
        else
        {
            r32 ParentLayoutMinY = ParentLayout.Bounds.Min.y;
            Direction = LayoutDirection_TopDown;
            MenuBounds = rect2{
                v2{ Widget.Bounds.Min.x, ParentLayoutMinY },
                v2{ Widget.Bounds.Max.x, Widget.Bounds.Min.y }
            };
        }
        
        ui_layout Layout = ui_CreateLayout(Interface, MenuBounds, Direction);
        Layout.WidgetReference = Widget.Id;
        ui_PushLayout(Interface, Layout);
    }
    
    return State->Value;
}

internal bool
ui_BeginDropdown(ui_interface* Interface, gs_string Text, rect2 Bounds)
{
    ui_widget Widget = ui_CreateWidget(Text);
    ui_WidgetSetFlag(&Widget, UIWidgetFlag_Clickable);
    ui_WidgetSetFlag(&Widget, UIWidgetFlag_DrawBackground);
    ui_eval_result Result = ui_EvaluateWidget(Interface, &Widget, Bounds);
    return ui_EvaluateDropdown(Interface, Widget, Result);
}

internal bool
ui_BeginDropdown(ui_interface* Interface, gs_string Text)
{
    ui_widget Widget = ui_CreateWidget(Text);
    ui_WidgetSetFlag(&Widget, UIWidgetFlag_Clickable);
    ui_WidgetSetFlag(&Widget, UIWidgetFlag_DrawBackground);
    ui_eval_result Result = ui_EvaluateWidget(Interface, &Widget);
    return ui_EvaluateDropdown(Interface, Widget, Result);
}

internal void
ui_EndDropdown(ui_interface* Interface)
{
    ui_layout Layout = Interface->LayoutStack[Interface->LayoutStackCount - 1];
    ui_widget_retained_state* State = ui_GetRetainedState(Interface, Layout.WidgetReference);
    if (State)
    {
        if (State->Value)
        {
            ui_PopLayout(Interface);
        }
    }
}

//
// OLD
//

enum selection_state
{
    Selection_None,
    Selection_Selected,
    Selection_Deselected,
};

struct interface_list
{
    rect2 ListBounds;
    
    v2 ListElementDimensions;
    v2 ElementLabelIndent;
    
    v4 TextColor;
    v4* LineBGColors;
    s32 LineBGColorsCount;
    v4 LineBGHoverColor;
    
    s32 ListElementsCount;
};

internal rect2
DrawListElementBackground(interface_list* List, mouse_state Mouse, render_command_buffer* RenderBuffer)
{
    rect2 LineBounds = {};
    LineBounds.Min = v2{
        List->ListBounds.Min.x,
        List->ListBounds.Max.y - (List->ListElementDimensions.y * (List->ListElementsCount + 1))
    };
    LineBounds.Max = LineBounds.Min + List->ListElementDimensions;
    
    v4 Color = List->LineBGColors[List->ListElementsCount % List->LineBGColorsCount];
    if (PointIsInRect(LineBounds, Mouse.Pos))
    {
        Color = List->LineBGHoverColor;
    }
    
    PushRenderQuad2D(RenderBuffer, LineBounds.Min, LineBounds.Max, Color);
    return LineBounds;
}

internal rect2
DrawListElement(gs_string Label, interface_list* List, mouse_state Mouse, render_command_buffer* RenderBuffer, interface_config Interface)
{
    rect2 Bounds = DrawListElementBackground(List, Mouse, RenderBuffer);
    
    v2 LabelPosition = Bounds.Min + List->ElementLabelIndent;
    DrawString(RenderBuffer, Label, Interface.Font, LabelPosition, List->TextColor);
    
    List->ListElementsCount++;
    return Bounds;
}


internal r32
EvaluateColorChannelSlider (render_command_buffer* RenderBuffer, v4 ChannelMask, v2 Min, v2 Max, r32 Current, mouse_state Mouse)
{
    r32 Result = Current;
    
    // TODO(Peter): Can this come from outside the function? Would rather pass rect around than min/max
    rect2 Rect = rect2{ Min, Max };
    
    render_quad_batch_constructor Batch = PushRenderQuad2DBatch(RenderBuffer, 2);
    
    v4 LeftColor = ChannelMask * 0;
    LeftColor.a = 1.f;
    v4 RightColor = ChannelMask;
    PushQuad2DOnBatch(&Batch,
                      RectBottomLeft(Rect), RectBottomRight(Rect),
                      RectTopRight(Rect), RectTopLeft(Rect),
                      v2{0, 0}, v2{1, 0}, v2{1, 1}, v2{0, 1},
                      LeftColor, RightColor, RightColor, LeftColor);
    
    if (MouseButtonTransitionedDown(Mouse.LeftButtonState))
    {
        if (PointIsInRect(Rect, Mouse.DownPos))
        {
            Result = ((r32)Mouse.Pos.x - Min.x) / (Max.x - Min.x);
            Result = Clamp01(Result);
        }
    }
    
    r32 DragBarWidth = 8;
    v2 DragBarMin = v2{
        LerpR32(Result, Min.x, Max.x) - (DragBarWidth / 2),
        Min.y - 2
    };
    v2 DragBarMax = DragBarMin + v2{DragBarWidth, (Max.y - Min.y) + 4};
    
    PushQuad2DOnBatch(&Batch, DragBarMin, DragBarMax, v4{.3f, .3f, .3f, 1.f});
    
    return Result;
}

internal b32
EvaluateColorPicker (render_command_buffer* RenderBuffer, v4* Value, v2 PanelMin, interface_config Config, mouse_state Mouse)
{
    b32 ShouldClose = false;
    
    v2 PanelMax = v2{400, 500};
    // TODO(Peter): Can this get passed from outside? rather pass rect2 than min/max pairs
    rect2 PanelRect = rect2{PanelMin, PanelMax};
    if (MouseButtonTransitionedDown(Mouse.LeftButtonState) && !PointIsInRect(PanelRect, Mouse.Pos))
    {
        ShouldClose = true;
    }
    else
    {
        PushRenderQuad2D(RenderBuffer, PanelRect.Min, PanelRect.Max, v4{.5f, .5f, .5f, 1.f});
        
        v2 TitleMin = v2{PanelRect.Min.x + 5, PanelRect.Max.y - (Config.Font->PixelHeight + 5)};
        DrawString(RenderBuffer, MakeString("Color Picker"), Config.Font,
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

typedef gs_string search_lister_get_list_item_at_offset(u8* ListMemory, s32 ListLength, gs_string Searchgs_string, s32 Offset);

internal search_lister_result
EvaluateSearchLister (ui_interface* Interface, v2 TopLeft, v2 Dimension, gs_string Title,
                      gs_string* ItemList, s32* ListLUT, s32 ListLength,
                      s32 HotItem,
                      gs_string* Searchgs_string, s32 Searchgs_stringCursorPosition)
{
    search_lister_result Result = {};
    Result.ShouldRemainOpen = true;
    Result.HotItem = HotItem;
    
    // TODO(Peter): Was tired. Nothing wrong with the code below
    InvalidCodePath;
#if 0
    // Title Bar
    rect2 TitleBarBounds = rect2{v2{TopLeft.x, TopLeft.y - 30}, v2{TopLeft.x + 300, TopLeft.y}};
    ui_FillRect(Interface, TitleBarBounds, v4{.3f, .3f, .3f, 1.f});
    ui_Drawgs_string(Interface, Title, TitleBarBounds, Interface->Style.TextColor);
    
    MakeStringBuffer(Debuggs_string, 256);
    PrintF(&Debuggs_string, "Hot Item: %d  |  Filtered Items: %d", HotItem, ListLength);
    rect2 DebugBounds = MakeRectMinWidth(v2{ TopLeft.x + 256, TopLeft.y - 25}, v2{256, Interface->Style.LineHeight});
    ui_Drawgs_string(Interface, Debuggs_string, DebugBounds, Interface->Style.TextColor);
    
    // Search Bar
    PushRenderQuad2D(RenderBuffer, v2{TopLeft.x, TopLeft.y - 30}, v2{TopLeft.x + 300, TopLeft.y}, v4{.3f, .3f, .3f, 1.f});
    Drawgs_stringWithCursor(RenderBuffer, *Searchgs_string, Searchgs_stringCursorPosition, Font, v2{TopLeft.x, TopLeft.y - 25}, WhiteV4, GreenV4);
    TopLeft.y -= 30;
    
    for (s32 i = 0; i < ListLength; i++)
    {
        s32 FilteredIndex = ListLUT[i];
        gs_string ListItemgs_string = ItemList[FilteredIndex];
        
        v2 Min = v2{TopLeft.x, TopLeft.y - 30};
        v2 Max = Min + Dimension - v2{0, Config.Margin.y};
        
        v4 ButtonColor = Config.ButtonColor_Inactive;
        if (i == HotItem)
        {
            ButtonColor = Config.ButtonColor_Active;
        }
        
        if (ui_Button(Interface, ListItemgs_string, rect2{Min, Max}))
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