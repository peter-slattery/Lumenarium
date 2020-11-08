//
// File: foldhaus_interface.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_INTERFACE_CPP

////////////////////////////////////////
//
//     Panels
//
///////////////////////////////////////

enum panel_edit_mode
{
    PanelEdit_Invalid,
    
    PanelEdit_Modify,
    PanelEdit_Destroy,
    
    PanelEdit_Count,
};

//
// Drag Panel Border Operation Mode

OPERATION_STATE_DEF(drag_panel_border_operation_state)
{
    panel* Panel;
    
    // NOTE(Peter): InitialPanelBounds is the bounds of the panel we are modifying,
    // it stores the value calculated when the operation mode is kicked off.
    rect2 InitialPanelBounds;
    panel_split_direction PanelEdgeDirection;
    panel_edit_mode PanelEditMode;
};

OPERATION_RENDER_PROC(UpdateAndRenderDragPanelBorder)
{
    drag_panel_border_operation_state* OpState = (drag_panel_border_operation_state*)Operation.OpStateMemory;
    rect2 PanelBounds = OpState->InitialPanelBounds;
    
    if (OpState->PanelEditMode == PanelEdit_Modify)
    {
        v4 EdgePreviewColor = v4{.3f, .3f, .3f, 1.f};
        
        v2 EdgePreviewMin = {};
        v2 EdgePreviewMax = {};
        if (OpState->PanelEdgeDirection == PanelSplit_Horizontal)
        {
            EdgePreviewMin = v2{PanelBounds.Min.x, Mouse.Pos.y};
            EdgePreviewMax = v2{PanelBounds.Max.x, Mouse.Pos.y + 1};
        }
        else if (OpState->PanelEdgeDirection == PanelSplit_Vertical)
        {
            EdgePreviewMin = v2{Mouse.Pos.x, PanelBounds.Min.y};
            EdgePreviewMax = v2{Mouse.Pos.x + 1, PanelBounds.Max.y};
        }
        
        PushRenderQuad2D(RenderBuffer, EdgePreviewMin, EdgePreviewMax, EdgePreviewColor);
    }
    else if (OpState->PanelEditMode == PanelEdit_Destroy)
    {
        rect2 PanelToDeleteBounds = {};
        if (OpState->PanelEdgeDirection == PanelSplit_Horizontal)
        {
            r32 SplitY = LerpR32(OpState->Panel->SplitPercent, PanelBounds.Min.y, PanelBounds.Max.y);
            if (Mouse.Pos.y > SplitY)
            {
                PanelToDeleteBounds = GetTopPanelBounds(OpState->Panel, PanelBounds);
            }
            else
            {
                PanelToDeleteBounds = GetBottomPanelBounds(OpState->Panel, PanelBounds);
            }
        }
        else if (OpState->PanelEdgeDirection == PanelSplit_Vertical)
        {
            r32 SplitX = LerpR32(OpState->Panel->SplitPercent, PanelBounds.Min.x, PanelBounds.Max.x);
            if (Mouse.Pos.x > SplitX)
            {
                PanelToDeleteBounds = GetRightPanelBounds(OpState->Panel, PanelBounds);
            }
            else
            {
                PanelToDeleteBounds = GetLeftPanelBounds(OpState->Panel, PanelBounds);
            }
        }
        v4 OverlayColor = v4{0, 0, 0, .3f};
        PushRenderQuad2D(RenderBuffer, PanelToDeleteBounds.Min, PanelToDeleteBounds.Max, OverlayColor);
    }
}

FOLDHAUS_INPUT_COMMAND_PROC(EndDragPanelBorderOperation)
{
    drag_panel_border_operation_state* OpState = GetCurrentOperationState(State->Modes, drag_panel_border_operation_state);
    panel* Panel = OpState->Panel;
    rect2 PanelBounds = OpState->InitialPanelBounds;
    
    if (OpState->PanelEditMode == PanelEdit_Modify)
    {
        if (Panel->SplitDirection == PanelSplit_Horizontal)
        {
            r32 NewSplitY = Mouse.Pos.y;
            if (NewSplitY <= PanelBounds.Min.y)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Top, &State->PanelSystem);
            }
            else if (NewSplitY >= PanelBounds.Max.y)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Bottom, &State->PanelSystem);
            }
            else
            {
                Panel->SplitPercent = (NewSplitY  - PanelBounds.Min.y) / Rect2Height(PanelBounds);
                Panel_UpdateLayout(Panel, PanelBounds);
            }
        }
        else if (Panel->SplitDirection == PanelSplit_Vertical)
        {
            r32 NewSplitX = Mouse.Pos.x;
            if (NewSplitX <= PanelBounds.Min.x)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Right, &State->PanelSystem);
            }
            else if (NewSplitX >= PanelBounds.Max.x)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Left, &State->PanelSystem);
            }
            else
            {
                Panel->SplitPercent = (NewSplitX  - PanelBounds.Min.x) / Rect2Width(PanelBounds);
                Panel_UpdateLayout(Panel, PanelBounds);
            }
        }
    }
    else // PanelEdit_Destroy
    {
        if (OpState->PanelEdgeDirection == PanelSplit_Horizontal)
        {
            r32 SplitY = LerpR32(OpState->Panel->SplitPercent, PanelBounds.Min.y, PanelBounds.Max.y);
            if (Mouse.Pos.y > SplitY)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Bottom, &State->PanelSystem);
            }
            else
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Top, &State->PanelSystem);
            }
        }
        else if (OpState->PanelEdgeDirection == PanelSplit_Vertical)
        {
            r32 SplitX = LerpR32(OpState->Panel->SplitPercent, PanelBounds.Min.x, PanelBounds.Max.x);
            if (Mouse.Pos.x > SplitX)
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Left, &State->PanelSystem);
            }
            else
            {
                ConsolidatePanelsKeepOne(Panel, Panel->Right, &State->PanelSystem);
            }
        }
    }
    
    DeactivateCurrentOperationMode(&State->Modes);
}

input_command DragPanelBorderCommands[] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, EndDragPanelBorderOperation },
    { KeyCode_MouseRightButton, KeyCode_Invalid, Command_Ended, EndDragPanelBorderOperation },
};

internal void
BeginDragPanelBorder(panel* Panel, panel_edit_mode PanelEditMode, rect2 PanelBounds, panel_split_direction PanelEdgeDirection, mouse_state Mouse, app_state* State)
{
    operation_mode* DragPanelBorder = ActivateOperationModeWithCommands(&State->Modes, DragPanelBorderCommands, UpdateAndRenderDragPanelBorder);
    drag_panel_border_operation_state* OpState = CreateOperationState(DragPanelBorder, &State->Modes, drag_panel_border_operation_state);
    OpState->Panel = Panel;
    OpState->InitialPanelBounds = PanelBounds;
    OpState->PanelEdgeDirection = PanelEdgeDirection;
    OpState->PanelEditMode = PanelEditMode;
}

// ----------------

//
// Drag To Split Panel Operation


OPERATION_STATE_DEF(split_panel_operation_state)
{
    panel* Panel;
    
    // NOTE(Peter): InitialPanelBounds is the bounds of the panel we are modifying,
    // it stores the value calculated when the operation mode is kicked off.
    rect2 InitialPanelBounds;
};

OPERATION_RENDER_PROC(UpdateAndRenderSplitPanel)
{
    split_panel_operation_state* OpState = (split_panel_operation_state*)Operation.OpStateMemory;
    rect2 PanelBounds = OpState->InitialPanelBounds;
    v4 EdgePreviewColor = v4{.3f, .3f, .3f, 1.f};
    
    r32 MouseDeltaX = Abs(Mouse.Pos.x - Mouse.DownPos.x);
    r32 MouseDeltaY = Abs(Mouse.Pos.y - Mouse.DownPos.y);
    
    v2 EdgePreviewMin = {};
    v2 EdgePreviewMax = {};
    if (MouseDeltaY > MouseDeltaX) // Horizontal Split
    {
        EdgePreviewMin = v2{PanelBounds.Min.x, Mouse.Pos.y};
        EdgePreviewMax = v2{PanelBounds.Max.x, Mouse.Pos.y + 1};
    }
    else // Vertical Split
    {
        EdgePreviewMin = v2{Mouse.Pos.x, PanelBounds.Min.y};
        EdgePreviewMax = v2{Mouse.Pos.x + 1, PanelBounds.Max.y};
    }
    
    PushRenderQuad2D(RenderBuffer, EdgePreviewMin, EdgePreviewMax, EdgePreviewColor);
}

FOLDHAUS_INPUT_COMMAND_PROC(EndSplitPanelOperation)
{
    split_panel_operation_state* OpState = GetCurrentOperationState(State->Modes, split_panel_operation_state);
    panel* Panel = OpState->Panel;
    rect2 PanelBounds = OpState->InitialPanelBounds;
    
    r32 XDistance = Abs(Mouse.Pos.x - Mouse.DownPos.x);
    r32 YDistance = Abs(Mouse.Pos.y - Mouse.DownPos.y);
    
    if (XDistance > YDistance)
    {
        r32 XPercent = (Mouse.Pos.x - PanelBounds.Min.x) / Rect2Width(PanelBounds);
        SplitPanelVertically(Panel, XPercent, &State->PanelSystem, State, Context);
    }
    else
    {
        r32 YPercent = (Mouse.Pos.y - PanelBounds.Min.y) / Rect2Height(PanelBounds);
        SplitPanelHorizontally(Panel, YPercent, &State->PanelSystem, State, Context);
    }
    
    s32 PanelTypeIndex = Panel->TypeIndex;
    gs_data PanelStateMemory = Panel->StateMemory;
    Panel_SetCurrentType(Panel->Left, &State->PanelSystem, PanelTypeIndex, PanelStateMemory, State, Context);
    
    Panel_SetType(Panel->Right, &State->PanelSystem, PanelTypeIndex, State, Context);
    
    DeactivateCurrentOperationMode(&State->Modes);
}

input_command SplitPanelCommands[] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, EndSplitPanelOperation },
};

internal void
BeginSplitPanelOperation(panel* Panel, mouse_state Mouse, app_state* State)
{
    operation_mode* SplitPanel = ActivateOperationModeWithCommands(&State->Modes, SplitPanelCommands, UpdateAndRenderSplitPanel);
    split_panel_operation_state* OpState = CreateOperationState(SplitPanel, &State->Modes, split_panel_operation_state);
    OpState->Panel = Panel;
    OpState->InitialPanelBounds = Panel->Bounds;
}


// ----------------

#define PANEL_EDGE_CLICK_MAX_DISTANCE 6

internal b32
HandleMouseDownPanelInteractionOrRecurse(panel* Panel, panel_edit_mode PanelEditMode, mouse_state Mouse, app_state* State)
{
    b32 HandledMouseInput = false;
    
    // TODO(pjs): this can probably live in panel_with_layout
    rect2 PanelSplitButtonBounds = rect2{ Panel->Bounds.Min, Panel->Bounds.Min + v2{25, 25} };
    
    if (Panel->SplitDirection == PanelSplit_NoSplit
        && PointIsInRect(PanelSplitButtonBounds, Mouse.DownPos))
    {
        BeginSplitPanelOperation(Panel, Mouse, State);
        HandledMouseInput = true;
    }
    else if (Panel->SplitDirection != PanelSplit_NoSplit)
    {
        u32 ElementIndex = 0;
        switch(Panel->SplitDirection)
        {
            case PanelSplit_Vertical: { ElementIndex = 0; } break;
            case PanelSplit_Horizontal: { ElementIndex = 1; } break;
            InvalidDefaultCase;
        }
        
        r32 SplitPosition = LerpR32(Panel->SplitPercent, Panel->Bounds.Min.E[ElementIndex], Panel->Bounds.Max.E[ElementIndex]);
        r32 ClickDistanceFromSplit = Abs(Mouse.DownPos.E[ElementIndex] - SplitPosition);
        if (ClickDistanceFromSplit < PANEL_EDGE_CLICK_MAX_DISTANCE)
        {
            BeginDragPanelBorder(Panel, PanelEditMode, Panel->Bounds, Panel->SplitDirection, Mouse, State);
        }
        else
        {
            if (PointIsInRect(Panel->Bottom->Bounds, Mouse.DownPos))
            {
                HandleMouseDownPanelInteractionOrRecurse(Panel->Bottom, PanelEditMode, Mouse, State);
            }
            else if (PointIsInRect(Panel->Top->Bounds, Mouse.DownPos))
            {
                HandleMouseDownPanelInteractionOrRecurse(Panel->Top, PanelEditMode, Mouse, State);
            }
        }
    }
    
    return HandledMouseInput;
}

internal b32
HandleMousePanelInteraction(panel_system* PanelSystem, rect2 WindowBounds, mouse_state Mouse, app_state* State)
{
    b32 HandledMouseInput = false;
    
    panel* FirstPanel = PanelSystem->Panels + 0;
    panel_edit_mode EditMode = PanelEdit_Invalid;
    
    if (MouseButtonTransitionedDown(Mouse.LeftButtonState))
    {
        EditMode = PanelEdit_Modify;
    }
    else if (MouseButtonTransitionedDown(Mouse.RightButtonState))
    {
        EditMode = PanelEdit_Destroy;
    }
    
    if (EditMode != PanelEdit_Invalid)
    {
        HandledMouseInput = HandleMouseDownPanelInteractionOrRecurse(FirstPanel, EditMode, Mouse, State);
    }
    
    return HandledMouseInput;
}

internal void
DrawPanelBorder(panel Panel, v2 PanelMin, v2 PanelMax, v4 Color, mouse_state* Mouse, render_command_buffer* RenderBuffer)
{
    r32 MouseLeftEdgeDistance = Abs(Mouse->Pos.x - PanelMin.x);
    r32 MouseRightEdgeDistance = Abs(Mouse->Pos.x - PanelMax.x);
    r32 MouseTopEdgeDistance = Abs(Mouse->Pos.y - PanelMax.y);
    r32 MouseBottomEdgeDistance = Abs(Mouse->Pos.y - PanelMin.y);
    
    PushRenderBoundingBox2D(RenderBuffer, PanelMin, PanelMax, 1, Color);
    v4 HighlightColor = v4{.3f, .3f, .3f, 1.f};
    r32 HighlightThickness = 1;
    if (MouseLeftEdgeDistance < PANEL_EDGE_CLICK_MAX_DISTANCE)
    {
        v2 LeftEdgeMin = PanelMin;
        v2 LeftEdgeMax = v2{PanelMin.x + HighlightThickness, PanelMax.y};
        PushRenderQuad2D(RenderBuffer, LeftEdgeMin, LeftEdgeMax, HighlightColor);
        Mouse->CursorType = CursorType_HorizontalArrows;
    }
    else if (MouseRightEdgeDistance < PANEL_EDGE_CLICK_MAX_DISTANCE)
    {
        v2 RightEdgeMin = v2{PanelMax.x - HighlightThickness, PanelMin.y};
        v2 RightEdgeMax = PanelMax;
        PushRenderQuad2D(RenderBuffer, RightEdgeMin, RightEdgeMax, HighlightColor);
        Mouse->CursorType = CursorType_HorizontalArrows;
    }
    else if (MouseTopEdgeDistance < PANEL_EDGE_CLICK_MAX_DISTANCE)
    {
        v2 TopEdgeMin = v2{PanelMin.x, PanelMax.y - HighlightThickness};
        v2 TopEdgeMax = PanelMax;
        PushRenderQuad2D(RenderBuffer, TopEdgeMin, TopEdgeMax, HighlightColor);
        Mouse->CursorType = CursorType_VerticalArrows;
    }
    else if (MouseBottomEdgeDistance < PANEL_EDGE_CLICK_MAX_DISTANCE)
    {
        v2 BottomEdgeMin = PanelMin;
        v2 BottomEdgeMax = v2{PanelMax.x, PanelMin.y + HighlightThickness};
        PushRenderQuad2D(RenderBuffer, BottomEdgeMin, BottomEdgeMax, HighlightColor);
        Mouse->CursorType = CursorType_VerticalArrows;
    }
}

internal void
DrawPanelFooter(panel* Panel, render_command_buffer* RenderBuffer, rect2 FooterBounds, mouse_state Mouse, app_state* State, context Context)
{
    PushRenderQuad2D(RenderBuffer, FooterBounds.Min, v2{FooterBounds.Max.x, FooterBounds.Min.y + 25}, v4{.5f, .5f, .5f, 1.f});
    PushRenderQuad2D(RenderBuffer, FooterBounds.Min, FooterBounds.Min + v2{25, 25}, WhiteV4);
    
    rect2 PanelSelectBtnBounds = MakeRect2MinDim(FooterBounds.Min + v2{30, 1}, v2{100, 23});
    
    if (ui_BeginDropdown(&State->Interface, MakeString("Select"), PanelSelectBtnBounds))
    {
        for (s32 i = 0; i < GlobalPanelDefsCount; i++)
        {
            panel_definition Def = State->PanelSystem.PanelDefs[i];
            gs_string DefName = MakeString(Def.PanelName, Def.PanelNameLength);
            if (ui_Button(&State->Interface, DefName))
            {
                Panel_SetType(Panel, &State->PanelSystem, i, State, Context);
            }
        }
    }
    ui_EndDropdown(&State->Interface);
}

internal void
RenderPanel(panel* Panel, rect2 PanelBounds, rect2 WindowBounds, render_command_buffer* RenderBuffer, app_state* State, context Context, mouse_state Mouse)
{
    u32 PanelType = Panel->TypeIndex;
    Assert(PanelType >= 0);
    Assert(PanelType < State->PanelSystem.PanelDefsCount);
    
    rect2 FooterBounds = rect2{
        PanelBounds.Min,
        v2{PanelBounds.Max.x, PanelBounds.Min.y + 25},
    };
    rect2 PanelViewBounds = rect2{
        v2{PanelBounds.Min.x, FooterBounds.Max.y},
        PanelBounds.Max,
    };
    
    panel_definition Definition = State->PanelSystem.PanelDefs[PanelType];
    Definition.Render(Panel, PanelViewBounds, RenderBuffer, State, Context);
    
    PushRenderOrthographic(RenderBuffer, WindowBounds);
    DrawPanelFooter(Panel, RenderBuffer, FooterBounds, Mouse, State, Context);
}

internal void
DrawPanelRecursive(panel* Panel, render_command_buffer* RenderBuffer, mouse_state* Mouse, app_state* State, context Context)
{
    switch (Panel->SplitDirection)
    {
        case PanelSplit_Horizontal:
        case PanelSplit_Vertical:
        {
            DrawPanelRecursive(Panel->Left, RenderBuffer, Mouse, State, Context);
            DrawPanelRecursive(Panel->Right, RenderBuffer, Mouse, State, Context);
        }break;
        
        case PanelSplit_NoSplit:
        {
            panel* OverridePanel = Panel_GetModalOverride(Panel);
            RenderPanel(OverridePanel, OverridePanel->Bounds, State->WindowBounds, RenderBuffer, State, Context, *Mouse);
            v4 BorderColor = v4{0, 0, 0, 1};
            
            PushRenderOrthographic(RenderBuffer, State->WindowBounds);
            DrawPanelBorder(*OverridePanel, OverridePanel->Bounds.Min, OverridePanel->Bounds.Max, BorderColor, Mouse, RenderBuffer);
        }break;
        
        InvalidDefaultCase;
    }
}

internal void
DrawAllPanels(panel_system System, render_command_buffer* RenderBuffer, mouse_state* Mouse, app_state* State, context Context)
{
    panel* PanelAt = System.Panels + 0;
    DrawPanelRecursive(PanelAt, RenderBuffer, Mouse, State, Context);
}

#define FOLDHAUS_INTERFACE_CPP
#endif // FOLDHAUS_INTERFACE_CPP