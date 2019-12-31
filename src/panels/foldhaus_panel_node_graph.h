struct node_graph_state
{
    v2 ViewOffset;
};


struct temp_node_connection
{
    u32 DownstreamNodeIndex;
    u32 DownstreamNodePort;
    
    u32 UpstreamNodeIndex;
    u32 UpstreamNodePort;
};

struct visual_node
{
    node_specification Spec;
    v2 Position;
};

#define TEMP_NODE_LIST_MAX 10
global_variable u32 TEMP_NodeListUsed = 0;
global_variable node_specification TEMP_NodeList[TEMP_NODE_LIST_MAX];

#define TEMP_CONNECTIONS_LIST_MAX 10
global_variable u32 TEMP_NodeConnectionsUsed = 0;
global_variable temp_node_connection TEMP_NodeConnections[TEMP_CONNECTIONS_LIST_MAX];

internal void
PushNodeOnNodeList(node_specification Spec)
{
    if (TEMP_NodeListUsed < TEMP_NODE_LIST_MAX)
    {
        u32 Index = TEMP_NodeListUsed++;
        TEMP_NodeList[Index] = Spec;
    }
}

internal void
PushConnectionOnConnectionsList(u32 UpstreamNodeIndex, u32 UpstreamNodePort, u32 DownstreamNodeIndex, u32 DownstreamNodePort)
{
    if (TEMP_NodeConnectionsUsed < TEMP_CONNECTIONS_LIST_MAX)
    {
        u32 Index = TEMP_NodeConnectionsUsed++;
        TEMP_NodeConnections[Index].DownstreamNodeIndex = DownstreamNodeIndex;
        TEMP_NodeConnections[Index].DownstreamNodePort = DownstreamNodePort;
        TEMP_NodeConnections[Index].UpstreamNodeIndex = UpstreamNodeIndex;
        TEMP_NodeConnections[Index].UpstreamNodePort = UpstreamNodePort;;
    }
}


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

internal visual_node*
ArrangeNodes(node_specification* NodeList, u32 NodesCount, temp_node_connection* ConnectionList, u32 ConnectionsCount, r32 NodeWidth, r32 LayerDistance, memory_arena* Storage)
{
    // Figure out how to arrange nodes
    u32 LayerCount = 1;
    
    u32* NodeLayers = PushArray(Storage, u32, NodesCount);
    GSZeroMemory((u8*)NodeLayers, sizeof(u32) * NodesCount);
    
    for (u32 c = 0; c < ConnectionsCount; c++)
    {
        temp_node_connection Connection = TEMP_NodeConnections[c];
        
        u32 UpstreamNodeInitialLayer = NodeLayers[Connection.UpstreamNodeIndex];
        u32 DownstreamNodeLayer = NodeLayers[Connection.DownstreamNodeIndex];
        
        NodeLayers[Connection.UpstreamNodeIndex] = GSMax(UpstreamNodeInitialLayer, DownstreamNodeLayer + 1);
        LayerCount = GSMax(NodeLayers[Connection.UpstreamNodeIndex] + 1, LayerCount);
    }
    
    // Place Layer Columns
    v2* LayerPositions = PushArray(Storage, v2, LayerCount);
    for (u32 l = 0; l < LayerCount; l++)
    {
        u32 FromRight = LayerCount - l;
        LayerPositions[l] = v2{ (NodeWidth + LayerDistance) * FromRight, 0 };
    }
    
    // Place nodes
    visual_node* VisualNodes = PushArray(Storage, visual_node, NodesCount);
    for (u32 n = 0; n < NodesCount; n++)
    {
        VisualNodes[n].Spec = TEMP_NodeList[n];
        
        u32 NodeLayer = NodeLayers[n];
        VisualNodes[n].Position = LayerPositions[NodeLayer];
        LayerPositions[NodeLayer].y -= 200;
    }
    
    return VisualNodes;
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
    
    visual_node* VisualNodes = ArrangeNodes(&TEMP_NodeList[0], TEMP_NodeListUsed, 
                                            &TEMP_NodeConnections[0], TEMP_NodeConnectionsUsed,
                                            NodeWidth, LayerDistance,
                                            &State->Transient);
    
    DrawGrid(GraphState->ViewOffset, v2{100, 100}, GraphBounds, RenderBuffer);
    
    render_quad_batch_constructor ConnectionsLayer = PushRenderQuad2DBatch(RenderBuffer, TEMP_NodeConnectionsUsed);
    for (u32 i = 0; i < TEMP_NodeConnectionsUsed; i++)
    {
        temp_node_connection Connection = TEMP_NodeConnections[i];
        visual_node UpstreamNode = VisualNodes[Connection.UpstreamNodeIndex];
        visual_node DownstreamNode = VisualNodes[Connection.DownstreamNodeIndex];
        
        v2 LineStart = GraphState->ViewOffset + UpstreamNode.Position + v2{NodeWidth, 0} - (v2{0, LineHeight} * (Connection.UpstreamNodePort + 2)) + v2{0, LineHeight / 3};
        v2 LineEnd = GraphState->ViewOffset + DownstreamNode.Position - (v2{0, LineHeight} * (Connection.DownstreamNodePort + 2)) + v2{0, LineHeight / 3};
        
        PushLine2DOnBatch(&ConnectionsLayer, LineStart, LineEnd, 1.5f, WhiteV4);
    }
    
    for (u32 i = 0; i < TEMP_NodeListUsed; i++)
    {
        visual_node VisualNode = VisualNodes[i];
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
            PushNodeOnNodeList(Spec);
        }
    }
}