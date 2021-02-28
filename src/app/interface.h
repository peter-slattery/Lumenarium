//
// File: interface.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef INTERFACE_H

#define InterfaceAssert(IMemPtr) Assert(IMemPtr && (u64)IMemPtr != 0x5 && (u64)IMemPtr != 0xC)

enum gs_string_alignment
{
    Align_Left,
    Align_Center,
    Align_Right,
};

internal void
ClipUVRect(rect2* Bounds, rect2* UVs, rect2 ClippingBox)
{
    rect2 NewBounds = Rect2Union(*Bounds, ClippingBox);
    
    r32 OldWidth = Rect2Width(*Bounds);
    r32 OldHeight = Rect2Height(*Bounds);
    
    v2 MinInsetPercent = v2{
        (NewBounds.Min.x - Bounds->Min.x) / OldWidth,
        (NewBounds.Min.y - Bounds->Min.y) / OldHeight,
    };
    
    v2 MaxInsetPercent = v2{
        (NewBounds.Max.x - Bounds->Min.x) / OldWidth,
        (NewBounds.Max.y - Bounds->Min.y) / OldHeight,
    };
    
    UVs->Min.x = LerpR32(MinInsetPercent.x, UVs->Min.x, UVs->Max.x);
    UVs->Min.y = LerpR32(MinInsetPercent.y, UVs->Min.y, UVs->Max.y);
    UVs->Max.x = LerpR32(MaxInsetPercent.x, UVs->Min.x, UVs->Max.x);
    UVs->Max.y = LerpR32(MaxInsetPercent.y, UVs->Min.y, UVs->Max.y);
    
    *Bounds = NewBounds;
}

internal void
DrawCharacter_ (render_quad_batch_constructor* BatchConstructor, r32 MinX, r32 MinY, codepoint_bitmap CodepointInfo, rect2 ClippingBox, v4 Color)
{
    rect2 Bounds = {};
    Bounds.Min.x = FloorR32(MinX);
    Bounds.Min.y = FloorR32(MinY);
    Bounds.Max.x = Bounds.Min.x + (CodepointInfo.Width);
    Bounds.Max.y = Bounds.Min.y + (CodepointInfo.Height);
    
    rect2 UVBounds = {};
    UVBounds.Min = CodepointInfo.UVMin;
    UVBounds.Max = CodepointInfo.UVMax;
    
    ClipUVRect(&Bounds, &UVBounds, ClippingBox);
    
    s32 AlignedMinX = (s32)(MinX);
    s32 AlignedMinY = (s32)(MinY);
    s32 AlignedMaxX = AlignedMinX + (CodepointInfo.Width);
    s32 AlignedMaxY = AlignedMinY + (CodepointInfo.Height);
    
    PushQuad2DOnBatch(BatchConstructor,
                      Rect2BottomLeft(Bounds), Rect2BottomRight(Bounds),
                      Rect2TopRight(Bounds), Rect2TopLeft(Bounds),
                      UVBounds.Min, UVBounds.Max,
                      Color);
}

internal v2
DrawCharacterLeftAligned (render_quad_batch_constructor* BatchConstructor, char C, bitmap_font Font, v2 Position, rect2 ClippingBox, v4 Color)
{
    s32 GlyphDataIndex = GetIndexForCodepoint(Font, C);
    codepoint_bitmap CodepointInfo = Font.CodepointValues[GlyphDataIndex];
    
    // NOTE(Peter):
    r32 MinX = Position.x + CodepointInfo.XOffset;
    r32 MinY = Position.y + CodepointInfo.YOffset;
    DrawCharacter_(BatchConstructor, MinX, MinY, CodepointInfo, ClippingBox, Color);
    
    // NOTE(Peter):
    v2 PointAfterCharacter = v2{Position.x + CodepointInfo.Width, Position.y};
    return PointAfterCharacter;
}

internal v2
DrawCharacterRightAligned (render_quad_batch_constructor* BatchConstructor, char C, bitmap_font Font, v2 Position, rect2 ClippingBox, v4 Color)
{
    s32 GlyphDataIndex = GetIndexForCodepoint(Font, C);
    codepoint_bitmap CodepointInfo = Font.CodepointValues[GlyphDataIndex];
    
    // NOTE(Peter):
    r32 MinX = Position.x - (CodepointInfo.XOffset + CodepointInfo.Width);
    r32 MinY = Position.y + CodepointInfo.YOffset + CodepointInfo.YOffset;
    DrawCharacter_(BatchConstructor, MinX, MinY, CodepointInfo, ClippingBox, Color);
    
    // NOTE(Peter):
    v2 PointAfterCharacter = v2{Position.x-(CodepointInfo.Width + CodepointInfo.XOffset), Position.y};
    return PointAfterCharacter;
}

internal v2
DrawStringLeftAligned (render_quad_batch_constructor* BatchConstructor, s32 Length, char* gs_string, v2 InitialRegisterPosition, bitmap_font* Font, rect2 ClippingBox, v4 Color)
{
    v2 RegisterPosition = InitialRegisterPosition;
    char* C = gs_string;
    for (s32 i = 0; i < Length; i++)
    {
        v2 PositionAfterCharacter = DrawCharacterLeftAligned(BatchConstructor, *C, *Font, RegisterPosition, ClippingBox, Color);
        RegisterPosition.x = PositionAfterCharacter.x;
        C++;
    }
    return RegisterPosition;
}

internal v2
DrawStringRightAligned (render_quad_batch_constructor* BatchConstructor, s32 Length, char* gs_string, v2 InitialRegisterPosition, bitmap_font* Font, rect2 ClippingBox, v4 Color)
{
    v2 RegisterPosition = InitialRegisterPosition;
    char* C = gs_string + Length - 1;
    for (s32 i = Length - 1; i >= 0; i--)
    {
        v2 PositionAfterCharacter = DrawCharacterRightAligned(BatchConstructor, *C, *Font, RegisterPosition, ClippingBox, Color);
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
    
    // TODO(pjs): I don't like this solution but it'll do for now and I want to focus on other problems
    // especially since I think this one will go away once I finish the ui overhaul
    rect2 InfiniteClipBox = {};
    InfiniteClipBox.Min.x = -100000;
    InfiniteClipBox.Min.y = -100000;
    InfiniteClipBox.Max.x = 100000;
    InfiniteClipBox.Max.y = 100000;
    
    v2 RegisterPosition = Position;
    if (Alignment == Align_Left)
    {
        RegisterPosition = DrawStringLeftAligned(&BatchConstructor, StringExpand(String), RegisterPosition, Font, InfiniteClipBox, Color);
    }
    else if (Alignment == Align_Right)
    {
        RegisterPosition = DrawStringRightAligned(&BatchConstructor, StringExpand(String), RegisterPosition, Font, InfiniteClipBox, Color);
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

#if 0
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
#endif

enum ui_widget_flag
{
    UIWidgetFlag_ExpandsToFitChildren,
    
    UIWidgetFlag_DrawBackground,
    UIWidgetFlag_DrawString,
    UIWidgetFlag_DrawOutline,
    UIWidgetFlag_DrawHorizontalFill,
    UIWidgetFlag_DrawVerticalFill,
    UIWidgetFlag_DrawFillReversed,
    UIWidgetFlag_DrawFillAsHandle,
    
    UIWidgetFlag_Clickable,
    UIWidgetFlag_Selectable,
    UIWidgetFlag_Typable,
};

struct ui_widget_id
{
    u64 Id;
    u64 ZIndex;
};

enum ui_layout_direction
{
    LayoutDirection_TopDown,
    LayoutDirection_BottomUp,
    LayoutDirection_Inherit,
};

struct ui_column
{
    r32 XMin;
    r32 XMax;
};

struct ui_widget
{
    ui_widget_id Id;
    
    gs_string String;
    gs_string_alignment Alignment;
    
    rect2 Bounds;
    u64 Flags;
    
    ui_widget* Next;
    
    // Slider
    r32 FillPercent;
    
    // Layout
    ui_widget* Parent;
    
    v2 Margin;
    r32 RowHeight;
    r32 RowYAt;
    
    ui_layout_direction FillDirection;
    
    ui_column* Columns;
    u32 ColumnsCount;
    u32 ColumnsFilled;
    
    // NOTE(pjs): I'm not sure this will stay but
    // its here so that when we end things like a dropdown,
    // we can check the retained state of that dropdown
    ui_widget_id WidgetReference;
    
    u64 ChildZIndexOffset;
    
    ui_widget* ChildrenRoot;
    ui_widget* ChildrenHead;
    u32 ChildCount;
};

struct ui_eval_result
{
    bool Clicked;
    bool Held;
    v2 DragDelta;
};

struct interface_config
{
    v4 PanelBG;
    
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

struct ui_widget_retained_state
{
    ui_widget_id Id;
    bool Value;
    r32 InitialValueR32;
    u32 FramesSinceAccess;
    
    // For use in layouts that allow you to scroll / pan
    v2 ChildrenDrawOffset;
    
    gs_string EditString;
    
    // For dropdowns and rows to be able to error check not closing
    // a layout you open
    u32 MaxChildren;
};

struct ui_interface
{
    interface_config Style;
    
    mouse_state Mouse;
    rect2 WindowBounds;
    
    // A per-frame string of the characters which have been typed
    gs_const_string TempInputString;
    
    render_command_buffer* RenderBuffer;
    
    ui_widget* Widgets;
    u64 WidgetsCount;
    u64 WidgetsCountMax;
    
    ui_widget* DrawOrderHead;
    ui_widget* DrawOrderRoot;
    
    ui_widget_id HotWidget;
    // This should really never get higher than 1 or 2
    u8 HotWidgetFramesSinceUpdate;
    
    ui_widget_id ActiveWidget;
    ui_widget_id LastActiveWidget;
    
    ui_widget* ActiveLayout;
    
#define RETAINED_STATE_MAX 128
    ui_widget_retained_state RetainedState[RETAINED_STATE_MAX];
    u64 RetainedStateCount;
    
    gs_memory_arena* PerFrameMemory;
    
    // TODO(pjs): DONT USE THIS
    // Right now you only need this to create EditStrings for ui_widget_retained_state's
    // and even for those, you eventually want a better solution than "create a string and it lives forever"
    // TODO(pjs): Get rid of the need for this vvv
    gs_memory_arena* Permanent;
};

internal void
ui_InterfaceReset(ui_interface* Interface)
{
    Interface->WidgetsCount = 0;
    Interface->DrawOrderHead = 0;
    Interface->DrawOrderRoot = 0;
    ClearArena(Interface->PerFrameMemory);
    InterfaceAssert(Interface->PerFrameMemory);
    
    for (u32 i = 0; i < Interface->RetainedStateCount; i++)
    {
        Interface->RetainedState[i].FramesSinceAccess += 1;
        if (Interface->RetainedState[i].FramesSinceAccess > 1)
        {
            Interface->RetainedState[i] = {0};
        }
    }
    
    Interface->LastActiveWidget = Interface->ActiveWidget;
}

internal bool
ui_WidgetIdsEqual(ui_widget_id A, ui_widget_id B)
{
    bool Result = (A.Id == B.Id);// && (A.ParentId == B.ParentId);
    return Result;
}

internal void
ui_WidgetSetFlag(ui_widget* Widget, u64 Flag)
{
    u64 Value = ((u64)1 << Flag);
    Widget->Flags = Widget->Flags | Value;
}

internal void
ui_WidgetClearFlag(ui_widget* Widget, u64 Flag)
{
    u64 Value = ((u64)1 << Flag);
    Widget->Flags = Widget->Flags & ~Value;
}

internal bool
ui_WidgetIsFlagSet(ui_widget Widget, u64 Flag)
{
    u64 Value = ((u64)1 << Flag);
    bool Result = (Widget.Flags & Value);
    return Result;
}

internal void
ui_WidgetSetChildrenPopover(ui_widget* Widget)
{
    Widget->ChildZIndexOffset = 1000;
}

internal ui_widget*
ui_WidgetGetWidgetWithId(ui_widget* Parent, ui_widget_id Id)
{
    ui_widget* Result = 0;
    
    if (ui_WidgetIdsEqual(Parent->Id, Id))
    {
        Result = Parent;
    }
    else if (Parent->ChildrenRoot != 0)
    {
        for (ui_widget* At = Parent->ChildrenRoot; At != 0; At = At->Next)
        {
            Result = ui_WidgetGetWidgetWithId(At, Id);
            if (Result != 0)
            {
                break;
            }
        }
    }
    
    return Result;
}

internal ui_widget*
ui_InterfaceGetWidgetWithId(ui_interface* Interface, ui_widget_id Id)
{
    ui_widget* Result = 0;
    
    for (ui_widget* At = Interface->DrawOrderRoot; At != 0; At = At->Next)
    {
        Result = ui_WidgetGetWidgetWithId(At, Id);
        if (Result != 0)
        {
            break;
        }
    }
    
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
            Interface->RetainedState[i].FramesSinceAccess = 0;
            Result = Interface->RetainedState + i;
            break;
        }
    }
    return Result;
}

internal ui_widget_retained_state*
ui_CreateRetainedState(ui_interface* Interface, ui_widget* Widget)
{
    u64 Index = Interface->RetainedStateCount++;
    ui_widget_retained_state* Result = Interface->RetainedState + Index;
    Result->Id = Widget->Id;
    Result->EditString = PushString(Interface->Permanent, 256);
    return Result;
}

internal ui_widget_retained_state*
ui_GetOrCreateRetainedState(ui_interface* Interface, ui_widget* Widget)
{
    ui_widget_retained_state* State = ui_GetRetainedState(Interface, Widget->Id);
    if (!State)
    {
        State = ui_CreateRetainedState(Interface, Widget);
    }
    return State;
}

internal ui_widget*
ui_CreateWidget(ui_interface* Interface, gs_string String)
{
    InterfaceAssert(Interface->PerFrameMemory);
    Assert(Interface->WidgetsCount < Interface->WidgetsCountMax);
    u64 Index = Interface->WidgetsCount++;
    ui_widget* Result = Interface->Widgets + Index;
    ZeroStruct(Result);
    
    Result->Parent = Interface->ActiveLayout;
    
    u64 Id = HashDJB2ToU64(StringExpand(String));
    if (Result->Parent)
    {
        Id = HashAppendDJB2ToU32(Id, Result->Parent->Id.Id);
        Id = HashAppendDJB2ToU32(Id, Result->Parent->ChildCount);
        //Result->Id.ParentId = Result->Parent->Id.Id;
    }
    Result->Id.Id = Id;
    
    u64 ZIndex = Index + 1;
    if (Result->Parent)
    {
        Result->ChildZIndexOffset += Result->Parent->ChildZIndexOffset;
        ZIndex += Result->Parent->ChildZIndexOffset;
    }
    Result->Id.ZIndex = ZIndex;
    
    Result->String = PushStringCopy(Interface->PerFrameMemory, String.ConstString);
    InterfaceAssert(Interface->PerFrameMemory);
    Result->Alignment = Align_Left;
    Result->Next = 0;
    Result->ChildrenRoot = 0;
    Result->ChildrenHead = 0;
    Result->Flags = 0;
    ui_WidgetSetFlag(Result, UIWidgetFlag_ExpandsToFitChildren);
    
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

static rect2
ui_ReserveBounds(ui_interface* Interface, ui_widget* Widget, bool Inset)
{
    Assert(Widget->ColumnsCount > 0);
    rect2 Bounds = {0};
    u32 ColumnIndex = Widget->ChildCount % Widget->ColumnsCount;
    
    ui_column Column = Widget->Columns[ColumnIndex];
    Bounds.Min.x = Column.XMin;
    Bounds.Min.y = Widget->RowYAt;
    Bounds.Max.x = Column.XMax;
    Bounds.Max.y = Bounds.Min.y + Widget->RowHeight;
    
    if (Inset)
    {
        Bounds.Min.x += Widget->Margin.x;
        Bounds.Min.y += Widget->Margin.y;
        Bounds.Max.x -= Widget->Margin.x;
        Bounds.Max.y -= Widget->Margin.y;
    }
    
    if (Widget->ChildCount == 0)
    {
        ui_widget_retained_state* State = ui_GetRetainedState(Interface, Widget->Id);
        if (State)
        {
            Bounds = Rect2Translate(Bounds, State->ChildrenDrawOffset);
        }
    }
    
    return Bounds;
}

internal void
ui_CommitBounds(ui_widget* Parent, rect2 Bounds)
{
    u32 ColumnIndex = Parent->ChildCount % Parent->ColumnsCount;
    if (ColumnIndex == 0)
    {
        switch (Parent->FillDirection)
        {
            case LayoutDirection_BottomUp:
            {
                Parent->RowYAt = Bounds.Max.y;
                if (ui_WidgetIsFlagSet(*Parent, UIWidgetFlag_ExpandsToFitChildren))
                {
                    Parent->Bounds.Max.y = Parent->RowYAt;
                }
            }break;
            
            case LayoutDirection_TopDown:
            {
                Parent->RowYAt = Bounds.Min.y - Parent->RowHeight;
                if (ui_WidgetIsFlagSet(*Parent, UIWidgetFlag_ExpandsToFitChildren))
                {
                    Parent->Bounds.Min.y = Bounds.Min.y;
                }
            }break;
        }
    }
}

internal void
ui_ExpandParentToFit(ui_widget* Widget)
{
    ui_widget* Parent = Widget->Parent;
    switch (Widget->FillDirection)
    {
        case LayoutDirection_TopDown:
        {
            Parent->Bounds.Min.y = Min(Parent->Bounds.Min.y, Widget->Bounds.Min.y - Parent->Margin.y);
        }break;
        
        case LayoutDirection_BottomUp:
        {
            Parent->Bounds.Max.y = Max(Parent->Bounds.Max.y, Widget->Bounds.Max.y + Parent->Margin.y);
        }break;
        
        InvalidDefaultCase;
    }
}

internal void
ui_WidgetCreateColumns(ui_widget* Widget, u32 ColumnsCount, ui_interface* Interface)
{
    Widget->Columns = PushArray(Interface->PerFrameMemory, ui_column, ColumnsCount);
    InterfaceAssert(Interface->PerFrameMemory);
    Widget->ColumnsCount = ColumnsCount;
    Widget->ColumnsFilled = 0;
}

internal void
ui_WidgetInitUniformColumns(ui_widget* Widget)
{
    r32 CurrentRowWidth = Rect2Width(Widget->Bounds);
    r32 ColumnWidth = CurrentRowWidth / Widget->ColumnsCount;
    for (u32 i = 0; i < Widget->ColumnsCount; i++)
    {
        ui_column* Column = Widget->Columns + i;
        Column->XMin = Widget->Bounds.Min.x + (ColumnWidth * i);
        Column->XMax = Column->XMin + ColumnWidth;
    }
}

internal ui_widget*
ui_CreateLayoutWidget(ui_interface* Interface, rect2 Bounds, gs_string Name, ui_layout_direction FillDir = LayoutDirection_Inherit)
{
    ui_widget* Result = ui_CreateWidget(Interface, Name);
    //ui_WidgetSetFlag(Result, UIWidgetFlag_DrawOutline);
    
    Result->Bounds = Bounds;
    Result->Margin = Interface->Style.Margin;
    Result->RowHeight = Interface->Style.RowHeight;
    
    // Single Column Layout
    ui_WidgetCreateColumns(Result, 1, Interface);
    ui_WidgetInitUniformColumns(Result);
    
    if (FillDir == LayoutDirection_Inherit && Result->Parent != 0)
    {
        Result->FillDirection = Result->Parent->FillDirection;
    }
    else
    {
        Result->FillDirection = FillDir;
    }
    
    switch(Result->FillDirection)
    {
        case LayoutDirection_BottomUp:
        {
            Result->RowYAt = Bounds.Min.y;
        }break;
        
        case LayoutDirection_TopDown:
        {
            Result->RowYAt = Bounds.Max.y - Result->RowHeight;
        }break;
        
        InvalidDefaultCase;
    }
    
    return Result;
}

static ui_widget*
ui_PushOverlayLayout(ui_interface* Interface, rect2 Bounds, ui_layout_direction FillDir, gs_string Name)
{
    ui_widget* Result = ui_CreateLayoutWidget(Interface, Bounds, Name, FillDir);
    SLLPushOrInit(Interface->DrawOrderRoot, Interface->DrawOrderHead, Result);
    Interface->ActiveLayout = Result;
    return Result;
}

static ui_widget*
ui_PushLayout(ui_interface* Interface, rect2 Bounds, ui_layout_direction FillDir, gs_string Name)
{
    ui_widget* Result = ui_CreateLayoutWidget(Interface, Bounds, Name, FillDir);
    
    if (Interface->DrawOrderRoot)
    {
        SLLPushOrInit(Interface->ActiveLayout->ChildrenRoot, Interface->ActiveLayout->ChildrenHead, Result);
        Interface->ActiveLayout->ChildCount++;
    }
    else
    {
        SLLPushOrInit(Interface->DrawOrderRoot, Interface->DrawOrderHead, Result);
    }
    
    Interface->ActiveLayout = Result;
    return Result;
}

static ui_widget*
ui_PushLayout(ui_interface* Interface, gs_string Name, bool Inset = true)
{
    rect2 Bounds = {};
    ui_layout_direction Direction = LayoutDirection_TopDown;
    if (Interface->ActiveLayout)
    {
        Bounds = ui_ReserveBounds(Interface, Interface->ActiveLayout, Inset);
        Direction = Interface->ActiveLayout->FillDirection;
    }
    else
    {
        Bounds.Min.x = Interface->WindowBounds.Min.x;
        Bounds.Min.y = Interface->WindowBounds.Max.y;
        Bounds.Max.x = Interface->WindowBounds.Max.x;
        Bounds.Max.y = Interface->WindowBounds.Max.y;
        
        if (Inset)
        {
            Bounds.Min.x += Interface->Style.Margin.x;
            Bounds.Max.x -= Interface->Style.Margin.x;
        }
        
        Direction = LayoutDirection_TopDown;
    }
    return ui_PushLayout(Interface, Bounds, Direction, Name);
}

internal void
ui_ExpandToFitChildren(ui_widget* Parent)
{
    if (!ui_WidgetIsFlagSet(*Parent, UIWidgetFlag_ExpandsToFitChildren)) { return; }
    
    v2 Extents = { Parent->Bounds.Max.y, Parent->Bounds.Min.y };
    for (ui_widget* Child = Parent->ChildrenRoot; Child != 0; Child = Child->Next)
    {
        Extents.x = Min(Extents.x, Child->Bounds.Min.y);
        Extents.y = Max(Extents.y, Child->Bounds.Max.y);
    }
    
    switch(Parent->FillDirection)
    {
        case LayoutDirection_BottomUp:
        {
            Parent->Bounds.Max.y = Max(Extents.y + Parent->Margin.y, Parent->Bounds.Max.y);
        }break;
        
        case LayoutDirection_TopDown:
        {
            Parent->Bounds.Min.y = Min(Extents.x - Parent->Margin.y, Parent->Bounds.Min.y);
        }break;
        
        InvalidDefaultCase;
    }
}

static void
ui_PopLayout(ui_interface* Interface, gs_string LayoutName)
{
    Assert(Interface->ActiveLayout != 0);
    
    ui_widget* Layout = Interface->ActiveLayout;
    
    // NOTE(pjs): If this isn't true then a layout was opened without being closed
    // Go check for ui_PushLayout, ui_BeginDropdown, ui_BeginRow, etc that don't have
    // a corresponding ui_Pop/ui_End*
    Assert(StringsEqual(Layout->String, LayoutName));
    
    ui_ExpandToFitChildren(Layout);
    
    Interface->ActiveLayout = Interface->ActiveLayout->Parent;
    
    // NOTE(pjs): This indicates that the parent layout should
    // expand to fit the layout that we just popped
    if (Interface->ActiveLayout != 0 &&
        Interface->ActiveLayout->ChildrenHead == Layout)
    {
        ui_CommitBounds(Interface->ActiveLayout, Layout->Bounds);
    }
}

static ui_widget*
ui_BeginRow(ui_interface* Interface, u32 ColumnsMax)
{
    ui_widget* Layout = ui_PushLayout(Interface, MakeString("Row"), false);
    ui_WidgetCreateColumns(Layout, ColumnsMax, Interface);
    ui_WidgetInitUniformColumns(Layout);
    return Layout;
}

enum ui_column_size_rule
{
    UIColumnSize_Fixed,
    UIColumnSize_Percent,
    UIColumnSize_Fill,
    UIColumnSize_MaxWidth,
};

struct ui_column_spec
{
    ui_column_size_rule Rule;
    union
    {
        r32 Width;
        r32 Percent;
    };
};

static ui_widget*
ui_BeginRow(ui_interface* Interface, u32 ColumnsMax, ui_column_spec* ColumnRules)
{
    ui_widget* Layout = ui_PushLayout(Interface, MakeString("Row"), false);
    ui_WidgetCreateColumns(Layout, ColumnsMax, Interface);
    
    // First Pass, determine widths of each column, and how much space is left to be divided by the fill columns
    // If a size is specified, it is stored in Column->XMax
    r32 RowWidth = Rect2Width(Layout->Bounds);
    r32 RemainingSpace = RowWidth;
    u32 FillColumnsCount = 0;
    for (u32 i = 0; i < Layout->ColumnsCount; i++)
    {
        ui_column_spec Spec = ColumnRules[i];
        ui_column* Column = Layout->Columns + i;
        
        switch (Spec.Rule)
        {
            case UIColumnSize_Fixed:
            {
                Column->XMax = Spec.Width;
                RemainingSpace -= Column->XMax;
            }break;
            
            case UIColumnSize_Percent:
            {
                Column->XMax = Spec.Percent * RowWidth;
                RemainingSpace -= Column->XMax;
            }break;
            
            case UIColumnSize_Fill:
            {
                FillColumnsCount += 1;
            }break;
            
            case UIColumnSize_MaxWidth:
            {
                if (RemainingSpace >= Spec.Width)
                {
                    Column->XMax = Spec.Width;
                }
                else
                {
                    Column->XMax = RemainingSpace;
                }
                RemainingSpace -= Column->XMax;
            }break;
            
            InvalidDefaultCase;
        }
    }
    
    r32 FillColumnWidth = RemainingSpace / FillColumnsCount;
    
    // Second Pass, specify the actual XMin and XMax of each column
    r32 ColumnStartX = Layout->Bounds.Min.x;
    for (u32 i = 0; i < Layout->ColumnsCount; i++)
    {
        ui_column_spec Spec = ColumnRules[i];
        ui_column* Column = Layout->Columns + i;
        
        r32 ColumnWidth = 0;
        switch (Spec.Rule)
        {
            case UIColumnSize_Fixed:
            case UIColumnSize_Percent:
            case UIColumnSize_MaxWidth:
            {
                ColumnWidth = Column->XMax;
            }break;
            
            case UIColumnSize_Fill:
            {
                ColumnWidth = FillColumnWidth;
            }break;
        }
        
        Column->XMin = ColumnStartX ;
        Column->XMax = Column->XMin + Max(0, ColumnWidth);
        ColumnStartX = Column->XMax;
    }
    
    return Layout;
}

static void
ui_EndRow(ui_interface* Interface)
{
    ui_PopLayout(Interface, MakeString("Row"));
}

static rect2
ui_LayoutRemaining(ui_widget Layout)
{
    rect2 Result = Layout.Bounds;
    Result.Max.y = Layout.RowYAt;
    return Result;
}

// Widgets

internal ui_eval_result
ui_EvaluateWidget(ui_interface* Interface, ui_widget* Widget, rect2 Bounds)
{
    ui_eval_result Result = {};
    
    Widget->Bounds = Bounds;
    SLLPushOrInit(Interface->ActiveLayout->ChildrenRoot, Interface->ActiveLayout->ChildrenHead, Widget);
    Interface->ActiveLayout->ChildCount += 1;
    ui_CommitBounds(Widget->Parent, Widget->Bounds);
    
    if (PointIsInRect(Widget->Parent->Bounds, Interface->Mouse.Pos) &&
        PointIsInRect(Widget->Bounds, Interface->Mouse.Pos))
    {
        if (MouseButtonTransitionedDown(Interface->Mouse.LeftButtonState))
        {
            if (ui_WidgetIdsEqual(Interface->HotWidget, Widget->Id))
            {
                Result.Clicked = true;
                Interface->ActiveWidget = Widget->Id;
            }
        }
        
        if (Interface->HotWidget.ZIndex == 0 ||
            Interface->HotWidget.ZIndex <= Widget->Id.ZIndex)
        {
            Interface->HotWidget = Widget->Id;
            Interface->HotWidgetFramesSinceUpdate = 0;
        }
    }
    else
    {
        if (ui_WidgetIdsEqual(Interface->ActiveWidget, Widget->Id) &&
            MouseButtonTransitionedDown(Interface->Mouse.LeftButtonState))
        {
            Interface->ActiveWidget = {};
        }
    }
    
    if (ui_WidgetIdsEqual(Interface->ActiveWidget, Widget->Id))
    {
        // click & drag
        if (MouseButtonHeldDown(Interface->Mouse.LeftButtonState))
        {
            Result.Held = true;
            Result.DragDelta = Interface->Mouse.Pos - Interface->Mouse.DownPos;
        }
        
        if (ui_WidgetIsFlagSet(*Widget, UIWidgetFlag_Typable) &&
            Interface->TempInputString.Length > 0)
        {
            ui_widget_retained_state* State = ui_GetRetainedState(Interface, Widget->Id);
            
            for (u32 i = 0; i < Interface->TempInputString.Length; i++)
            {
                if (Interface->TempInputString.Str[i] == '\b' &&
                    State->EditString.Length > 0)
                {
                    State->EditString.Length -= 1;
                }
                else
                {
                    OutChar(&State->EditString, Interface->TempInputString.Str[i]);
                }
            }
        }
    }
    
#if 0
    // if you can click it
    if (ui_WidgetIsFlagSet(*Widget, UIWidgetFlag_Clickable))
    {
        // updating hot widget, and handling mouse clicks
        if (PointIsInRect(Widget->Parent->Bounds, Interface->Mouse.Pos) &&
            PointIsInRect(Widget->Bounds, Interface->Mouse.Pos))
        {
            if (ui_WidgetIdsEqual(Interface->HotWidget, Widget->Id) && MouseButtonTransitionedDown(Interface->Mouse.LeftButtonState))
            {
                Result.Clicked = true;
                Interface->ActiveWidget = Widget->Id;
            }
            
            Interface->HotWidget = Widget->Id;
        }
        
        // click and drag
        if (MouseButtonHeldDown(Interface->Mouse.LeftButtonState) &&
            PointIsInRect(Widget->Bounds, Interface->Mouse.DownPos))
        {
            Result.Held = true;
            Result.DragDelta = Interface->Mouse.Pos - Interface->Mouse.DownPos;
        }
        
        // if this is the active widget (its been clicked)
        if (ui_WidgetIdsEqual(Interface->ActiveWidget, Widget->Id))
        {
            // if you can select it
            if (ui_WidgetIsFlagSet(*Widget, UIWidgetFlag_Selectable))
            {
                //
                if (MouseButtonTransitionedDown(Interface->Mouse.LeftButtonState) &&
                    !PointIsInRect(Widget->Bounds, Interface->Mouse.Pos))
                {
                    Interface->ActiveWidget = {};
                }
                
                if (ui_WidgetIsFlagSet(*Widget, UIWidgetFlag_Typable) &&
                    Interface->TempInputString.Length > 0)
                {
                    ui_widget_retained_state* State = ui_GetRetainedState(Interface, Widget->Id);
                    
                    // TODO(pjs): Backspace?
                    for (u32 i = 0; i < Interface->TempInputString.Length; i++)
                    {
                        if (Interface->TempInputString.Str[i] == '\b')
                        {
                            State->EditString.Length -= 1;
                        }
                        else
                        {
                            OutChar(&State->EditString, Interface->TempInputString.Str[i]);
                        }
                    }
                }
            }
            else if (MouseButtonTransitionedUp(Interface->Mouse.LeftButtonState))
            {
                Interface->ActiveWidget = {};
            }
        }
    }
#endif
    
    Assert(Widget->Parent != 0);
    return Result;
}

internal ui_eval_result
ui_EvaluateWidget(ui_interface* Interface, ui_widget* Widget)
{
    ui_widget* Layout = Interface->ActiveLayout;
    rect2 Bounds = ui_ReserveBounds(Interface, Layout, true);
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
ui_Label(ui_interface* Interface, gs_string String, rect2 Bounds, gs_string_alignment Alignment = Align_Left)
{
    DEBUG_TRACK_FUNCTION;
    ui_widget* Widget = ui_CreateWidget(Interface, String);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawString);
    ui_EvaluateWidget(Interface, Widget, Bounds);
}

internal void
ui_Label(ui_interface* Interface, gs_string String, gs_string_alignment Alignment = Align_Left)
{
    DEBUG_TRACK_FUNCTION;
    ui_widget* Widget = ui_CreateWidget(Interface, String);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawString);
    ui_EvaluateWidget(Interface, Widget);
}

internal void
ui_TextEntrySetFlags(ui_widget* Widget, gs_string EditString)
{
    Widget->String = EditString;
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawString);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawBackground);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_Clickable);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_Selectable);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_Typable);
}

internal bool
ui_TextEntry(ui_interface* Interface, gs_string Identifier, gs_string* Value)
{
    ui_widget* Widget = ui_CreateWidget(Interface, Identifier);
    ui_widget_retained_state* State = ui_GetRetainedState(Interface, Widget->Id);
    if (!State)
    {
        State = ui_CreateRetainedState(Interface, Widget);
    }
    PrintF(&State->EditString, "%S", *Value);
    
    ui_TextEntrySetFlags(Widget, State->EditString);
    
    ui_eval_result Result = ui_EvaluateWidget(Interface, Widget);
    bool StringEdited = !StringsEqual(*Value, State->EditString);
    PrintF(Value, "%S", State->EditString);
    
    return StringEdited;
}

internal u64
ui_TextEntryU64(ui_interface* Interface, gs_string String, u64 CurrentValue)
{
    ui_widget* Widget = ui_CreateWidget(Interface, String);
    ui_widget_retained_state* State = ui_GetRetainedState(Interface, Widget->Id);
    if (!State)
    {
        State = ui_CreateRetainedState(Interface, Widget);
        PrintF(&State->EditString, "%u", CurrentValue);
    }
    ui_TextEntrySetFlags(Widget, State->EditString);
    
    ui_eval_result Result = ui_EvaluateWidget(Interface, Widget);
    parse_uint_result ParseResult = ValidateAndParseUInt(State->EditString.ConstString);
    u64 ValueResult = CurrentValue;
    if (ParseResult.Success)
    {
        ValueResult = ParseResult.Value;
    }
    return ValueResult;
}

internal r64
ui_TextEntryR64(ui_interface* Interface, gs_string String, r64 CurrentValue)
{
    ui_widget* Widget = ui_CreateWidget(Interface, String);
    ui_widget_retained_state* State = ui_GetRetainedState(Interface, Widget->Id);
    if (!State)
    {
        State = ui_CreateRetainedState(Interface, Widget);
        PrintF(&State->EditString, "%f", CurrentValue);
    }
    ui_TextEntrySetFlags(Widget, State->EditString);
    ui_eval_result Result = ui_EvaluateWidget(Interface, Widget);
    parse_float_result ParseResult = ValidateAndParseFloat(State->EditString.ConstString);
    r64 ValueResult = CurrentValue;
    if (ParseResult.Success)
    {
        ValueResult = ParseResult.Value;
    }
    return ValueResult;
}

internal ui_widget*
ui_CreateButtonWidget(ui_interface* Interface, gs_string Text)
{
    ui_widget* Widget = ui_CreateWidget(Interface, Text);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_Clickable);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawBackground);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawString);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawOutline);
    return Widget;
}

internal b32
ui_Button(ui_interface* Interface, gs_string Text)
{
    ui_widget* Widget = ui_CreateButtonWidget(Interface, Text);
    ui_eval_result Result = ui_EvaluateWidget(Interface, Widget);
    return Result.Clicked;
}

internal b32
ui_Button(ui_interface* Interface, gs_string Text, rect2 Bounds)
{
    ui_widget* Widget = ui_CreateButtonWidget(Interface, Text);
    ui_eval_result Result = ui_EvaluateWidget(Interface, Widget, Bounds);
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
    ui_widget* Widget = ui_CreateWidget(Interface, Text);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawBackground);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_Clickable);
    // TODO(pjs): Reimplement alternating color backgrounds
    Widget->Bounds = Bounds;
    ui_eval_result Result = ui_EvaluateWidget(Interface, Widget);
    return Result.Clicked;
}

static b32
ui_LayoutListButton(ui_interface* Interface, gs_string Text, u32 ListItemIndex)
{
    // TODO(pjs): Reimplement alternating colors
    return ui_Button(Interface, Text);
}

internal bool
ui_EvaluateDropdown(ui_interface* Interface, ui_widget* Widget, ui_eval_result EvalResult)
{
    ui_widget_retained_state* State = ui_GetRetainedState(Interface, Widget->Id);
    if (!State)
    {
        State = ui_CreateRetainedState(Interface, Widget);
    }
    
    if (EvalResult.Clicked)
    {
        State->Value = !State->Value;
    }
    
    if (State->Value)
    {
        ui_widget ParentLayout = *Interface->ActiveLayout;
        
        r32 SpaceAbove = Interface->WindowBounds.Max.y - Widget->Bounds.Max.y;
        r32 SpaceBelow = Widget->Bounds.Min.y - Interface->WindowBounds.Min.y;
        ui_layout_direction Direction = LayoutDirection_TopDown;
        rect2 MenuBounds = {};
        
        if (SpaceAbove > SpaceBelow)
        {
            r32 ParentLayoutMaxY = ParentLayout.Bounds.Max.y;
            Direction = LayoutDirection_BottomUp;
            MenuBounds = rect2{
                v2{ Widget->Bounds.Min.x - ParentLayout.Margin.x, Widget->Bounds.Max.y },
                v2{ Widget->Bounds.Max.x + ParentLayout.Margin.x, ParentLayoutMaxY }
            };
        }
        else
        {
            r32 ParentLayoutMinY = ParentLayout.Bounds.Min.y;
            Direction = LayoutDirection_TopDown;
            MenuBounds = rect2{
                v2{ Widget->Bounds.Min.x - ParentLayout.Margin.x, ParentLayoutMinY },
                v2{ Widget->Bounds.Max.x + ParentLayout.Margin.x, Widget->Bounds.Min.y }
            };
        }
        
        ui_widget* Layout = ui_PushOverlayLayout(Interface, MenuBounds, Direction, MakeString("DropdownLayout"));
        Layout->Margin.y = 0;
        Layout->WidgetReference = Widget->Id;
        ui_WidgetClearFlag(Layout, UIWidgetFlag_DrawOutline);
        ui_WidgetSetChildrenPopover(Layout);
    }
    
    return State->Value;
}

internal bool
ui_BeginDropdown(ui_interface* Interface, gs_string Text, rect2 Bounds)
{
    ui_widget* Widget = ui_CreateWidget(Interface, Text);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_Clickable);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawBackground);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawString);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawOutline);
    
    ui_eval_result Result = ui_EvaluateWidget(Interface, Widget, Bounds);
    return ui_EvaluateDropdown(Interface, Widget, Result);
}

internal bool
ui_BeginDropdown(ui_interface* Interface, gs_string Text)
{
    ui_widget* Widget = ui_CreateWidget(Interface, Text);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_Clickable);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawBackground);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawString);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawOutline);
    ui_eval_result Result = ui_EvaluateWidget(Interface, Widget);
    return ui_EvaluateDropdown(Interface, Widget, Result);
}

internal void
ui_EndDropdown(ui_interface* Interface)
{
    ui_widget* Layout = Interface->ActiveLayout;
    ui_widget_retained_state* State = ui_GetRetainedState(Interface, Layout->WidgetReference);
    if (State)
    {
        if (State->Value)
        {
            ui_PopLayout(Interface, MakeString("DropdownLayout"));
        }
    }
}

internal r32
ui_EvaluateRangeSlider(ui_interface* Interface, ui_widget* Widget, ui_eval_result EvalResult, r32 Value, r32 MinValue, r32 MaxValue)
{
    r32 NewValue = Value;
    ui_widget_retained_state* State = ui_GetOrCreateRetainedState(Interface, Widget);
    
    if (EvalResult.Clicked)
    {
        State->InitialValueR32 = Value;
    }
    
    if (EvalResult.Held)
    {
        r32 Percent = (Interface->Mouse.Pos.x - Widget->Bounds.Min.x) / Rect2Width(Widget->Bounds);
        NewValue = LerpR32(Percent, MinValue, MaxValue);
    }
    
    NewValue = Clamp(MinValue, NewValue, MaxValue);
    Widget->FillPercent = RemapR32(NewValue, MinValue, MaxValue, 0, 1);
    return NewValue;
}

internal ui_widget*
ui_CreateRangeSliderWidget(ui_interface* Interface, gs_string Text, r32 Value)
{
    ui_widget* Widget = ui_CreateWidget(Interface, Text);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_Clickable);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawBackground);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawString);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawHorizontalFill);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawOutline);
    Widget->String = PushStringF(Interface->PerFrameMemory, 128, "%f", Value);
    InterfaceAssert(Interface->PerFrameMemory);
    return Widget;
}

internal r32
ui_RangeSlider(ui_interface* Interface, gs_string Text, r32 Value, r32 ValueMin, r32 ValueMax)
{
    ui_widget* Widget = ui_CreateRangeSliderWidget(Interface, Text, Value);
    ui_eval_result Result = ui_EvaluateWidget(Interface, Widget);
    return ui_EvaluateRangeSlider(Interface, Widget, Result, Value, ValueMin, ValueMax);
    
}

internal r32
ui_RangeSlider(ui_interface* Interface, gs_string Text, rect2 Bounds, r32 Value, r32 ValueMin, r32 ValueMax)
{
    ui_widget* Widget = ui_CreateRangeSliderWidget(Interface, Text, Value);
    ui_eval_result Result = ui_EvaluateWidget(Interface, Widget, Bounds);
    return ui_EvaluateRangeSlider(Interface, Widget, Result, Value, ValueMin, ValueMax);
}

internal bool
ui_Toggle(ui_interface* Interface, gs_string Text, bool Value)
{
    ui_widget* Widget = ui_CreateWidget(Interface, Text);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_Clickable);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawBackground);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawHorizontalFill);
    ui_WidgetSetFlag(Widget, UIWidgetFlag_DrawOutline);
    ui_eval_result Eval = ui_EvaluateWidget(Interface, Widget);
    
    bool Result = Eval.Clicked ? !Value : Value;
    Widget->FillPercent = Result ? 1.0f : 0.0f;
    return Result;
}

internal void
ui_BeginList(ui_interface* Interface, gs_string Text, u32 ViewportRows, u32 ElementCount)
{
    if (ElementCount < ViewportRows)
    {
        ViewportRows = ElementCount;
    }
    
    ui_column_spec ColumnRules[] = {
        { UIColumnSize_Fixed, 32 },
        { UIColumnSize_Fill, 0 },
    };
    ui_widget* Layout = ui_BeginRow(Interface, 2, ColumnRules);
    ui_WidgetClearFlag(Layout, UIWidgetFlag_ExpandsToFitChildren);
    
    ui_widget_retained_state* State = ui_GetRetainedState(Interface, Layout->Id);
    if (!State)
    {
        State = ui_CreateRetainedState(Interface, Layout);
        State->InitialValueR32 = 1.0f;
    }
    
    r32 LayoutHeight = Layout->RowHeight * ViewportRows;
    switch (Layout->Parent->FillDirection)
    {
        case LayoutDirection_TopDown:
        {
            Layout->Bounds.Min.y = Layout->Bounds.Max.y - LayoutHeight;
        }break;
        
        case LayoutDirection_BottomUp:
        {
            Layout->Bounds.Max.y = Layout->Bounds.Min.y + LayoutHeight;
        }break;
        
        InvalidDefaultCase;
    }
    
    // Create the scroll bar
    //
    // TODO(pjs): Maybe make this a vertical slider widget?
    
    ui_widget* SliderRegion = ui_CreateWidget(Interface, MakeString("Slider"));
    ui_WidgetSetFlag(SliderRegion, UIWidgetFlag_DrawOutline);
    ui_WidgetSetFlag(SliderRegion, UIWidgetFlag_DrawVerticalFill);
    ui_WidgetSetFlag(SliderRegion, UIWidgetFlag_DrawFillAsHandle);
    
    ui_WidgetSetFlag(SliderRegion, UIWidgetFlag_Clickable);
    
    rect2 SliderBounds = ui_ReserveBounds(Interface, Layout, true);
    SliderBounds.Min.y = Layout->Bounds.Min.y + Layout->Margin.y;
    SliderBounds.Max.y = Layout->Bounds.Max.y - Layout->Margin.y;
    
    ui_eval_result SliderEval = ui_EvaluateWidget(Interface, SliderRegion, SliderBounds);
    if (SliderEval.Clicked || SliderEval.Held)
    {
        r32 Percent = (Interface->Mouse.Pos.y - SliderRegion->Bounds.Min.y) / Rect2Height(SliderRegion->Bounds);
        State->InitialValueR32 = Clamp01(Percent);
    }
    SliderRegion->FillPercent = State->InitialValueR32;
    
    // Create the viewport that offsets list contents (and at render time determines what is visible)
    //
    ui_widget* ViewportLayout = ui_PushLayout(Interface, MakeString("Contents"));
    ui_WidgetClearFlag(ViewportLayout, UIWidgetFlag_ExpandsToFitChildren);
    
    ViewportLayout->Bounds.Min.y = SliderBounds.Min.y;
    ViewportLayout->Bounds.Max.y = SliderBounds.Max.y;
    
    s32 ScrollableElements = Max(0, ElementCount - ViewportRows);
    ui_widget_retained_state* ViewportState = ui_GetOrCreateRetainedState(Interface, ViewportLayout);
    ViewportState->ChildrenDrawOffset.x = 0;
    ViewportState->ChildrenDrawOffset.y = ((1.0f - State->InitialValueR32) * (r32)(ScrollableElements)) * ViewportLayout->RowHeight;
}

internal void
ui_EndList(ui_interface* Interface)
{
    // Pop the Viewport Layout
    ui_PopLayout(Interface, MakeString("Contents"));
    // Pop the actual list layout
    ui_EndRow(Interface);
}

internal void
ui_BeginMousePopup(ui_interface* Interface, rect2 Bounds, ui_layout_direction FillDir, gs_string Text)
{
    rect2 FollowMouseBounds = Rect2Translate(Bounds, Interface->Mouse.Pos);
    ui_widget* Layout = ui_PushOverlayLayout(Interface, FollowMouseBounds, FillDir, MakeString("MousePopup"));
    ui_WidgetSetFlag(Layout, UIWidgetFlag_DrawBackground);
}

internal void
ui_EndMousePopup(ui_interface* Interface)
{
    ui_PopLayout(Interface, MakeString("MousePopup"));
}

//
internal void
ui_BeginLabelRow(ui_interface* Interface, gs_string Label, u32 Count = 2)
{
    ui_BeginRow(Interface, Count);
    ui_Label(Interface, Label);
}

internal bool
ui_LabeledToggle(ui_interface* Interface, gs_string Label, bool Value)
{
    ui_BeginLabelRow(Interface, Label);
    bool Result = ui_Toggle(Interface, Label, Value);
    ui_EndRow(Interface);
    return Result;
}

internal r32
ui_LabeledRangeSlider(ui_interface* Interface, gs_string Label, r32 Value, r32 ValueMin, r32 ValueMax)
{
    ui_BeginLabelRow(Interface, Label);
    r32 Result = ui_RangeSlider(Interface, Label, Value, ValueMin, ValueMax);
    ui_EndRow(Interface);
    return Result;
}

internal void
ui_LabeledTextEntry(ui_interface* Interface, gs_string Label, gs_string* Value)
{
    ui_BeginLabelRow(Interface, Label);
    ui_TextEntry(Interface, Label, Value);
    ui_EndRow(Interface);
}


internal u64
ui_LabeledTextEntryU64(ui_interface* Interface, gs_string Label, u32 Value)
{
    ui_BeginLabelRow(Interface, Label);
    u64 Result = ui_TextEntryU64(Interface, Label, Value);
    ui_EndRow(Interface);
    return Result;
}

internal bool
ui_BeginLabeledDropdown(ui_interface* Interface, gs_string Label, gs_string DropdownValue)
{
    ui_BeginLabelRow(Interface, Label);
    return ui_BeginDropdown(Interface, DropdownValue);
}

internal void
ui_EndLabeledDropdown(ui_interface* Interface)
{
    ui_EndDropdown(Interface);
    ui_EndRow(Interface);
}

internal ui_interface
ui_InterfaceCreate(context Context, interface_config Style, gs_memory_arena* Permanent)
{
    ui_interface Result = {0};
    Result.Style = Style;
    
    gs_file FontFile = ReadEntireFile(Context.ThreadContext.FileHandler, ConstString("data/Anonymous Pro.ttf"));
    Result.Style.Font = PushStruct(Permanent, bitmap_font);
    *Result.Style.Font = TextFont_Create(FontFile, 512, Style.FontSize, Context, Permanent);
    
    Result.Style.RowHeight = ui_GetTextLineHeight(Result) + (2 * Result.Style.Margin.y);
    
    Result.WidgetsCountMax = 4096;
    Result.Widgets = PushArray(Permanent, ui_widget, Result.WidgetsCountMax);
    Result.PerFrameMemory = PushStruct(Permanent, gs_memory_arena);
    *Result.PerFrameMemory = CreateMemoryArena(Context.ThreadContext.Allocator);
    InterfaceAssert(Result.PerFrameMemory);
    
    Result.Permanent = Permanent;
    
    return Result;
}

#define INTERFACE_H
#endif // INTERFACE_H