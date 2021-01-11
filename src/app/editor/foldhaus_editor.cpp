//
// File: foldhaus_editor.cpp
// Author: Peter Slattery
// Creation Date: 2020-10-24
//
#ifndef FOLDHAUS_EDITOR_CPP

internal void
Editor_HandleInput (app_state* State, rect2 WindowBounds, input_queue InputQueue, mouse_state Mouse, context Context)
{
    DEBUG_TRACK_FUNCTION;
    
    b32 MouseInputHandled = HandleMousePanelInteraction(&State->PanelSystem, State->WindowBounds, Mouse, State);
    
    gs_string TextInputString = PushString(State->Transient, 32);
    
    panel* ActivePanel = PanelSystem_GetPanelContainingPoint(&State->PanelSystem, Mouse.Pos);
    if (ActivePanel)
    {
        panel_definition ActiveDef = State->PanelSystem.PanelDefs[ActivePanel->TypeIndex];
        
        input_command_registry ActiveCommands = {};
        if (State->Modes.ActiveModesCount > 0)
        {
            ActiveCommands = State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1].Commands;
        }
        else if (ActiveDef.InputCommands)
        {
            ActiveCommands.Commands = ActiveDef.InputCommands;
            ActiveCommands.Size = sizeof(*ActiveDef.InputCommands) / sizeof(ActiveDef.InputCommands[0]);
            ActiveCommands.Used = ActiveCommands.Size;
        }
        
        // Fill up the command queue
        for (s32 EventIdx = 0; EventIdx < InputQueue.QueueUsed; EventIdx++)
        {
            input_entry Event = InputQueue.Entries[EventIdx];
            
            bool IsMouseEvent = (Event.Key == KeyCode_MouseLeftButton ||
                                 Event.Key == KeyCode_MouseMiddleButton ||
                                 Event.Key == KeyCode_MouseRightButton);
            if (IsMouseEvent && MouseInputHandled)
            {
                continue;
            }
            
            // NOTE(Peter): These are in the order Down, Up, Held because we want to privalege
            // Down and Up over Held. In other words, we don't want to call a Held command on the
            // frame when the button was released, even if the command is registered to both events
            if (KeyTransitionedDown(Event))
            {
                if (!FindAndPushExistingCommand(ActiveCommands, Event, Command_Began, &State->CommandQueue))
                {
                    char KeyASCII = KeyCodeToChar(Event.Key);
                    if (KeyASCII)
                    {
                        OutChar(&TextInputString, KeyASCII);
                    }
                }
            }
            else if (KeyTransitionedUp(Event))
            {
                FindAndPushExistingCommand(ActiveCommands, Event, Command_Ended, &State->CommandQueue);
            }
            else if (KeyHeldDown(Event))
            {
                if (!FindAndPushExistingCommand(ActiveCommands, Event, Command_Held, &State->CommandQueue))
                {
                    char KeyASCII = KeyCodeToChar(Event.Key);
                    if (KeyASCII)
                    {
                        OutChar(&TextInputString, KeyASCII);
                    }
                }
            }
        }
    }
    
    // Execute all commands in CommandQueue
    for (s32 CommandIdx = State->CommandQueue.Used - 1; CommandIdx >= 0; CommandIdx--)
    {
        command_queue_entry* Entry = &State->CommandQueue.Commands[CommandIdx];
        if (Entry->Command.Proc)
        {
            Entry->Command.Proc(State, Entry->Event, Mouse, Context, ActivePanel);
        }
        else
        {
            EndCurrentOperationMode(State);
        }
    }
    
    State->Interface.TempInputString = TextInputString.ConstString;
    
    ClearCommandQueue(&State->CommandQueue);
}

internal void
Editor_Update(app_state* State, context* Context, input_queue InputQueue)
{
    Context->Mouse.CursorType = CursorType_Arrow;
    State->WindowBounds = Context->WindowBounds;
    State->Interface.Mouse = Context->Mouse;
    
    State->Interface.HotWidgetFramesSinceUpdate += 1;
    if (State->Interface.HotWidgetFramesSinceUpdate > 1)
    {
        State->Interface.HotWidget = {};
    }
    
    PanelSystem_UpdateLayout(&State->PanelSystem, State->WindowBounds);
    Editor_HandleInput(State, State->WindowBounds, InputQueue, Context->Mouse, *Context);
}

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
            
            rect2 FillBounds = {};
            if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawHorizontalFill))
            {
                FillBounds.Min.y = Widget.Bounds.Min.y;
                FillBounds.Max.y = Widget.Bounds.Max.y;
                r32 FillToPoint = LerpR32(Widget.FillPercent, Widget.Bounds.Min.x, Widget.Bounds.Max.x);
                if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawFillReversed))
                {
                    FillBounds.Min.x = FillToPoint;
                    FillBounds.Max.x = Widget.Bounds.Max.x;
                }
                else if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawFillAsHandle))
                {
                    FillBounds.Min.x = FillToPoint - 5;
                    FillBounds.Max.x = FillToPoint + 5;
                }
                else
                {
                    FillBounds.Min.x = Widget.Bounds.Min.x;
                    FillBounds.Max.x = FillToPoint;
                }
            }
            else if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawVerticalFill))
            {
                FillBounds.Min.x = Widget.Bounds.Min.x;
                FillBounds.Max.x = Widget.Bounds.Max.x;
                r32 FillToPoint = LerpR32(Widget.FillPercent, Widget.Bounds.Min.y, Widget.Bounds.Max.y);
                if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawFillReversed))
                {
                    FillBounds.Min.y = FillToPoint;
                    FillBounds.Max.y = Widget.Bounds.Max.y;
                }
                else if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawFillAsHandle))
                {
                    FillBounds.Min.y = FillToPoint - 5;
                    FillBounds.Max.y = FillToPoint + 5;
                }
                else
                {
                    FillBounds.Min.y = Widget.Bounds.Min.y;
                    FillBounds.Max.y = FillToPoint;
                }
            }
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

#include "../interface_test.cpp"

FOLDHAUS_INPUT_COMMAND_PROC(ActiveWidget_TypeCharacter)
{
    ui_widget* ActiveWidget = ui_InterfaceGetWidgetWithId(&State->Interface, State->Interface.ActiveWidget);
    ui_widget_retained_state* WidgetState = ui_GetRetainedState(&State->Interface, ActiveWidget->Id);
    if (WidgetState)
    {
        char AsciiValue = CharacterFromKeyCode(Event.Key);
        if (AsciiValue)
        {
            OutChar(&WidgetState->EditString, AsciiValue);
        }
    }
}

FOLDHAUS_INPUT_COMMAND_PROC(ActiveWidget_DeleteBackwards)
{
    ui_widget* ActiveWidget = ui_InterfaceGetWidgetWithId(&State->Interface, State->Interface.ActiveWidget);
    ui_widget_retained_state* WidgetState = ui_GetRetainedState(&State->Interface, ActiveWidget->Id);
    if (WidgetState)
    {
        WidgetState->EditString.Length -= 1;
    }
}

FOLDHAUS_INPUT_COMMAND_PROC(ActiveWidget_EndTypingMode)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

OPERATION_RENDER_PROC(ActiveWidget_EndTypingMode)
{
    ui_widget* ActiveWidget = ui_InterfaceGetWidgetWithId(&State->Interface, State->Interface.ActiveWidget);
    ui_widget* LastActiveWidget = ui_InterfaceGetWidgetWithId(&State->Interface, State->Interface.LastActiveWidget);
    if (ActiveWidget == 0 && LastActiveWidget != 0)
    {
        // if there was an active widget last frame that was typable, we want to deactivate the typing mode
        DeactivateCurrentOperationMode(&State->Modes);
    }
}

input_command InterfaceTypingCommands [] = {
    { KeyCode_A, KeyCode_Invalid, Command_Began | Command_Held, ActiveWidget_TypeCharacter },
    { KeyCode_B, KeyCode_Invalid, Command_Began | Command_Held, ActiveWidget_TypeCharacter },
    { KeyCode_C, KeyCode_Invalid, Command_Began | Command_Held, ActiveWidget_TypeCharacter },
    { KeyCode_D, KeyCode_Invalid, Command_Began | Command_Held, ActiveWidget_TypeCharacter },
    { KeyCode_E, KeyCode_Invalid, Command_Began | Command_Held, ActiveWidget_TypeCharacter },
    { KeyCode_Enter, KeyCode_Invalid, Command_Began, ActiveWidget_EndTypingMode },
    { KeyCode_Backspace, KeyCode_Invalid, Command_Began | Command_Held, ActiveWidget_DeleteBackwards },
};

internal void
Editor_Render(app_state* State, context* Context, render_command_buffer* RenderBuffer)
{
    State->Interface.WindowBounds = Context->WindowBounds;
    PushRenderOrthographic(RenderBuffer, State->WindowBounds);
    PushRenderClearScreen(RenderBuffer);
    
#if 0
    InterfaceTest_Render(State, Context, RenderBuffer);
#else
    ui_InterfaceReset(&State->Interface);
    State->Interface.RenderBuffer = RenderBuffer;
    ui_PushLayout(&State->Interface, Context->WindowBounds, LayoutDirection_TopDown, MakeString("Editor Layout"));
    
    DrawAllPanels(State->PanelSystem, RenderBuffer, &Context->Mouse, State, *Context);
    
    for (s32 m = 0; m < State->Modes.ActiveModesCount; m++)
    {
        operation_mode OperationMode = State->Modes.ActiveModes[m];
        if (OperationMode.Render != 0)
        {
            OperationMode.Render(State, RenderBuffer, OperationMode, Context->Mouse, *Context);
        }
    }
    
    ui_PopLayout(&State->Interface);
#endif
    
    // Draw the Interface
    if (State->Interface.DrawOrderRoot != 0)
    {
        ui_widget Widget = *State->Interface.DrawOrderRoot;
        Editor_DrawWidget(State, Context, RenderBuffer, Widget, Context->WindowBounds);
        
#if 0
        // TODO(pjs): got distracted halfway through getting typing input into the interface
        if (ui_WidgetIdsEqual(State->Interface.ActiveWidget, State->Interface.LastActiveWidget))
        {
            ui_widget* ActiveWidget = ui_InterfaceGetWidgetWithId(&State->Interface, State->Interface.ActiveWidget);
            if (ActiveWidget != 0 &&
                ui_WidgetIsFlagSet(*ActiveWidget, UIWidgetFlag_Typable))
            {
                operation_mode* TypingMode = ActivateOperationModeWithCommands(&State->Modes, InterfaceTypingCommands, ActiveWidget_EndTypingMode);
            }
        }
#endif
    }
    
    Context->GeneralWorkQueue->CompleteQueueWork(Context->GeneralWorkQueue, Context->ThreadContext);
    Context->GeneralWorkQueue->ResetWorkQueue(Context->GeneralWorkQueue);
    
}


#define FOLDHAUS_EDITOR_CPP
#endif // FOLDHAUS_EDITOR_CPP