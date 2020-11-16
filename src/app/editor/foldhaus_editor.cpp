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
Editor_DrawWidgetString(app_state* State, context* Context, render_command_buffer* RenderBuffer, ui_widget Widget, rect2 ClippingBox, v4 Color)
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
            PushRenderQuad2DClipped(RenderBuffer, FillBounds, WidgetParentUnion, Color);
            
            if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawString) && Widget.String.Length > 0)
            {
                // TODO(pjs): Mask this text by the horizontal fill
                // TODO(pjs): add this color to the style
                v4 TextColor = BlackV4;
                Editor_DrawWidgetString(State, Context, RenderBuffer, Widget, WidgetParentUnion, TextColor);
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

global r32 TestSlider_Value = 5;
global r32 TestSlider_Min = 0;
global r32 TestSlider_Max = 10;
global bool TestToggle = true;

internal void
TestRender(app_state* State, context* Context, render_command_buffer* RenderBuffer)
{
    ui_InterfaceReset(&State->Interface);
    State->Interface.RenderBuffer = RenderBuffer;
    State->Interface.WindowBounds = Context->WindowBounds;
    
    gs_string A = MakeString("TestRender Layout");
    
    ui_PushLayout(&State->Interface, A);
    {
#if 1
        ui_Label(&State->Interface, MakeString("Spacer"));
        ui_Label(&State->Interface, MakeString("Spacer"));
        ui_Label(&State->Interface, MakeString("Spacer"));
        ui_Label(&State->Interface, MakeString("Spacer"));
        ui_Label(&State->Interface, MakeString("Spacer"));
        
        ui_BeginList(&State->Interface, MakeString("TestList"), 5, 16);
        {
            ui_BeginRow(&State->Interface, 3);
            for (u32 i = 0; i < 16; i++)
            {
                
                ui_Button(&State->Interface, MakeString("B"));
                ui_Button(&State->Interface, MakeString("C"));
                ui_Button(&State->Interface, MakeString("D"));
            }
            ui_EndRow(&State->Interface);
        }
        ui_EndList(&State->Interface);
        //ui_Button(&State->Interface, MakeString("B"));
        //ui_Button(&State->Interface, MakeString("C"));
        //TestSlider_Value = ui_RangeSlider(&State->Interface, MakeString("TestSlider"), TestSlider_Value, TestSlider_Min, TestSlider_Max);
#elif 0
        ui_PushLayout(&State->Interface, MakeString("Outer"));
        {
            for (u32 i = 0; i < 3; i++)
            {
                ui_Button(&State->Interface, MakeString("A"));
            }
        }
        ui_PopLayout(&State->Interface);
        
        ui_BeginRow(&State->Interface, 2);
        {
            ui_PushLayout(&State->Interface, MakeString("TestLayout"));
            {
                for (u32 i = 0; i < 5; i++)
                {
                    ui_Button(&State->Interface, MakeString("TestButon"));
                }
            }
            ui_PopLayout(&State->Interface);
            
            ui_PushLayout(&State->Interface, MakeString("TestLayout"));
            {
                ui_Button(&State->Interface, MakeString("TestButon"));
                TestToggle = ui_Toggle(&State->Interface, MakeString("Toggle"), TestToggle);
                TestSlider_Value = ui_RangeSlider(&State->Interface, MakeString("TestSlider"), TestSlider_Value, TestSlider_Min, TestSlider_Max);
                if (ui_BeginDropdown(&State->Interface, MakeString("TestDropdown")))
                {
                    ui_Button(&State->Interface, MakeString("TestButon"));
                    ui_Button(&State->Interface, MakeString("TestButon"));
                    ui_Button(&State->Interface, MakeString("TestButon"));
                }
                ui_EndDropdown(&State->Interface);
            }
            ui_PopLayout(&State->Interface);
        }
        ui_EndRow(&State->Interface);
        
        ui_PushLayout(&State->Interface, MakeString("Outer"));
        {
            for (u32 i = 0; i < 3; i++)
            {
                ui_Button(&State->Interface, MakeString("B"));
            }
        }
        ui_PopLayout(&State->Interface);
#else
        ui_BeginList(&State->Interface, MakeString("Test List"), 10);
        {
            for (u32 i = 0; i < 32; i++)
            {
                ui_Button(&State->Interface, MakeString("Option"));
            }
        }
        ui_EndList(&State->Interface);
#endif
    }
    ui_PopLayout(&State->Interface);
}

internal void
Editor_Render(app_state* State, context* Context, render_command_buffer* RenderBuffer)
{
    PushRenderOrthographic(RenderBuffer, State->WindowBounds);
    PushRenderClearScreen(RenderBuffer);
    
#if 0
    TestRender(State, Context, RenderBuffer);
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
    }
    
    Context->GeneralWorkQueue->CompleteQueueWork(Context->GeneralWorkQueue, Context->ThreadContext);
    Context->GeneralWorkQueue->ResetWorkQueue(Context->GeneralWorkQueue);
    
}


#define FOLDHAUS_EDITOR_CPP
#endif // FOLDHAUS_EDITOR_CPP