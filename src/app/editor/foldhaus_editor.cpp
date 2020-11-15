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
                FindAndPushExistingCommand(ActiveCommands, Event, Command_Began, &State->CommandQueue);
            }
            else if (KeyTransitionedUp(Event))
            {
                FindAndPushExistingCommand(ActiveCommands, Event, Command_Ended, &State->CommandQueue);
            }
            else if (KeyHeldDown(Event))
            {
                FindAndPushExistingCommand(ActiveCommands, Event, Command_Held, &State->CommandQueue);
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
    
    ClearCommandQueue(&State->CommandQueue);
}

internal void
Editor_Update(app_state* State, context* Context, input_queue InputQueue)
{
    Context->Mouse.CursorType = CursorType_Arrow;
    State->WindowBounds = Context->WindowBounds;
    State->Interface.Mouse = Context->Mouse;
    
    PanelSystem_UpdateLayout(&State->PanelSystem, State->WindowBounds);
    Editor_HandleInput(State, State->WindowBounds, InputQueue, Context->Mouse, *Context);
}

internal void
Editor_DrawWidgetString(app_state* State, context* Context, render_command_buffer* RenderBuffer, ui_widget Widget, v4 Color)
{
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
            RegisterPosition = DrawStringLeftAligned(&BatchConstructor, StringExpand(Widget.String), RegisterPosition, State->Interface.Style.Font, Color);
        }break;
        
        case Align_Right:
        {
            RegisterPosition = DrawStringRightAligned(&BatchConstructor, StringExpand(Widget.String), RegisterPosition, State->Interface.Style.Font, Color);
        }break;
        
        InvalidDefaultCase;
    }
}

internal void
Editor_DrawWidget(app_state* State, context* Context, render_command_buffer* RenderBuffer, ui_widget Widget)
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
        PushRenderQuad2D(RenderBuffer, Widget.Bounds.Min, Widget.Bounds.Max, Color);
    }
    
    if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawString) && Widget.String.Length > 0)
    {
        v4 Color = State->Interface.Style.TextColor;
        Editor_DrawWidgetString(State, Context, RenderBuffer, Widget, Color);
    }
    
    if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawHorizontalFill))
    {
        v4 Color = PinkV4; //State->Interface.Style.ButtonColor_Active;
        if (ui_WidgetIdsEqual(Widget.Id, State->Interface.HotWidget))
        {
            Color = GreenV4; //State->Interface.Style.ButtonColor_Selected;
        }
        if (ui_WidgetIdsEqual(Widget.Id, State->Interface.ActiveWidget))
        {
            Color = TealV4; //State->Interface.Style.ButtonColor_Selected;
        }
        
        rect2 HFillBounds = {};
        HFillBounds.Min = Widget.Bounds.Min;
        HFillBounds.Max = {
            LerpR32(Widget.HorizontalFillPercent, Widget.Bounds.Min.x, Widget.Bounds.Max.x),
            Widget.Bounds.Max.y
        };
        PushRenderQuad2D(RenderBuffer, HFillBounds.Min, HFillBounds.Max, Color);
        
        if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawString) && Widget.String.Length > 0)
        {
            // TODO(pjs): Mask this text by the horizontal fill
            // TODO(pjs): add this color to the style
            v4 TextColor = BlackV4;
            Editor_DrawWidgetString(State, Context, RenderBuffer, Widget, TextColor);
        }
    }
    
    
    if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawOutline))
    {
        // TODO(pjs): replace these with values from the style
        r32 Thickness = 1.0f;
        v4 Color = WhiteV4;
        PushRenderBoundingBox2D(RenderBuffer, Widget.Bounds.Min, Widget.Bounds.Max, Thickness, Color);
    }
    
    if (Widget.ChildrenRoot)
    {
        Editor_DrawWidget(State, Context, RenderBuffer, *Widget.ChildrenRoot);
    }
    
    if (Widget.Next)
    {
        Editor_DrawWidget(State, Context, RenderBuffer, *Widget.Next);
    }
}

global r32 TestSlider_Value = 5;
global r32 TestSlider_Min = 0;
global r32 TestSlider_Max = 10;
global bool TestToggle = true;

internal void
TestRender(app_state* State, context* Context, render_command_buffer* RenderBuffer)
{
    ui_InterfaceReset(&State->Interface);
    State->Interface.RenderBuffer = RenderBuffer;
    
    ui_PushLayout(&State->Interface, Context->WindowBounds, LayoutDirection_TopDown, MakeString("TestRender Layout"));
    
    ui_widget_id Ids[2];
    
    gs_string String = MakeString("Select");
    ui_StartRow(&State->Interface, 2);
    for (u32 j = 0; j < 2; j++)
    {
        if (ui_BeginDropdown(&State->Interface, String))
        {
            for (u32 i = 0; i < State->PanelSystem.PanelDefsCount; i++)
            {
                panel_definition Def = State->PanelSystem.PanelDefs[i];
                gs_string DefName = MakeString(Def.PanelName, Def.PanelNameLength);
                if (ui_Button(&State->Interface, DefName))
                {
                    
                }
            }
        }
        ui_EndDropdown(&State->Interface);
    }
    ui_EndRow(&State->Interface);
    TestSlider_Value = ui_RangeSlider(&State->Interface, MakeString("Test Slider"), TestSlider_Value, TestSlider_Min, TestSlider_Max);
    
    TestToggle = ui_Toggle(&State->Interface, MakeString("test toggle"), TestToggle);
    
    ui_Button(&State->Interface, MakeString("Hello"));
    
    ui_PopLayout(&State->Interface);
    
    Assert(!ui_WidgetIdsEqual(Ids[0], Ids[1]));
}

internal void
Editor_Render(app_state* State, context* Context, render_command_buffer* RenderBuffer)
{
    PushRenderOrthographic(RenderBuffer, State->WindowBounds);
    PushRenderClearScreen(RenderBuffer);
    
#if 1
    TestRender(State, Context, RenderBuffer);
    //ui_widget_id IdTwo = TestRender(State, Context, RenderBuffer);
    //Assert(ui_WidgetIdsEqual(IdOne, IdTwo));
    
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
        Editor_DrawWidget(State, Context, RenderBuffer, Widget);
    }
    
    Context->GeneralWorkQueue->CompleteQueueWork(Context->GeneralWorkQueue, Context->ThreadContext);
    Context->GeneralWorkQueue->ResetWorkQueue(Context->GeneralWorkQueue);
    
}


#define FOLDHAUS_EDITOR_CPP
#endif // FOLDHAUS_EDITOR_CPP