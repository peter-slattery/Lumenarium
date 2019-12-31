internal void
PushNodeOnWorkspace(s32 NodeSpecificationIndex, pattern_node_workspace* Workspace)
{
    pattern_node* NewNode = Workspace->Nodes.TakeElement();
    NewNode->SpecificationIndex = NodeSpecificationIndex;
}








// vv Old vv

internal node_list_iterator
GetNodeListIterator(node_list List)
{
    node_list_iterator Result = {};
    Result.List = List;
    Result.CurrentBuffer = List.First;
    Result.At = Result.CurrentBuffer->Headers;
    Result.TotalIndexAt = 0;
    Result.BufferIndexAt = 0;
    
    return Result;
}

internal b32
NodeIteratorIsValid(node_list_iterator Iter)
{
    b32 Result = (Iter.At != 0); 
    Result &= Iter.TotalIndexAt < Iter.List.TotalUsed && Iter.TotalIndexAt >= 0;
    Result &= Iter.BufferIndexAt < Iter.CurrentBuffer->Used && Iter.BufferIndexAt >= 0;
    return Result;
}

internal void
Next (node_list_iterator* Iter)
{
    if (Iter->BufferIndexAt < Iter->CurrentBuffer->Used)
    {
        Iter->At++;
        Iter->TotalIndexAt++;
        Iter->BufferIndexAt++;
        if (Iter->At->Handle == 0) { Next(Iter); }
    }
    else if (Iter->CurrentBuffer->Next)
    {
        Iter->CurrentBuffer = Iter->CurrentBuffer->Next;
        Iter->At = Iter->CurrentBuffer->Headers;
        Iter->TotalIndexAt++;
        Iter->BufferIndexAt = 0;
        if (Iter->At->Handle == 0) { Next(Iter); }
    }
    else
    {
        Iter->At = 0;
        Iter->TotalIndexAt = -1;
        Iter->BufferIndexAt = -1;
    }
}

internal node_list_buffer*
AllocateNodeListBuffer (memory_arena* Storage, s32 Count)
{
    node_list_buffer* Result = PushStruct(Storage, node_list_buffer);;
    Result->Headers = PushArray(Storage, node_header, Count);
    Result->Max = Count;
    Result->Used = 0;
    Result->Next = 0;
    return Result;
}

internal node_list*
AllocateNodeList (memory_arena* Storage, s32 InitialCount)
{
    node_list* Result = PushStruct(Storage, node_list);
    Result->First = AllocateNodeListBuffer(Storage, InitialCount);
    Result->Head = Result->First;
    Result->TotalMax = InitialCount;
    Result->TotalUsed = 0;
    Result->HandleAccumulator = 0;
    return Result;
}

global_variable char* OutputName = "Output";

internal string
GetNodeName (node_header Node)
{
    string Result = {};
    
    node_specification Spec = NodeSpecifications[Node.Type];
    Result = MakeString(Spec.Name, Spec.NameLength);
    
    return Result;
}

internal node_header*
PushNodeOnList (node_list* List, s32 ConnectionsCount, v2 Min, v2 Dim, memory_arena* Storage)
{
    node_header* Result = 0;
    
    if ((List->TotalUsed + 1) >= List->TotalMax)
    {
        node_list_buffer* Buf = AllocateNodeListBuffer(Storage, List->Head->Max);
        List->Head->Next = Buf;
        List->Head = Buf;
        List->TotalMax += Buf->Max;
    }
    Assert(List->TotalUsed + 1 <= List->TotalMax);
    
    Result = List->Head->Headers + List->Head->Used;
    Result->Handle = ++List->HandleAccumulator;
    
    // :ConnectionsToStretchyBuffer
    Assert(List->ConnectionsUsed + ConnectionsCount < NODE_LIST_CONNECTIONS_MAX);
    Result->ConnectionsCount = ConnectionsCount;
    Result->Connections = (node_connection*)(List->Connections + List->ConnectionsUsed);
    List->ConnectionsUsed += ConnectionsCount;
    
    for (s32 c = 0; c < Result->ConnectionsCount; c++)
    {
        Result->Connections[c].NodeHandle = Result->Handle;
    }
    
    Result->Min = Min;
    Result->Dim = Dim;
    
    List->Head->Used++;
    List->TotalUsed++;
    
    return Result;
}

internal void
FreeNodeOnList (node_list* List, node_header* Node)
{
    // TODO(Peter): 
}

internal void
InitializeNodeConnection (node_connection* Connection, node_struct_member Member, node_header* Node)
{
    Connection->Type = Member.Type;
    Connection->UpstreamNodeHandle = 0;
    Connection->UpstreamNodePortIndex = -1;
    Connection->DownstreamNodeHandle = 0;
    Connection->DownstreamNodePortIndex = -1;
    Connection->DirectionMask = Member.IsInput;
    
    Connection->Ptr = Node->PersistentData + Member.Offset;
    
    switch (Member.Type)
    {
        case MemberType_s32:
        {
            *Connection->S32ValuePtr = 0;
        }break;
        
        case MemberType_r32:
        {
            *Connection->R32ValuePtr = 0;
        }break;
        
        case MemberType_v4:
        {
            *Connection->V4ValuePtr = v4{0, 0, 0, 1};
        }break;
        
        case MemberType_NODE_COLOR_BUFFER:
        {
            *Connection->LEDsValuePtr = {};
        }break;
        
        InvalidDefaultCase;
    }
    
}

inline r32
CalculateNodeHeight (s32 Members)
{
    r32 Result = (NODE_PORT_STEP * Members) + NODE_HEADER_HEIGHT;
    return Result;
}

internal node_header*
PushNodeOnListFromSpecification (node_list* List, node_type Type, v2 Min, memory_arena* Storage)
{
    node_header* Node = 0;
    
    node_specification Spec = NodeSpecifications[Type];
    
    // :NodesDontNeedToKnowTheirBounds
    r32 NodeHeight = CalculateNodeHeight (Spec.MemberListLength);
    Node = PushNodeOnList(List, 
                          Spec.MemberListLength,
                          Min, 
                          v2{150, NodeHeight}, 
                          Storage);
    Node->Type = Type;
    Node->PersistentData = PushArray(Storage, u8, Spec.DataStructSize);
    
    node_struct_member* MemberList = Spec.MemberList;
    for (u32 MemberIdx = 0; MemberIdx < Spec.MemberListLength; MemberIdx++)
    {
        node_struct_member Member = MemberList[MemberIdx];
        InitializeNodeConnection(Node->Connections + MemberIdx, Member, Node);
    }
    
    return Node;
}

internal node_header*
PushOutputNodeOnList (node_list* List, v2 Min, memory_arena* Storage)
{
    node_header* Result = PushNodeOnListFromSpecification(List, NodeType_OutputNode, Min, Storage);
    return Result;
}

internal node_header*
GetNodeWithHandle(node_list* List, s32 Handle)
{
    DEBUG_TRACK_FUNCTION;
    node_header* Result = 0;
    
    node_list_iterator Iter = GetNodeListIterator(*List);
    while (NodeIteratorIsValid(Iter))
    {
        if(Iter.At->Handle == Handle)
        {
            Result = Iter.At;
            break;
        }
        Next(&Iter);
    }
    
    return Result;
}

internal rect
CalculateNodeBounds (node_header* Node, node_render_settings Settings)
{
    rect Result = {};
    Result.Min = Node->Min;
    Result.Max = Node->Min + Node->Dim + v2{0, NODE_HEADER_HEIGHT};
    return Result;
}

internal rect
CalculateNodeInputPortBounds (node_header* Node, s32 Index, node_render_settings RenderSettings)
{
    rect Result = {};
    
    Result.Min = v2{
        Node->Min.x,
        Node->Min.y + Node->Dim.y - ((NODE_PORT_STEP * (Index + 1)) + NODE_HEADER_HEIGHT)};
    Result.Max = Result.Min + NODE_PORT_DIM;
    
    return Result;
}

internal rect
CalculateNodeInputValueBounds (node_header* Node, s32 Index, node_render_settings RenderSettings)
{
    rect Result = {};
    rect Port = CalculateNodeInputPortBounds(Node, Index, RenderSettings);
    Result.Min = v2{Port.Max.x, Port.Min.y};
    Result.Max = Result.Min + v2{NODE_PORT_DIM.x * 2, NODE_PORT_DIM.y};
    return Result;
}

internal rect
CalculateNodeOutputPortBounds (node_header* Node, s32 Index, node_render_settings RenderSettings)
{
    rect Result = {};
    Result.Min = v2{
        Node->Min.x + Node->Dim.x - NODE_PORT_DIM.x,
        Node->Min.y + Node->Dim.y - ((NODE_PORT_STEP * (Index + 1)) + NODE_HEADER_HEIGHT)};
    Result.Max = Result.Min + NODE_PORT_DIM;
    return Result;
}

internal rect
CalculateNodeOutputValueBounds (node_header* Node, s32 Index, node_render_settings RenderSettings)
{
    rect Result = {};
    rect Port = CalculateNodeOutputPortBounds(Node, Index, RenderSettings);
    Result.Min = v2{Port.Min.x - (NODE_PORT_DIM.x * 2), Port.Min.y};
    Result.Max = v2{Port.Min.x, Port.Max.y};
    return Result;
}

internal rect
GetBoundsOfPortConnectedToInput (node_header* Node, s32 PortIndex, node_list* NodeList, node_render_settings RenderSettings)
{
    node_header* ConnectedNode =
        GetNodeWithHandle(NodeList, Node->Connections[PortIndex].UpstreamNodeHandle);
    rect Result = CalculateNodeOutputPortBounds(ConnectedNode, Node->Connections[PortIndex].UpstreamNodePortIndex, RenderSettings);
    return Result;
}

internal rect
GetBoundsOfPortConnectedToOutput (node_header* Node, s32 PortIndex, node_list* NodeList, node_render_settings RenderSettings)
{
    node_header* ConnectedNode = GetNodeWithHandle(NodeList, Node->Connections[PortIndex].DownstreamNodeHandle);
    rect Result = CalculateNodeInputPortBounds(ConnectedNode, Node->Connections[PortIndex].DownstreamNodePortIndex, RenderSettings);
    return Result;
}

internal rect
CalculateNodeDragHandleBounds (rect NodeBounds, s32 Index, node_render_settings RenderSettings)
{
    rect Result {};
    v2 HorizontalOffset = v2{Width(NodeBounds) / 3, 0};
    Result.Min = v2{NodeBounds.Min.x, NodeBounds.Max.y - NODE_HEADER_HEIGHT} + (HorizontalOffset * Index);
    Result.Max = Result.Min + v2{HorizontalOffset.x, NODE_HEADER_HEIGHT};
    return Result;
}

internal node_interaction
NewEmptyNodeInteraction ()
{
    node_interaction Result = {};
    Result.NodeHandle = 0;
    Result.InputPort = -1;
    Result.InputValue = -1;
    Result.OutputPort = -1;
    Result.OutputValue = -1;
    return Result;
}

internal node_interaction
NewNodeInteraction (s32 NodeHandle, v2 MouseOffset)
{
    node_interaction Result = {};
    Result.NodeHandle = NodeHandle;
    Result.MouseOffset = MouseOffset;
    Result.InputPort = -1;
    Result.InputValue = -1;
    Result.OutputPort = -1;
    Result.OutputValue = -1;
    return Result;
}

internal b32
IsDraggingNode (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) && 
                  (Interaction.InputPort < 0 && Interaction.InputValue < 0) && 
                  (Interaction.OutputPort < 0 && Interaction.InputValue < 0));
    return Result;
}

internal b32
IsDraggingNodePort (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) &&
                  (Interaction.InputPort >= 0 || Interaction.OutputPort >= 0) &&
                  (Interaction.InputValue < 0 && Interaction.OutputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeValue (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) &&
                  (Interaction.InputPort < 0 && Interaction.OutputPort < 0) &&
                  (Interaction.InputValue >= 0 || Interaction.OutputValue >= 0));
    return Result;
}

internal b32
IsDraggingNodeInput (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) && 
                  (Interaction.InputPort >= 0 || Interaction.InputValue >= 0) && 
                  (Interaction.OutputPort < 0 && Interaction.OutputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeInputPort (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) && 
                  (Interaction.InputPort >= 0) && (Interaction.InputValue < 0) && 
                  (Interaction.OutputPort < 0 && Interaction.OutputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeInputValue (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) && 
                  (Interaction.InputPort < 0) &&  (Interaction.InputValue >= 0) && 
                  (Interaction.OutputPort < 0 && Interaction.OutputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeOutput (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) && 
                  (Interaction.OutputPort >= 0 || Interaction.OutputValue >= 0) && 
                  (Interaction.InputPort < 0 && Interaction.InputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeOutputPort (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) && 
                  (Interaction.OutputPort >= 0) && (Interaction.OutputValue < 0) && 
                  (Interaction.InputPort < 0 && Interaction.InputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeOutputValue (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) && 
                  (Interaction.OutputPort < 0) &&  (Interaction.OutputValue >= 0) && 
                  (Interaction.InputPort < 0 && Interaction.InputValue < 0));
    return Result;
}

internal b32
ConnectionIsConnected (node_connection Connection)
{
    b32 Result = (Connection.UpstreamNodeHandle > 0) || (Connection.DownstreamNodeHandle > 0);
    return Result;
}

internal b32
ConnectionIsConnected (node_header* Node, s32 Index)
{
    b32 Result = ConnectionIsConnected(Node->Connections[Index]);
    return Result;
}

internal b32
ConnectionHasUpstreamConnection (node_connection Connection)
{
    b32 Result = (Connection.UpstreamNodeHandle > 0);
    return Result;
}

internal b32
ConnectionHasUpstreamConnection (node_header* Node, s32 Index)
{
    b32 Result = ConnectionHasUpstreamConnection(Node->Connections[Index]);
    return Result;
}

internal b32
ConnectionHasDownstreamConnection (node_connection Connection)
{
    b32 Result = (Connection.DownstreamNodeHandle > 0);
    return Result;
}

internal b32
ConnectionHasDownstreamConnection (node_header* Node, s32 Index)
{
    b32 Result = ConnectionHasDownstreamConnection(Node->Connections[Index]);
    return Result;
}

internal b32
ConnectionIsInput (node_connection Connection)
{
    b32 Result = (Connection.DirectionMask & IsInputMember) > 0;
    return Result;
}

internal b32
ConnectionIsInput (node_header* Node, s32 ConnectionIdx)
{
    return ConnectionIsInput(Node->Connections[ConnectionIdx]);
}

internal b32
ConnectionIsOutput (node_connection Connection)
{
    b32 Result = (Connection.DirectionMask & IsOutputMember) > 0;
    return Result;
}

internal b32
ConnectionIsOutput (node_header* Node, s32 ConnectionIdx)
{
    return ConnectionIsOutput(Node->Connections[ConnectionIdx]);
}

internal b32
CheckForRecursionWithHandle (node_list* NodeList, s32 LookForNodeHandle, node_header* StartNode)
{
    DEBUG_TRACK_FUNCTION;
    b32 Result = false;
    
    for (s32 Connection = 0; Connection < StartNode->ConnectionsCount; Connection++)
    {
        if (!ConnectionIsOutput(StartNode->Connections[Connection])) { continue; }
        
        if (StartNode->Connections[Connection].DownstreamNodeHandle == LookForNodeHandle)
        {
            Result = true;
            break;
        }
        
        if (StartNode->Connections[Connection].DownstreamNodeHandle > 0)
        {
            node_header* NextNode = GetNodeWithHandle(NodeList, StartNode->Connections[Connection].DownstreamNodeHandle);
            Result = CheckForRecursionWithHandle(NodeList, LookForNodeHandle, NextNode);
            if (Result) { break; }
        }
    }
    
    return Result;
}

internal b32
PortTypesMatch (node_header* UpstreamNode, s32 UpstreamNode_OutputPort, node_header* DownstreamNode, s32 DownstreamNode_InputPort)
{
    Assert(ConnectionIsOutput(UpstreamNode, UpstreamNode_OutputPort));
    Assert(ConnectionIsInput(DownstreamNode, DownstreamNode_InputPort));
    b32 Result = UpstreamNode->Connections[UpstreamNode_OutputPort].Type == DownstreamNode->Connections[DownstreamNode_InputPort].Type;
    return Result;
}

internal void
ConnectNodes(node_list* NodeList, 
             s32 UpstreamNodeHandle, s32 UpstreamNodePort, 
             s32 DownstreamNodeHandle, s32 DownstreamNodePort)
{
    node_header* UpstreamNode = 0;
    node_header* DownstreamNode = GetNodeWithHandle(NodeList, DownstreamNodeHandle);
    if (!CheckForRecursionWithHandle(NodeList, UpstreamNodeHandle, DownstreamNode))
    {
        UpstreamNode = GetNodeWithHandle(NodeList, UpstreamNodeHandle);
        if (PortTypesMatch(UpstreamNode, UpstreamNodePort, 
                           DownstreamNode, DownstreamNodePort))
        {
            Assert(ConnectionIsOutput(UpstreamNode, UpstreamNodePort));
            Assert(ConnectionIsInput(DownstreamNode, DownstreamNodePort));
            
            DownstreamNode->Connections[DownstreamNodePort].UpstreamNodeHandle = UpstreamNodeHandle;
            DownstreamNode->Connections[DownstreamNodePort].UpstreamNodePortIndex = UpstreamNodePort;
            UpstreamNode->Connections[UpstreamNodePort].DownstreamNodeHandle = DownstreamNodeHandle;
            UpstreamNode->Connections[UpstreamNodePort].DownstreamNodePortIndex = DownstreamNodePort;
        }
    }
}

internal void
UnconnectNodes (node_list* NodeList, 
                s32 DownstreamNodeHandle, s32 DownstreamNode_OutputPort, s32 UpstreamNodeHandle,  s32 UpstreamNode_InputPort)
{
    node_header* DownstreamNode = GetNodeWithHandle(NodeList, DownstreamNodeHandle);
    node_header* UpstreamNode = GetNodeWithHandle(NodeList, UpstreamNodeHandle);
    
    Assert(ConnectionIsOutput(DownstreamNode, DownstreamNode_OutputPort));
    Assert(ConnectionIsInput(UpstreamNode, UpstreamNode_InputPort));
    
    DownstreamNode->Connections[DownstreamNode_OutputPort].DownstreamNodeHandle = 0;
    DownstreamNode->Connections[DownstreamNode_OutputPort].DownstreamNodePortIndex = -1;
    UpstreamNode->Connections[UpstreamNode_InputPort].UpstreamNodeHandle = 0;
    UpstreamNode->Connections[UpstreamNode_InputPort].UpstreamNodePortIndex = -1;
}

internal node_header*
GetNodeUnderPoint (node_list* NodeList, v2 Point, node_render_settings RenderSettings)
{
    DEBUG_TRACK_FUNCTION;
    node_header* Result = 0;
    
    node_list_iterator NodeIter = GetNodeListIterator(*NodeList);
    while (NodeIteratorIsValid(NodeIter))
    {
        node_header* Node = NodeIter.At;
        rect NodeBounds = CalculateNodeBounds(Node, RenderSettings);
        if (PointIsInRect(Point, NodeBounds))
        {
            Result = Node;
            break;
        }
        Next(&NodeIter);
    }
    
    return Result;
}

internal node_interaction
GetNodeInteractionType (node_header* ActiveNode, v2 MousePos, node_render_settings RenderSettings)
{
    DEBUG_TRACK_FUNCTION;
    
    node_interaction Interaction = NewNodeInteraction(ActiveNode->Handle, ActiveNode->Min - MousePos);
    
    rect NodeBounds = CalculateNodeBounds(ActiveNode, RenderSettings);
    
    
    for (s32 Connection = 0; Connection < ActiveNode->ConnectionsCount; Connection++)
    {
        // Inputs
        if (ConnectionIsInput(ActiveNode, Connection))
        {
            rect InputBounds = CalculateNodeInputPortBounds(ActiveNode, Connection, RenderSettings);
            rect ValueBounds = CalculateNodeInputValueBounds(ActiveNode, Connection, RenderSettings);
            if (PointIsInRect(MousePos, InputBounds))
            {
                Interaction.InputPort = Connection;
                Interaction.MouseOffset = MousePos - InputBounds.Min;
            }
            else if(PointIsInRect(MousePos, ValueBounds))
            {
                Interaction.InputValue = Connection;
                Interaction.MouseOffset = MousePos - ValueBounds.Min;
            }
        }
        
        // Outputs
        if (ConnectionIsOutput(ActiveNode, Connection))
        {
            rect OutputBounds = CalculateNodeOutputPortBounds(ActiveNode, Connection, RenderSettings);
            rect ValueBounds = CalculateNodeOutputValueBounds(ActiveNode, Connection, RenderSettings);
            if (PointIsInRect(MousePos, OutputBounds))
            {
                Interaction.OutputPort = Connection;
                Interaction.MouseOffset = MousePos - OutputBounds.Min;
            }
            else if(PointIsInRect(MousePos, ValueBounds))
            {
                Interaction.OutputValue = Connection;
                Interaction.MouseOffset = MousePos - ValueBounds.Min;
            }
        }
    }
    
    // Drag Handles
    rect DragUpstreamHandleBounds = CalculateNodeDragHandleBounds(NodeBounds, 0, RenderSettings);
    rect DragAllHandleBounds = CalculateNodeDragHandleBounds(NodeBounds, 1, RenderSettings);
    rect DragDownstreamHandleBounds = CalculateNodeDragHandleBounds(NodeBounds, 2, RenderSettings);
    if (PointIsInRect(MousePos, DragUpstreamHandleBounds))
    {
        Interaction.Flags = NodeInteraction_AllUpstream;
    }
    else if (PointIsInRect(MousePos, DragAllHandleBounds))
    {
        Interaction.Flags = NodeInteraction_AllUpstream | NodeInteraction_AllDownstream;
    }
    else if (PointIsInRect(MousePos, DragDownstreamHandleBounds))
    {
        Interaction.Flags = NodeInteraction_AllDownstream;
    }
    
    return Interaction;
}

internal void
TryConnectNodes (node_interaction Interaction, v2 Point, node_list* NodeList, node_render_settings RenderSettings)
{
    DEBUG_TRACK_FUNCTION;
    
    if (IsDraggingNodeOutput(Interaction))
    {
        node_header* UpstreamNode = GetNodeUnderPoint(NodeList, Point, RenderSettings);
        if (UpstreamNode)
        {
            for (s32 Connection = 0; Connection < UpstreamNode->ConnectionsCount; Connection++)
            {
                if (ConnectionIsOutput(UpstreamNode, Connection)) { continue; }
                
                rect InputBounds = CalculateNodeInputPortBounds(UpstreamNode, Connection, RenderSettings);
                if (PointIsInRect(Point, InputBounds))
                {
                    ConnectNodes(NodeList, Interaction.NodeHandle, Interaction.OutputPort,
                                 UpstreamNode->Handle, Connection);
                    break;
                }
            }
        }
    }
    else if (IsDraggingNodeInput(Interaction))
    {
        node_header* DownstreamNode = GetNodeUnderPoint(NodeList, Point, RenderSettings);
        if (DownstreamNode)
        {
            for (s32 Connection = 0; Connection < DownstreamNode->ConnectionsCount; Connection++)
            {
                if (ConnectionIsInput(DownstreamNode, Connection)) { continue; }
                
                rect OutputBounds = CalculateNodeOutputPortBounds(DownstreamNode, Connection, RenderSettings);
                if (PointIsInRect(Point, OutputBounds))
                {
                    ConnectNodes(NodeList, 
                                 DownstreamNode->Handle, Connection,
                                 Interaction.NodeHandle, Interaction.InputPort);
                    break;
                }
            }
        }
    }
}

internal void
PlaceNode (node_list* NodeList, node_header* Node, v2 Position, b32 Flags)
{
    DEBUG_TRACK_FUNCTION;
    
    v2 Offset = Position - Node->Min;
    
    if (Flags & NodeInteraction_AllDownstream)
    {
        for (s32 Connection = 0; Connection < Node->ConnectionsCount; Connection++)
        {
            if (!ConnectionIsOutput(Node, Connection)) { continue; }
            
            s32 ConnectionHandle = Node->Connections[Connection].DownstreamNodeHandle;
            if (ConnectionHandle > 0)
            {
                node_header* ConnectedNode = GetNodeWithHandle(NodeList, ConnectionHandle);
                v2 CurrPos = ConnectedNode->Min;
                v2 NewPos = CurrPos + Offset;
                // NOTE(Peter): Have to negate the all downstream component so it doesn't turn around and try
                // to move this node again.
                PlaceNode(NodeList, ConnectedNode, NewPos, Flags & ~NodeInteraction_AllUpstream);
            }
        }
    }
    
    if (Flags & NodeInteraction_AllUpstream)
    {
        for (s32 Connection = 0; Connection < Node->ConnectionsCount; Connection++)
        {
            if (!ConnectionIsInput(Node, Connection)) { continue; }
            
            s32 ConnectionHandle = Node->Connections[Connection].UpstreamNodeHandle;
            if (ConnectionHandle > 0)
            {
                node_header* ConnectedNode = GetNodeWithHandle(NodeList, ConnectionHandle);
                v2 CurrPos = ConnectedNode->Min;
                v2 NewPos = CurrPos + Offset;
                // NOTE(Peter): Have to negate the all upstream component so it doesn't turn around and try
                // to move this node again.
                PlaceNode(NodeList, ConnectedNode, NewPos, Flags & ~NodeInteraction_AllDownstream);
            }
        }
    }
    
    Node->Min = Position;
}

internal void
UpdateDraggingNode (v2 MousePos, node_interaction Interaction, node_list* NodeList, node_render_settings RenderSettings)
{
    DEBUG_TRACK_FUNCTION;
    
    if (IsDraggingNode(Interaction))
    {
        node_header* ActiveNode = GetNodeWithHandle(NodeList, Interaction.NodeHandle);
        PlaceNode(NodeList, ActiveNode, MousePos + Interaction.MouseOffset, Interaction.Flags);
    }
}

internal void
UpdateDraggingNodePort (v2 MousePos, node_interaction Interaction, node_list* NodeList, node_render_settings RenderSettings, render_command_buffer* RenderBuffer)
{
    DEBUG_TRACK_FUNCTION;
    
    if (IsDraggingNodePort(Interaction))
    {
        node_header* ActiveNode = GetNodeWithHandle(NodeList, Interaction.NodeHandle);
        rect PortBounds = {};
        if (IsDraggingNodeInput(Interaction))
        {
            PortBounds = CalculateNodeInputPortBounds(ActiveNode, Interaction.InputPort, RenderSettings);
        }
        else if (IsDraggingNodeOutput(Interaction))
        {
            PortBounds = CalculateNodeOutputPortBounds(ActiveNode, Interaction.OutputPort, RenderSettings);
        }
        
        v2 PortCenter = CalculateRectCenter(PortBounds);
        PushRenderLine2D(RenderBuffer, PortCenter, MousePos, 1, WhiteV4);
    }
}

internal void
UpdateDraggingNodeValue (v2 MousePos, v2 LastFrameMousePos, node_interaction Interaction, node_list* NodeList, node_render_settings RenderSettings, app_state* State)
{
    DEBUG_TRACK_FUNCTION;
    
    if(IsDraggingNodeValue(Interaction))
    {
        v2 MouseDelta = MousePos - LastFrameMousePos;
        node_header* Node = GetNodeWithHandle(NodeList, Interaction.NodeHandle);
        
        node_connection* Connection = 0;
        if (IsDraggingNodeInputValue(Interaction))
        {
            Connection = Node->Connections + Interaction.InputValue;
            Assert(ConnectionIsInput(*Connection));
        }
        else if (IsDraggingNodeOutputValue(Interaction))
        {
            Connection = Node->Connections + Interaction.OutputValue;
            Assert(ConnectionIsOutput(*Connection));
        }
        Assert(Connection);
        
        switch (Connection->Type)
        {
            case MemberType_s32:
            {
                *Connection->S32ValuePtr += (s32)(MouseDelta.y * .05f);
            }break;
            
            case MemberType_r32:
            {
                *Connection->R32ValuePtr += (MouseDelta.y * .05f);
            }break;
            
            case MemberType_v4:
            {
            }break;
            
            case MemberType_NODE_COLOR_BUFFER: {} break; // NOTE(Peter): Unused for now
            InvalidDefaultCase;
        }
    }
}


internal void UpdateNodeCalculation (node_header* Node, node_list* NodeList, 
                                     memory_arena* Permanent, memory_arena* Transient,
                                     led* LEDs, pixel* ColorsInit, s32 LEDCount, r32 DeltaTime);

internal void
UpdateNodesConnectedUpstream (node_header* Node, node_list* NodeList, 
                              memory_arena* Permanent, memory_arena* Transient,
                              led* LEDs, pixel* ColorsInit, s32 LEDCount, r32 DeltaTime)
{
    for (s32 ConnectionIdx = 0; ConnectionIdx < Node->ConnectionsCount; ConnectionIdx++)
    {
        node_connection* Connection = 0;
        if (ConnectionIsInput(Node->Connections[ConnectionIdx]))
        {
            Connection = Node->Connections + ConnectionIdx;
            
            if (ConnectionHasUpstreamConnection(*Connection))
            {
                node_header* UpstreamNode = GetNodeWithHandle(NodeList, Connection->UpstreamNodeHandle);
                if (!UpstreamNode->UpdatedThisFrame)
                {
                    UpdateNodeCalculation(UpstreamNode, NodeList, Permanent, Transient, LEDs, ColorsInit, LEDCount, DeltaTime);
                }
                
                node_connection UpstreamConnection = UpstreamNode->Connections[Connection->UpstreamNodePortIndex];
                
                switch (Connection->Type)
                {
                    case MemberType_s32:
                    {
                        *Connection->S32ValuePtr = *UpstreamConnection.S32ValuePtr;
                    }break;
                    
                    case MemberType_r32:
                    {
                        *Connection->R32ValuePtr = *UpstreamConnection.R32ValuePtr;
                    }break;
                    
                    case MemberType_v4:
                    {
                        *Connection->V4ValuePtr = *UpstreamConnection.V4ValuePtr;
                    }break;
                    
                    case MemberType_NODE_COLOR_BUFFER:
                    {
                        *Connection->LEDsValuePtr = *UpstreamConnection.LEDsValuePtr;
                    }break;
                    
                    InvalidDefaultCase;
                }
            }
        }
    }
}

internal void
UpdateNodeCalculation (node_header* Node, node_list* NodeList, 
                       memory_arena* Permanent, memory_arena* Transient,
                       led* LEDs, pixel* ColorsInit, s32 LEDCount, r32 DeltaTime)
{
    DEBUG_TRACK_FUNCTION;
    Assert(Node->PersistentData != 0);
    Assert(Node->Type != NodeType_OutputNode);
    
    // NOTE(Peter): Have to subtract one here so that we account for the 
    // NodeType_OutputNode entry in the enum
    node_specification Spec = NodeSpecifications[Node->Type];
    node_struct_member* MemberList = Spec.MemberList;
    
    // NOTE(Peter): We do this at the beginning in case there is a node connected to this one
    // which has a connection that is both an Input and Output. In that case, if UpdatedThisFrame
    // were not set before hand, the two nodes would pingpong back and forth trying to get 
    // eachother to update.
    Node->UpdatedThisFrame = true;
    
    pixel* Colors = ColorsInit;
    
    UpdateNodesConnectedUpstream(Node, NodeList, Permanent, Transient, LEDs, Colors, LEDCount, DeltaTime);
    
    for (s32 ConnectionIdx = 0; ConnectionIdx < Node->ConnectionsCount; ConnectionIdx++)
    {
        node_connection Connection = Node->Connections[ConnectionIdx];
        
        // TODO(Peter): We're currently passing in a pointer to the leds array for every single
        // NODE_COLOR_BUFFER. We shouldn't do that, and just require each data structure that
        // needs the leds to request that as its own member/parameter.
        if (Connection.Type == MemberType_NODE_COLOR_BUFFER)
        {
            node_led_color_connection* ColorConnection = Connection.LEDsValuePtr;
            if (!ColorConnection->Colors)
            {
                pixel* ColorsCopy = PushArray(Transient, pixel, LEDCount);
                GSMemSet((u8*)ColorsCopy, 0, sizeof(pixel) * LEDCount);
                
                ColorConnection->Colors = ColorsCopy;
                ColorConnection->LEDs = LEDs;
                ColorConnection->LEDCount = LEDCount;
            }
        }
    }
    
    CallNodeProc(Node, Node->PersistentData, LEDs, LEDCount, DeltaTime);
    
    for (s32 ConnectionIdx = 0; ConnectionIdx < Node->ConnectionsCount; ConnectionIdx++)
    {
        if (!ConnectionIsOutput(Node, ConnectionIdx)) { continue; }
        node_connection* Connection = Node->Connections + ConnectionIdx;
    }
}

internal void
UpdateOutputNodeCalculations (node_header* OutputNode, node_list* NodeList, 
                              memory_arena* Permanent, memory_arena* Transient, 
                              led* LEDs, pixel* Colors, s32 LEDCount, r32 DeltaTime)
{
    Assert(OutputNode->Type == NodeType_OutputNode);
    
    UpdateNodesConnectedUpstream(OutputNode, NodeList, Permanent, Transient, LEDs, Colors, LEDCount, DeltaTime);
    
    node_connection ColorsConnection = OutputNode->Connections[0];
    if (ColorsConnection.LEDsValuePtr->Colors)
    {
        pixel* DestPixel = Colors;
        pixel* SourcePixel = ColorsConnection.LEDsValuePtr->Colors;
        for (s32 i = 0; i < LEDCount; i++)
        {
            *DestPixel++ = *SourcePixel++;
        }
    }
}

#if 0
// Trying to put updating nodes in terms of connections, rather than nodes.
internal void
UpdateAllNodesFloodFill (node_list* NodeList, memory_arena* Permanent, memory_arena* Transient, led* LEDs, pixel* Colors, s32 LEDCount, r32 DeltaTime)
{
    s32 NodesUpdated = 0;
    
    s32 DEBUGIterations = 0;
    while(NodesUpdated < NodeList->TotalUsed)
    {
        node_list_iterator NodeIter = GetNodeListIterator(*NodeList);
        while (NodeIteratorIsValid(NodeIter))
        {
            node_header* Node = NodeIter.At;
            
            s32 ConnectionsReady = 0;
            // Check if all upstream connections have been updated
            // TODO(Peter): we should move the HasBeenUpdated field into the connections
            // and have connections push their updates upstream
            for (s32 c = 0; c < Node->ConnectionsCount; c++)
            {
                node_connection* Connection = Node->Connections + c;
                if (ConnectionIsInput(*Connection) &&
                    ConnectionHasUpstreamConnection(*Connection))
                {
                    node_header* UpstreamNode = GetNodeWithHandle(NodeList, Connection->UpstreamNodeHandle);
                    if (UpstreamNode->UpdatedThisFrame)
                    {
                        ConnectionsReady += 1;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            
            if (ConnectionsReady == Node->ConnectionsCount)
            {
                
            }
        }
        DEBUGIterations++;
    }
}
#endif

internal void
UpdateAllNodeCalculations (node_list* NodeList, memory_arena* Permanent, memory_arena* Transient, led* LEDs, pixel* Colors, s32 LEDCount, r32 DeltaTime)
{
    node_list_iterator NodeIter = GetNodeListIterator(*NodeList);
    while (NodeIteratorIsValid(NodeIter))
    {
        node_header* Node = NodeIter.At;
        if (!Node->UpdatedThisFrame)
        {
            UpdateNodeCalculation(Node, NodeList, Permanent, Transient, LEDs, Colors, LEDCount, DeltaTime);
        }
        Next(&NodeIter);
    }
}

internal void
ClearTransientNodeColorBuffers (node_list* NodeList)
{
    node_list_iterator NodeIter = GetNodeListIterator(*NodeList);
    while (NodeIteratorIsValid(NodeIter))
    {
        node_header* Node = NodeIter.At;
        for (s32 ConnectionIdx = 0; ConnectionIdx < Node->ConnectionsCount; ConnectionIdx++)
        {
            node_connection* Connection = Node->Connections + ConnectionIdx;
            if (Connection->Type == MemberType_NODE_COLOR_BUFFER)
            {
                Connection->LEDsValuePtr->Colors = 0;
            }
        }
        Next(&NodeIter);
    }
}

internal void
DrawValueDisplay (render_command_buffer* RenderBuffer, rect Bounds, node_connection Value, bitmap_font* Font)
{
    PushRenderQuad2D(RenderBuffer, Bounds.Min, Bounds.Max, BlackV4);
    
    char Buffer[32];
    string String = MakeString(Buffer, 32);
    
    switch (Value.Type)
    {
        case MemberType_s32:
        {
            PrintF(&String, "%.*d", 4, *Value.S32ValuePtr);
            DrawString(RenderBuffer, String, Font, Bounds.Min + v2{2, 2}, WhiteV4);
        }break;
        
        case MemberType_r32:
        {
            PrintF(&String, "%.*f", 4, *Value.R32ValuePtr);
            DrawString(RenderBuffer, String, Font, Bounds.Min + v2{2, 2}, WhiteV4);
        }break;
        
        case MemberType_v4:
        {
            PushRenderQuad2D(RenderBuffer, Bounds.Min + v2{2, 2}, Bounds.Max - v2{2, 2}, *Value.V4ValuePtr);
        }break;
        
        case MemberType_NODE_COLOR_BUFFER:
        {
            PrintF(&String, "LEDs");
            DrawString(RenderBuffer, String, Font, Bounds.Min +  v2{2, 2}, WhiteV4);
        }break;
        
        InvalidDefaultCase;
    }
}

internal void
DrawPort (render_command_buffer* RenderBuffer, rect Bounds, v4 Color)
{
    PushRenderQuad2D(RenderBuffer, Bounds.Min, Bounds.Max, Color);
}

internal void
ResetNodesUpdateState (node_list* NodeList)
{
    node_list_iterator NodeIter = GetNodeListIterator(*NodeList);
    while (NodeIteratorIsValid(NodeIter))
    {
        node_header* Node = NodeIter.At;
        Node->UpdatedThisFrame = false;
        Next(&NodeIter);
    }
}

internal b32 
SpecificationPassesFilter(string SpecificationName, string SearchString)
{
    return (SearchString.Length == 0 || StringContainsStringCaseInsensitive(SpecificationName, SearchString));
}

internal s32
NodeListerConvertHotItemToListIndex (s32 HotItem, u8* NodeSpecificationsList, s32 NodeSpecificationsListCount,
                                     string SearchString)
{
    s32 ListIndex = 0;
    s32 FilteredItemsCount = 0;
    
    for (s32 i = 0; i < NodeSpecificationsListCount; i++)
    {
        node_specification* Specification = (node_specification*)NodeSpecificationsList + i;
        string ItemName = MakeString(Specification->Name);
        b32 PassesFilter = SpecificationPassesFilter(ItemName, SearchString);
        if (PassesFilter)
        {
            if (FilteredItemsCount == HotItem)
            {
                break;
            }
            FilteredItemsCount++;
        }
        ListIndex++;
    }
    
    return ListIndex;
}

internal string
NodeListerGetNodeName (u8* NodeSpecificationsList, s32 NodeSpecificationsListCount, string SearchString, s32 Offset)
{
    s32 FilteredItemsCount = 0;
    node_specification* Specification = (node_specification*)NodeSpecificationsList;
    string Result = {};
    for (s32 i = 0; i < NodeSpecificationsListCount; i++)
    {
        string ItemName = MakeString(Specification->Name);
        b32 PassesFilter = SpecificationPassesFilter(ItemName, SearchString);
        if (PassesFilter)
        {
            if (FilteredItemsCount == Offset)
            {
                Result = ItemName;
                break;
            }
            FilteredItemsCount++;
        }
        
        Specification++;
    }
    
    return Result;
}