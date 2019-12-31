struct visual_node
{
    node_specification Spec;
    v2 Position;
};

struct visual_port
{
    gs_list_handle SparseNodeHandle;
    u32 PortIndex;
    rect PortBounds;
};

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
    
    visual_port* VisualPorts;
    u32          VisualPortsCount;
    
    u32 LayerCount;
    v2* LayerPositions;
    
    b32 ConnectionIsInProgress;
    v2 InProgressConnectionStart;
    v2 InProgressConnectionEnd;
};

struct node_graph_state
{
    v2 ViewOffset;
    
    memory_arena LayoutMemory;
    node_layout Layout;
    
    b32 LayoutIsDirty;
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
    visual_port VisualPort;
    u32 VisualPortIndex;
    b32 IsInput;
};

OPERATION_RENDER_PROC(UpdateConnectNodeOperation)
{
    panel_and_bounds NodeGraphPanel = GetPanelContainingPoint(Mouse.DownPos, &State->PanelSystem, State->WindowBounds);
    node_graph_state* GraphState = (node_graph_state*)NodeGraphPanel.Panel->PanelStateMemory;
    
    GraphState->Layout.InProgressConnectionEnd = Mouse.Pos;
}

FOLDHAUS_INPUT_COMMAND_PROC(EndConnectNodesOperation)
{
    connect_nodes_operation_state* OpState = GetCurrentOperationState(State->Modes, connect_nodes_operation_state);
    
    panel_and_bounds NodeGraphPanel = GetPanelContainingPoint(Mouse.DownPos, &State->PanelSystem, State->WindowBounds);
    node_graph_state* GraphState = (node_graph_state*)NodeGraphPanel.Panel->PanelStateMemory;
    GraphState->Layout.ConnectionIsInProgress = false;
    
    for (u32 p = 0; p < GraphState->Layout.VisualPortsCount; p++)
    {
        visual_port VisualPort = GraphState->Layout.VisualPorts[p];
        rect ViewAdjustedBounds = RectOffsetByVector(VisualPort.PortBounds, GraphState->ViewOffset);
        if (PointIsInRect(Mouse.Pos, ViewAdjustedBounds))
        {
            visual_port UpstreamPort = (OpState->IsInput & IsInputMember) ? VisualPort : OpState->VisualPort;
            visual_port DownstreamPort = (OpState->IsInput & IsInputMember) ? OpState->VisualPort : VisualPort;
            
            // Make Connection
            pattern_node_connection Connection = {};
            Connection.UpstreamNodeHandle = UpstreamPort.SparseNodeHandle;
            Connection.DownstreamNodeHandle = DownstreamPort.SparseNodeHandle;
            Connection.UpstreamPortIndex = UpstreamPort.PortIndex;
            Connection.DownstreamPortIndex = DownstreamPort.PortIndex;
            
            State->NodeWorkspace.Connections.PushElementOnBucket(Connection);
            GraphState->LayoutIsDirty = true;
        }
    }
    
    EndCurrentOperationMode(State, Event, Mouse);
}

input_command ConnectNodesOperationCommands[] = {
    { KeyCode_MouseLeftButton, KeyCode_Invalid, Command_Ended, EndConnectNodesOperation },
};

internal void
BeginConnectNodesOperation(visual_port VisualPort, u32 VisualPortIndex, mouse_state Mouse, app_state* State)
{
    operation_mode* ConnectNodesOperation = ActivateOperationModeWithCommands(&State->Modes, ConnectNodesOperationCommands, UpdateConnectNodeOperation);
    connect_nodes_operation_state* OpState = CreateOperationState(ConnectNodesOperation, &State->Modes, connect_nodes_operation_state);
    OpState->VisualPort = VisualPort;
    OpState->VisualPortIndex = VisualPortIndex;
    
    panel_and_bounds NodeGraphPanel = GetPanelContainingPoint(Mouse.DownPos, &State->PanelSystem, State->WindowBounds);
    node_graph_state* GraphState = (node_graph_state*)NodeGraphPanel.Panel->PanelStateMemory;
    
    GraphState->Layout.ConnectionIsInProgress = true;
    GraphState->Layout.InProgressConnectionStart = CalculateRectCenter(VisualPort.PortBounds);
}

//
// Node Graph Panel
//

input_command NodeGraph_Commands[] = {
    { 0 }
};

PANEL_INIT_PROC(NodeGraph_Init)
{
    // TODO(Peter): We aren't able to free this memory. We need a system for 
    // taking fixed size chunks off the Memory stack and then reusing them. THis 
    // should probably live outside the paneling system.
    Panel->PanelStateMemory = (u8*)PushStruct(&State->Permanent, node_graph_state);
    node_graph_state* GraphState = (node_graph_state*)Panel->PanelStateMemory;
    GraphState->LayoutIsDirty = true;
}

PANEL_CLEANUP_PROC(NodeGraph_Cleanup)
{
    node_graph_state* GraphState = (node_graph_state*)Panel->PanelStateMemory;
    FreeMemoryArena(&GraphState->LayoutMemory);
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

internal void
DrawNodePorts(node_specification Spec, b32 InputMask, v2 Position, r32 LineHeight, string_alignment TextAlign, v2 TextOffset, interface_config Interface, render_command_buffer* RenderBuffer, mouse_state Mouse)
{
    rect PortBounds = rect{v2{0, 0}, v2{6, 6}};
    
    v2 LinePosition = Position;
    for (u32 i = 0; i < Spec.MemberListLength; i++)
    {
        node_struct_member Member = Spec.MemberList[i];
        if ((Member.IsInput & InputMask) > 0)
        {
            // TODO(Peter): Can we make this rely on the same data that we use to 
            // render the actual connection points?
            string MemberName = MakeString(Member.Name, CharArrayLength(Member.Name));
            DrawString(RenderBuffer, MemberName, Interface.Font, LinePosition + TextOffset, WhiteV4, TextAlign);
            LinePosition.y -= LineHeight;
        }
    }
}

internal void
DrawNode (v2 Position, node_specification NodeSpecification, r32 NodeWidth, r32 LineHeight, interface_config Interface, render_command_buffer* RenderBuffer, mouse_state Mouse)
{
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
    
    DrawNodePorts(NodeSpecification, IsInputMember, LinePosition, LineHeight, Align_Left, TextOffset, Interface, RenderBuffer, Mouse);
    
    v2 OutputLinePosition = v2{LinePosition.x + NodeDim.x, LinePosition.y };
    v2 OutputTextOffset = v2{-TextOffset.x, TextOffset.y};
    DrawNodePorts(NodeSpecification, IsOutputMember, OutputLinePosition, LineHeight, Align_Right, OutputTextOffset, Interface, RenderBuffer, Mouse);
}

internal node_layout
ArrangeNodes(pattern_node_workspace Workspace, r32 NodeWidth, r32 LayerDistance, r32 LineHeight, memory_arena* Storage)
{
    node_layout Result = {};
    
    Result.SparseToContiguousNodeMapCount = Workspace.Nodes.OnePastLastUsed;
    Result.SparseToContiguousNodeMap = PushArray(Storage, s32, Result.SparseToContiguousNodeMapCount);
    u32 DestinationIndex = 0;
    Result.VisualPortsCount = 0;
    for (u32 i = 0; i < Result.SparseToContiguousNodeMapCount; i++)
    {
        gs_list_entry<pattern_node>* Entry = Workspace.Nodes.GetEntryAtIndex(i);
        if (!EntryIsFree(Entry)) 
        { 
            Result.SparseToContiguousNodeMap[i] = DestinationIndex++;
            
            pattern_node Node = Entry->Value;
            node_specification Spec = NodeSpecifications[Node.SpecificationIndex];
            Result.VisualPortsCount += Spec.MemberListLength;
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
    
    // Place nodes and connections
    Result.VisualNodesCount = Workspace.Nodes.Used;
    Result.VisualNodes = PushArray(Storage, visual_node, Result.VisualNodesCount);
    
    u32 VisualPortsUsed = 0;
    Result.VisualPorts = PushArray(Storage, visual_port, Result.VisualPortsCount);
    
    for (u32 n = 0; n < Result.SparseToContiguousNodeMapCount; n++)
    {
        u32 NodeIndex = Result.SparseToContiguousNodeMap[n];
        gs_list_entry<pattern_node>* NodeEntry = Workspace.Nodes.GetEntryAtIndex(NodeIndex);
        pattern_node Node = NodeEntry->Value;
        
        u32 SpecIndex = Node.SpecificationIndex;
        node_specification Spec = NodeSpecifications[SpecIndex];
        u32 NodeLayer = Result.VisualNodeLayers[n];
        
        visual_node* VisualNode = Result.VisualNodes + n;
        VisualNode->Spec = Spec;
        VisualNode->Position = Result.LayerPositions[NodeLayer];
        Result.LayerPositions[NodeLayer].y -= 200;
        
        // NOTE(Peter): These start at 2 to account for the offset past the node title
        s32 InputsCount = 2; 
        s32 OutputsCount = 2;
        for (u32 p = 0; p < Spec.MemberListLength; p++)
        {
            node_struct_member Member = Spec.MemberList[p];
            
            rect PortBounds = {0};
            v2 PortDim = v2{8, 8};
            PortBounds.Min = VisualNode->Position + v2{0, PortDim.y / 2}; 
            if ((Member.IsInput & IsInputMember) > 0)
            {
                PortBounds.Min.y -= LineHeight * InputsCount++;
                PortBounds.Min.x -= PortDim.x;
            }
            else if ((Member.IsInput & IsOutputMember) > 0)
            {
                PortBounds.Min.y -= LineHeight * OutputsCount++;
                PortBounds.Min.x += NodeWidth;
            }
            PortBounds.Max = PortBounds.Min + v2{8, 8};
            
            visual_port* VisualPort = Result.VisualPorts + VisualPortsUsed++;
            VisualPort->SparseNodeHandle = NodeEntry->Handle;
            VisualPort->PortIndex = p;
            VisualPort->PortBounds = PortBounds;
        }
    }
    
    return Result;
}

internal 
PANEL_RENDER_PROC(NodeGraph_Render)
{
    node_graph_state* GraphState = (node_graph_state*)Panel.PanelStateMemory;
    b32 MouseHandled = false;
    
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
    
    if (GraphState->LayoutIsDirty)
    {
        // NOTE(Peter): Resset the LayoutMemory arena so we can use it again.
        // If LayoutIsDirty, then we need to recalculate all the members of GraphState->Layout
        // so we might as well just clear the whole thing (we aren't freeing, just reusing)
        ClearArena(&GraphState->LayoutMemory);
        GraphState->Layout = {};
        
        GraphState->Layout = ArrangeNodes(State->NodeWorkspace, NodeWidth, LayerDistance, LineHeight, &GraphState->LayoutMemory);
        GraphState->LayoutIsDirty = false;
    }
    
    DrawGrid(GraphState->ViewOffset, v2{100, 100}, GraphBounds, RenderBuffer);
    
    render_quad_batch_constructor ConnectionsLayer = PushRenderQuad2DBatch(RenderBuffer, State->NodeWorkspace.Connections.Used + 1);
    for (u32 i = 0; i < State->NodeWorkspace.Connections.Used; i++)
    {
        pattern_node_connection Connection = *State->NodeWorkspace.Connections.GetElementAtIndex(i);
        
        u32 UpstreamNodeVisualIndex = GraphState->Layout.SparseToContiguousNodeMap[Connection.UpstreamNodeHandle.Index];
        u32 DownstreamNodeVisualIndex = GraphState->Layout.SparseToContiguousNodeMap[Connection.DownstreamNodeHandle.Index];
        
        visual_node UpstreamNode = GraphState->Layout.VisualNodes[UpstreamNodeVisualIndex];
        visual_node DownstreamNode = GraphState->Layout.VisualNodes[DownstreamNodeVisualIndex];
        
        v2 LineStart = GraphState->ViewOffset + UpstreamNode.Position + v2{NodeWidth, 0} - (v2{0, LineHeight} * (Connection.UpstreamPortIndex + 2)) + v2{0, LineHeight / 3};
        v2 LineEnd = GraphState->ViewOffset + DownstreamNode.Position - (v2{0, LineHeight} * (Connection.DownstreamPortIndex + 2)) + v2{0, LineHeight / 3};
        
        PushLine2DOnBatch(&ConnectionsLayer, LineStart, LineEnd, 1.5f, WhiteV4);
    }
    
    if (GraphState->Layout.ConnectionIsInProgress)
    {
        PushLine2DOnBatch(&ConnectionsLayer, 
                          GraphState->Layout.InProgressConnectionStart, 
                          GraphState->Layout.InProgressConnectionEnd,
                          1.5f, WhiteV4);
    }
    
    for (u32 i = 0; i < GraphState->Layout.VisualNodesCount; i++)
    {
        visual_node VisualNode = GraphState->Layout.VisualNodes[i];
        DrawNode(VisualNode.Position + GraphState->ViewOffset, VisualNode.Spec, NodeWidth, LineHeight, State->Interface, RenderBuffer, Mouse);
    }
    
    for (u32 p = 0; p < GraphState->Layout.VisualPortsCount; p++)
    {
        visual_port VisualPort = GraphState->Layout.VisualPorts[p];
        VisualPort.PortBounds.Min += GraphState->ViewOffset;
        VisualPort.PortBounds.Max += GraphState->ViewOffset;
        
        v4 PortColor = WhiteV4;
        if (PointIsInRange(Mouse.Pos, VisualPort.PortBounds.Min, VisualPort.PortBounds.Max))
        {
            PortColor = PinkV4;
            if (MouseButtonTransitionedDown(Mouse.LeftButtonState))
            {
                BeginConnectNodesOperation(VisualPort, p, Mouse, State);
                MouseHandled = true;
            }
        }
        
        PushRenderQuad2D(RenderBuffer, VisualPort.PortBounds.Min, VisualPort.PortBounds.Max, PortColor);
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
            GraphState->LayoutIsDirty = true;
            MouseHandled = true;
        }
    }
    
    if (!MouseHandled && MouseButtonTransitionedDown(Mouse.LeftButtonState))
    {
        BeginPanNodeGraph(State, {}, Mouse);
    }
}