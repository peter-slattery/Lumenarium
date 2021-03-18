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
    
    Assert(State->Interface.PerFrameMemory &&
           (u64)State->Interface.PerFrameMemory != 0x5);
    
    PanelSystem_UpdateLayout(&State->PanelSystem, State->WindowBounds);
    Editor_HandleInput(State, State->WindowBounds, InputQueue, Context->Mouse, *Context);
}

internal void
Editor_Render(app_state* State, context* Context, render_command_buffer* RenderBuffer)
{
    State->Interface.WindowBounds = Context->WindowBounds;
    PushRenderOrthographic(RenderBuffer, State->WindowBounds);
    PushRenderClearScreen(RenderBuffer);
    
    
    ui_InterfaceReset(&State->Interface);
    State->Interface.RenderBuffer = RenderBuffer;
    ui_PushLayout(&State->Interface, Context->WindowBounds, LayoutDirection_TopDown, MakeString("Editor Layout"));
    {
        DrawAllPanels(State->PanelSystem, RenderBuffer, &Context->Mouse, State, *Context);
        
        for (s32 m = 0; m < State->Modes.ActiveModesCount; m++)
        {
            operation_mode OperationMode = State->Modes.ActiveModes[m];
            if (OperationMode.Render != 0)
            {
                OperationMode.Render(State, RenderBuffer, OperationMode, Context->Mouse, *Context);
            }
        }
    }
    ui_PopLayout(&State->Interface, MakeString("Editor Layout"));
    
    
    // Draw the Interface
    if (State->Interface.DrawOrderRoot != 0)
    {
        ui_widget* Widget = State->Interface.DrawOrderRoot;
        Editor_DrawWidgetList(State, Context, RenderBuffer, Widget, Context->WindowBounds);
    }
    
    Context->GeneralWorkQueue->CompleteQueueWork(Context->GeneralWorkQueue, Context->ThreadContext);
    ResetWorkQueue(Context->GeneralWorkQueue);
    
}


#define FOLDHAUS_EDITOR_CPP
#endif // FOLDHAUS_EDITOR_CPP