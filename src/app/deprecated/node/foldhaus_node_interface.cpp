//
// File: foldhaus_node_interface.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLHAUS_NODE_INTERFACE_CPP

////////////////////////////////////////
//
//     Node Lister
//
///////////////////////////////////////

struct node_lister_operation_state
{
    search_lister SearchLister;
    v2 ListPosition;
};

internal void
RenderNodeLister(panel Panel, rect2 PanelBounds, render_command_buffer* RenderBufer, app_state* State, context Context, mouse_state Mouse)
{
    node_lister_operation_state* OpState = (node_lister_operation_state*)Operation.OpStateMemory;
    
    v2 TopLeft = OpState->ListPosition;
    v2 Dimension = v2{300, 30};
    
    // Filter the lister
    OpState->SearchLister.Filter = State->ActiveTextEntry.Buffer;
    FilterSearchLister(&OpState->SearchLister);
    
    // Display Search Lister
    search_lister_result NodeListerResult = EvaluateSearchLister (&State->Interface_, TopLeft, Dimension,
                                                                  MakeStringLiteral("Nodes List"),
                                                                  OpState->SearchLister.SourceList,
                                                                  OpState->SearchLister.FilteredIndexLUT,
                                                                  OpState->SearchLister.FilteredListCount,
                                                                  OpState->SearchLister.HotItem,
                                                                  &State->ActiveTextEntry.Buffer,
                                                                  State->ActiveTextEntry.CursorPosition);
}

FOLDHAUS_INPUT_COMMAND_PROC(NodeListerNextItem)
{
    node_lister_operation_state* OpState = GetCurrentOperationState(State->Modes, node_lister_operation_state);
    OpState->SearchLister.HotItem = GetNextFilteredItem(OpState->SearchLister);
}

FOLDHAUS_INPUT_COMMAND_PROC(NodeListerPrevItem)
{
    node_lister_operation_state* OpState = GetCurrentOperationState(State->Modes, node_lister_operation_state);
    OpState->SearchLister.HotItem = GetPrevFilteredItem(OpState->SearchLister);
}

FOLDHAUS_INPUT_COMMAND_PROC(CloseNodeLister)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

FOLDHAUS_INPUT_COMMAND_PROC(SelectAndCloseNodeLister)
{
    node_lister_operation_state* OpState = GetCurrentOperationState(State->Modes, node_lister_operation_state);
    s32 FilteredNodeIndex = OpState->SearchLister.HotItem;
    if (FilteredNodeIndex >= 0)
    {
        s32 NodeIndex = OpState->SearchLister.FilteredIndexLUT[FilteredNodeIndex];
        PushNodeOnListFromSpecification(State->NodeList, (node_type)NodeIndex,
                                        Mouse.Pos, State->Permanent);
    }
    CloseNodeLister(State, Event, Mouse);
}

input_command UniverseViewCommads [] = {
    { KeyCode_DownArrow, KeyCode_Invalid, Command_Began, NodeListerNextItem },
    { KeyCode_UpArrow, KeyCode_Invalid, Command_Began, NodeListerPrevItem },
    { KeyCode_Enter, KeyCode_Invalid, Command_Began, SelectAndCloseNodeLister },
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Began, CloseNodeLister },
    { KeyCode_Esc, KeyCode_Invalid, Command_Began, CloseNodeLister },
    DEFAULT_TEXT_ENTRY_INPUT_COMMANDS_ARRAY_ENTRY,
};

FOLDHAUS_INPUT_COMMAND_PROC(OpenNodeLister)
{
    operation_mode* AddNodeOperation = ActivateOperationModeWithCommands(&State->Modes, UniverseViewCommads);
    
    AddNodeOperation->Render = RenderNodeLister;
    
    node_lister_operation_state* OpState = CreateOperationState(AddNodeOperation,
                                                                &State->Modes,
                                                                node_lister_operation_state);
    {
        OpState->SearchLister.SourceListCount = NodeSpecificationsCount;
        OpState->SearchLister.SourceList = PushArray(&State->Modes.Arena, gs_string, OpState->SearchLister.SourceListCount);
        {
            for (s32 i = 0; i < OpState->SearchLister.SourceListCount; i++)
            {
                OpState->SearchLister.SourceList[i] = MakeString(
                                                                 NodeSpecifications[i].Name,
                                                                 NodeSpecifications[i].NameLength);
            }
        }
        OpState->SearchLister.Filter = MakeString(PushArray(&State->Modes.Arena, char, 64), 0, 64);
        
        OpState->SearchLister.FilteredListMax = OpState->SearchLister.SourceListCount;
        OpState->SearchLister.FilteredListCount = 0;
        OpState->SearchLister.FilteredIndexLUT = PushArray(&State->Modes.Arena, s32, OpState->SearchLister.SourceListCount);
    }
    
    OpState->ListPosition = Mouse.Pos;
    SetTextInputDestinationTogs_string(&State->ActiveTextEntry, &OpState->SearchLister.Filter);
}

////////////////////////////////////////
//
//    Node Color Picker
//
///////////////////////////////////////

struct color_picker_operation_state
{
    v4* ValueAddr;
};

internal void
CloseColorPicker(app_state* State)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

FOLDHAUS_INPUT_COMMAND_PROC(CloseColorPickerCommand)
{
    CloseColorPicker(State);
}

internal void
RenderColorPicker(panel Panel, rect2 PanelBounds, render_command_buffer* RenderBufer, app_state* State, context Context, mouse_state Mouse)
{
    color_picker_operation_state* OpState = (color_picker_operation_state*)Operation.OpStateMemory;
    
    
    b32 ShouldClose = EvaluateColorPicker(RenderBuffer, OpState->ValueAddr,
                                          v2{200, 200}, State->Interface, Mouse);
    
    if (ShouldClose)
    {
        CloseColorPicker(State);
    }
}

input_command ColorPickerCommands [] = {
    { KeyCode_Esc, KeyCode_Invalid, Command_Began, CloseColorPickerCommand },
};

internal void
OpenColorPicker(app_state* State, node_connection* Connection)
{
    operation_mode* ColorPickerMode = ActivateOperationModeWithCommands(&State->Modes, ColorPickerCommands);
    ColorPickerMode->Render = RenderColorPicker;
    
    color_picker_operation_state* OpState = CreateOperationState(ColorPickerMode,
                                                                 &State->Modes,
                                                                 color_picker_operation_state);
    OpState->ValueAddr = Connection->V4ValuePtr;
}


////////////////////////////////////////
//
//    Node Field Text Edit
//
///////////////////////////////////////

FOLDHAUS_INPUT_COMMAND_PROC(EndNodeFieldTextEdit)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

input_command NodeFieldTextEditCommands [] = {
    { KeyCode_Enter, KeyCode_Invalid, Command_Began, EndNodeFieldTextEdit },
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Began, EndNodeFieldTextEdit },
    DEFAULT_TEXT_ENTRY_INPUT_COMMANDS_ARRAY_ENTRY,
};

internal void
BeginNodeFieldTextEdit(app_state* State, node_connection* Connection)
{
    operation_mode* NodeFieldTextEditMode = ActivateOperationModeWithCommands(&State->Modes,
                                                                              NodeFieldTextEditCommands);
    
    SetTextInputDestinationToFloat(&State->ActiveTextEntry, Connection->R32ValuePtr);
}

////////////////////////////////////////
//
//    Node Port Mouse Drag
//
///////////////////////////////////////

struct drag_node_port_operation_state
{
    node_interaction Interaction;
};

internal void
RenderDraggingNodePort(panel Panel, rect2 PanelBounds, render_command_buffer* RenderBufer, app_state* State, context Context, mouse_state Mouse)
{
    drag_node_port_operation_state* OpState = (drag_node_port_operation_state*)Operation.OpStateMemory;
    UpdateDraggingNodePort(Mouse.Pos, OpState->Interaction, State->NodeList,
                           State->NodeRenderSettings, RenderBuffer);
}

FOLDHAUS_INPUT_COMMAND_PROC(EndDraggingNodePort)
{
    drag_node_port_operation_state* OpState = GetCurrentOperationState(State->Modes, drag_node_port_operation_state);
    
    TryConnectNodes(OpState->Interaction, Mouse.Pos, State->NodeList, State->NodeRenderSettings);
    DeactivateCurrentOperationMode(&State->Modes);
}

input_command DragNodePortInputCommands[] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, EndDraggingNodePort },
};

internal void
BeginDraggingNodePort(app_state* State, node_interaction Interaction)
{
    operation_mode* DragNodePortMode = ActivateOperationModeWithCommands(
                                                                         &State->Modes,
                                                                         DragNodePortInputCommands);
    DragNodePortMode->Render = RenderDraggingNodePort;
    
    drag_node_port_operation_state* OpState = CreateOperationState(DragNodePortMode,
                                                                   &State->Modes,
                                                                   drag_node_port_operation_state);
    OpState->Interaction = Interaction;
}

////////////////////////////////////////
//
//    Node Field Mouse Drag
//
///////////////////////////////////////

internal void
RenderDragNodeField(panel Panel, rect2 PanelBounds, render_command_buffer* RenderBufer, app_state* State, context Context, mouse_state Mouse)
{
    // TODO(Peter):
    //UpdateDraggingNodeValue(Mouse.Pos, Mouse.OldPos, OpState->Interaction, State->NodeList, State->NodeRenderSettings, State);
}

internal void
BeginInteractWithNodeField(app_state* State, node_interaction Interaction)
{
    // TODO(Peter):
}

////////////////////////////////////////
//
//    Node Mouse Drag
//
///////////////////////////////////////

struct drag_node_operation_state
{
    node_interaction Interaction;
};

internal void
RenderDraggingNode(panel Panel, rect2 PanelBounds, render_command_buffer* RenderBufer, app_state* State, context Context, mouse_state Mouse)
{
    drag_node_operation_state* OpState = GetCurrentOperationState(State->Modes, drag_node_operation_state);
    UpdateDraggingNode(Mouse.Pos, OpState->Interaction, State->NodeList,
                       State->NodeRenderSettings);
}

FOLDHAUS_INPUT_COMMAND_PROC(EndDraggingNode)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

input_command DragNodeInputCommands[] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, EndDraggingNode },
};

internal void
BeginDraggingNode(app_state* State, node_interaction Interaction)
{
    operation_mode* DragNodeMode = ActivateOperationModeWithCommands(
                                                                     &State->Modes,
                                                                     DragNodeInputCommands);
    DragNodeMode->Render = RenderDraggingNode;
    
    drag_node_operation_state* OpState = CreateOperationState(DragNodeMode,
                                                              &State->Modes,
                                                              drag_node_operation_state);
    OpState->Interaction = Interaction;
}

////////////////////////////////////////
//
//    Node View
//
///////////////////////////////////////

struct node_view_operation_state
{
    s32 SelectedNodeHandle;
};

FOLDHAUS_INPUT_COMMAND_PROC(NodeViewBeginMouseDragInteraction)
{
    node_view_operation_state* OpState = GetCurrentOperationState(State->Modes, node_view_operation_state);
    
    node_header* Node = GetNodeUnderPoint(State->NodeList, Mouse.DownPos, State->NodeRenderSettings);
    if (Node)
    {
        node_interaction NewInteraction = GetNodeInteractionType(Node,
                                                                 Mouse.Pos,
                                                                 State->NodeRenderSettings);
        if (IsDraggingNodePort(NewInteraction))
        {
            BeginDraggingNodePort(State, NewInteraction);
        }
        else if(IsDraggingNodeValue(NewInteraction))
        {
            // TODO(Peter): This probably wants to live in a mouse held action
            // the first frame we realize we're held over a field, just transition to
            // drag node field
            //BeginInteractWithNodeField(State, NewInteraction, State->NodeRenderSettings);
        }
        else // IsDraggingNode
        {
            OpState->SelectedNodeHandle = Node->Handle;
            BeginDraggingNode(State, NewInteraction);
        }
    }
    else
    {
        OpState->SelectedNodeHandle = 0;
    }
}

FOLDHAUS_INPUT_COMMAND_PROC(NodeViewBeginMouseSelectInteraction)
{
    node_view_operation_state* OpState = GetCurrentOperationState(State->Modes, node_view_operation_state);
    
    node_header* Node = GetNodeUnderPoint(State->NodeList, Mouse.Pos, State->NodeRenderSettings);
    if (Node)
    {
        node_interaction NewInteraction = GetNodeInteractionType(Node,
                                                                 Mouse.Pos,
                                                                 State->NodeRenderSettings);
        if(IsDraggingNodeValue(NewInteraction))
        {
            node_connection* Connection = Node->Connections + NewInteraction.InputValue;
            struct_member_type InputType = Connection->Type;
            
            if (InputType == MemberType_r32)
            {
                BeginNodeFieldTextEdit(State, Connection);
            }
            else if (InputType == MemberType_v4)
            {
                OpenColorPicker(State, Connection);
            }
        }
    }
}

internal void
RenderNodeView(panel Panel, rect2 PanelBounds, render_command_buffer* RenderBufer, app_state* State, context Context, mouse_state Mouse)
{
    node_view_operation_state* OpState = (node_view_operation_state*)Operation.OpStateMemory;
    
    DEBUG_TRACK_FUNCTION;
    
    MakeStringBuffer(NodeHeaderBuffer, 128);
    
    node_header* SelectedNode = GetNodeWithHandle(State->NodeList, OpState->SelectedNodeHandle);
    
    node_list_iterator NodeIter = GetNodeListIterator(*State->NodeList);
    while (NodeIteratorIsValid(NodeIter))
    {
        node_header* Node = NodeIter.At;
        
        rect2 NodeBounds = CalculateNodeBounds(Node, State->NodeRenderSettings);
        b32 DrawFields = PointIsInRect(Mouse.Pos, NodeBounds);
        
        if (Node == SelectedNode)
        {
            PushRenderQuad2D(RenderBuffer, NodeBounds.Min - v2{2, 2}, NodeBounds.Max + v2{2, 2}, WhiteV4);
        }
        
        PushRenderQuad2D(RenderBuffer, NodeBounds.Min, NodeBounds.Max, v4{.5f, .5f, .5f, 1.f});
        
        // TODO(Peter): This is just for debug purposes. We can remove and go back to just having
        // Node->Name in Drawgs_string
        gs_string NodeName = GetNodeName(*Node);
        PrintF(&NodeHeaderBuffer, "%.*s: %d", NodeName.Length, NodeName.Memory, Node->Handle);
        DrawString(RenderBuffer, NodeHeaderBuffer, State->NodeRenderSettings.Font,
                   v2{NodeBounds.Min.x + 5, NodeBounds.Max.y - (State->NodeRenderSettings.Font->PixelHeight + NODE_HEADER_HEIGHT + 5)},
                   WhiteV4);
        
        for (s32 Connection = 0; Connection < Node->ConnectionsCount; Connection++)
        {
            v4 PortColor = State->NodeRenderSettings.PortColors[Node->Connections[Connection].Type];
            
            // Inputs
            if (ConnectionIsInput(Node, Connection))
            {
                rect2 PortBounds = CalculateNodeInputPortBounds(Node, Connection, State->NodeRenderSettings);
                DrawPort(RenderBuffer, PortBounds, PortColor);
                
                //
                // TODO(Peter): I don't like excluding OutputNode, feels too much like a special case
                // but I don't want to get in to the meta programming right now.
                // We should just generate a spec and struct member types for NodeType_OutputNode
                //
                // :ExcludingOutputNodeSpecialCase
                //
                if (Node->Type != NodeType_OutputNode && DrawFields)
                {
                    node_specification Spec = NodeSpecifications[Node->Type];
                    node_struct_member Member = Spec.MemberList[Connection];
                    DrawString(RenderBuffer, MakeString(Member.Name),
                               State->NodeRenderSettings.Font,
                               v2{PortBounds.Min.x - 8, PortBounds.Min.y}, WhiteV4, Align_Right);
                }
                
                rect2 ValueBounds = CalculateNodeInputValueBounds(Node, Connection, State->NodeRenderSettings);
                DrawValueDisplay(RenderBuffer, ValueBounds, Node->Connections[Connection], State->NodeRenderSettings.Font);
                
                // NOTE(Peter): its way easier to draw the connection on the input port b/c its a 1:1 relationship,
                // whereas output ports might have many connections, they really only know about the most recent one
                // Not sure if this is a problem. We mostly do everything backwards here, starting at the
                // most downstream node and working back up to find dependencies.
                if (ConnectionHasUpstreamConnection(Node, Connection))
                {
                    rect2 ConnectedPortBounds = GetBoundsOfPortConnectedToInput(Node, Connection, State->NodeList, State->NodeRenderSettings);
                    v2 InputCenter = CalculateRectCenter(PortBounds);
                    v2 OutputCenter = CalculateRectCenter(ConnectedPortBounds);
                    PushRenderLine2D(RenderBuffer, OutputCenter, InputCenter, 1, WhiteV4);
                }
            }
            
            // Outputs
            if (ConnectionIsOutput(Node, Connection))
            {
                rect2 PortBounds = CalculateNodeOutputPortBounds(Node, Connection, State->NodeRenderSettings);
                DrawPort(RenderBuffer, PortBounds, PortColor);
                
                if (DrawFields)
                {
                    node_specification Spec = NodeSpecifications[Node->Type];
                    node_struct_member Member = Spec.MemberList[Connection];
                    DrawString(RenderBuffer, MakeString(Member.Name),
                               State->NodeRenderSettings.Font,
                               v2{PortBounds.Max.x + 8, PortBounds.Min.y}, WhiteV4);
                }
                
                rect2 ValueBounds = CalculateNodeOutputValueBounds(Node, Connection, State->NodeRenderSettings);
                DrawValueDisplay(RenderBuffer, ValueBounds, Node->Connections[Connection], State->NodeRenderSettings.Font);
            }
            
            for (s32 Button = 0; Button < 3; Button++)
            {
                rect2 ButtonRect = CalculateNodeDragHandleBounds(NodeBounds, Button, State->NodeRenderSettings);
                PushRenderQuad2D(RenderBuffer, ButtonRect.Min, ButtonRect.Max, DragButtonColors[Button]);
            }
        }
        
        Next(&NodeIter);
    }
}

FOLDHAUS_INPUT_COMMAND_PROC(NodeViewDeleteNode)
{
    node_view_operation_state* OpState = GetCurrentOperationState(State->Modes, node_view_operation_state);
    if (OpState->SelectedNodeHandle > 0)
    {
        node_header* SelectedNode = GetNodeWithHandle(State->NodeList, OpState->SelectedNodeHandle);
        FreeNodeOnList(State->NodeList, SelectedNode);
    }
}

FOLDHAUS_INPUT_COMMAND_PROC(CloseNodeView)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

input_command NodeViewCommands [] = {
    { KeyCode_Tab, KeyCode_Invalid, Command_Began, CloseNodeView},
    { KeyCode_A, KeyCode_Invalid, Command_Began, OpenNodeLister},
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Began, NodeViewBeginMouseDragInteraction},
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, NodeViewBeginMouseSelectInteraction},
    { KeyCode_X, KeyCode_Invalid, Command_Began, NodeViewDeleteNode},
};

FOLDHAUS_INPUT_COMMAND_PROC(OpenNodeView)
{
    operation_mode* NodeViewMode = ActivateOperationModeWithCommands(&State->Modes, NodeViewCommands);
    NodeViewMode->Render = RenderNodeView;
    
    node_view_operation_state* OpState = CreateOperationState(NodeViewMode,
                                                              &State->Modes,
                                                              node_view_operation_state);
    
    OpState->SelectedNodeHandle = 0;
}


#define FOLHAUS_NODE_INTERFACE_CPP
#endif // FOLHAUS_NODE_INTERFACE_CPP
