struct node_graph_state
{
    v2 ViewOffset;
};

struct visual_node
{
    node_specification Spec;
    v2 Position;
};

//
// Pan Node Graph
//

OPERATION_STATE_DEF(pan_node_graph_operation_state)
{
    v2 InitialViewOffset;
    
    // TODO(Peter): I DON"T LIKE THIS!!!! 
    // We should have a way to access the panel that created an operation mode or something
    v2* ViewOffset;
};

OPERATION_RENDER_PROC(UpdatePanNodeGraph)
{
    pan_node_graph_operation_state* OpState = (pan_node_graph_operation_state*)Operation.OpStateMemory;
    v2 MouseDelta = Mouse.Pos - Mouse.DownPos;
    *OpState->ViewOffset = MouseDelta + OpState->InitialViewOffset;
}

input_command PanNodeGraphCommands[] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, EndCurrentOperationMode },
};

FOLDHAUS_INPUT_COMMAND_PROC(BeginPanNodeGraph)
{
    operation_mode* PanNodeGraph = ActivateOperationModeWithCommands(&State->Modes, PanNodeGraphCommands, UpdatePanNodeGraph);
    pan_node_graph_operation_state* OpState = CreateOperationState(PanNodeGraph, &State->Modes, pan_node_graph_operation_state);
    
    panel* NodeGraphPanel = State->HotPanel;
    node_graph_state* NodeGraphState = (node_graph_state*)NodeGraphPanel->PanelStateMemory;
    OpState->InitialViewOffset = NodeGraphState->ViewOffset;
    OpState->ViewOffset = &NodeGraphState->ViewOffset;
}

//
// Connect Nodes
//

OPERATION_STATE_DEF(connect_nodes_operation_state)
{
    u32 NodeIndex;
    u32 PortIndex;
};

OPERATION_RENDER_PROC(UpdateConnectNodeOperation)
{
    
}

FOLDHAUS_INPUT_COMMAND_PROC(EndConnectNodesOperation)
{
    connect_nodes_operation_state* OpState = GetCurrentOperationState(State->Modes, connect_nodes_operation_state);
    
    EndCurrentOperationMode(State, Event, Mouse);
}

input_command ConnectNodesOperationCommands[] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Began, EndConnectNodesOperation },
};

internal void
BeginConnectNodesOperation(u32 NodeIndex, u32 PortIndex, app_state* State)
{
    operation_mode* ConnectNodesOperation = ActivateOperationModeWithCommands(&State->Modes, ConnectNodesOperationCommands, UpdateConnectNodeOperation);
    connect_nodes_operation_state* OpState = CreateOperationState(ConnectNodesOperation, &State->Modes, connect_nodes_operation_state);
    OpState->NodeIndex = NodeIndex;
    OpState->PortIndex = PortIndex;
}

//
// Node Graph Panel
//

input_command NodeGraph_Commands[] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Began, BeginPanNodeGraph }
};

PANEL_INIT_PROC(NodeGraph_Init)
{
    // TODO(Peter): We aren't able to free this memory. We need a system for 
    // taking fixed size chunks off the Memory stack and then reusing them. THis 
    // should probably live outside the paneling system.
    Panel->PanelStateMemory = (u8*)PushStruct(&State->Permanent, node_graph_state);
}

PANEL_CLEANUP_PROC(NodeGraph_Cleanup)
{
    
}

internal void
DrawGrid (v2 Offset, v2 GridSquareDim, rect PanelBounds, render_command_buffer* RenderBuffer)
{
    r32 LineValue = .16f;
    v4 LineColor = v4{LineValue, LineValue, LineValue, 1.f};
    
    v2 GridSquareOffset = v2{
        GSModF(Offset.x, GridSquareDim.x),
        GSModF(Offset.y, GridSquareDim.y),
    };
    v2 GridOrigin = PanelBounds.Min + GridSquareOffset;
    
    // Draw Vertical Lines
    r32 XOffset = 0;
    while (GridOrigin.x + XOffset < PanelBounds.Max.x)
    {
        v2 LineMin = v2{ GridOrigin.x + XOffset, PanelBounds.Min.y };
        v2 LineMax = v2{ LineMin.x + 1, PanelBounds.Max.y };
        PushRenderQuad2D(RenderBuffer, LineMin, LineMax, LineColor);
        
        XOffset += GridSquareDim.x;
    }
    
    // Draw Horizontal Lines
    r32 YOffset = 0;
    while (GridOrigin.y + YOffset < PanelBounds.Max.y)
    {
        v2 LineMin = v2{ PanelBounds.Min.x, GridOrigin.y + YOffset };
        v2 LineMax = v2{ PanelBounds.Max.x, LineMin.y + 1,  };
        PushRenderQuad2D(RenderBuffer, LineMin, LineMax, LineColor);
        
        YOffset += GridSquareDim.y;
    }
    
}

internal s32
DrawNodePorts(node_specification Spec, b32 InputMask, v2 Position, r32 LineHeight, string_alignment TextAlign, v2 TextOffset, interface_config Interface, render_command_buffer* RenderBuffer, mouse_state Mouse)
{
    s32 PortClicked = -1;
    
    rect PortBounds = rect{v2{0, 0}, v2{6, 6}};
    
    v2 LinePosition = Position;
    for (u32 i = 0; i < Spec.MemberListLength; i++)
    {
        node_struct_member Member = Spec.MemberList[i];
        if ((Member.IsInput & InputMask) > 0)
        {
            string MemberName = MakeString(Member.Name, CharArrayLength(Member.Name));
            DrawString(RenderBuffer, MemberName, Interface.Font, LinePosition + TextOffset, WhiteV4, TextAlign);
            
            rect PositionedPortBounds = PortBounds;
            PositionedPortBounds.Min += LinePosition + v2{0, LineHeight / 4};
            PositionedPortBounds.Max += LinePosition + v2{0, LineHeight / 4};
            if (TextAlign == Align_Left)
            {
                PositionedPortBounds.Min -= v2{PortBounds.Max.x, 0};
                PositionedPortBounds.Max -= v2{PortBounds.Max.x, 0};
            }
            
            
            PushRenderQuad2D(RenderBuffer, PositionedPortBounds.Min, PositionedPortBounds.Max, WhiteV4);
            
            if (MouseButtonTransitionedDown(Mouse.LeftButtonState)
                && PointIsInRect(Mouse.DownPos, PositionedPortBounds))
            {
                PortClicked = i;
            }
            
            LinePosition.y -= LineHeight;
        }
    }
    
    return PortClicked;
}

internal s32
DrawNode (v2 Position, node_specification NodeSpecification, r32 NodeWidth, r32 LineHeight, interface_config Interface, render_command_buffer* RenderBuffer, mouse_state Mouse)
{
    s32 PortClicked = -1;
    
    u32 InputMembers = 0;
    u32 OutputMembers = 0;
    for (u32 i = 0; i < NodeSpecification.MemberListLength; i++)
    {
        node_struct_member Member = NodeSpecification.MemberList[i];
        if ((Member.IsInput & IsOutputMember) > 0) { OutputMembers++; }
        if ((Member.IsInput & IsInputMember) > 0) { InputMembers++; }
    }
    u32 LineCount = 1 + GSMax(InputMembers, OutputMembers);
    
    v2 NodeDim = v2{
        NodeWidth, 
        (LineHeight * LineCount) + Interface.Margin.y,
    };
    rect NodeBounds = rect{
        v2{ Position.x, Position.y - NodeDim.y },
        v2{ Position.x + NodeDim.x, Position.y },
    };
    
    PushRenderQuad2D(RenderBuffer, NodeBounds.Min, NodeBounds.Max, v4{.16f, .16f, .16f, 1.f});
    
    v2 LinePosition = v2{ NodeBounds.Min.x, NodeBounds.Max.y - LineHeight };
    v2 TextOffset = v2{Interface.Margin.x, 0};
    
    PushRenderQuad2D(RenderBuffer, LinePosition, LinePosition + v2{NodeWidth, LineHeight}, v4{1.f, .24f, .39f, 1.f});
    string NodeName = MakeString(NodeSpecification.Name, NodeSpecification.NameLength);
    DrawString(RenderBuffer, NodeName, Interface.Font, LinePosition + TextOffset, WhiteV4);
    LinePosition.y -= LineHeight;
    
    // Draw Ports
    s32 InputPortClicked = DrawNodePorts(NodeSpecification, IsInputMember, LinePosition, LineHeight, Align_Left, TextOffset, Interface, RenderBuffer, Mouse);
    
    v2 OutputLinePosition = v2{LinePosition.x + NodeDim.x, LinePosition.y };
    v2 OutputTextOffset = v2{-TextOffset.x, TextOffset.y};
    s32 OutputPortClicked = DrawNodePorts(NodeSpecification, IsOutputMember, OutputLinePosition, LineHeight, Align_Right, OutputTextOffset, Interface, RenderBuffer, Mouse);
    
    
    if (InputPortClicked >= 0)
    {
        PortClicked = InputPortClicked;
    }
    else if (OutputPortClicked >= 0)
    {
        PortClicked = OutputPortClicked;
    }
    
    return PortClicked;
}

struct node_layout
{
    // NOTE(Peter): This Map is a sparse array.
    // index i corresponds to index i in some list of nodes
    // the value at index i is the index of that node in a compressed list
    // if the value at i is -1, that means the entry is free
    s32* SparseToContiguousNodeMap;
    u32  SparseToContiguousNodeMapCount;
    
    visual_node* VisualNodes;
    u32*         VisualNodeLayers;
    u32          VisualNodesCount;
    
    u32 LayerCount;
    v2* LayerPositions;
};

internal u32
FindLayerForNodeInList(gs_list_handle NodeHandle, gs_bucket<gs_list_handle> NodeLUT)
{
    u32 Index = 0;
    // TODO(Peter): This is turning this layout code into an n^2 lookup
    for (u32 i = 0; i < NodeLUT.Used; i++)
    {
        gs_list_handle Handle = *NodeLUT.GetElementAtIndex(i);
        if (GSListHandlesAreEqual(Handle, NodeHandle))
        {
            Index = i;
            break;
        }
    }
    return Index;
}

internal node_layout
ArrangeNodes(pattern_node_workspace Workspace, r32 NodeWidth, r32 LayerDistance, memory_arena* Storage)
{
    node_layout Result = {};
    
    Result.SparseToContiguousNodeMapCount = Workspace.Nodes.OnePastLastUsed;
    Result.SparseToContiguousNodeMap = PushArray(Storage, s32, Result.SparseToContiguousNodeMapCount);
    u32 DestinationIndex = 0;
    for (u32 i = 0; i < Result.SparseToContiguousNodeMapCount; i++)
    {
        gs_list_entry<pattern_node>* Entry = Workspace.Nodes.GetEntryAtIndex(i);
        if (!EntryIsFree(Entry)) 
        { 
            Result.SparseToContiguousNodeMap[i] = DestinationIndex++;
        }
        else
        {
            Result.SparseToContiguousNodeMap[i] = -1;
        }
    }
    
    // Figure out how to arrange nodes
    Result.LayerCount = 1;
    
    Result.VisualNodeLayers = PushArray(Storage, u32, Workspace.Nodes.Used);
    GSZeroMemory((u8*)Result.VisualNodeLayers, sizeof(u32) * Workspace.Nodes.Used);
    
    for (u32 c = 0; c < Workspace.Connections.Used; c++)
    {
        pattern_node_connection Connection = *Workspace.Connections.GetElementAtIndex(c);
        
        u32 UpstreamNodeLayerIndex = Result.SparseToContiguousNodeMap[Connection.UpstreamNodeHandle.Index];
        u32 DownstreamNodeLayerIndex = Result.SparseToContiguousNodeMap[Connection.DownstreamNodeHandle.Index];
        
        u32 UpstreamNodeInitialLayer = Result.VisualNodeLayers[UpstreamNodeLayerIndex];
        u32 DownstreamNodeLayer = Result.VisualNodeLayers[DownstreamNodeLayerIndex];
        
        Result.VisualNodeLayers[UpstreamNodeLayerIndex] = GSMax(UpstreamNodeInitialLayer, DownstreamNodeLayer + 1);
        Result.LayerCount = GSMax(Result.VisualNodeLayers[UpstreamNodeLayerIndex] + 1, Result.LayerCount);
    }
    
    // Place Layer Columns
    Result.LayerPositions = PushArray(Storage, v2, Result.LayerCount);
    for (u32 l = 0; l < Result.LayerCount; l++)
    {
        u32 FromRight = Result.LayerCount - l;
        Result.LayerPositions[l] = v2{ (NodeWidth + LayerDistance) * FromRight, 0 };
    }
    
    // Place nodes
    Result.VisualNodesCount = Workspace.Nodes.Used;
    Result.VisualNodes = PushArray(Storage, visual_node, Result.VisualNodesCount);
    for (u32 n = 0; n < Workspace.Nodes.Used; n++)
    {
        u32 NodeIndex = Result.SparseToContiguousNodeMap[n];
        pattern_node* Node = Workspace.Nodes.GetElementAtIndex(NodeIndex);
        u32 SpecIndex = Node->SpecificationIndex;
        Result.VisualNodes[n].Spec = NodeSpecifications[SpecIndex];
        
        u32 NodeLayer = Result.VisualNodeLayers[n];
        Result.VisualNodes[n].Position = Result.LayerPositions[NodeLayer];
        Result.LayerPositions[NodeLayer].y -= 200;
    }
    
    return Result;
}

internal 
PANEL_RENDER_PROC(NodeGraph_Render)
{
    node_graph_state* GraphState = (node_graph_state*)Panel.PanelStateMemory;
    
    rect NodeSelectionWindowBounds = rect{
        PanelBounds.Min,
        v2{PanelBounds.Min.x + 300, PanelBounds.Max.y},
    };
    rect GraphBounds = rect{
        v2{NodeSelectionWindowBounds.Max.x, PanelBounds.Min.y},
        PanelBounds.Max,
    };
    
    r32 NodeWidth = 150;
    r32 LayerDistance = 100;
    r32 LineHeight = (State->Interface.Font->PixelHeight + (2 * State->Interface.Margin.y));
    
    node_layout NodeLayout = ArrangeNodes(State->NodeWorkspace, NodeWidth, LayerDistance, &State->Transient);
    
    DrawGrid(GraphState->ViewOffset, v2{100, 100}, GraphBounds, RenderBuffer);
    
    render_quad_batch_constructor ConnectionsLayer = PushRenderQuad2DBatch(RenderBuffer, State->NodeWorkspace.Connections.Used);
    for (u32 i = 0; i < State->NodeWorkspace.Connections.Used; i++)
    {
        pattern_node_connection Connection = *State->NodeWorkspace.Connections.GetElementAtIndex(i);
        
        u32 UpstreamNodeVisualIndex = NodeLayout.SparseToContiguousNodeMap[Connection.UpstreamNodeHandle.Index];
        u32 DownstreamNodeVisualIndex = NodeLayout.SparseToContiguousNodeMap[Connection.DownstreamNodeHandle.Index];
        
        visual_node UpstreamNode = NodeLayout.VisualNodes[UpstreamNodeVisualIndex];
        visual_node DownstreamNode = NodeLayout.VisualNodes[DownstreamNodeVisualIndex];
        
        v2 LineStart = GraphState->ViewOffset + UpstreamNode.Position + v2{NodeWidth, 0} - (v2{0, LineHeight} * (Connection.UpstreamPortIndex + 2)) + v2{0, LineHeight / 3};
        v2 LineEnd = GraphState->ViewOffset + DownstreamNode.Position - (v2{0, LineHeight} * (Connection.DownstreamPortIndex + 2)) + v2{0, LineHeight / 3};
        
        PushLine2DOnBatch(&ConnectionsLayer, LineStart, LineEnd, 1.5f, WhiteV4);
    }
    
    for (u32 i = 0; i < NodeLayout.VisualNodesCount; i++)
    {
        visual_node VisualNode = NodeLayout.VisualNodes[i];
        s32 PortClicked = DrawNode(VisualNode.Position + GraphState->ViewOffset, VisualNode.Spec, NodeWidth, LineHeight, State->Interface, RenderBuffer, Mouse);
        if (PortClicked >= 0)
        {
            BeginConnectNodesOperation(i, PortClicked, State);
        }
    }
    
    // Node Selection Panel
    v4 LineBGColors[] = {
        { .16f, .16f, .16f, 1.f },
        { .18f, .18f, .18f, 1.f },
    };
    
    interface_list List = {};
    List.LineBGColors = LineBGColors;
    List.LineBGColorsCount = sizeof(LineBGColors) / sizeof(LineBGColors[0]);
    List.LineBGHoverColor = v4{ .22f, .22f, .22f, 1.f };
    List.TextColor = WhiteV4;
    List.ListBounds = NodeSelectionWindowBounds;
    List.ListElementDimensions = v2{
        Width(NodeSelectionWindowBounds), 
        (r32)(State->Interface.Font->PixelHeight + 8),
    };
    List.ElementLabelIndent = v2{10, 4};
    
    string TitleString = MakeStringLiteral("Available Nodes");
    DrawListElement(TitleString, &List, Mouse, RenderBuffer, State->Interface);
    
    for (s32 i = 0; i < NodeSpecificationsCount; i++)
    {
        node_specification Spec = NodeSpecifications[i];
        string NodeName = MakeString(Spec.Name, Spec.NameLength);
        rect ElementBounds = DrawListElement(NodeName, &List, Mouse, RenderBuffer, State->Interface);
        
        if (MouseButtonTransitionedDown(Mouse.LeftButtonState) 
            && PointIsInRect(Mouse.DownPos, ElementBounds))
        {
            PushNodeOnWorkspace(i, &State->NodeWorkspace);
        }
    }
}