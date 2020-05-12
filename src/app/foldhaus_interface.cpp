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
    PanelEdit_Modify,
    PanelEdit_Destroy,
    
    PanelEdit_Count,
};

internal void
SetPanelDefinition(panel* Panel, s32 NewPanelDefinitionIndex, app_state* State)
{
    s32 OldPanelDefinitionIndex = Panel->PanelDefinitionIndex;
    Panel->PanelDefinitionIndex = NewPanelDefinitionIndex;
    
    if(OldPanelDefinitionIndex >= 0)
    {
        GlobalPanelDefs[OldPanelDefinitionIndex].Cleanup(Panel, State);
    }
    GlobalPanelDefs[NewPanelDefinitionIndex].Init(Panel, State);
}

//
// Drag Panel Border Operation Mode

OPERATION_STATE_DEF(drag_panel_border_operation_state)
{
    panel* Panel;
    
    // NOTE(Peter): InitialPanelBounds is the bounds of the panel we are modifying,
    // it stores the value calculated when the operation mode is kicked off.
    rect InitialPanelBounds;
    panel_split_direction PanelEdgeDirection;
    panel_edit_mode PanelEditMode;
};

OPERATION_RENDER_PROC(UpdateAndRenderDragPanelBorder)
{
    drag_panel_border_operation_state* OpState = (drag_panel_border_operation_state*)Operation.OpStateMemory;
    rect PanelBounds = OpState->InitialPanelBounds;
    
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
        rect PanelToDeleteBounds = {};
        if (OpState->PanelEdgeDirection == PanelSplit_Horizontal)
        {
            r32 SplitY = GSLerp(PanelBounds.Min.y, PanelBounds.Max.y, OpState->Panel->SplitPercent);
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
            r32 SplitX = GSLerp(PanelBounds.Min.x, PanelBounds.Max.x, OpState->Panel->SplitPercent);
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
    rect PanelBounds = OpState->InitialPanelBounds;
    
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
                Panel->SplitPercent = (NewSplitY  - PanelBounds.Min.y) / gs_Height(PanelBounds);
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
                Panel->SplitPercent = (NewSplitX  - PanelBounds.Min.x) / gs_Width(PanelBounds);
            }
        }
    }
    else // PanelEdit_Destroy
    {
        if (OpState->PanelEdgeDirection == PanelSplit_Horizontal)
        {
            r32 SplitY = GSLerp(PanelBounds.Min.y, PanelBounds.Max.y, OpState->Panel->SplitPercent);
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
            r32 SplitX = GSLerp(PanelBounds.Min.x, PanelBounds.Max.x, OpState->Panel->SplitPercent);
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
BeginDragPanelBorder(panel* Panel, panel_edit_mode PanelEditMode, rect PanelBounds, panel_split_direction PanelEdgeDirection, mouse_state Mouse, app_state* State)
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
    rect InitialPanelBounds;
};

OPERATION_RENDER_PROC(UpdateAndRenderSplitPanel)
{
    split_panel_operation_state* OpState = (split_panel_operation_state*)Operation.OpStateMemory;
    rect PanelBounds = OpState->InitialPanelBounds;
    v4 EdgePreviewColor = v4{.3f, .3f, .3f, 1.f};
    
    r32 MouseDeltaX = GSAbs(Mouse.Pos.x - Mouse.DownPos.x);
    r32 MouseDeltaY = GSAbs(Mouse.Pos.y - Mouse.DownPos.y);
    
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
    rect PanelBounds = OpState->InitialPanelBounds;
    
    r32 XDistance = GSAbs(Mouse.Pos.x - Mouse.DownPos.x);
    r32 YDistance = GSAbs(Mouse.Pos.y - Mouse.DownPos.y);
    
    if (XDistance > YDistance)
    {
        r32 XPercent = (Mouse.Pos.x - PanelBounds.Min.x) / gs_Width(PanelBounds);
        SplitPanelVertically(Panel, XPercent, PanelBounds, &State->PanelSystem);
    }
    else
    {
        r32 YPercent = (Mouse.Pos.y - PanelBounds.Min.y) / gs_Height(PanelBounds);
        SplitPanelHorizontally(Panel, YPercent, PanelBounds, &State->PanelSystem);
    }
    
    Panel->Left->Panel.PanelDefinitionIndex = Panel->PanelDefinitionIndex;
    Panel->Left->Panel.PanelStateMemory = Panel->PanelStateMemory;
    Panel->Left->Panel.PanelStateMemorySize = Panel->PanelStateMemorySize;
    
    SetPanelDefinition(&Panel->Right->Panel, Panel->PanelDefinitionIndex, State);
    
    DeactivateCurrentOperationMode(&State->Modes);
}

input_command SplitPanelCommands[] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, EndSplitPanelOperation },
};

internal void
BeginSplitPanelOperation(panel* Panel, rect PanelBounds, mouse_state Mouse, app_state* State)
{
    operation_mode* SplitPanel = ActivateOperationModeWithCommands(&State->Modes, SplitPanelCommands, UpdateAndRenderSplitPanel);
    split_panel_operation_state* OpState = CreateOperationState(SplitPanel, &State->Modes, split_panel_operation_state);
    OpState->Panel = Panel;
    OpState->InitialPanelBounds = PanelBounds;
}


// ----------------

#define PANEL_EDGE_CLICK_MAX_DISTANCE 6

internal b32
HandleMouseDownPanelInteractionOrRecurse(panel* Panel, panel_edit_mode PanelEditMode, rect PanelBounds, mouse_state Mouse, app_state* State)
{
    b32 HandledMouseInput = false;
    
    if (Panel->SplitDirection == PanelSplit_NoSplit
        && PointIsInRange(Mouse.DownPos, PanelBounds.Min, PanelBounds.Min + v2{25, 25}))
    {
        BeginSplitPanelOperation(Panel, PanelBounds, Mouse, State);
        HandledMouseInput = true;
    }
    else if (Panel->SplitDirection == PanelSplit_Horizontal)
    {
        r32 SplitY = GSLerp(PanelBounds.Min.y, PanelBounds.Max.y, Panel->SplitPercent);
        r32 ClickDistanceFromSplit = GSAbs(Mouse.DownPos.y - SplitY);
        if (ClickDistanceFromSplit < PANEL_EDGE_CLICK_MAX_DISTANCE)
        {
            BeginDragPanelBorder(Panel, PanelEditMode, PanelBounds, PanelSplit_Horizontal, Mouse, State);
            HandledMouseInput = true;
        }
        else
        {
            rect TopPanelBounds = GetTopPanelBounds(Panel, PanelBounds);
            rect BottomPanelBounds = GetBottomPanelBounds(Panel, PanelBounds);
            if (gs_PointIsInRect(Mouse.DownPos, BottomPanelBounds))
            {
                HandleMouseDownPanelInteractionOrRecurse(&Panel->Bottom->Panel, PanelEditMode, BottomPanelBounds, Mouse, State);
            }
            if (gs_PointIsInRect(Mouse.DownPos, TopPanelBounds))
            {
                HandleMouseDownPanelInteractionOrRecurse(&Panel->Top->Panel, PanelEditMode, TopPanelBounds, Mouse, State);
            }
        }
    }
    else if (Panel->SplitDirection == PanelSplit_Vertical)
    {
        r32 SplitX = GSLerp(PanelBounds.Min.x, PanelBounds.Max.x, Panel->SplitPercent);
        r32 ClickDistanceFromSplit = GSAbs(Mouse.DownPos.x - SplitX);
        if (ClickDistanceFromSplit < PANEL_EDGE_CLICK_MAX_DISTANCE)
        {
            BeginDragPanelBorder(Panel, PanelEditMode, PanelBounds, PanelSplit_Vertical, Mouse, State);
            HandledMouseInput = true;
        }
        else
        {
            rect LeftPanelBounds = GetLeftPanelBounds(Panel, PanelBounds);
            rect RightPanelBounds = GetRightPanelBounds(Panel, PanelBounds);
            if (gs_PointIsInRect(Mouse.DownPos, LeftPanelBounds))
            {
                HandleMouseDownPanelInteractionOrRecurse(&Panel->Left->Panel, PanelEditMode, LeftPanelBounds, Mouse, State);
            }
            if (gs_PointIsInRect(Mouse.DownPos, RightPanelBounds))
            {
                HandleMouseDownPanelInteractionOrRecurse(&Panel->Right->Panel, PanelEditMode, RightPanelBounds, Mouse, State);
            }
        }
    }
    
    return HandledMouseInput;
}

internal b32
HandleMousePanelInteraction(panel_system* PanelSystem, rect WindowBounds, mouse_state Mouse, app_state* State)
{
    b32 HandledMouseInput = false;
    
    panel* FirstPanel = &PanelSystem->Panels[0].Panel;
    if (MouseButtonTransitionedDown(Mouse.LeftButtonState))
    {
        HandledMouseInput = HandleMouseDownPanelInteractionOrRecurse(FirstPanel, PanelEdit_Modify, WindowBounds, Mouse, State);
    }
    else if (MouseButtonTransitionedDown(Mouse.RightButtonState))
    {
        HandledMouseInput = HandleMouseDownPanelInteractionOrRecurse(FirstPanel, PanelEdit_Destroy, WindowBounds, Mouse, State);
    }
    
    return HandledMouseInput;
}

internal void
DrawPanelBorder(panel Panel, v2 PanelMin, v2 PanelMax, v4 Color, mouse_state* Mouse, render_command_buffer* RenderBuffer)
{
    r32 MouseLeftEdgeDistance = GSAbs(Mouse->Pos.x - PanelMin.x);
    r32 MouseRightEdgeDistance = GSAbs(Mouse->Pos.x - PanelMax.x);
    r32 MouseTopEdgeDistance = GSAbs(Mouse->Pos.y - PanelMax.y);
    r32 MouseBottomEdgeDistance = GSAbs(Mouse->Pos.y - PanelMin.y);
    
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
DrawPanelFooter(panel* Panel, render_command_buffer* RenderBuffer, rect FooterBounds, mouse_state Mouse, app_state* State)
{
    PushRenderQuad2D(RenderBuffer, FooterBounds.Min, v2{FooterBounds.Max.x, FooterBounds.Min.y + 25}, v4{.5f, .5f, .5f, 1.f});
    PushRenderQuad2D(RenderBuffer, FooterBounds.Min, FooterBounds.Min + v2{25, 25}, WhiteV4);
    
    rect PanelSelectBtnBounds = gs_MakeRectMinWidth(FooterBounds.Min + v2{30, 1}, v2{100, 23});
    
    if (Panel->PanelSelectionMenuOpen)
    {
        rect ButtonBounds = gs_MakeRectMinWidth(v2{ PanelSelectBtnBounds.Min.x, FooterBounds.Max.y }, v2{ 100, 25 });
        
        v2 MenuMin = ButtonBounds.Min;
        v2 MenuMax = v2{ButtonBounds.Min.x + gs_Width(ButtonBounds), ButtonBounds.Min.y + (gs_Height(ButtonBounds) * GlobalPanelDefsCount)};
        if (MouseButtonTransitionedDown(Mouse.LeftButtonState)
            && !PointIsInRange(Mouse.DownPos, MenuMin, MenuMax))
        {
            Panel->PanelSelectionMenuOpen = false;
        }
        
        
        for (s32 i = 0; i < GlobalPanelDefsCount; i++)
        {
            panel_definition Def = GlobalPanelDefs[i];
            string DefName = MakeString(Def.PanelName, Def.PanelNameLength);
            if (ui_Button(&State->Interface_, DefName, ButtonBounds))
            {
                SetPanelDefinition(Panel, i, State);
                Panel->PanelSelectionMenuOpen = false;
            }
            
            ButtonBounds = gs_TranslateRectY(ButtonBounds, gs_Height(ButtonBounds));
        }
    }
    
    if (ui_Button(&State->Interface_, MakeStringLiteral("Select"), PanelSelectBtnBounds))
    {
        Panel->PanelSelectionMenuOpen = !Panel->PanelSelectionMenuOpen;
    }
    
}

internal void
RenderPanel(panel* Panel, rect PanelBounds, rect WindowBounds, render_command_buffer* RenderBuffer, app_state* State, context Context, mouse_state Mouse)
{
    Assert(Panel->PanelDefinitionIndex >= 0);
    
    rect FooterBounds = rect{
        PanelBounds.Min,
        v2{PanelBounds.Max.x, PanelBounds.Min.y + 25},
    };
    rect PanelViewBounds = rect{
        v2{PanelBounds.Min.x, FooterBounds.Max.y},
        PanelBounds.Max,
    };
    
    panel_definition Definition = GlobalPanelDefs[Panel->PanelDefinitionIndex];
    Definition.Render(*Panel, PanelViewBounds, RenderBuffer, State, Context, Mouse);
    
    PushRenderOrthographic(RenderBuffer, WindowBounds.Min.x, WindowBounds.Min.y, WindowBounds.Max.x, WindowBounds.Max.y);
    DrawPanelFooter(Panel, RenderBuffer, FooterBounds, Mouse, State);
}

internal void
DrawAllPanels(panel_layout PanelLayout, render_command_buffer* RenderBuffer, mouse_state* Mouse, app_state* State, context Context)
{
    for (u32 i = 0; i < PanelLayout.PanelsCount; i++)
    {
        panel_with_layout PanelWithLayout = PanelLayout.Panels[i];
        panel* Panel = PanelWithLayout.Panel;
        rect PanelBounds = PanelWithLayout.Bounds;
        
        RenderPanel(Panel, PanelBounds, State->WindowBounds, RenderBuffer, State, Context, *Mouse);
        v4 BorderColor = v4{0, 0, 0, 1};
        
        PushRenderOrthographic(RenderBuffer, State->WindowBounds.Min.x, State->WindowBounds.Min.y, State->WindowBounds.Max.x, State->WindowBounds.Max.y);
        DrawPanelBorder(*Panel, PanelBounds.Min, PanelBounds.Max, BorderColor, Mouse, RenderBuffer);
    }
}


#define FOLDHAUS_INTERFACE_CPP
#endif // FOLDHAUS_INTERFACE_CPP