//
// File: foldhaus_panel_node_graph.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_PANEL_NODE_GRAPH_H

struct visual_node
{
    node_specification_ Spec;
    v2 Position;
};

struct visual_port
{
    gs_list_handle SparseNodeHandle;
    u32 PortIndex;
    rect PortBounds;
};

struct visual_connection
{
    u32 UpstreamVisualPortIndex;
    u32 DownstreamVisualPortIndex;
    v2 UpstreamPosition;
    v2 DownstreamPosition;
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
    
    visual_connection* VisualConnections;
    u32                VisualConnectionsCount;
    
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
        rect ViewAdjustedBounds = gs_RectOffsetByVector(VisualPort.PortBounds, GraphState->ViewOffset);
        if (gs_PointIsInRect(Mouse.Pos, ViewAdjustedBounds))
        {
            visual_port UpstreamPort = (OpState->IsInput & IsInputMember) ? VisualPort : OpState->VisualPort;
            visual_port DownstreamPort = (OpState->IsInput & IsInputMember) ? OpState->VisualPort : VisualPort;
            
            PushNodeConnectionOnWorkspace(UpstreamPort.SparseNodeHandle, UpstreamPort.PortIndex,
                                          DownstreamPort.SparseNodeHandle, DownstreamPort.PortIndex,
                                          &State->NodeWorkspace, &State->Transient);
            
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
    GraphState->Layout.InProgressConnectionStart = gs_CalculateRectCenter(VisualPort.PortBounds);
}

//
// Node Graph Panel
//

GSMetaTag(panel_commands);
input_command NodeGraph_Commands[] = {{}};
s32 NodeGraph_CommandsCount = 0;

GSMetaTag(panel_init);
GSMetaTag(panel_type_node_graph);
internal void
NodeGraph_Init(panel* Panel, app_state* State)
{
    // TODO(Peter): We aren't able to free this memory. We need a system for
    // taking fixed size chunks off the Memory stack and then reusing them. THis
    // should probably live outside the paneling system.
    // TODO: :FreePanelMemory
    Panel->PanelStateMemory = (u8*)PushStruct(&State->Permanent, node_graph_state);
    node_graph_state* GraphState = (node_graph_state*)Panel->PanelStateMemory;
    GraphState->LayoutIsDirty = true;
}

GSMetaTag(panel_cleanup);
GSMetaTag(panel_type_node_graph);
internal void
NodeGraph_Cleanup(panel* Panel, app_state* State)
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
DrawNodePorts(gsm_struct_type_info NodeDataTypeInfo, b32 InputMask, v2 Position, r32 LineHeight, string_alignment TextAlign, v2 TextOffset, interface_config Interface, render_command_buffer* RenderBuffer, mouse_state Mouse)
{
    rect PortBounds = rect{v2{0, 0}, v2{6, 6}};
    
    v2 LinePosition = Position;
    for (u32 i = 0; i < NodeDataTypeInfo.MembersCount; i++)
    {
        gsm_struct_member_type_info Member = NodeDataTypeInfo.Members[i];
        if (MemberIsInput(Member))
        {
            // TODO(Peter): Can we make this rely on the same data that we use to
            // render the actual connection points?
            string MemberName = MakeString(Member.Identifier, Member.IdentifierLength);
            DrawString(RenderBuffer, MemberName, Interface.Font, LinePosition + TextOffset, WhiteV4, TextAlign);
            LinePosition.y -= LineHeight;
        }
    }
}

internal void
DrawNode (v2 Position, node_specification_ NodeSpecification, gs_list_handle NodeHandle, r32 NodeWidth, r32 LineHeight, interface_config Interface, render_command_buffer* RenderBuffer, mouse_state Mouse, memory_arena* Scratch)
{
    gsm_struct_type_info NodeDataTypeInfo = StructTypes[NodeSpecification.DataType];
    
    u32 InputMembers = 0;
    u32 OutputMembers = 0;
    for (u32 i = 0; i < NodeDataTypeInfo.MembersCount; i++)
    {
        gsm_struct_member_type_info Member = NodeDataTypeInfo.Members[i];
        if (MemberIsInput(Member)) { InputMembers++; }
        if (MemberIsOutput(Member)) { OutputMembers++; }
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
    
    string NodePrintName = MakeString(PushArray(Scratch, char, 256), 0, 256);
    PrintF(&NodePrintName, "%S [%d]", NodeSpecification.Identifier, NodeHandle.Index);
    DrawString(RenderBuffer, NodePrintName, Interface.Font, LinePosition + TextOffset, WhiteV4);
    LinePosition.y -= LineHeight;
    
    v2 InputLinePosition = LinePosition;
    v2 OutputLinePosition = v2{LinePosition.x + NodeDim.x, LinePosition.y };
    v2 OutputTextOffset = v2{-TextOffset.x, TextOffset.y};
    for (u32 i = 0; i < NodeDataTypeInfo.MembersCount; i++)
    {
        gsm_struct_member_type_info Member = NodeDataTypeInfo.Members[i];
        string MemberName = MakeString(Member.Identifier, Member.IdentifierLength);
        
        // TODO(Peter): Can we make this rely on the same data that we use to
        // render the actual connection points?
        if (MemberIsInput(Member))
        {
            DrawString(RenderBuffer, MemberName, Interface.Font, LinePosition + TextOffset, WhiteV4, Align_Left);
            InputLinePosition.y -= LineHeight;
        }
        else if (MemberIsOutput(Member))
        {
            DrawString(RenderBuffer, MemberName, Interface.Font, LinePosition + TextOffset, WhiteV4, Align_Right);
            OutputLinePosition.y -= LineHeight;
        }
    }
}

internal s32
GetVisualPortIndexForNode(gs_list_handle SparseNodeHandle, u32 PortIndex, node_layout Layout)
{
    s32 Result = -1;
    
    for (u32 i = 0; i < Layout.VisualPortsCount; i++)
    {
        visual_port Port = Layout.VisualPorts[i];
        if (GSListHandlesAreEqual(Port.SparseNodeHandle, SparseNodeHandle) && Port.PortIndex == PortIndex)
        {
            Result = i;
            break;
        }
    }
    
    return Result;
}

internal node_layout
ArrangeNodes(pattern_node_workspace Workspace, r32 NodeWidth, r32 LayerDistance, r32 LineHeight, memory_arena* Storage, app_state* State)
{
    node_layout Result = {};
    
    for (u32 n = 0; n < Workspace.Nodes.Used; n++)
    {
        gs_list_handle NodeHandle = Workspace.SortedNodeHandles[n];
        pattern_node Node = *Workspace.Nodes.GetElementWithHandle(NodeHandle);
        
        u32 SpecIndex = Node.SpecificationIndex;
        
        node_specification_ Spec = NodeSpecifications[SpecIndex];
        gsm_struct_type_info NodeDataTypeInfo = StructTypes[Spec.DataType];
        Result.VisualPortsCount += NodeDataTypeInfo.MembersCount;;
    }
    
    // Place nodes and connections
    Result.VisualNodesCount = Workspace.Nodes.Used;
    Result.VisualNodes = PushArray(Storage, visual_node, Result.VisualNodesCount);
    
    u32 VisualPortsUsed = 0;
    Result.VisualPorts = PushArray(Storage, visual_port, Result.VisualPortsCount);
    
    for (u32 n = 0; n < Workspace.Nodes.Used; n++)
    {
        gs_list_handle NodeHandle = Workspace.SortedNodeHandles[n];
        pattern_node Node = *Workspace.Nodes.GetElementWithHandle(NodeHandle);
        
        u32 SpecIndex = Node.SpecificationIndex;
        
        node_specification_ Spec = NodeSpecifications[SpecIndex];
        gsm_struct_type_info NodeDataTypeInfo = StructTypes[Spec.DataType];
        
        visual_node* VisualNode = Result.VisualNodes + n;
        VisualNode->Spec = Spec;
        VisualNode->Position = v2{(1.5f * NodeWidth) * n, 0};
        
        // NOTE(Peter): These start at 2 to account for the offset past the node title
        s32 InputsCount = 2;
        s32 OutputsCount = 2;
        for (u32 p = 0; p < NodeDataTypeInfo.MembersCount; p++)
        {
            gsm_struct_member_type_info Member = NodeDataTypeInfo.Members[p];
            
            rect PortBounds = {0};
            v2 PortDim = v2{8, 8};
            PortBounds.Min = VisualNode->Position + v2{0, PortDim.y / 2};
            if (MemberIsInput(Member))
            {
                PortBounds.Min.y -= LineHeight * InputsCount++;
                PortBounds.Min.x -= PortDim.x;
            }
            else if (MemberIsOutput(Member))
            {
                PortBounds.Min.y -= LineHeight * OutputsCount++;
                PortBounds.Min.x += NodeWidth;
            }
            PortBounds.Max = PortBounds.Min + v2{8, 8};
            
            visual_port* VisualPort = Result.VisualPorts + VisualPortsUsed++;
            VisualPort->SparseNodeHandle = NodeHandle;
            VisualPort->PortIndex = p;
            VisualPort->PortBounds = PortBounds;
        }
    }
    
    Result.VisualConnectionsCount = 0;
    
    Result.VisualConnectionsCount = Workspace.Connections.Used;
    Result.VisualConnections = PushArray(Storage, visual_connection, Result.VisualConnectionsCount);
    for (u32 c = 0; c < Workspace.Connections.Used; c++)
    {
        pattern_node_connection* Connection = Workspace.Connections.GetElementAtIndex(c);
        
        visual_connection* VisualConnection = Result.VisualConnections + c;
        VisualConnection->UpstreamVisualPortIndex = GetVisualPortIndexForNode(Connection->UpstreamNodeHandle, Connection->UpstreamPortIndex, Result);
        VisualConnection->DownstreamVisualPortIndex = GetVisualPortIndexForNode(Connection->DownstreamNodeHandle, Connection->DownstreamPortIndex, Result);
        
        visual_port UpstreamPort = Result.VisualPorts[VisualConnection->UpstreamVisualPortIndex];
        visual_port DownstreamPort = Result.VisualPorts[VisualConnection->DownstreamVisualPortIndex];
        
        VisualConnection->UpstreamPosition = gs_CalculateRectCenter(UpstreamPort.PortBounds);
        VisualConnection->DownstreamPosition = gs_CalculateRectCenter(DownstreamPort.PortBounds);
    }
    
    return Result;
}

GSMetaTag(panel_render);
GSMetaTag(panel_type_node_graph);
internal void
NodeGraph_Render(panel Panel, rect PanelBounds, render_command_buffer* RenderBuffer, app_state* State, context Context, mouse_state Mouse)
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
    r32 LineHeight = ui_GetTextLineHeight(State->Interface_);
    
    if (GraphState->LayoutIsDirty)
    {
        // NOTE(Peter): Resset the LayoutMemory arena so we can use it again.
        // If LayoutIsDirty, then we need to recalculate all the members of GraphState->Layout
        // so we might as well just clear the whole thing (we aren't freeing, just reusing)
        ClearArena(&GraphState->LayoutMemory);
        GraphState->Layout = {};
        
        GraphState->Layout = ArrangeNodes(State->NodeWorkspace, NodeWidth, LayerDistance, LineHeight, &GraphState->LayoutMemory, State);
        GraphState->LayoutIsDirty = false;
    }
    
    DrawGrid(GraphState->ViewOffset, v2{100, 100}, GraphBounds, RenderBuffer);
    
    for (u32 i = 0; i < GraphState->Layout.VisualConnectionsCount; i++)
    {
        visual_connection Connection = GraphState->Layout.VisualConnections[i];
        
        v2 Start = GraphState->ViewOffset + Connection.UpstreamPosition;
        v2 End = GraphState->ViewOffset + Connection.DownstreamPosition;
        PushRenderLine2D(RenderBuffer, Start, End, 1.5f, WhiteV4);
        
        v2 TempDim = v2{6, 6};
        PushRenderQuad2D(RenderBuffer, Start - TempDim, Start + TempDim, PinkV4);
        PushRenderQuad2D(RenderBuffer, End - TempDim, End + TempDim, YellowV4);
    }
    
    if (GraphState->Layout.ConnectionIsInProgress)
    {
        PushRenderLine2D(RenderBuffer,
                         GraphState->Layout.InProgressConnectionStart,
                         GraphState->Layout.InProgressConnectionEnd,
                         1.5f, WhiteV4);
    }
    
    for (u32 i = 0; i < GraphState->Layout.VisualNodesCount; i++)
    {
        visual_node VisualNode = GraphState->Layout.VisualNodes[i];
        gs_list_handle NodeHandle = State->NodeWorkspace.SortedNodeHandles[i];
        DrawNode(VisualNode.Position + GraphState->ViewOffset, VisualNode.Spec, NodeHandle, NodeWidth, LineHeight, State->Interface_.Style, RenderBuffer, Mouse, &State->Transient);
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
        gs_Width(NodeSelectionWindowBounds),
        ui_GetTextLineHeight(State->Interface_)
    };
    List.ElementLabelIndent = v2{10, 4};
    
    string TitleString = MakeStringLiteral("Available Nodes");
    DrawListElement(TitleString, &List, Mouse, RenderBuffer, State->Interface_.Style);
    
    for (u32 i = 0; i < NodeType_Count; i++)
    {
        node_specification_ Spec = NodeSpecifications[i];
        rect ElementBounds = DrawListElement(Spec.Identifier, &List, Mouse, RenderBuffer, State->Interface_.Style);
        
        if (MouseButtonTransitionedDown(Mouse.LeftButtonState)
            && gs_PointIsInRect(Mouse.DownPos, ElementBounds))
        {
            PushNodeOnWorkspace(i, &State->NodeWorkspace, &State->Transient);
            GraphState->LayoutIsDirty = true;
            MouseHandled = true;
        }
    }
    
    if (!MouseHandled && MouseButtonTransitionedDown(Mouse.LeftButtonState))
    {
        BeginPanNodeGraph(State, {}, Mouse);
    }
}


#define FOLDHAUS_PANEL_NODE_GRAPH_H
#endif // FOLDHAUS_PANEL_NODE_GRAPH_H
