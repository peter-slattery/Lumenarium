////////////////////////////////////////
//
//     Universe View
//
///////////////////////////////////////

struct universe_view_operation_state
{
    b32 MouseDown;
    v2 DisplayOffset;
    r32 Zoom;
};

OPERATION_RENDER_PROC(RenderUniverseView)
{
    DEBUG_TRACK_SCOPE(DrawUniverseOutputDisplay);
    
    universe_view_operation_state* OpState = (universe_view_operation_state*)Operation.OpStateMemory;
    
    string TitleBarString = InitializeEmptyString(PushArray(State->Transient, char, 64), 64);
    
    v2 DisplayArea_Dimension = v2{600, 600};
    
    v2 DisplayContents_Offset = OpState->DisplayOffset;
    
    //
    // TODO(Peter): I don't like this. Dragging the Universe view should be an operation mode, just
    // like rotating the 3D view, but modes don't have access to the state of modes above them in the stack
    // (and attempting to cast those states to the appropriate type seems risky)
    //
    // :NeedToPassStateDownModeChain
    //
    if (OpState->MouseDown)
    {
        DisplayContents_Offset += (Mouse.Pos - Mouse.DownPos);
    }
    
    v2 DisplayArea_TopLeft = v2{300, (r32)RenderBuffer->ViewHeight - 50} + DisplayContents_Offset;
    v2 UniverseDisplayDimension = v2{100, 100} * OpState->Zoom;
    v2 Padding = v2{25, 50} * OpState->Zoom;
    
    v2 UniverseDisplayTopLeft = DisplayArea_TopLeft;
    
    sacn_universe_buffer* UniverseList = State->SACN.UniverseBuffer;
    while(UniverseList)
    {
        for (s32 UniverseIdx = 0;
             UniverseIdx < UniverseList->Used;
             UniverseIdx++)
        {
            sacn_universe* Universe = UniverseList->Universes + UniverseIdx;
            
            DrawSACNUniversePixels(RenderBuffer, Universe, 
                                   UniverseDisplayTopLeft, UniverseDisplayDimension);
            
            if (OpState->Zoom > .5f)
            {
                v2 TitleDisplayStart = UniverseDisplayTopLeft + v2{0, 12};
                PrintF(&TitleBarString, "Universe %d", Universe->Universe);
                DrawString(RenderBuffer, TitleBarString, State->Interface.Font, 
                           TitleDisplayStart, WhiteV4);
            }
            
            UniverseDisplayTopLeft.x += UniverseDisplayDimension.x + Padding.x;
            if (UniverseDisplayTopLeft.x > DisplayArea_TopLeft.x + DisplayArea_Dimension.x)
            {
                UniverseDisplayTopLeft.x = DisplayArea_TopLeft.x;
                UniverseDisplayTopLeft.y -= UniverseDisplayDimension.y + Padding.y;
            }
            
            if (UniverseDisplayTopLeft.y < DisplayArea_TopLeft.y - DisplayArea_Dimension.y)
            {
                break;
            }
        }
        UniverseList = UniverseList->Next;
    }
}

// TODO(Peter): Something isn't working with my laptop trackpad's zoom
FOLDHAUS_INPUT_COMMAND_PROC(UniverseZoom)
{
    universe_view_operation_state* OpState = GetCurrentOperationState(State->Modes, universe_view_operation_state);
    r32 DeltaZoom = (r32)(Mouse.Scroll) / 120;
    OpState->Zoom = GSClamp(0.1f, OpState->Zoom + DeltaZoom, 4.f);
}

FOLDHAUS_INPUT_COMMAND_PROC(UniverseViewEndPan)
{
    // :NeedToPassStateDownModeChain
    universe_view_operation_state* OpState = GetCurrentOperationState(State->Modes, universe_view_operation_state);
    OpState->MouseDown = false;
    OpState->DisplayOffset = OpState->DisplayOffset + (Mouse.Pos - Mouse.DownPos);
}

FOLDHAUS_INPUT_COMMAND_PROC(UniverseViewBeginPan)
{
    // :NeedToPassStateDownModeChain
    universe_view_operation_state* OpState = GetCurrentOperationState(State->Modes, universe_view_operation_state);
    OpState->MouseDown = true;
}

FOLDHAUS_INPUT_COMMAND_PROC(CloseUniverseView)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

input_command UniverseViewCommands [] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Began, UniverseViewBeginPan },
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, UniverseViewEndPan },
    { KeyCode_U, KeyCode_Invalid, Command_Began, CloseUniverseView },
};

FOLDHAUS_INPUT_COMMAND_PROC(OpenUniverseView)
{
    operation_mode* UniverseViewMode = ActivateOperationModeWithCommands(&State->Modes, "Universe View", UniverseViewCommands);
    UniverseViewMode->Render = RenderUniverseView;
    
    // State Setup
    universe_view_operation_state* OpState = CreateOperationState(UniverseViewMode, 
                                                                  &State->Modes, 
                                                                  universe_view_operation_state); 
    OpState->DisplayOffset = v2{0, 0};
    OpState->Zoom = 1.0f;
}

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

OPERATION_RENDER_PROC(RenderNodeLister)
{
    node_lister_operation_state* OpState = (node_lister_operation_state*)Operation.OpStateMemory;
    
    v2 TopLeft = OpState->ListPosition;
    v2 Dimension = v2{300, 30};
    
    // Filter the lister
    OpState->SearchLister.Filter = State->ActiveTextEntry.Buffer;
    FilterSearchLister(&OpState->SearchLister);
    
    // Display Search Lister
    search_lister_result NodeListerResult = EvaluateSearchLister (RenderBuffer, TopLeft, Dimension, 
                                                                  MakeStringLiteral("Nodes List"),
                                                                  OpState->SearchLister.SourceList,
                                                                  OpState->SearchLister.FilteredIndexLUT,
                                                                  OpState->SearchLister.FilteredListCount,
                                                                  OpState->SearchLister.HotItem,
                                                                  &State->ActiveTextEntry.Buffer,
                                                                  State->ActiveTextEntry.CursorPosition,
                                                                  State->Font, State->Interface, Mouse);
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
        PushNodeOnListFromSpecification(State->NodeList, NodeSpecifications[NodeIndex],
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
    operation_mode* AddNodeOperation = ActivateOperationModeWithCommands(&State->Modes, "Node Lister", UniverseViewCommads);
    
    AddNodeOperation->Render = RenderNodeLister;
    
    node_lister_operation_state* OpState = CreateOperationState(AddNodeOperation, 
                                                                &State->Modes, 
                                                                node_lister_operation_state);
    {
        OpState->SearchLister.SourceListCount = NodeSpecificationsCount;
        OpState->SearchLister.SourceList = PushArray(&State->Modes.Arena, string, OpState->SearchLister.SourceListCount);
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
    SetTextInputDestinationToString(&State->ActiveTextEntry, &OpState->SearchLister.Filter);
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

OPERATION_RENDER_PROC(RenderColorPicker)
{
    color_picker_operation_state* OpState = (color_picker_operation_state*)Operation.OpStateMemory;
    
    
    b32 ShouldClose = EvaluateColorPicker(RenderBuffer, OpState->ValueAddr, 
                                          v2{200, 200}, State->Interface, Mouse);
    
    if (ShouldClose)
    {
        CloseColorPicker(State);
    }
}

internal void
OpenColorPicker(app_state* State, node_connection* Connection)
{
    operation_mode* ColorPickerMode = ActivateOperationMode(&State->Modes, "Color Picker");
    ColorPickerMode->Render = RenderColorPicker;
    
    color_picker_operation_state* OpState = CreateOperationState(ColorPickerMode, 
                                                                 &State->Modes, 
                                                                 color_picker_operation_state);
    OpState->ValueAddr = &Connection->V4Value;
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
                                                                              "Node Field Text Edit",
                                                                              NodeFieldTextEditCommands);
    
    SetTextInputDestinationToFloat(&State->ActiveTextEntry, &Connection->R32Value);
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

OPERATION_RENDER_PROC(RenderDraggingNodePort)
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
        "Drag Node Port",
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

OPERATION_RENDER_PROC(RenderDragNodeField)
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

OPERATION_RENDER_PROC(RenderDraggingNode)
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
        "Drag Node",
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

OPERATION_RENDER_PROC(RenderNodeView)
{
    node_view_operation_state* OpState = (node_view_operation_state*)Operation.OpStateMemory;
    
    DEBUG_TRACK_FUNCTION;
    
    MakeStringBuffer(NodeHeaderBuffer, 128);
    
    node_header* SelectedNode = GetNodeWithHandle(State->NodeList, OpState->SelectedNodeHandle);
    
    node_list_iterator NodeIter = GetNodeListIterator(*State->NodeList);
    while (NodeIteratorIsValid(NodeIter))
    {
        node_header* Node = NodeIter.At;
        
        rect NodeBounds = CalculateNodeBounds(Node, State->NodeRenderSettings);
        b32 DrawFields = PointIsInRect(Mouse.Pos, NodeBounds);
        
        if (Node == SelectedNode)
        {
            PushRenderQuad2D(RenderBuffer, NodeBounds.Min - v2{2, 2}, NodeBounds.Max + v2{2, 2}, WhiteV4);
        }
        
        PushRenderQuad2D(RenderBuffer, NodeBounds.Min, NodeBounds.Max, v4{.5f, .5f, .5f, 1.f});
        
        // TODO(Peter): This is just for debug purposes. We can remove and go back to just having
        // Node->Name in DrawString
        PrintF(&NodeHeaderBuffer, "%.*s: %d", Node->Name.Length, Node->Name.Memory, Node->Handle);
        DrawString(RenderBuffer, NodeHeaderBuffer, State->NodeRenderSettings.Font,
                   v2{NodeBounds.Min.x + 5, NodeBounds.Max.y - (State->NodeRenderSettings.Font->PixelHeight + NODE_HEADER_HEIGHT + 5)},
                   WhiteV4);
        
        for (s32 Connection = 0; Connection < Node->ConnectionsCount; Connection++)
        {
            v4 PortColor = State->NodeRenderSettings.PortColors[Node->Connections[Connection].Type];
            
            // Inputs
            if (ConnectionIsInput(Node, Connection))
            {
                rect PortBounds = CalculateNodeInputPortBounds(Node, Connection, State->NodeRenderSettings);
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
                    node_specification Spec = NodeSpecifications[Node->Type - 1];
                    node_struct_member Member = Spec.MemberList[Connection];
                    DrawString(RenderBuffer, MakeString(Member.Name), 
                               State->NodeRenderSettings.Font,
                               v2{PortBounds.Min.x - 8, PortBounds.Min.y}, WhiteV4, Align_Right);
                }
                
                rect ValueBounds = CalculateNodeInputValueBounds(Node, Connection, State->NodeRenderSettings);
                DrawValueDisplay(RenderBuffer, ValueBounds, Node->Connections[Connection], State->NodeRenderSettings.Font);
                
                // NOTE(Peter): its way easier to draw the connection on the input port b/c its a 1:1 relationship,
                // whereas output ports might have many connections, they really only know about the most recent one
                // Not sure if this is a problem. We mostly do everything backwards here, starting at the 
                // most downstream node and working back up to find dependencies.
                if (ConnectionHasUpstreamConnection(Node, Connection))
                {
                    rect ConnectedPortBounds = GetBoundsOfPortConnectedToInput(Node, Connection, State->NodeList, State->NodeRenderSettings);
                    v2 InputCenter = CalculateRectCenter(PortBounds);
                    v2 OutputCenter = CalculateRectCenter(ConnectedPortBounds);
                    PushRenderLine2D(RenderBuffer, OutputCenter, InputCenter, 1, WhiteV4);
                }
            }
            
            // Outputs
            if (ConnectionIsOutput(Node, Connection))
            {
                rect PortBounds = CalculateNodeOutputPortBounds(Node, Connection, State->NodeRenderSettings);
                DrawPort(RenderBuffer, PortBounds, PortColor);
                
                if (DrawFields)
                {
                    node_specification Spec = NodeSpecifications[Node->Type - 1];
                    node_struct_member Member = Spec.MemberList[Connection];
                    DrawString(RenderBuffer, MakeString(Member.Name), 
                               State->NodeRenderSettings.Font,
                               v2{PortBounds.Max.x + 8, PortBounds.Min.y}, WhiteV4);
                }
                
                rect ValueBounds = CalculateNodeOutputValueBounds(Node, Connection, State->NodeRenderSettings);
                DrawValueDisplay(RenderBuffer, ValueBounds, Node->Connections[Connection], State->NodeRenderSettings.Font);
            }
            
            for (s32 Button = 0; Button < 3; Button++)
            {
                rect ButtonRect = CalculateNodeDragHandleBounds(NodeBounds, Button, State->NodeRenderSettings);
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
    operation_mode* NodeViewMode = ActivateOperationModeWithCommands(&State->Modes, "Node View", NodeViewCommands);
    NodeViewMode->Render = RenderNodeView;
    
    node_view_operation_state* OpState = CreateOperationState(NodeViewMode, 
                                                              &State->Modes, 
                                                              node_view_operation_state);
    OpState->SelectedNodeHandle = 0;
}

////////////////////////////////////////
//
//     3D View Mouse Rotate
//
///////////////////////////////////////

struct mouse_rotate_view_operation_state
{
    v4 CameraStartPos;
};

OPERATION_RENDER_PROC(Update3DViewMouseRotate)
{
    mouse_rotate_view_operation_state* OpState = (mouse_rotate_view_operation_state*)Operation.OpStateMemory;
    
    v2 TotalDeltaPos = Mouse.Pos - Mouse.DownPos;
    
    m44 XRotation = GetXRotation(-TotalDeltaPos.y * State->PixelsToWorldScale);
    m44 YRotation = GetYRotation(TotalDeltaPos.x * State->PixelsToWorldScale);
    m44 Combined = XRotation * YRotation;
    
    State->Camera.Position = V3(Combined * OpState->CameraStartPos);
}

FOLDHAUS_INPUT_COMMAND_PROC(End3DViewMouseRotate)
{
    DeactivateCurrentOperationMode(&State->Modes);
}

input_command MouseRotateViewCommands [] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, End3DViewMouseRotate},
};

FOLDHAUS_INPUT_COMMAND_PROC(Begin3DViewMouseRotate)
{
    operation_mode* RotateViewMode = ActivateOperationModeWithCommands(&State->Modes, "Rotate 3D View", MouseRotateViewCommands);
    RotateViewMode->Render = Update3DViewMouseRotate;
    
    mouse_rotate_view_operation_state* OpState = CreateOperationState(RotateViewMode,
                                                                      &State->Modes,
                                                                      mouse_rotate_view_operation_state);
    OpState->CameraStartPos = V4(State->Camera.Position, 1);
}