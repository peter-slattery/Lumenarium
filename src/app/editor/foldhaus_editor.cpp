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
    
    b32 PanelSystemHandledInput = HandleMousePanelInteraction(&State->PanelSystem, State->WindowBounds, Mouse, State);
    
    if (!PanelSystemHandledInput)
    {
        input_command_registry ActiveCommands = {};
        if (State->Modes.ActiveModesCount > 0)
        {
            ActiveCommands = State->Modes.ActiveModes[State->Modes.ActiveModesCount - 1].Commands;
        }
        else
        {
            panel* PanelWithMouseOverIt = PanelSystem_GetPanelContainingPoint(&State->PanelSystem, Mouse.Pos);
            if (!PanelWithMouseOverIt) { return; }
            State->HotPanel = PanelWithMouseOverIt;
            
            s32 PanelTypeIndex = PanelWithMouseOverIt->TypeIndex;
            panel_definition PanelDefinition = State->PanelSystem.PanelDefs[PanelTypeIndex];
            if (!PanelDefinition.InputCommands) { return; }
            
            ActiveCommands.Commands = PanelDefinition.InputCommands;
            ActiveCommands.Size = sizeof(*PanelDefinition.InputCommands) / sizeof(PanelDefinition.InputCommands[0]);
            ActiveCommands.Used = ActiveCommands.Size;
        }
        
        for (s32 EventIdx = 0; EventIdx < InputQueue.QueueUsed; EventIdx++)
        {
            input_entry Event = InputQueue.Entries[EventIdx];
            
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
        Entry->Command.Proc(State, Entry->Event, Mouse, Context);
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
Editor_Render(app_state* State, context* Context, render_command_buffer* RenderBuffer)
{
    PushRenderOrthographic(RenderBuffer, State->WindowBounds);
    PushRenderClearScreen(RenderBuffer);
    
    ui_InterfaceReset(&State->Interface);
    State->Interface.RenderBuffer = RenderBuffer;
    
    ui_layout Layout = ui_CreateLayout(&State->Interface, Context->WindowBounds);
    ui_PushLayout(&State->Interface, Layout);
    
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
    
    // Draw the Interface
    for (u32 i = 0; i < State->Interface.WidgetsCount; i++)
    {
        ui_widget Widget = State->Interface.Widgets[i];
        
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
        
        if (Widget.String.Length > 0)
        {
            v4 Color = State->Interface.Style.TextColor;
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
        
        if (ui_WidgetIsFlagSet(Widget, UIWidgetFlag_DrawOutline))
        {
            // TODO(pjs): replace these with values from the style
            r32 Thickness = 1.0f;
            v4 Color = WhiteV4;
            PushRenderBoundingBox2D(RenderBuffer, Widget.Bounds.Min, Widget.Bounds.Max, Thickness, Color);
        }
    }
    
    Context->GeneralWorkQueue->CompleteQueueWork(Context->GeneralWorkQueue, Context->ThreadContext);
    Context->GeneralWorkQueue->ResetWorkQueue(Context->GeneralWorkQueue);
    
}


#define FOLDHAUS_EDITOR_CPP
#endif // FOLDHAUS_EDITOR_CPP